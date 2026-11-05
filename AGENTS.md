# Agent Technical Memory & Codebase Guide (`AGENTS.md`)

This document serves as the persistent knowledge base for AI coding agents and reverse engineers working on **XtremeTool Universal** (supporting both **GTA V Enhanced v1.2** on `GTA5_Enhanced.exe` and **GTA V Legacy v1.1** on `GTA5.exe`). It documents the exact source layout, symbol map, runtime execution flow, memory signatures, assembly patches, and original binary VMA mappings for both editions.

---

## 1. High-Level Architecture & Execution Lifecycle

`XtremeTool.asi` is a Windows x64 Dynamic Link Library (`.dll` renamed to `.asi`) loaded by **ScriptHookV**'s ASI Loader (`dinput8.dll`) at process startup.

### Startup Sequence (`DllMain`)
Unlike gameplay scripts that register a ScriptHookV `scriptRegister` loop, **XtremeTool** must patch memory allocator sizes (`HEAP_SIZE`, `POOL_SIZE`, `RESOURCE_SIZE`) and pool initialization routines **before** the RAGE engine finishes initializing its heaps and entity pools. Therefore, all work happens synchronously during `DLL_PROCESS_ATTACH`:

1. [`DllMain`](src/dllmain.cpp) triggers on `fdwReason == DLL_PROCESS_ATTACH` (`fdwReason == 1`).
2. [`Logger::Initialize`](src/Logger.cpp) opens `XtremeTool.log` in append mode and clears/truncates its contents via `std::ofstream(..., std::ios::out | std::ios::trunc)`.
3. [`Logger::Log`](src/Logger.cpp) writes `"Starting XtremeTool (Universal: Enhanced & Legacy)"` with an `[INFO]` timestamp.
4. [`Config::Initialize`](src/Config.cpp) opens `XtremeTool.ini`, strips whitespace and `#` comments, parses all 15 key-value pairs into global variables under `namespace Config`, and logs `"Config initialized"` (or `"Couldn't find config file, initializing with default values"`).
5. [`Game::Initialize`](src/Game.cpp) executes:
   - Calls [`Game::DetectEdition`](src/Game.cpp) to determine whether the host process is `GTA5_Enhanced.exe` (`Edition::Enhanced`) or `GTA5.exe` (`Edition::Legacy`), with a byte-signature fallback if the executable was renamed.
   - Dispatches to either [`Game::InitializeEnhanced`](src/Game.cpp) or [`Game::InitializeLegacy`](src/Game.cpp), executing all edition-specific pattern scans and memory patches and logging `"Game initialized"`.

---

## 2. Where to Find Every Function & Symbol

