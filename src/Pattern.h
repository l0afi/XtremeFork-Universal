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
// Note: Matches XtremeTool v1.2's VirtualProtect behavior (protects 13 bytes so all written bytes are covered).
void WriteMovRaxImm64Nop3(uintptr_t address, int64_t value);

} // namespace Memory
