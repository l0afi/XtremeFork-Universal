#include "Pattern.h"
#include <windows.h>
#include <psapi.h>
#include <cstring>
#include <cstdlib>
#include <climits>

namespace Memory {

ModuleRegion GetMainModuleRegion() {
    MODULEINFO modInfo{};
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO))) {
        return {
            reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll),
            static_cast<size_t>(modInfo.SizeOfImage)
        };
    }
    return {};
}

uintptr_t FindPatternRange(uintptr_t startAddr, uintptr_t endAddr, const char* pattern, const char* mask) {
    if (!pattern || !mask || startAddr >= endAddr) {
        return 0;
    }

    const size_t maskLen = std::strlen(mask);
    if (maskLen == 0 || startAddr + maskLen > endAddr) {
        return 0;
    }

    const auto* patBytes = reinterpret_cast<const uint8_t*>(pattern);
    for (uintptr_t cur = startAddr; cur < endAddr; ++cur) {
        bool matched = true;
        for (size_t i = 0; i < maskLen; ++i) {
            if (mask[i] == 'x') {
                if (*reinterpret_cast<const uint8_t*>(cur + i) != patBytes[i]) {
                    matched = false;
                    break;
                }
            }
        }
        if (matched) {
            return cur;
        }
    }

    return 0;
}

uintptr_t FindPattern(const char* pattern, const char* mask) {
    ModuleRegion region = GetMainModuleRegion();
    if (region.base == 0 || region.size == 0) {
        return 0;
    }
    return FindPatternRange(region.base, region.base + region.size, pattern, mask);
}

void PatchBytes(uintptr_t address, const void* data, size_t size) {
    DWORD oldProtect = 0;
    VirtualProtect(reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect);
    std::memcpy(reinterpret_cast<void*>(address), data, size);
    DWORD dummy = 0;
    VirtualProtect(reinterpret_cast<LPVOID>(address), size, oldProtect, &dummy);
}

void WriteU32(uintptr_t address, uint32_t value) {
    PatchBytes(address, &value, sizeof(uint32_t));
}

void AddU32(uintptr_t address, int32_t delta) {
    uint32_t updated = *reinterpret_cast<uint32_t*>(address) + static_cast<uint32_t>(delta);
    WriteU32(address, updated);
}

void WriteMovEaxImm32(uintptr_t address, uint32_t value) {
    uint8_t patch[5];
    patch[0] = 0xB8; // mov eax, imm32
    std::memcpy(&patch[1], &value, sizeof(uint32_t));
    PatchBytes(address, patch, sizeof(patch));
}

void WriteMovRaxImm64Nop3(uintptr_t address, int64_t value) {
    uint8_t patch[13];
    patch[0] = 0x48; // REX.W
    patch[1] = 0xB8; // movabs rax, imm64
    std::memcpy(&patch[2], &value, sizeof(int64_t));
    patch[10] = 0x90; // nop
    patch[11] = 0x90; // nop
    patch[12] = 0x90; // nop
    PatchBytes(address, patch, sizeof(patch));
}

uint8_t* AllocateTrampolineNear(uintptr_t anchorAddr, size_t size) {
    SYSTEM_INFO sysInfo{};
    GetSystemInfo(&sysInfo);
    const uintptr_t gran = sysInfo.dwAllocationGranularity ? sysInfo.dwAllocationGranularity : 0x10000;

    const uintptr_t maxDelta = 0x70000000ULL; // ~1.75 GB (well within signed 32-bit RIP-relative range)
    const uintptr_t minAddr = (anchorAddr > maxDelta)
        ? ((anchorAddr - maxDelta) & ~(gran - 1))
        : gran;
    const uintptr_t maxAddr = anchorAddr + maxDelta;

    // Search downward then upward from anchorAddr for a free allocation granularity slot
    for (uintptr_t offset = gran; offset < maxDelta; offset += gran) {
        if (anchorAddr > offset) {
            uintptr_t candidate = (anchorAddr - offset) & ~(gran - 1);
            if (candidate >= minAddr) {
                if (void* mem = VirtualAlloc(reinterpret_cast<LPVOID>(candidate), size,
                                             MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
                    return static_cast<uint8_t*>(mem);
                }
            }
        }
        uintptr_t candidateUp = (anchorAddr + offset) & ~(gran - 1);
        if (candidateUp < maxAddr) {
            if (void* mem = VirtualAlloc(reinterpret_cast<LPVOID>(candidateUp), size,
                                         MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) {
                return static_cast<uint8_t*>(mem);
            }
        }
    }

    // Fallback to VirtualAlloc anywhere in the address space
    return static_cast<uint8_t*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
}

void WriteIndirectJump64(uintptr_t jmpSite, uint8_t* trampoline, uintptr_t fallbackPtrSlot, size_t nopPadBytes) {
    uint64_t trampAddr = reinterpret_cast<uint64_t>(trampoline);
    uintptr_t internalPtrSlot = reinterpret_cast<uintptr_t>(trampoline + 0x80);
    int64_t ripNext = static_cast<int64_t>(jmpSite + 6);
    int64_t delta = static_cast<int64_t>(internalPtrSlot) - ripNext;

    int32_t disp32 = 0;
    if (delta >= INT32_MIN && delta <= INT32_MAX) {
        // Store the 8-byte pointer inside our own allocated trampoline page (+0x80)
        std::memcpy(trampoline + 0x80, &trampAddr, sizeof(uint64_t));
        disp32 = static_cast<int32_t>(delta);
    } else {
        // Fallback to writing pointer slot at fallbackPtrSlot
        PatchBytes(fallbackPtrSlot, &trampAddr, sizeof(uint64_t));
        disp32 = static_cast<int32_t>(static_cast<int64_t>(fallbackPtrSlot) - ripNext);
    }

    uint8_t stub[16];
    stub[0] = 0xFF;
    stub[1] = 0x25;
    std::memcpy(&stub[2], &disp32, sizeof(int32_t));
    for (size_t i = 0; i < nopPadBytes && (6 + i) < sizeof(stub); ++i) {
        stub[6 + i] = 0x90;
    }
    PatchBytes(jmpSite, stub, 6 + nopPadBytes);
}

} // namespace Memory
