#pragma once

#include <cstdint>
#include <string>

namespace Game {

enum class Edition {
    Unknown = 0,
    Enhanced, // GTA5_Enhanced.exe (Gen9 / Enhanced Edition)
    Legacy    // GTA5.exe (Gen8 / Legacy Edition)
};

inline constexpr const char* kVersionString = "v1.2 (Enhanced & Legacy)";
inline constexpr const char* kEnhancedExecutable = "GTA5_Enhanced.exe";
inline constexpr const char* kLegacyExecutable = "GTA5.exe";
inline constexpr int32_t kDefaultPedPoolLimit = 256;
inline constexpr int32_t kMaxSafePedPoolLimit = 340;
inline constexpr int32_t kLegacyDefaultMaxSpawnCap = 1000;
inline constexpr uint32_t kLimitFixInstance4Value = 240000; // 0x3A980

// Detects whether the current host process is GTA5_Enhanced.exe or GTA5.exe
// (with signature fallback if the executable was renamed).
Edition DetectEdition();
bool IsEnhancedEdition();

// Runs XtremeFork-Universal configuration, logging, and edition-specific memory patches.
void Initialize();

// ===========================================================================
// GTA V Enhanced Edition (GTA5_Enhanced.exe) Patchers
// ===========================================================================
bool PatchMainHeapSize32(uint32_t heapBytes);
bool PatchMainHeapSize64(uint64_t heapBytes);

bool PatchPoolHeapSize32(uint32_t poolBytes);
bool PatchPoolHeapSize64(uint64_t poolBytes);

bool PatchResourceHeapSize32(uint32_t resourceBytes);
bool PatchResourceHeapSize64(uint64_t resourceBytes);

void PatchPedMemoryBudget(uint32_t pedMemBytes);
void PatchSkipPedsPathfindPass();

void PatchPedSpawnAmbientMultiplier(int32_t multiplier);
void PatchPedSpawnScenarioMultiplier(int32_t multiplier);
void PatchVehSpawnAmbientMultiplier(int32_t multiplier);
void PatchVehSpawnParkedMultiplier(int32_t multiplier);

void PatchMaxPeds(int32_t maxPeds);
void PatchMaxVehAmbient(int32_t maxVehAmbient);
void PatchMaxVehParked(int32_t maxVehParked);

void PatchPedPoolLimit(int32_t pedLimit);
void PatchLimitFixInstance1(int32_t pedLimit);
void PatchLimitFixInstance2(int32_t pedLimit);
void PatchLimitFixInstance3(int32_t pedLimit);
void PatchLimitFixInstance4(uint32_t value);

// ===========================================================================
// GTA V Legacy Edition (GTA5.exe) Patchers
// ===========================================================================
bool PatchLegacyMainHeapSize32(uint32_t heapBytes);
bool PatchLegacyMainHeapSize64(uint64_t heapBytes);

bool PatchLegacyPoolHeapSize32(uint32_t poolBytes);
bool PatchLegacyPoolHeapSize64(uint64_t poolBytes);

bool PatchLegacyResourceHeapSize32(uint32_t resourceBytes);
bool PatchLegacyResourceHeapSize64(uint64_t resourceBytes);

void PatchLegacyPedMemoryBudget(uint32_t pedMemBytes);
void PatchLegacySkipPedsPathfindPass();

void PatchLegacyPedSpawnAmbientMultiplier(int32_t multiplier);
void PatchLegacyPedSpawnScenarioMultiplier(int32_t multiplier);
void PatchLegacyVehSpawnAmbientMultiplier(int32_t multiplier);
void PatchLegacyVehSpawnParkedMultiplier(int32_t multiplier);

void PatchLegacyMaxPeds(int32_t maxPeds);
void PatchLegacyMaxVehAmbient(int32_t maxVehAmbient);
void PatchLegacyMaxVehParked(int32_t maxVehParked);

} // namespace Game
