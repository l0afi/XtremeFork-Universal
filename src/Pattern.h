#pragma once

#include <cstddef>
#include <cstdint>

namespace Memory {

struct ModuleRegion {
    uintptr_t base = 0;
    size_t size = 0;
};

// Queries the main executable module base and image size via GetModuleInformation.
ModuleRegion GetMainModuleRegion();

// Scans [startAddr, endAddr) for a byte pattern + mask ('x' = exact match, '?' = wildcard).
// Returns 0 if not found.
uintptr_t FindPatternRange(uintptr_t startAddr, uintptr_t endAddr, const char* pattern, const char* mask);

// Scans the entire main executable module for a byte pattern + mask.
// Returns 0 if not found.
uintptr_t FindPattern(const char* pattern, const char* mask);

// Writes arbitrary bytes to target address after temporarily setting PAGE_EXECUTE_READWRITE.
void PatchBytes(uintptr_t address, const void* data, size_t size);

// Writes a 32-bit unsigned integer to target address with VirtualProtect.
void WriteU32(uintptr_t address, uint32_t value);

// Adds a signed 32-bit delta to the 32-bit value at target address with VirtualProtect.
void AddU32(uintptr_t address, int32_t delta);

// Writes `mov eax, imm32` (5 bytes: B8 <imm32>) at target address.
void WriteMovEaxImm32(uintptr_t address, uint32_t value);

// Writes `mov rax, imm64; nop; nop; nop` (13 bytes: 48 B8 <imm64> 90 90 90) at target address.
// Note: Protects all 13 bytes so the entire instruction + NOP padding is safely covered.
void WriteMovRaxImm64Nop3(uintptr_t address, int64_t value);

// Allocates a 4KB executable trampoline page near `anchorAddr` (within +/- 2 GB) via VirtualAlloc
// so RIP-relative jumps (`FF 25 <disp32>`) can store their 8-byte target pointer inside the
// trampoline page (`trampoline + 0x80`) instead of overwriting game `.text` bytes.
// Falls back to malloc + VirtualProtect if no near free region is found.
uint8_t* AllocateTrampolineNear(uintptr_t anchorAddr, size_t size = 0x1000);

// Installs a 6-byte RIP-relative indirect jump (`FF 25 <disp32>`) at `jmpSite` targeting `trampoline`.
// If `trampoline + 0x80` is within +/- 2 GB of `jmpSite + 6`, stores the 8-byte target pointer inside
// `trampoline + 0x80` (zero game code corruption). Otherwise falls back to `fallbackPtrSlot`.
void WriteIndirectJump64(uintptr_t jmpSite, uint8_t* trampoline, uintptr_t fallbackPtrSlot, size_t nopPadBytes = 0);

} // namespace Memory