| File | Namespace / Symbol | Line Reference | Responsibility |
| :--- | :--- | :--- | :--- |
| [`src/dllmain.cpp`](src/dllmain.cpp) | `DllMain` | `L7-L16` | DLL entry point; triggers `Logger`, `Config`, and `Game` initialization on `DLL_PROCESS_ATTACH`. |
| [`src/Logger.h`](src/Logger.h) / [`src/Logger.cpp`](src/Logger.cpp) | `Logger::Initialize` | `Logger.cpp#L11-L15` | Opens and truncates `XtremeTool.log`. |
| [`src/Logger.h`](src/Logger.h) / [`src/Logger.cpp`](src/Logger.cpp) | `Logger::Log` | `Logger.cpp#L17-L31` | Formats local system time (`%Y-%m-%d %X`) and writes `[INFO] <msg>` to `XtremeTool.log`. |
| [`src/Config.h`](src/Config.h) / [`src/Config.cpp`](src/Config.cpp) | `Config::Initialize` | `Config.cpp#L35-L92` | Parses `XtremeTool.ini` and populates all 15 configuration globals. |
| [`src/Config.h`](src/Config.h) / [`src/Config.cpp`](src/Config.cpp) | `Config::*` globals | `Config.cpp#L10-L28` | Stores parsed settings (`pedSpawnAmbientRate`, `heapSize`, `pedLimit`, etc.). |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::GetMainModuleRegion` | `Pattern.cpp#L8-L18` | Queries `GetModuleHandleW(nullptr)` and `K32GetModuleInformation` for the host EXE base and image size. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::FindPatternRange` | `Pattern.cpp#L20-L47` | Scans `[startAddr, endAddr)` using raw byte pattern + `'x'`/`'?'` mask. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::FindPattern` | `Pattern.cpp#L49-L55` | Scans the entire main executable module for a byte signature. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::PatchBytes` | `Pattern.cpp#L57-L63` | Unlocks memory via `VirtualProtect(..., PAGE_EXECUTE_READWRITE)`, copies bytes, and restores protection. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteU32` / `AddU32` | `Pattern.cpp#L65-L72` | Writes or adds a 32-bit immediate at the target address. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteMovEaxImm32` | `Pattern.cpp#L74-L79` | Writes 5-byte `B8 <imm32>` (`mov eax, imm32`) over a 5-byte `E8` call instruction. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteMovRaxImm64Nop3` | `Pattern.cpp#L81-L90` | Writes 13-byte `48 B8 <imm64> 90 90 90` (`movabs rax, imm64; nop; nop; nop`). |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::AllocateTrampolineNear` | `Pattern.cpp#L92-L127` | Allocates a `PAGE_EXECUTE_READWRITE` page within `±1.75 GB` of `anchorAddr` via `VirtualAlloc`. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteIndirectJump64` | `Pattern.cpp#L129-L154` | Emits `FF 25 <disp32>` pointing to `[trampoline + 0x80]` so no game `.text` bytes are corrupted. |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::DetectEdition` | `Game.cpp#L40-L64` | Detects `GTA5_Enhanced.exe` vs `GTA5.exe` by module filename with byte-pattern fallback. |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::Initialize` | `Game.cpp#L66-L73` | Dispatches to `InitializeEnhanced()` or `InitializeLegacy()`. |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::*` (Enhanced) | `Game.cpp#L98-L410` | All 15 patchers for `GTA5_Enhanced.exe` (matching *XtremeTool Enhanced v1.2*). |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::*Legacy` | `Game.cpp#L416-L660` | All 12 patchers for `GTA5.exe` (matching *XtremeTool Legacy v1.1* with 64-bit fixes). |

---

## 3. Stability Fixes Over Original Binaries

1. **Restored 64-Bit Trampolines on Legacy (`GTA5.exe`) & Safe Pointer Slots on Both Editions**:
   - In the original *XtremeTool 1.1 Legacy* binary (`Config::Initialize` at `0x18000490e`), the author hardcoded `use64BitHeapSize = 0`, `use64BitPoolSize = 0`, and `use64BitResourceSize = 0` at the end of config parsing because Legacy's 64-bit patch functions wrote their 8-byte trampoline target address to `[jmpSite - 0x1FF]` (and Enhanced wrote to `[match - 0xFFF]`), corrupting unrelated instructions in `.text`.
   - Our [`Memory::AllocateTrampolineNear`](src/Pattern.cpp) reserves a `VirtualAlloc` page within `±1.75 GB` of the hook site, and [`Memory::WriteIndirectJump64`](src/Pattern.cpp) stores the 8-byte target address at `trampoline + 0x80` (inside our own allocated page), eliminating `.text` corruption and allowing 64-bit heaps to stay enabled on both Legacy and Enhanced.
2. **Missing `VirtualProtect` in Legacy 32-Bit Heap/Pool/Resource & Pathfind Patches**:
   - Original *XtremeTool 1.1 Legacy* wrote directly to `*reinterpret_cast<uint32_t*>(match + offset)` in `SetHeapSize`, `SetPoolSize`, and `SetResourceSize` without calling `VirtualProtect`, and passed `size = 3` to `VirtualProtect` in `SkipPedsPathfindPass` while writing 5 bytes (`4D 31 D2 90 90`). All writes in our universal build go through [`Memory::PatchBytes`](src/Pattern.cpp) with full byte-length protection.
