#include "Game.h"
#include "Config.h"
#include "Logger.h"
#include "Pattern.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

namespace Game {

// ---------------------------------------------------------------------------
// Primary Heap Signatures (also used as fallback edition probes)
// ---------------------------------------------------------------------------
static constexpr const char* kHeapSizePattern =
    "\xBA\x00\x00\x00\x00\x45\x31\xC0\x41\xB1\x01\xE8\x00\x00\x00\x00\x48\x8D\x0D";
static constexpr const char* kHeapSizeMask = "x????xxxxxxx????xxx";

static constexpr const char* kLegacyHeapSizePattern =
    "\x48\x8D\x0D\x00\x00\x00\x00\x41\xB1\x01\x45\x33\xC0\xBA";
static constexpr const char* kLegacyHeapSizeMask = "xxx????xxxxxxx";

Edition DetectEdition() {
    char moduleName[MAX_PATH] = {};
    HMODULE hModule = GetModuleHandleW(nullptr);
    if (GetModuleBaseNameA(GetCurrentProcess(), hModule, moduleName, sizeof(moduleName)) != 0) {
        if (_stricmp(moduleName, kEnhancedExecutable) == 0) {
            return Edition::Enhanced;
        }
        if (_stricmp(moduleName, kLegacyExecutable) == 0) {
            return Edition::Legacy;
        }
    }

    // Fallback signature probe if the user renamed the game executable
    if (Memory::FindPattern(kHeapSizePattern, kHeapSizeMask) != 0) {
        return Edition::Enhanced;
    }
    if (Memory::FindPattern(kLegacyHeapSizePattern, kLegacyHeapSizeMask) != 0) {
        return Edition::Legacy;
    }

    return Edition::Unknown;
}

bool IsEnhancedEdition() {
    return DetectEdition() == Edition::Enhanced;
}

// ===========================================================================
// GTA V ENHANCED EDITION (GTA5_Enhanced.exe) PATCHERS
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. Enhanced Main Heap Size (32-bit & 64-bit)
// Pattern: BA ?? ?? ?? ?? 45 31 C0 41 B1 01 E8 ?? ?? ?? ?? 48 8D 0D
// Mask:    x????xxxxxxx????xxx
// ---------------------------------------------------------------------------
bool PatchMainHeapSize32(uint32_t heapBytes) {
    uintptr_t match = Memory::FindPattern(kHeapSizePattern, kHeapSizeMask);
    if (!match) {
        Logger::Log("Failed to find heap size pattern");
        return false;
    }

    uintptr_t immAddr = match + 1;
    Logger::Log("Overwriting heap size - OK");
    Memory::WriteU32(immAddr, heapBytes);
    return true;
}

bool PatchMainHeapSize64(uint64_t heapBytes) {
    uintptr_t match = Memory::FindPattern(kHeapSizePattern, kHeapSizeMask);
    if (!match) {
        Logger::Log("Failed to find heap size pattern");
        return false;
    }

    Logger::Log("Overwriting heap size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(match, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload:
    // +0x00: 48 BA <8-byte heapBytes>   ; movabs rdx, heapBytes
    // +0x0A: 45 31 C0                   ; xor r8d, r8d
    // +0x0D: 49 B9 <8-byte (match + 8)> ; movabs r9, match + 8
    // +0x17: 41 FF E1                   ; jmp r9
    uint8_t code[26];
    code[0] = 0x48;
    code[1] = 0xBA;
    std::memcpy(&code[2], &heapBytes, sizeof(uint64_t));
    code[10] = 0x45;
    code[11] = 0x31;
    code[12] = 0xC0;
    code[13] = 0x49;
    code[14] = 0xB9;
    uint64_t retAddr = match + 8;
    std::memcpy(&code[15], &retAddr, sizeof(uint64_t));
    code[23] = 0x41;
    code[24] = 0xFF;
    code[25] = 0xE1;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(match, trampoline, match - 0xFFF, 2);
    return true;
}

// ---------------------------------------------------------------------------
// 2. Enhanced Pool Heap Size (32-bit & 64-bit)
// Pattern: 48 ?? ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 41 B8 00 00 01 00 41 B9 ?? ?? ?? ?? E8
// Mask:    x??????x??????xxxxxxxx????x
// ---------------------------------------------------------------------------
static constexpr const char* kPoolSizePattern =
    "\x48\x00\x00\x00\x00\x00\x00\x48\x00\x00\x00\x00\x00\x00\x41\xB8\x00\x00\x01\x00\x41\xB9\x00\x00\x00\x00\xE8";
static constexpr const char* kPoolSizeMask = "x??????x??????xxxxxxxx????x";

bool PatchPoolHeapSize32(uint32_t poolBytes) {
    uintptr_t match = Memory::FindPattern(kPoolSizePattern, kPoolSizeMask);
    if (!match) {
        Logger::Log("Failed to find pool size pattern");
        return false;
    }

    uintptr_t immAddr = match + 0x16;
    Logger::Log("Overwriting pool size - OK");
    Memory::WriteU32(immAddr, poolBytes);
    return true;
}

bool PatchPoolHeapSize64(uint64_t poolBytes) {
    uintptr_t match = Memory::FindPattern(kPoolSizePattern, kPoolSizeMask);
    if (!match) {
        Logger::Log("Failed to find pool size pattern");
        return false;
    }

    uintptr_t baseAnchor = match + 0x10;
    Logger::Log("Overwriting pool size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(baseAnchor, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload:
    // +0x00: 49 B9 <8-byte poolBytes>          ; movabs r9, poolBytes
    // +0x0A: 48 B8 <8-byte (baseAnchor + 0xA)> ; movabs rax, baseAnchor + 0xA
    // +0x14: FF E0                             ; jmp rax
    uint8_t code[22];
    code[0] = 0x49;
    code[1] = 0xB9;
    std::memcpy(&code[2], &poolBytes, sizeof(uint64_t));
    code[10] = 0x48;
    code[11] = 0xB8;
    uint64_t retAddr = baseAnchor + 0x0A;
    std::memcpy(&code[12], &retAddr, sizeof(uint64_t));
    code[20] = 0xFF;
    code[21] = 0xE0;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(baseAnchor + 4, trampoline, baseAnchor - 0xFFB, 0);
    return true;
}

// ---------------------------------------------------------------------------
// 3. Enhanced Resource Heap Size (32-bit & 64-bit)
// Pattern: 48 ?? ?? ?? ?? ?? ?? BA 00 20 00 00 41 B8 ?? ?? ?? ??
// Mask:    x??????xxxxxxxx????
// ---------------------------------------------------------------------------
static constexpr const char* kResourceSizePattern =
    "\x48\x00\x00\x00\x00\x00\x00\xBA\x00\x20\x00\x00\x41\xB8\x00\x00\x00\x00";
static constexpr const char* kResourceSizeMask = "x??????xxxxxxxx????";

bool PatchResourceHeapSize32(uint32_t resourceBytes) {
    uintptr_t match = Memory::FindPattern(kResourceSizePattern, kResourceSizeMask);
    if (!match) {
        Logger::Log("Failed to find resource size pattern");
        return false;
    }

    uintptr_t immAddr = match + 0x0E;
    Logger::Log("Overwriting resource size - OK");
    Memory::WriteU32(immAddr, resourceBytes);
    return true;
}

bool PatchResourceHeapSize64(uint64_t resourceBytes) {
    uintptr_t match = Memory::FindPattern(kResourceSizePattern, kResourceSizeMask);
    if (!match) {
        Logger::Log("Failed to find resource size pattern");
        return false;
    }

    uintptr_t baseAnchor = match + 0x08;
    Logger::Log("Overwriting resource size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(baseAnchor, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload:
    // +0x00: 49 B8 <8-byte resourceBytes>      ; movabs r8, resourceBytes
    // +0x0A: 48 B8 <8-byte (baseAnchor + 0xA)> ; movabs rax, baseAnchor + 0xA
    // +0x14: FF E0                             ; jmp rax
    uint8_t code[22];
    code[0] = 0x49;
    code[1] = 0xB8;
    std::memcpy(&code[2], &resourceBytes, sizeof(uint64_t));
    code[10] = 0x48;
    code[11] = 0xB8;
    uint64_t retAddr = baseAnchor + 0x0A;
    std::memcpy(&code[12], &retAddr, sizeof(uint64_t));
    code[20] = 0xFF;
    code[21] = 0xE0;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(baseAnchor + 4, trampoline, baseAnchor - 0xFFB, 0);
    return true;
}

// ---------------------------------------------------------------------------
// 4. Enhanced Ped & Vehicle Memory Budget (scans all instances of `05 60 E3 16 00`)
// ---------------------------------------------------------------------------
void PatchPedMemoryBudget(uint32_t pedMemBytes) {
    Memory::ModuleRegion region = Memory::GetMainModuleRegion();
    if (!region.base || !region.size) {
        return;
    }

    static constexpr const char* kBudgetPattern = "\x05\x60\xE3\x16\x00";
    static constexpr const char* kBudgetMask = "xxxxx";

    uintptr_t cur = region.base;
    const uintptr_t end = region.base + region.size;
    int instanceIndex = 1;
    int foundCount = 0;

    while (cur < end) {
        uintptr_t match = Memory::FindPatternRange(cur, end, kBudgetPattern, kBudgetMask);
        if (!match) {
            break;
        }

        uintptr_t immAddr = match + 1;
        cur = match + 1;

        Logger::Log("Overwriting ped memory budget instance " + std::to_string(instanceIndex) + " - OK");
        Memory::WriteU32(immAddr, pedMemBytes);

        ++foundCount;
        ++instanceIndex;
    }

    if (foundCount == 0) {
        Logger::Log("Failed to find ped memory budget pattern");
    }
}

// ---------------------------------------------------------------------------
// 5. Enhanced Skip Peds Pathfind Pass
// Pattern: 48 89 44 24 30 89 5C 24 20 48 8D 0D
// Mask:    xxxxxxxxxxxx
// ---------------------------------------------------------------------------
void PatchSkipPedsPathfindPass() {
    static constexpr const char* kPathfindPattern =
        "\x48\x89\x44\x24\x30\x89\x5C\x24\x20\x48\x8D\x0D";
    static constexpr const char* kPathfindMask = "xxxxxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPathfindPattern, kPathfindMask);
    if (!match) {
        Logger::Log("Failed to find peds pathfind pass pattern");
        return;
    }

    Logger::Log("Overwriting peds pathfind pass - OK");
    // 31 DB 90 90 90 -> xor ebx, ebx; nop; nop; nop
    const uint8_t patch[5] = { 0x31, 0xDB, 0x90, 0x90, 0x90 };
    Memory::PatchBytes(match, patch, sizeof(patch));
}

// ---------------------------------------------------------------------------
// 6. Enhanced Spawn Multipliers (Ped Ambient, Ped Scenario, Veh Ambient, Veh Parked)
// ---------------------------------------------------------------------------
void PatchPedSpawnAmbientMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x28\x48\x83\xC1\x2C\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find ped spawn ambient pattern");
        return;
    }

    Logger::Log("Overwriting ped spawn ambient rate - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(multiplier * 100));
}

void PatchPedSpawnScenarioMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x20\x48\x83\xC1\x24\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find ped spawn scenario pattern");
        return;
    }

    Logger::Log("Overwriting ped spawn scenario rate - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(multiplier * 100));
}

void PatchVehSpawnAmbientMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x4C\x48\x83\xC1\x50\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find veh spawn ambient pattern");
        return;
    }

