#pragma once

#include <cstdint>

struct Config {
    // [MEMORY_SETTINGS]
    bool use64BitHeapSize = false;
    int32_t heapSize = 2000;

    bool use64BitPoolSize = false;
    int32_t poolSize = 1000;

    bool use64BitResourceSize = false;
    int32_t resourceSize = 4000;

    int32_t pedMemSize = 10000000;
    int32_t vehMemSize = 10000000;

    // [MISC]
    bool skipPedsPathfindPass = false;

    // [SPAWN_SETTINGS]
    int32_t pedSpawnAmbientMultiplier = 10;
    int32_t pedSpawnScenarioMultiplier = 1;
    int32_t vehSpawnAmbientMultiplier = 2;
    int32_t vehSpawnParkedMultiplier = 1;

    // [MEMORY_SETTINGS] Pool limits
    int32_t pedLimit = 256;
    int32_t vehLimit = 300;

    static Config Load(const wchar_t* iniPath = L".\\XtremeTool.ini");
};