3. **64-Bit Overflow Fix for `>= 4096 MB` Heaps**:
   - Both original binaries computed `heapSize * 0x100000` in 32-bit signed `int` before sign-extending via `movsxd`. All MB-to-byte conversions now use `static_cast<int64_t>(size) * 0x100000LL` or `static_cast<uint64_t>(size) * 0x100000ULL`.

---

## 4. Original Binary VMA Map — GTA V Enhanced (`XtremeTool Enhanced v1.2`)

Image base: `0x180000000`

| Function | Original VMA | Source Implementation |
| :--- | :--- | :--- |
| `DllMain` | `0x180006fc0` | [`src/dllmain.cpp`](src/dllmain.cpp) |
| `Logger::Initialize` | `0x180002d00` | [`src/Logger.cpp`](src/Logger.cpp) |
| `Logger::Log` | `0x180002ac0` | [`src/Logger.cpp`](src/Logger.cpp) |
| `Config::Initialize` | `0x1800015d0` | [`src/Config.cpp`](src/Config.cpp) |
| `Game::Initialize` | `0x180005110` | [`src/Game.cpp`](src/Game.cpp) (`Game::InitializeEnhanced`) |
| `Game::SetPedSpawnAmbientRate` | `0x180005340` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetPedSpawnScenarioRate` | `0x180005500` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetVehSpawnAmbientRate` | `0x1800056c0` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetVehSpawnParkedRate` | `0x180005880` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetHeapSize` | `0x180005a40` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetPoolSize` | `0x180005ca0` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetResourceSize` | `0x180005f00` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetPedMemSize` | `0x180006160` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetVehMemSize` | `0x180006310` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetPedPoolSize` | `0x1800064c0` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetMaxPeds` | `0x180006740` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetMaxVehAmbient` | `0x180006900` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SetMaxVehParked` | `0x180006ac0` | [`src/Game.cpp`](src/Game.cpp) |
| `Game::SkipPedsPathfindPass` | `0x180006c80` | [`src/Game.cpp`](src/Game.cpp) |
| `Pattern::FindPattern` | `0x180006f10` | [`src/Pattern.cpp`](src/Pattern.cpp) |

---

## 5. Original Binary VMA Map — GTA V Legacy (`XtremeTool 1.1 Legacy`)

Image base: `0x180000000`

| Function | Original VMA | Source Implementation |
| :--- | :--- | :--- |
| `DllMain` | `0x180007330` | [`src/dllmain.cpp`](src/dllmain.cpp) |
| `Logger::Initialize` | `0x1800040c8` | [`src/Logger.cpp`](src/Logger.cpp) |
| `Logger::Log` | `0x180003be0` | [`src/Logger.cpp`](src/Logger.cpp) |
| `Config::Initialize` | `0x1800041ce` | [`src/Config.cpp`](src/Config.cpp) |
| `Game::Initialize` | `0x180005b50` | [`src/Game.cpp`](src/Game.cpp) (`Game::InitializeLegacy`) |
| `Game::SetHeapSize` (Legacy) | `0x180005d5d` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetHeapSizeLegacy`) |
| `Game::SetPoolSize` (Legacy) | `0x180006129` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetPoolSizeLegacy`) |
| `Game::SetResourceSize` (Legacy) | `0x1800064c7` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetResourceSizeLegacy`) |
| `Game::SetPedMemSize` (Legacy) | `0x180006865` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetPedMemSizeLegacy`) |
| `Game::SkipPedsPathfindPass` (Legacy) | `0x180006ce6` | [`src/Game.cpp`](src/Game.cpp) (`Game::SkipPedsPathfindPassLegacy`) |
| `Game::SetPedSpawnAmbientRate` (Legacy) | `0x180006e2d` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetPedSpawnAmbientRateLegacy`) |
| `Game::SetPedSpawnScenarioRate` (Legacy) | `0x180006ed2` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetPedSpawnScenarioRateLegacy`) |
| `Game::SetVehSpawnAmbientRate` (Legacy) | `0x180006f77` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetVehSpawnAmbientRateLegacy`) |
| `Game::SetVehSpawnParkedRate` (Legacy) | `0x18000701c` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetVehSpawnParkedRateLegacy`) |
| `Game::SetMaxPeds` (Legacy) | `0x1800070c1` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetMaxPedsLegacy`) |
| `Game::SetMaxVehAmbient` (Legacy) | `0x180007152` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetMaxVehAmbientLegacy`) |
| `Game::SetMaxVehParked` (Legacy) | `0x1800071e3` | [`src/Game.cpp`](src/Game.cpp) (`Game::SetMaxVehParkedLegacy`) |