    Logger::Log("Overwriting veh spawn ambient rate - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(multiplier * 100));
}

void PatchVehSpawnParkedMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x58\x48\x83\xC1\x5C\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find veh spawn parked pattern");
        return;
    }

    Logger::Log("Overwriting veh spawn parked rate - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(multiplier * 100));
}

// ---------------------------------------------------------------------------
// 7. Enhanced Max Peds & Max Vehicles Spawn Caps
// ---------------------------------------------------------------------------
void PatchMaxPeds(int32_t maxPeds) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x30\x48\x83\xC1\x34\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max peds pattern");
        return;
    }

    Logger::Log("Overwriting max peds - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(maxPeds));
}

void PatchMaxVehAmbient(int32_t maxVehAmbient) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x68\x48\x83\xC1\x6C\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max veh ambient pattern");
        return;
    }

    Logger::Log("Overwriting max veh ambient - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(maxVehAmbient));
}

void PatchMaxVehParked(int32_t maxVehParked) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x74\x48\x83\xC1\x78\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max veh parked pattern");
        return;
    }

    Logger::Log("Overwriting max veh parked - OK");
    Memory::WriteMovRaxImm64Nop3(match, static_cast<int64_t>(maxVehParked));
}

// ---------------------------------------------------------------------------
// 8. Enhanced Ped Pool Limit (> 256) Engine Fixes
// ---------------------------------------------------------------------------
void PatchPedPoolLimit(int32_t pedLimit) {
    // Instance 1
    static constexpr const char* kPattern1 =
        "\xE8\x00\x00\x00\x00\x48\x89\xF9\xBA\x00\x00\x00\x00\x41\xB8\x01\x00\x00\x00\xE8\x00\x00\x00\x00\x89\xC1\xE8";
    static constexpr const char* kMask1 = "x????xxxx????xxxxxxx????xx?";

    uintptr_t match1 = Memory::FindPattern(kPattern1, kMask1);
    if (!match1) {
        Logger::Log("Failed to find ped limit pattern instance 1");
        return;
    }

    uintptr_t target1 = match1 + 0x13;
    Logger::Log("Overwriting ped limit instance 1 - OK");
    Memory::WriteMovEaxImm32(target1, static_cast<uint32_t>(pedLimit));

    // Instance 2
    static constexpr const char* kPattern2 =
        "\xB9\x30\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x89\xC6\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xE8";
    static constexpr const char* kMask2 = "xxxxxx????xxx????????????x";

    uintptr_t match2 = Memory::FindPattern(kPattern2, kMask2);
    if (!match2) {
        Logger::Log("Failed to find ped limit pattern instance 2");
        return;
    }

    uintptr_t target2 = match2 + 0x19;
    Logger::Log("Overwriting ped limit instance 2 - OK");
    Memory::WriteMovEaxImm32(target2, static_cast<uint32_t>(pedLimit));
}

