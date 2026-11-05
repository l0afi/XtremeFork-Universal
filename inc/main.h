#pragma once

#include <windows.h>

#define IMPORT __declspec(dllimport)

// ScriptHookV SDK main header for GTA V / GTA V Enhanced Edition ASI plugins

/* textures */
IMPORT int createTexture(const char *texFileName);
IMPORT void drawTexture(int id, int index, int level, int time,
	float sizeX, float sizeY, float centerX, float centerY,
	float posX, float posY, float rotation, float screenHeightScaleFactor,
	float r, float g, float b, float a);

/* scripts */
typedef void(*KeyboardHandler)(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
typedef void(*TWndProcFn)(UINT uMsg, WPARAM wParam, LPARAM lParam);

IMPORT void scriptWait(DWORD time);
IMPORT void scriptRegister(HMODULE module, void(*LP_SCRIPT_MAIN)());
IMPORT void scriptRegisterAdditionalThread(HMODULE module, void(*LP_SCRIPT_MAIN)());
IMPORT void scriptUnregister(HMODULE module);
IMPORT void scriptUnregister(void(*LP_SCRIPT_MAIN)()); // deprecated

IMPORT void nativeInit(UINT64 hash);
IMPORT void nativePush64(UINT64 val);
IMPORT PUINT64 nativeCall();

inline void WAIT(DWORD time) { scriptWait(time); }
inline void TERMINATE() { WAIT(MAXDWORD); }

/* keyboard */
IMPORT void keyboardHandlerRegister(KeyboardHandler handler);
IMPORT void keyboardHandlerUnregister(KeyboardHandler handler);

/*features*/
IMPORT BYTE *getScriptHandleBaseAddress(int handle);

enum eGameVersion : int
{
	VER_UNK = -1,
	VER_1_0_335_2_STEAM,
	VER_1_0_335_2_NOSTEAM,
	VER_ENHANCED
};

IMPORT eGameVersion getGameVersion();

/* global variables */
IMPORT UINT64 *getGlobalPtr(int globalId);

/* world pools */
IMPORT int worldGetAllVehicles(int *arr, int arrSize);
IMPORT int worldGetAllPeds(int *arr, int arrSize);
IMPORT int worldGetAllObjects(int *arr, int arrSize);
IMPORT int worldGetAllPickups(int *arr, int arrSize);
