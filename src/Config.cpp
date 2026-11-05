#include "Config.h"
#include <windows.h>

Config Config::Load(const wchar_t* iniPath) {
    Config cfg{};

    cfg.use64BitHeapSize = GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"USE_64BIT_HEAP_SIZE", 0, iniPath) != 0;
    cfg.heapSize = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"HEAP_SIZE", 2000, iniPath));

    cfg.use64BitPoolSize = GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"USE_64BIT_POOL_SIZE", 0, iniPath) != 0;
    cfg.poolSize = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"POOL_SIZE", 1000, iniPath));

    cfg.use64BitResourceSize = GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"USE_64BIT_RESOURCE_SIZE", 0, iniPath) != 0;
    cfg.resourceSize = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"RESOURCE_SIZE", 4000, iniPath));

    cfg.pedMemSize = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"PED_MEM_SIZE", 10000000, iniPath));
    cfg.vehMemSize = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"VEH_MEM_SIZE", 10000000, iniPath));

    cfg.pedLimit = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"PED_LIMIT", 256, iniPath));
    cfg.vehLimit = static_cast<int32_t>(GetPrivateProfileIntW(
        L"MEMORY_SETTINGS", L"VEH_LIMIT", 300, iniPath));

    cfg.skipPedsPathfindPass = GetPrivateProfileIntW(
        L"MISC", L"SKIP_PEDS_PATHFIND_PASS", 0, iniPath) != 0;

    cfg.pedSpawnAmbientMultiplier = static_cast<int32_t>(GetPrivateProfileIntW(
        L"SPAWN_SETTINGS", L"PED_SPAWN_AMBIENT_MULTIPLIER", 10, iniPath));
    cfg.pedSpawnScenarioMultiplier = static_cast<int32_t>(GetPrivateProfileIntW(
        L"SPAWN_SETTINGS", L"PED_SPAWN_SCENARIO_MULTIPLIER", 1, iniPath));
    cfg.vehSpawnAmbientMultiplier = static_cast<int32_t>(GetPrivateProfileIntW(
        L"SPAWN_SETTINGS", L"VEH_SPAWN_AMBIENT_MULTIPLIER", 2, iniPath));
    cfg.vehSpawnParkedMultiplier = static_cast<int32_t>(GetPrivateProfileIntW(
        L"SPAWN_SETTINGS", L"VEH_SPAWN_PARKED_MULTIPLIER", 1, iniPath));

    return cfg;
}