---

## 6. Complete Memory Signature Reference

### A. GTA V Enhanced (`GTA5_Enhanced.exe`) Signatures

| Function | Byte Pattern | Mask | Offset | Patch Operation |
| :--- | :--- | :--- | :--- | :--- |
| `SetPedSpawnAmbientRate` | `F3 0F 59 05 ?? ?? ?? ?? 48 8D 56 2C` | `xxxx????xxxx` | `+0x13` | `B8 <uint32(rate * 100)>` (`mov eax, imm32`) |
| `SetPedSpawnScenarioRate` | `F3 0F 11 46 38 48 8D 56 20` | `xxxxxxxxx` | `+0x0F` | `B8 <uint32(rate * 100)>` |
| `SetVehSpawnAmbientRate` | `48 89 46 44 48 8D 56 4C` | `xxxxxxxx` | `+0x0E` | `B8 <uint32(rate * 100)>` |
| `SetVehSpawnParkedRate` | `48 89 46 50 48 8D 56 58` | `xxxxxxxx` | `+0x0E` | `B8 <uint32(rate * 100)>` |
| `SetHeapSize` | `BA 00 00 00 50 41 B8 01 00 00 00` | `xxxxxxxxxxx` | `+0x01` (32b) / `+0x00` (64b) | 32b: `WriteU32(bytes)` at `+1`; 64b: `FF 25` (6B) at `+0` -> trampoline (`mov rdx, imm64; mov r8d, 1`) |
| `SetPoolSize` | `48 8D 0D ?? ?? ?? ?? 83 C8 04 41 B8` | `xxx????xxxxx` | `+0x0C` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xC`; 64b: `FF 25` (6B) at `+0xA` -> trampoline (`mov r8, imm64`) |
| `SetResourceSize` | `45 31 C9 83 C8 20 48 8D 0D ?? ?? ?? ?? 41 B8` | `xxxxxxxxx????xx` | `+0x0F` (32b) / `+0x0D` (64b) | 32b: `WriteU32(bytes)` at `+0xF`; 64b: `FF 25` (6B) at `+0xD` -> trampoline (`mov r8, imm64`) |
| `SetPedMemSize` | `41 BE 00 00 B0 0D` | `xxxxxx` | `+0x02` | `WriteU32(pedMemSize * 0x100000)` |
| `SetVehMemSize` | `41 BD 00 00 20 1C` | `xxxxxx` | `+0x02` | `WriteU32(vehMemSize * 0x100000)` |
| `SetPedPoolSize` (1) | `BA B7 4E 8B C6 E8` | `xxxxxx` | `+0x05` | `48 B8 <int64(pedLimit)> 90 90 90` (13B) |
| `SetPedPoolSize` (2) | `BA 8F C2 F5 3C E8 ?? ?? ?? ?? 48 63 D0` | `xxxxx????xxx` | `+0x05` | `48 B8 <int64(pedLimit)> 90 90 90` (13B) |
| `SetPedPoolSize` (3) | `48 89 C1 E8 ?? ?? ?? ?? B9 69 C5 E1 27` | `xxx????xxxxxx` | `+0x0D` | `48 B8 <int64(pedLimit)> 90 90 90` (13B) |
| `SetPedPoolSize` (4) | `BA 2B 67 B0 1B E8` | `xxxxxx` | `+0x05` | `AddU32` deltas `+0x40`, `+0x9C`, `+0xDE`, `+0x132` (`pedLimit - 256`) |
| `SetMaxPeds` | `F3 0F 58 0D ?? ?? ?? ?? F3 0F 59 C8 F3 0F 2C C1 E8` | `xxxx????xxxxxxxxx` | `+0x10` | `B8 <uint32(pedLimit - 10)>` |
| `SetMaxVehAmbient` | `48 8D 56 68 48 8D 4E 6C E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(vehLimit - 10)>` |
| `SetMaxVehParked` | `48 8D 56 74 48 8D 4E 78 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(vehLimit - 20)>` |
| `SkipPedsPathfindPass` | `0F 84 ?? ?? ?? ?? 0F B7 05 ?? ?? ?? ?? 66 0F 6E C0 0F 5B C0 F3 0F 59 05 ?? ?? ?? ?? F3 0F 2C C0` | `x????xxx????xxxxxxxxxxxx????xxxx` | `+0x1B` | `31 C0 90 90` (`xor eax, eax; nop; nop`) |

### B. GTA V Legacy (`GTA5.exe`) Signatures

| Function | Byte Pattern | Mask | Offset | Patch Operation |
| :--- | :--- | :--- | :--- | :--- |
| `SetHeapSizeLegacy` | `48 8D 0D ?? ?? ?? ?? 41 B1 01 45 33 C0 BA` | `xxx????xxxxxxx` | `+0x0E` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xE`; 64b: `FF 25` + `90 90` (8B) at `+0xA` -> trampoline (`xor r8d, r8d; mov rdx, imm64`) |
| `SetPoolSizeLegacy` | `83 C8 04 48 8D 0D ?? ?? ?? ?? 41 B8` | `xxxxxx????xx` | `+0x0C` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xC`; 64b: `FF 25` (6B) at `+0xA` -> trampoline (`mov r8, imm64`) |
| `SetResourceSizeLegacy` | `83 C8 20 48 8D 0D ?? ?? ?? ?? 41 B8` | `xxxxxx????xx` | `+0x0C` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xC`; 64b: `FF 25` (6B) at `+0xA` -> trampoline (`mov r8, imm64`) |
| `SetPedMemSizeLegacy` | `41 BE 60 E3 16 00` | `xxxxxx` | `+0x02` | `WriteU32(pedMemSize * 0x100000)` |
| `SkipPedsPathfindPassLegacy` | `48 8B 05 ?? ?? ?? ?? 4C 8B C7 48 ?? ?? ?? 30` | `xxx????xxxx???x` | `+0x0A` | `4D 31 D2 90 90` (`xor r10, r10; nop; nop`, 5B) |
| `SetPedSpawnAmbientRateLegacy` | `48 83 C1 2C F3 44 0F 59 D6 E8` | `xxxxxxxxxx` | `+0x09` | `B8 <uint32(rate * 100)>` |
| `SetPedSpawnScenarioRateLegacy` | `48 8D 51 20 48 83 C1 24 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(rate * 100)>` |
| `SetVehSpawnAmbientRateLegacy` | `48 8D 51 4C 48 83 C1 50 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(rate * 100)>` |
| `SetVehSpawnParkedRateLegacy` | `48 8D 51 58 48 83 C1 5C E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(rate * 100)>` |
| `SetMaxPedsLegacy` | `F3 0F 59 C2 F3 0F 2C D8 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxPeds)>` (min `1000`, or `pedLimit - 10` if higher) |
| `SetMaxVehAmbientLegacy` | `48 8D 51 68 48 83 C1 6C E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxVehAmbient)>` (min `1000`, or `vehLimit - 10` if higher) |
| `SetMaxVehParkedLegacy` | `48 8D 51 74 48 83 C1 78 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxVehParked)>` (min `1000`, or `vehLimit - 20` if higher) |
