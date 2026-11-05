#include "Pattern.h"
#include <windows.h>
#include <psapi.h>
#include <cstring>

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

} // namespace Memory