void PatchLimitFixInstance1(int32_t pedLimit) {
    Memory::ModuleRegion region = Memory::GetMainModuleRegion();
    if (!region.base || !region.size) {
        return;
    }

    const int32_t extraStack = (pedLimit - kDefaultPedPoolLimit) * 16;
    const uintptr_t end = region.base + region.size;

    static constexpr const char* kPattern1 =
        "\xB8\x48\x10\x00\x00\xE8\x00\x00\x00\x00\x48\x29\xC4\x48\x8B\x05\x00\x00\x00\x00\x48\x63\x70\x5C";
    static constexpr const char* kMask1 = "xxxxxx????xxxxxx????xxxx";

    uintptr_t match1 = Memory::FindPatternRange(region.base, end, kPattern1, kMask1);
    if (!match1) {
        Logger::Log("Failed to find limit fix instance 1 pattern");
        return;
    }

    uintptr_t target1 = match1 + 1;
    Logger::Log("Overwriting limit fix instance 1 - OK");
    Memory::AddU32(target1, extraStack);

    // Scan forward from match1 + 1 for `add rsp, 0x1048` (48 81 C4 48 10 00 00)
    static constexpr const char* kPattern1_1 = "\x48\x81\xC4\x48\x10\x00\x00";
    static constexpr const char* kMask1_1 = "xxxxxxx";

    uintptr_t match1_1 = Memory::FindPatternRange(match1 + 1, end, kPattern1_1, kMask1_1);
    if (!match1_1) {
        Logger::Log("Failed to find limit fix instance 1_1 pattern");
        return;
    }

    uintptr_t target1_1 = match1_1 + 3;
    Logger::Log("Overwriting limit fix instance 1_1 - OK");
    Memory::AddU32(target1_1, extraStack);
}

