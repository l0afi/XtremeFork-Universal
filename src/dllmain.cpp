#include <windows.h>
#include "Game.h"

#ifdef USE_SCRIPTHOOKV_REGISTER
#include "../inc/main.h"

static void ScriptMain() {
    while (true) {
        WAIT(0);
    }
}
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        // Apply engine heap, pool, resource, and spawn patches immediately on ASI load
        // before GTA5_Enhanced.exe initializes its memory allocators and pools.
        Game::Initialize();
#ifdef USE_SCRIPTHOOKV_REGISTER
        scriptRegister(hModule, ScriptMain);
#endif
        break;

    case DLL_PROCESS_DETACH:
#ifdef USE_SCRIPTHOOKV_REGISTER
        scriptUnregister(hModule);
#endif
        break;
    }
    return TRUE;
}
