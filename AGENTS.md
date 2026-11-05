# AGENTS.md — Technical Memory & Reverse-Engineering Notebook

This document serves as the persistent engineering memory and complete reverse-engineering reference for **XtremeTool Enhanced v1.2** (`XtremeTool.asi` for `GTA5_Enhanced.exe`). It documents how every subsystem works, where every function is located in both our C++20 source tree and the original v1.2 binary disassembly, and all byte patterns, masks, and instruction patches.

---

## 1. Quick Navigation & File Map

### Source Code (`src/` & `inc/`)
| File | Key Symbols | Purpose |
| :--- | :--- | :--- |
| [`src/dllmain.cpp`](src/dllmain.cpp) | `DllMain`, `ScriptMain` | ASI entry point (`DLL_PROCESS_ATTACH`). Calls `Game::Initialize()` immediately before game engine allocators run. Optionally registers with `ScriptHookV.lib` if `USE_SCRIPTHOOKV_REGISTER` is defined. |
| [`src/Config.h`](src/Config.h)<br>[`src/Config.cpp`](src/Config.cpp) | `struct Config`<br>`Config::Load` | Reads all 15 settings from `.\XtremeTool.ini` across `[MEMORY_SETTINGS]`, `[SPAWN_SETTINGS]`, and `[MISC]` via Win32 `GetPrivateProfileIntW`. |
| [`src/Logger.h`](src/Logger.h)<br>[`src/Logger.cpp`](src/Logger.cpp) | `Logger::Init`<br>`Logger::Log` | Opens `XtremeTool.log` (`std::ios::out \| std::ios::trunc`) and flushes each status line (`std::endl`). |
| [`src/Pattern.h`](src/Pattern.h)<br>[`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::GetMainModuleRegion`<br>`Memory::FindPatternRange`<br>`Memory::FindPattern`<br>`Memory::PatchBytes`<br>`Memory::WriteU32`<br>`Memory::AddU32`<br>`Memory::WriteMovEaxImm32`<br>`Memory::WriteMovRaxImm64Nop3` | Queries `GTA5_Enhanced.exe` base address and `SizeOfImage` via `GetModuleInformation`, performs `'x'`/`'?'` mask pattern scanning, and applies `VirtualProtect(PAGE_EXECUTE_READWRITE)` memory patches. |
| [`src/Game.h`](src/Game.h)<br>[`src/Game.cpp`](src/Game.cpp) | `Game::IsEnhancedEdition`<br>`Game::Initialize`<br>`Game::Patch*` (16 functions) | Orchestrates config validation, MB-to-byte conversion (`<< 20`), logging, and all 16 memory signature patches / 64-bit trampolines. |
| [`inc/main.h`](inc/main.h)<br>[`inc/types.h`](inc/types.h) | `scriptRegister`, `scriptUnregister`, `scriptWait`, `WAIT`, `TERMINATE` | Standard ScriptHookV SDK headers for GTA V / GTA V Enhanced Edition. |

### Build & Distribution Files
- [`CMakeLists.txt`](CMakeLists.txt) — CMake build definition producing `XtremeTool.asi`.
- [`XtremeToolEnhanced.sln`](XtremeToolEnhanced.sln) & [`XtremeToolEnhanced.vcxproj`](XtremeToolEnhanced.vcxproj) — Visual Studio 2022 (`v143`, C++20, x64) solution and project.
- [`README.md`](README.md) — User-facing documentation, configuration guide, and build instructions.
- [`dist/Put in main folder/XtremeTool.ini`](dist/Put%20in%20main%20folder/XtremeTool.ini) — Default configuration file shipped with v1.2.
- [`dist/Documentation_Config.txt`](dist/Documentation_Config.txt) — Original author's parameter reference.
- [`dist/ReadMe_Install.txt`](dist/ReadMe_Install.txt) — Original author's install instructions.

---

## 2. Original Binary (`XtremeTool.asi` v1.2) Mapping Table

When cross-referencing our C++ functions with the original `XtremeTool.asi` v1.2 PE64 binary (`ImageBase = 0x180000000`, `.text` VMA `0x180001000` = FileOffset `0x00400`, `.rdata` VMA `0x180011000` = FileOffset `0x0F400`), use this function map:

| Original VMA | Our C++ Function (`src/`) | Description |
| :--- | :--- | :--- |
| `0x1800010C0` | `DllMain` (`src/dllmain.cpp`) | Checks `fdwReason == 1` (`DLL_PROCESS_ATTACH`) and calls `0x180003000`. |
| `0x180002F00` | `Game::kVersionString` (`src/Game.h`) | Returns `"v1.2 Enhanced"` (`0x1800119A0`). |
| `0x180002F60` | `Game::IsEnhancedEdition` (`src/Game.cpp`) | Calls `K32GetModuleBaseNameA(GetCurrentProcess(), GetModuleHandleW(NULL), ...)` and compares with `"GTA5_Enhanced.exe"` (`0x1800119B0`). |
| `0x180003000` | `Game::Initialize` (`src/Game.cpp`) | Main orchestration routine: initializes log, loads config, clamps `PED_LIMIT <= 340`, checks `IsEnhancedEdition()`, logs all parameters, converts MB to bytes (`<< 20`), clamps `POOL_SIZE` if `>= HEAP_SIZE`, and invokes all patch functions in order. |
| `0x180004860` | `Config::Load` (`src/Config.cpp`) | Reads `.\XtremeTool.ini` (`0x180011A78`) using `GetPrivateProfileIntW`. |
| `0x180004A70` | `Logger::Log` (`src/Logger.cpp`) | Writes `std::string` + `std::endl` to the global `std::ofstream` (`0x180027CD8`, `"XtremeTool.log"`). |
| `0x180004AD0` | `Game::PatchLimitFixInstance1` (`src/Game.cpp`) | Adjusts main-thread stack allocation (`0x1048`) in `__chkstk` (`instance 1`) and `add rsp, 0x1048` (`instance 1_1`) by `(pedLimit - 256) * 16`. |
| `0x180004E50` | `Game::PatchLimitFixInstance2` (`src/Game.cpp`) | Adjusts `sub rsp, 0xC28` (`instance 2`), two pairs of internal `(0x820, 0x420)` offsets, and `add rsp, 0xC28` (`instance 2_1`) by `(pedLimit - 256) * 12` and `(pedLimit - 256) * 8`. |
| `0x180005480` | `Game::PatchLimitFixInstance3` (`src/Game.cpp`) | Overwrites `call` at `match + 0x0B` with `mov eax, pedLimit` (`B8 <imm32>`). |
| `0x1800056C0` | `Game::PatchMainHeapSize32` (`src/Game.cpp`) | Overwrites `mov edx, imm32` immediate at `match + 1` with 32-bit `heapBytes`. |
| `0x1800058B0` | `Game::PatchMainHeapSize64` (`src/Game.cpp`) | Allocates `0x1000`-byte RWX trampoline and hooks `match` with a 64-bit `movabs rdx, heapBytes` stub. |
| `0x180005C30` | `Game::PatchLimitFixInstance4` (`src/Game.cpp`) | Overwrites `match - 0x0C` with `mov eax, 240000; nop; nop; nop` (`B8 80 A9 03 00 90 90 90`). |
| `0x180005E70` | `Game::PatchPedMemoryBudget` (`src/Game.cpp`) | Loops over the module to find all occurrences of `05 60 E3 16 00` and overwrites `match + 1` with `pedMemBytes`. |
| `0x180006410` | `Game::PatchMaxPeds` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (pedLimit - 10); nop; nop; nop`. |
| `0x180006650` | `Game::PatchPedPoolLimit` (`src/Game.cpp`) | Overwrites `call` at `match1 + 0x13` and `match2 + 0x19` with `mov eax, pedLimit`. |
| `0x180006A50` | `Game::PatchPedSpawnAmbientMultiplier` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (multiplier * 100); nop; nop; nop`. |
| `0x180006C80` | `Game::PatchPedSpawnScenarioMultiplier` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (multiplier * 100); nop; nop; nop`. |
| `0x180006EC0` | `Game::PatchPoolHeapSize32` (`src/Game.cpp`) | Overwrites `mov r9d, imm32` immediate at `match + 0x16` with 32-bit `poolBytes`. |
| `0x1800070B0` | `Game::PatchPoolHeapSize64` (`src/Game.cpp`) | Allocates `0x1000`-byte RWX trampoline and hooks `match + 0x14` with a 64-bit `movabs r9, poolBytes` stub. |
| `0x180007400` | `Game::PatchResourceHeapSize32` (`src/Game.cpp`) | Overwrites `mov r8d, imm32` immediate at `match + 0x0E` with 32-bit `resourceBytes`. |
| `0x180007600` | `Game::PatchResourceHeapSize64` (`src/Game.cpp`) | Allocates `0x1000`-byte RWX trampoline and hooks `match + 0x0C` with a 64-bit `movabs r8, resourceBytes` stub. |
| `0x180007960` | `Game::PatchMaxVehAmbient` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (vehLimit - 10); nop; nop; nop`. |
| `0x180007B90` | `Game::PatchMaxVehParked` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (vehLimit - 20); nop; nop; nop`. |
| `0x180007E00` | `Game::PatchVehSpawnAmbientMultiplier` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (multiplier * 100); nop; nop; nop`. |
| `0x180008030` | `Game::PatchVehSpawnParkedMultiplier` (`src/Game.cpp`) | Overwrites `match` with `movabs rax, (multiplier * 100); nop; nop; nop`. |
| `0x180008270` | `Game::PatchSkipPedsPathfindPass` (`src/Game.cpp`) | Overwrites `match` with `xor ebx, ebx; nop; nop; nop` (`31 DB 90 90 90`). |
| `0x180008490` | `Memory::WriteU32` (`src/Pattern.cpp`) | `VirtualProtect` + 4-byte `uint32_t` write + restore `VirtualProtect`. |

---

## 3. Complete Signature & Patch Catalog

Below is the exact byte-for-byte specification of every pattern, mask, target offset, and machine-code patch used in [`src/Game.cpp`](src/Game.cpp).

### 3.1 Main Heap Size (`PatchMainHeapSize32` / `PatchMainHeapSize64`)
- **Pattern (`0x180011D28`, 19 bytes)**:
  `BA 00 00 00 00 45 31 C0 41 B1 01 E8 00 00 00 00 48 8D 0D`
- **Mask (`0x180011D10`, 19 chars)**:
  `x????xxxxxxx????xxx`
- **Disassembly Context in `GTA5_Enhanced.exe`**:
  ```asm
  match+0x00: BA xx xx xx xx    mov edx, <default_heap_size>
  match+0x05: 45 31 C0          xor r8d, r8d
  match+0x08: 41 B1 01          mov r9b, 1
  match+0x0B: E8 xx xx xx xx    call AllocateMainHeap
  match+0x10: 48 8D 0D ...      lea rcx, [rip + ...]
  ```
- **32-Bit Patch**:
  Writes `uint32_t(heapBytes)` to `match + 1`.
- **64-Bit Patch**:
  Because `mov edx, imm32` (`5` bytes) + `xor r8d, r8d` (`3` bytes) = `8` bytes total, a `10`-byte `movabs rdx, imm64` does not fit inline. Instead:
  1. Allocates `0x1000` bytes via `malloc(0x1000)` and marks it `PAGE_EXECUTE_READWRITE`.
  2. Stores the 64-bit trampoline pointer at `[match - 0xFFF]`.
  3. Writes a 6-byte RIP-relative indirect jump at `match`:
     `FF 25 FB EF FF FF` (`jmp qword ptr [rip - 0x1005]`, where `rip = match + 6`, so `(match + 6) - 0x1005 = match - 0xFFF`).
  4. Writes 26 bytes to the trampoline:
     - `+0x00`: `48 BA <8-byte heapBytes>` (`movabs rdx, heapBytes`)
     - `+0x0A`: `45 31 C0` (`xor r8d, r8d`)
     - `+0x0D`: `49 B9 <8-byte (match + 8)>` (`movabs r9, match + 8`)
     - `+0x17`: `41 FF E1` (`jmp r9`)

### 3.2 Pool Heap Size (`PatchPoolHeapSize32` / `PatchPoolHeapSize64`)
- **Pattern (`0x180011DA8`, 27 bytes)**:
  `48 00 00 00 00 00 00 48 00 00 00 00 00 00 41 B8 00 00 01 00 41 B9 00 00 00 00 E8`
- **Mask (`0x180011D88`, 27 chars)**:
  `x??????x??????xxxxxxxx????x`
- **Disassembly Context in `GTA5_Enhanced.exe`**:
  ```asm
  match+0x00: 48 ...            (7 bytes)
  match+0x07: 48 ...            (7 bytes)
  match+0x0E: 41 B8 00 00 01 00 mov r8d, 0x10000
  match+0x14: 41 B9 xx xx xx xx mov r9d, <default_pool_size>
  match+0x1A: E8 xx xx xx xx    call AllocatePoolHeap
  ```
- **32-Bit Patch**:
  Writes `uint32_t(poolBytes)` to `match + 0x16`.
- **64-Bit Patch**:
  Let `baseAnchor = match + 0x10` (so `baseAnchor + 4 = match + 0x14` and `baseAnchor + 0x0A = match + 0x1A`).
  1. Allocates `0x1000` bytes RWX trampoline.
  2. Writes trampoline address to `[baseAnchor - 0xFFB]` (`(baseAnchor + 4) - 0xFFF`).
  3. Writes `FF 25 FB EF FF FF` (`jmp qword ptr [rip - 0x1005]`) at `baseAnchor + 4` (`match + 0x14`, replacing the 6-byte `mov r9d, imm32`).
  4. Writes 22 bytes to the trampoline:
     - `+0x00`: `49 B9 <8-byte poolBytes>` (`movabs r9, poolBytes`)
     - `+0x0A`: `48 B8 <8-byte (baseAnchor + 0x0A)>` (`movabs rax, match + 0x1A`)
     - `+0x14`: `FF E0` (`jmp rax`)

### 3.3 Resource Heap Size (`PatchResourceHeapSize32` / `PatchResourceHeapSize64`)
- **Pattern (`0x180011E28`, 18 bytes)**:
  `48 00 00 00 00 00 00 BA 00 20 00 00 41 B8 00 00 00 00`
- **Mask (`0x180011E10`, 18 chars)**:
  `x??????xxxxxxxx????`
- **Disassembly Context in `GTA5_Enhanced.exe`**:
  ```asm
  match+0x00: 48 ...            (7 bytes)
  match+0x07: BA 00 20 00 00    mov edx, 0x2000
  match+0x0C: 41 B8 xx xx xx xx mov r8d, <default_resource_size>
  match+0x12: ...
  ```
- **32-Bit Patch**:
  Writes `uint32_t(resourceBytes)` to `match + 0x0E`.
- **64-Bit Patch**:
  Let `baseAnchor = match + 0x08` (so `baseAnchor + 4 = match + 0x0C` and `baseAnchor + 0x0A = match + 0x12`).
  1. Allocates `0x1000` bytes RWX trampoline.
  2. Writes trampoline address to `[baseAnchor - 0xFFB]`.
  3. Writes `FF 25 FB EF FF FF` at `baseAnchor + 4` (`match + 0x0C`, replacing the 6-byte `mov r8d, imm32`).
  4. Writes 22 bytes to the trampoline:
     - `+0x00`: `49 B8 <8-byte resourceBytes>` (`movabs r8, resourceBytes`)
     - `+0x0A`: `48 B8 <8-byte (baseAnchor + 0x0A)>` (`movabs rax, match + 0x12`)
     - `+0x14`: `FF E0` (`jmp rax`)

### 3.4 Ped & Vehicle Streaming Memory Budget (`PatchPedMemoryBudget`)
- **Pattern (`0x180011E90`, 5 bytes)**:
  `05 60 E3 16 00` (`add eax, 0x16E360` — `1,500,000` bytes)
- **Mask (`0x180011E88`, 5 chars)**:
  `xxxxx`
- **Patch**:
  Scans the entire executable in a loop (`cur = match + 1` after each hit) and overwrites the 4-byte immediate at `match + 1` with `pedMemBytes` (`PED_MEM_SIZE << 20`).

### 3.5 Skip Peds Pathfind Pass (`PatchSkipPedsPathfindPass`)
- **Pattern (`0x180011F08`, 12 bytes)**:
  `48 89 44 24 30 89 5C 24 20 48 8D 0D`
- **Mask (`0x180011EF8`, 12 chars)**:
  `xxxxxxxxxxxx`
- **Patch**:
  Overwrites the first 5 bytes (`48 89 44 24 30`, `mov [rsp+0x30], rax`) at `match` with `31 DB 90 90 90` (`xor ebx, ebx; nop; nop; nop`), so the subsequent `mov [rsp+0x20], ebx` writes `0` for the pathfinding pass count.

### 3.6 Spawn Multipliers & Max Entity Caps (`WriteMovRaxImm64Nop3`)
All 7 functions match a 9-byte pattern (`mask = "xxxxxxxxx"`) of the form:
```asm
48 8D 51 <off1>   lea rdx, [rcx + off1]
48 83 C1 <off2>   add rcx, off2
E8 xx xx xx xx    call ReadEncryptedConfigValue ; (13 bytes total from 48 8D 51 to end of E8 call)
```
And overwrite all 13 bytes at `match` with:
`48 B8 <8-byte int64_t value> 90 90 90` (`movabs rax, value; nop; nop; nop`).

| Function | Pattern Bytes (`9` bytes) | Value Written to `rax` |
| :--- | :--- | :--- |
| `PatchPedSpawnAmbientMultiplier` | `48 8D 51 28 48 83 C1 2C E8` | `int64_t(pedSpawnAmbientMultiplier * 100)` |
| `PatchPedSpawnScenarioMultiplier` | `48 8D 51 20 48 83 C1 24 E8` | `int64_t(pedSpawnScenarioMultiplier * 100)` |
| `PatchVehSpawnAmbientMultiplier` | `48 8D 51 4C 48 83 C1 50 E8` | `int64_t(vehSpawnAmbientMultiplier * 100)` |
| `PatchVehSpawnParkedMultiplier` | `48 8D 51 58 48 83 C1 5C E8` | `int64_t(vehSpawnParkedMultiplier * 100)` |
| `PatchMaxPeds` | `48 8D 51 30 48 83 C1 34 E8` | `int64_t(pedLimit - 10)` |
| `PatchMaxVehAmbient` | `48 8D 51 68 48 83 C1 6C E8` | `int64_t(vehLimit - 10)` |
| `PatchMaxVehParked` | `48 8D 51 74 48 83 C1 78 E8` | `int64_t(vehLimit - 20)` |

### 3.7 Ped Pool Limit (`> 256`) & Stack Allocation Fixes
When `PED_LIMIT` is raised above the default `256` (`0x100`), several functions in `GTA5_Enhanced.exe` allocate fixed-size arrays on the main thread's stack sized for 256 peds. Without patching those stack frames, raising `PED_LIMIT` causes stack corruption and an immediate crash.

1. **`PatchPedPoolLimit` (`0x180006650`)**:
   - **Instance 1**:
     - Pattern (`27` bytes): `E8 00 00 00 00 48 89 F9 BA 00 00 00 00 41 B8 01 00 00 00 E8 00 00 00 00 89 C1 E8`
     - Mask (`27` chars): `x????xxxx????xxxxxxx????xx?`
     - Patch at `match + 0x13`: `B8 <uint32_t pedLimit>` (`mov eax, pedLimit`).
   - **Instance 2**:
     - Pattern (`26` bytes): `B9 30 00 00 00 E8 00 00 00 00 48 89 C6 00 00 00 00 00 00 00 00 00 00 00 00 E8`
     - Mask (`26` chars): `xxxxxx????xxx????????????x`
     - Patch at `match + 0x19`: `B8 <uint32_t pedLimit>` (`mov eax, pedLimit`).

2. **`PatchLimitFixInstance1` (`0x180004AD0`)**:
   - Computes `extraStack = (pedLimit - 256) * 16`.
   - **Instance 1** (Prologue `__chkstk` size `0x1048` = `256 * 16 + 0x48`):
     - Pattern (`24` bytes): `B8 48 10 00 00 E8 00 00 00 00 48 29 C4 48 8B 05 00 00 00 00 48 63 70 5C`
     - Mask (`24` chars): `xxxxxx????xxxxxx????xxxx`
     - Adds `extraStack` to the `uint32_t` at `match + 1` (`0x1048 + extraStack`).
   - **Instance 1_1** (Epilogue `add rsp, 0x1048`):
     - Scans forward from `match + 1` for `48 81 C4 48 10 00 00` (`xxxxxxx`).
     - Adds `extraStack` to the `uint32_t` at `match1_1 + 3` (`0x1048 + extraStack`).

3. **`PatchLimitFixInstance2` (`0x180004E50`)**:
   - Computes `extra12 = (pedLimit - 256) * 12` and `extra8 = (pedLimit - 256) * 8`.
   - **Instance 2** (Prologue `sub rsp, 0xC28` where `0xC28` = `256 * 12 + 0x28`):
     - Pattern (`11` bytes): `41 56 56 57 53 48 81 EC 28 0C 00` (`xxxxxxxxxxx`).
     - Adds `extra12` to the `uint32_t` at `match + 8` (`0xC28 + extra12`).
   - **Internal Stack Buffer Offsets (`0x820` and `0x420`)**:
     - Scans forward from `match + 1` twice for pairs of `20 08 00 00` (`0x820` = `256 * 8 + 0x20`) followed by `20 04 00 00` (`0x420` = `256 * 4 + 0x20`).
     - Adds `extra8` to each `0x420` immediate and `extra12` to each `0x820` immediate.
   - **Instance 2_1** (Epilogue `add rsp, 0xC28`):
     - Scans forward for `48 81 C4 28 0C 00 00` (`xxxxxxx`).
     - Adds `extra12` to the `uint32_t` at `match2_1 + 3` (`0xC28 + extra12`).

4. **`PatchLimitFixInstance3` (`0x180005480`)**:
   - Pattern (`11` bytes): `BA 56 44 A5 8D 41 B8 01 00 00 00` (`xxxxxxxxxxx`).
   - Overwrites the 5-byte `call` at `match + 0x0B` with `B8 <uint32_t pedLimit>` (`mov eax, pedLimit`).

5. **`PatchLimitFixInstance4` (`0x180005C30`)**:
   - Always executed regardless of `PED_LIMIT`, with `value = 240000` (`0x3A980`).
   - Pattern (`10` bytes): `75 EF EB 20 8B 40 04 83 F8 FF` (`xxxxxxxxxx`).
   - Overwrites 8 bytes at `match - 0x0C` with `B8 80 A9 03 00 90 90 90` (`mov eax, 240000; nop; nop; nop`).