void PatchLimitFixInstance2(int32_t pedLimit) {
    Memory::ModuleRegion region = Memory::GetMainModuleRegion();
    if (!region.base || !region.size) {
        return;
    }

    const int32_t extra12 = (pedLimit - kDefaultPedPoolLimit) * 12;
    const int32_t extra8 = (pedLimit - kDefaultPedPoolLimit) * 8;
    const uintptr_t end = region.base + region.size;

    static constexpr const char* kPattern2 = "\x41\x56\x56\x57\x53\x48\x81\xEC\x28\x0C\x00";
    static constexpr const char* kMask2 = "xxxxxxxxxxx";

    uintptr_t match2 = Memory::FindPatternRange(region.base, end, kPattern2, kMask2);
    if (!match2) {
        Logger::Log("Failed to find limit fix instance 2 pattern");
        return;
    }

    uintptr_t target2 = match2 + 8; // points to 0x00000C28 in `sub rsp, 0xC28`
    Logger::Log("Overwriting limit fix instance 2 - OK");
    Memory::AddU32(target2, extra12);

    uintptr_t cur = match2 + 1;
    static constexpr const char* kImm820 = "\x20\x08\x00\x00";
    static constexpr const char* kImm420 = "\x20\x04\x00\x00";
    static constexpr const char* kMask4 = "xxxx";

    // Adjust the two pairs of (0x820, 0x420) buffer/stack offsets inside the function
    for (int pairIdx = 0; pairIdx < 2; ++pairIdx) {
        uintptr_t pos820 = Memory::FindPatternRange(cur, end, kImm820, kMask4);
        uintptr_t pos420 = 0;
        if (pos820) {
            cur = pos820 + 1;
            pos420 = Memory::FindPatternRange(cur, end, kImm420, kMask4);
            if (pos420) {
                cur = pos420 + 1;
            }
        }
        if (pos420 && pos820) {
            Memory::AddU32(pos420, extra8);
            Memory::AddU32(pos820, extra12);
        }
    }

    // Scan forward for `add rsp, 0xC28` (48 81 C4 28 0C 00 00)
    static constexpr const char* kPattern2_1 = "\x48\x81\xC4\x28\x0C\x00\x00";
    static constexpr const char* kMask2_1 = "xxxxxxx";

    uintptr_t match2_1 = Memory::FindPatternRange(cur, end, kPattern2_1, kMask2_1);
    if (!match2_1) {
        Logger::Log("Failed to find limit fix instance 2_1 pattern");
        return;
    }

    uintptr_t target2_1 = match2_1 + 3;
    Logger::Log("Overwriting limit fix instance 2_1 - OK");
    Memory::AddU32(target2_1, extra12);
}

void PatchLimitFixInstance3(int32_t pedLimit) {
    static constexpr const char* kPattern3 = "\xBA\x56\x44\xA5\x8D\x41\xB8\x01\x00\x00\x00";
    static constexpr const char* kMask3 = "xxxxxxxxxxx";

    uintptr_t match3 = Memory::FindPattern(kPattern3, kMask3);
    if (!match3) {
        Logger::Log("Failed to find limit fix instance 3 pattern");
        return;
    }

    uintptr_t target3 = match3 + 0x0B;
    Logger::Log("Overwriting limit fix instance 3 - OK");
    Memory::WriteMovEaxImm32(target3, static_cast<uint32_t>(pedLimit));
}

void PatchLimitFixInstance4(uint32_t value) {
    static constexpr const char* kPattern4 = "\x75\xEF\xEB\x20\x8B\x40\x04\x83\xF8\xFF";
    static constexpr const char* kMask4 = "xxxxxxxxxx";

    uintptr_t match4 = Memory::FindPattern(kPattern4, kMask4);
    if (!match4 || match4 < 0x0C) {
        Logger::Log("Failed to find limit fix instance 4 pattern");
        return;
    }

    uintptr_t target4 = match4 - 0x0C;
    Logger::Log("Overwriting limit fix instance 4 - OK");

    uint8_t patch[8];
    patch[0] = 0xB8; // mov eax, imm32
    std::memcpy(&patch[1], &value, sizeof(uint32_t));
    patch[5] = 0x90; // nop
    patch[6] = 0x90; // nop
    patch[7] = 0x90; // nop
    Memory::PatchBytes(target4, patch, sizeof(patch));
}

