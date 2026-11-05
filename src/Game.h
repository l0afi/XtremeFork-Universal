#pragma once

#include <cstdint>
#include <string>

namespace Game {

inline constexpr const char* kVersionString = "v1.2 Enhanced";
inline constexpr const char* kTargetExecutable = "GTA5_Enhanced.exe";
inline constexpr int32_t kDefaultPedPoolLimit = 256;
inline constexpr int32_t kMaxSafePedPoolLimit = 340;
inline constexpr uint32_t kLimitFixInstance4Value = 240000; // 0x3A980

// Checks whether the current host process is GTA5_Enhanced.exe.
bool IsEnhancedEdition();

// Runs all XtremeTool Enhanced v1.2 configuration, logging, and memory patches.
void Initialize();

// Individual memory patchers matching XtremeTool v1.2 Enhanced
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

} // namespace Game