// ===========================================================================
// GTA V LEGACY EDITION (GTA5.exe) PATCHERS
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. Legacy Main Heap Size (32-bit & 64-bit)
// Pattern: 48 8D 0D ?? ?? ?? ?? 41 B1 01 45 33 C0 BA
// Mask:    xxx????xxxxxxx
// ---------------------------------------------------------------------------
bool PatchLegacyMainHeapSize32(uint32_t heapBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyHeapSizePattern, kLegacyHeapSizeMask);
    if (!match) {
        Logger::Log("Failed to find heap size pattern");
        return false;
    }

    uintptr_t immAddr = match + 0x0E;
    Logger::Log("Overwriting heap size - OK");
    Memory::WriteU32(immAddr, heapBytes);
    return true;
}

bool PatchLegacyMainHeapSize64(uint64_t heapBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyHeapSizePattern, kLegacyHeapSizeMask);
    if (!match) {
        Logger::Log("Failed to find heap size pattern");
        return false;
    }

    uintptr_t jmpSite = match + 0x0A; // points to `45 33 C0 BA xx xx xx xx` (8 bytes)
    Logger::Log("Overwriting heap size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(jmpSite, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload (matches Legacy v1.1 register conventions):
    // +0x00: 48 BA <8-byte heapBytes>     ; movabs rdx, heapBytes
    // +0x0A: 45 31 C0                     ; xor r8d, r8d
    // +0x0D: 48 BB <8-byte (jmpSite + 8)> ; movabs rbx, jmpSite + 8
    // +0x17: FF E3                        ; jmp rbx
    uint8_t code[25];
    code[0] = 0x48;
    code[1] = 0xBA;
    std::memcpy(&code[2], &heapBytes, sizeof(uint64_t));
    code[10] = 0x45;
    code[11] = 0x31;
    code[12] = 0xC0;
    code[13] = 0x48;
    code[14] = 0xBB;
    uint64_t retAddr = jmpSite + 8;
    std::memcpy(&code[15], &retAddr, sizeof(uint64_t));
    code[23] = 0xFF;
    code[24] = 0xE3;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(jmpSite, trampoline, jmpSite - 0x1FF, 2);
    return true;
}

// ---------------------------------------------------------------------------
// 2. Legacy Pool Heap Size (32-bit & 64-bit)
// Pattern: 83 C8 04 48 8D 0D ?? ?? ?? ?? 41 B8
// Mask:    xxxxxx????xx
// ---------------------------------------------------------------------------
static constexpr const char* kLegacyPoolSizePattern =
    "\x83\xC8\x04\x48\x8D\x0D\x00\x00\x00\x00\x41\xB8";
static constexpr const char* kLegacyPoolSizeMask = "xxxxxx????xx";

bool PatchLegacyPoolHeapSize32(uint32_t poolBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyPoolSizePattern, kLegacyPoolSizeMask);
    if (!match) {
        Logger::Log("Failed to find pool size pattern");
        return false;
    }

    uintptr_t immAddr = match + 0x0C;
    Logger::Log("Overwriting pool size - OK");
    Memory::WriteU32(immAddr, poolBytes);
    return true;
}

bool PatchLegacyPoolHeapSize64(uint64_t poolBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyPoolSizePattern, kLegacyPoolSizeMask);
    if (!match) {
        Logger::Log("Failed to find pool size pattern");
        return false;
    }

    uintptr_t jmpSite = match + 0x0A; // points to `41 B8 xx xx xx xx` (6 bytes)
    Logger::Log("Overwriting pool size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(jmpSite, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload:
    // +0x00: 49 B8 <8-byte poolBytes>     ; movabs r8, poolBytes
    // +0x0A: 48 BB <8-byte (jmpSite + 6)> ; movabs rbx, jmpSite + 6
    // +0x14: FF E3                        ; jmp rbx
    uint8_t code[22];
    code[0] = 0x49;
    code[1] = 0xB8;
    std::memcpy(&code[2], &poolBytes, sizeof(uint64_t));
    code[10] = 0x48;
    code[11] = 0xBB;
    uint64_t retAddr = jmpSite + 6;
    std::memcpy(&code[12], &retAddr, sizeof(uint64_t));
    code[20] = 0xFF;
    code[21] = 0xE3;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(jmpSite, trampoline, jmpSite - 0x1FF, 0);
    return true;
}

// ---------------------------------------------------------------------------
// 3. Legacy Resource Heap Size (32-bit & 64-bit)
// Pattern: 83 C8 20 48 8D 0D ?? ?? ?? ?? 41 B8
// Mask:    xxxxxx????xx
// ---------------------------------------------------------------------------
static constexpr const char* kLegacyResourceSizePattern =
    "\x83\xC8\x20\x48\x8D\x0D\x00\x00\x00\x00\x41\xB8";
static constexpr const char* kLegacyResourceSizeMask = "xxxxxx????xx";

bool PatchLegacyResourceHeapSize32(uint32_t resourceBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyResourceSizePattern, kLegacyResourceSizeMask);
    if (!match) {
        Logger::Log("Failed to find resource size pattern");
        return false;
    }

    uintptr_t immAddr = match + 0x0C;
    Logger::Log("Overwriting resource size - OK");
    Memory::WriteU32(immAddr, resourceBytes);
    return true;
}

bool PatchLegacyResourceHeapSize64(uint64_t resourceBytes) {
    uintptr_t match = Memory::FindPattern(kLegacyResourceSizePattern, kLegacyResourceSizeMask);
    if (!match) {
        Logger::Log("Failed to find resource size pattern");
        return false;
    }

    uintptr_t jmpSite = match + 0x0A; // points to `41 B8 xx xx xx xx` (6 bytes)
    Logger::Log("Overwriting resource size - OK");

    uint8_t* trampoline = Memory::AllocateTrampolineNear(jmpSite, 0x1000);
    if (!trampoline) {
        return false;
    }

    // Trampoline payload:
    // +0x00: 49 B8 <8-byte resourceBytes> ; movabs r8, resourceBytes
    // +0x0A: 48 BA <8-byte (jmpSite + 6)> ; movabs rdx, jmpSite + 6
    // +0x14: FF E2                        ; jmp rdx
    uint8_t code[22];
    code[0] = 0x49;
    code[1] = 0xB8;
    std::memcpy(&code[2], &resourceBytes, sizeof(uint64_t));
    code[10] = 0x48;
    code[11] = 0xBA;
    uint64_t retAddr = jmpSite + 6;
    std::memcpy(&code[12], &retAddr, sizeof(uint64_t));
    code[20] = 0xFF;
    code[21] = 0xE2;

    Memory::PatchBytes(reinterpret_cast<uintptr_t>(trampoline), code, sizeof(code));
    Memory::WriteIndirectJump64(jmpSite, trampoline, jmpSite - 0x1FF, 0);
    return true;
}

// ---------------------------------------------------------------------------
// 4. Legacy Ped Memory Budget
// Pattern: 41 BE 60 E3 16 00 (`mov r14d, 0x16E360`)
// Mask:    xxxxxx
// ---------------------------------------------------------------------------
void PatchLegacyPedMemoryBudget(uint32_t pedMemBytes) {
    static constexpr const char* kBudgetPattern = "\x41\xBE\x60\xE3\x16\x00";
    static constexpr const char* kBudgetMask = "xxxxxx";

    uintptr_t match = Memory::FindPattern(kBudgetPattern, kBudgetMask);
    if (!match) {
        Logger::Log("Failed to find ped memory budget pattern");
        return;
    }

    Logger::Log("Overwriting ped memory budget - OK");
    Memory::WriteU32(match + 2, pedMemBytes);
}

// ---------------------------------------------------------------------------
// 5. Legacy Skip Peds Pathfind Pass
// Pattern: 48 8B 05 ?? ?? ?? ?? 4C 8B C7 48 ?? ?? ?? 30
// Mask:    xxx????xxxx???x
// ---------------------------------------------------------------------------
void PatchLegacySkipPedsPathfindPass() {
    static constexpr const char* kPathfindPattern =
        "\x48\x8B\x05\x00\x00\x00\x00\x4C\x8B\xC7\x48\x00\x00\x00\x30";
    static constexpr const char* kPathfindMask = "xxx????xxxx???x";

    uintptr_t match = Memory::FindPattern(kPathfindPattern, kPathfindMask);
    if (!match) {
        Logger::Log("Failed to find peds pathfind pass pattern");
        return;
    }

    Logger::Log("Overwriting peds pathfind pass - OK");
    // 4D 31 D2 90 90 -> xor r10, r10; nop; nop (protects all 5 bytes, fixing Legacy's 3-byte VirtualProtect bug)
    const uint8_t patch[5] = { 0x4D, 0x31, 0xD2, 0x90, 0x90 };
    Memory::PatchBytes(match + 0x0A, patch, sizeof(patch));
}

// ---------------------------------------------------------------------------
// 6. Legacy Spawn Multipliers (overwrites the 5-byte `E8` call with `B8 <imm32>`)
// ---------------------------------------------------------------------------
void PatchLegacyPedSpawnAmbientMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x83\xC1\x2C\xF3\x44\x0F\x59\xD6\xE8";
    static constexpr const char* kMask = "xxxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find ped spawn ambient pattern");
        return;
    }

    Logger::Log("Overwriting ped spawn ambient rate - OK");
    Memory::WriteMovEaxImm32(match + 9, static_cast<uint32_t>(multiplier * 100));
}

void PatchLegacyPedSpawnScenarioMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x20\x48\x83\xC1\x24\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find ped spawn scenario pattern");
        return;
    }

    Logger::Log("Overwriting ped spawn scenario rate - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(multiplier * 100));
}

void PatchLegacyVehSpawnAmbientMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x4C\x48\x83\xC1\x50\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find veh spawn ambient pattern");
        return;
    }

    Logger::Log("Overwriting veh spawn ambient rate - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(multiplier * 100));
}

void PatchLegacyVehSpawnParkedMultiplier(int32_t multiplier) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x58\x48\x83\xC1\x5C\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find veh spawn parked pattern");
        return;
    }

    Logger::Log("Overwriting veh spawn parked rate - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(multiplier * 100));
}

// ---------------------------------------------------------------------------
// 7. Legacy Max Peds & Max Vehicles Spawn Caps
// ---------------------------------------------------------------------------
void PatchLegacyMaxPeds(int32_t maxPeds) {
    static constexpr const char* kPattern = "\xF3\x0F\x59\xC2\xF3\x0F\x2C\xD8\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max peds pattern");
        return;
    }

    Logger::Log("Overwriting max peds - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(maxPeds));
}

void PatchLegacyMaxVehAmbient(int32_t maxVehAmbient) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x68\x48\x83\xC1\x6C\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max veh ambient pattern");
        return;
    }

    Logger::Log("Overwriting max veh ambient - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(maxVehAmbient));
}

void PatchLegacyMaxVehParked(int32_t maxVehParked) {
    static constexpr const char* kPattern = "\x48\x8D\x51\x74\x48\x83\xC1\x78\xE8";
    static constexpr const char* kMask = "xxxxxxxxx";

    uintptr_t match = Memory::FindPattern(kPattern, kMask);
    if (!match) {
        Logger::Log("Failed to find max veh parked pattern");
        return;
    }

    Logger::Log("Overwriting max veh parked - OK");
    Memory::WriteMovEaxImm32(match + 8, static_cast<uint32_t>(maxVehParked));
}

// ===========================================================================
//Main Initialization Sequence (Universal: Enhanced + Legacy)
// ===========================================================================
void Initialize() {
    Logger::Init("XtremeTool.log");
    Config cfg = Config::Load(L".\\XtremeTool.ini");

    Edition edition = DetectEdition();
    if (edition == Edition::Unknown) {
        Logger::Log("Unsupported executable: neither GTA5_Enhanced.exe nor GTA5.exe was detected.");
        return;
    }

    if (edition == Edition::Enhanced && cfg.pedLimit > kMaxSafePedPoolLimit) {
        Logger::Log("Config Error: Ped pool limit is 340 due to technical limitations (stack size of GTA 5 main thread)");
        cfg.pedLimit = kMaxSafePedPoolLimit;
    }

    uintptr_t gameBase = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));

    Logger::Log(std::string("XtremeFork-Universal ") + kVersionString + " (based on XtremeTool by XtremeV)");
    Logger::Log(std::string("Detected Edition: ") +
                (edition == Edition::Enhanced ? "GTA V Enhanced (GTA5_Enhanced.exe)" : "GTA V Legacy (GTA5.exe)"));
    Logger::Log("Loaded - Game Base: " + std::to_string(gameBase));
    Logger::Log("Ped Spawn Ambient Multiplier: " + std::to_string(cfg.pedSpawnAmbientMultiplier));
    Logger::Log("Ped Spawn Scenario Multiplier: " + std::to_string(cfg.pedSpawnScenarioMultiplier));
    Logger::Log("Veh Spawn Ambient Multiplier: " + std::to_string(cfg.vehSpawnAmbientMultiplier));
    Logger::Log("Veh Spawn Parked Multiplier: " + std::to_string(cfg.vehSpawnParkedMultiplier));
    Logger::Log(std::string("Use 64 Bit Main Heap Size: ") + (cfg.use64BitHeapSize ? "Yes" : "No"));
    Logger::Log("Main Heap Size: " + std::to_string(cfg.heapSize) + " MB");
    Logger::Log(std::string("Use 64 Bit Pool Heap Size: ") + (cfg.use64BitPoolSize ? "Yes" : "No"));
    Logger::Log("Pool Heap Size: " + std::to_string(cfg.poolSize) + " MB");
    Logger::Log(std::string("Use 64 Bit Resource Heap Size: ") + (cfg.use64BitResourceSize ? "Yes" : "No"));
    Logger::Log("Resource Heap Size: " + std::to_string(cfg.resourceSize) + " MB");
    Logger::Log("Ped Mem Budget: " + std::to_string(cfg.pedMemSize) + " MB");
    Logger::Log("Veh Mem Budget: " + std::to_string(cfg.vehMemSize) + " MB");
    if (edition == Edition::Enhanced) {
        Logger::Log("Ped Limit: " + std::to_string(cfg.pedLimit));
        Logger::Log("Veh Limit: " + std::to_string(cfg.vehLimit));
    }
    Logger::Log(std::string("Skip Peds Pathfind Pass: ") + (cfg.skipPedsPathfindPass ? "Yes" : "No"));

    // Convert MB to bytes (<< 20) using 64-bit arithmetic to avoid 32-bit shift wrap-around
    uint64_t heapBytes = static_cast<uint64_t>(cfg.heapSize) << 20;
    uint64_t poolBytes = static_cast<uint64_t>(cfg.poolSize) << 20;
    uint64_t resourceBytes = static_cast<uint64_t>(cfg.resourceSize) << 20;
    uint32_t pedMemBytes = static_cast<uint32_t>(cfg.pedMemSize) << 20;

    if (cfg.poolSize >= cfg.heapSize) {
        poolBytes = heapBytes / 2;
        Logger::Log("Pool Size is bigger than heap size, setting to " + std::to_string(poolBytes >> 20) + " MB");
    }

    if (edition == Edition::Enhanced) {
        // 1. Main Heap Size
        if (cfg.use64BitHeapSize) {
            PatchMainHeapSize64(heapBytes);
        } else {
            PatchMainHeapSize32(static_cast<uint32_t>(heapBytes));
        }

        // 2. Pool Heap Size
        if (cfg.use64BitPoolSize) {
            PatchPoolHeapSize64(poolBytes);
        } else {
            PatchPoolHeapSize32(static_cast<uint32_t>(poolBytes));
        }

        // 3. Resource Heap Size
        if (cfg.use64BitResourceSize) {
            PatchResourceHeapSize64(resourceBytes);
        } else {
            PatchResourceHeapSize32(static_cast<uint32_t>(resourceBytes));
        }

        // 4. Ped & Vehicle Memory Budget
        PatchPedMemoryBudget(pedMemBytes);

        // 5. Optional Pathfind Pass Skip
        if (cfg.skipPedsPathfindPass) {
            PatchSkipPedsPathfindPass();
        }

        // 6. Spawn Multipliers
        if (cfg.pedSpawnAmbientMultiplier != 1) {
            PatchPedSpawnAmbientMultiplier(cfg.pedSpawnAmbientMultiplier);
        }
        if (cfg.pedSpawnScenarioMultiplier != 1) {
            PatchPedSpawnScenarioMultiplier(cfg.pedSpawnScenarioMultiplier);
        }
        if (cfg.vehSpawnAmbientMultiplier != 1) {
            PatchVehSpawnAmbientMultiplier(cfg.vehSpawnAmbientMultiplier);
        }
        if (cfg.vehSpawnParkedMultiplier != 1) {
            PatchVehSpawnParkedMultiplier(cfg.vehSpawnParkedMultiplier);
        }

        // 7. Max Peds & Vehicles Spawn Caps
        PatchMaxPeds(cfg.pedLimit - 10);
        PatchMaxVehAmbient(cfg.vehLimit - 10);
        PatchMaxVehParked(cfg.vehLimit - 20);

        // 8. Ped Pool Limit (> 256) Fixes
        if (cfg.pedLimit != kDefaultPedPoolLimit) {
            PatchPedPoolLimit(cfg.pedLimit);
            PatchLimitFixInstance1(cfg.pedLimit);
            PatchLimitFixInstance2(cfg.pedLimit);
            PatchLimitFixInstance3(cfg.pedLimit);
        }

        // 9. Limit Fix Instance 4
        PatchLimitFixInstance4(kLimitFixInstance4Value);
    } else {
        // ===================================================================
        // GTA V Legacy (GTA5.exe) Initialization Sequence
        // Note: In Legacy, sizes <= 2047 MB fit cleanly into the native 32-bit
        // immediate operand without needing a trampoline jump; larger sizes
        // (or explicit 64-bit mode > 2047 MB) use our safe near-allocated trampoline.
        // ===================================================================
        const bool useHeap64 = cfg.use64BitHeapSize && (heapBytes > 0x7FFFFFFFULL);
        const bool usePool64 = cfg.use64BitPoolSize && (poolBytes > 0x7FFFFFFFULL);
        const bool useRes64  = cfg.use64BitResourceSize && (resourceBytes > 0x7FFFFFFFULL);

        // 1. Main Heap Size
        if (useHeap64) {
            if (!PatchLegacyMainHeapSize64(heapBytes)) return;
        } else {
            if (!PatchLegacyMainHeapSize32(static_cast<uint32_t>(heapBytes))) return;
        }

        // 2. Pool Heap Size
        if (usePool64) {
            if (!PatchLegacyPoolHeapSize64(poolBytes)) return;
        } else {
            if (!PatchLegacyPoolHeapSize32(static_cast<uint32_t>(poolBytes))) return;
        }

        // 3. Resource Heap Size
        if (useRes64) {
            if (!PatchLegacyResourceHeapSize64(resourceBytes)) return;
        } else {
            if (!PatchLegacyResourceHeapSize32(static_cast<uint32_t>(resourceBytes))) return;
        }

        // 4. Ped Memory Budget
        PatchLegacyPedMemoryBudget(pedMemBytes);

        // 5. Optional Pathfind Pass Skip
        if (cfg.skipPedsPathfindPass) {
            PatchLegacySkipPedsPathfindPass();
        }

        // 6. Spawn Multipliers
        if (cfg.pedSpawnAmbientMultiplier != 1) {
            PatchLegacyPedSpawnAmbientMultiplier(cfg.pedSpawnAmbientMultiplier);
        }
        if (cfg.pedSpawnScenarioMultiplier != 1) {
            PatchLegacyPedSpawnScenarioMultiplier(cfg.pedSpawnScenarioMultiplier);
        }
        if (cfg.vehSpawnAmbientMultiplier != 1) {
            PatchLegacyVehSpawnAmbientMultiplier(cfg.vehSpawnAmbientMultiplier);
        }
        if (cfg.vehSpawnParkedMultiplier != 1) {
            PatchLegacyVehSpawnParkedMultiplier(cfg.vehSpawnParkedMultiplier);
        }

        // 7. Max Peds & Vehicles Spawn Caps (1000 default in Legacy, or scaled if higher)
        const int32_t legacyMaxPeds = (std::max)(kLegacyDefaultMaxSpawnCap, cfg.pedLimit - 10);
        const int32_t legacyMaxVehAmbient = (std::max)(kLegacyDefaultMaxSpawnCap, cfg.vehLimit - 10);
        const int32_t legacyMaxVehParked = (std::max)(kLegacyDefaultMaxSpawnCap, cfg.vehLimit - 20);

        PatchLegacyMaxPeds(legacyMaxPeds);
        PatchLegacyMaxVehAmbient(legacyMaxVehAmbient);
        PatchLegacyMaxVehParked(legacyMaxVehParked);
    }
}

} // namespace Game
