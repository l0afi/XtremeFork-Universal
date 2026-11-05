# Agent Technical Memory & Codebase Guide — `XtremeFork-Universal` (`AGENTS.md`)

This document serves as the persistent knowledge base for AI coding agents and reverse engineers working on **XtremeFork-Universal**. It documents the supported **Grand Theft Auto V** game versions and builds, the exact source layout, symbol map, runtime execution flow, memory signatures, assembly patches, and upstream binary VMA mappings for both **GTA V Enhanced** and **GTA V Legacy**.

---

## 1. Supported GTA V Game Versions & Builds

**XtremeFork-Universal** dynamically detects the running executable and scans `.text` using position-independent byte patterns + masks rather than hardcoded RVAs, supporting both PC branches of Grand Theft Auto V:

| GTA V Edition | Executable | Engine Branch | Supported GTA V Build Versions | Upstream Reference Binary |
| :--- | :--- | :--- | :--- | :--- |
| **GTA V Enhanced** | `GTA5_Enhanced.exe` | RAGE Gen9 (PC Enhanced) | **`v1.0.811.8` – `v1.0.1158.13+`** *(including builds `811.8`, `812.8`, `813.11`, `814.9`, `1158.13`, and newer Title Update 1.70+ releases)* | *XtremeTool Enhanced v1.2* (`XtremeV`) |
| **GTA V Legacy** | `GTA5.exe` | RAGE Gen8 (PC Legacy) | **`v1.0.3095.0` – `v1.0.3889.0+`** *(including builds `1.0.3095.0`, `1.0.3179.0`, `1.0.3258.0`, `1.0.3351.0`, `1.0.3411.0`, `1.0.3442.0`, `1.0.3504.0`, and `1.0.3889.0`)* | *XtremeTool Legacy v1.1* (`XtremeV`) |

> [!NOTE]
> Earlier documentation mistakenly referred to `v1.2` and `v1.1` as GTA V versions—those are the version numbers of the upstream *XtremeTool* mod binaries (`XtremeTool Enhanced v1.2` and `XtremeTool Legacy v1.1`), whereas the actual GTA V game versions follow Rockstar's `1.0.<build>.<patch>` numbering shown above.

---

## 2. High-Level Architecture & Execution Lifecycle

`XtremeTool.asi` is a Windows x64 Dynamic Link Library (`.dll` renamed to `.asi`) loaded by **ScriptHookV**'s ASI Loader (`dinput8.dll`) at process startup.

### Startup Sequence (`DllMain`)
Unlike gameplay scripts that register a ScriptHookV `scriptRegister` loop, **XtremeFork-Universal** must patch memory allocator sizes (`HEAP_SIZE`, `POOL_SIZE`, `RESOURCE_SIZE`) and pool initialization routines **before** the RAGE engine finishes initializing its heaps and entity pools. Therefore, all work happens synchronously during `DLL_PROCESS_ATTACH`:

1. [`DllMain`](src/dllmain.cpp) triggers on `ul_reason_for_call == DLL_PROCESS_ATTACH` (`1`), calls `DisableThreadLibraryCalls(hModule)`, and invokes [`Game::Initialize`](src/Game.cpp).
2. [`Logger::Init`](src/Logger.cpp) opens and truncates `XtremeTool.log` via `std::make_unique<std::ofstream>(filename, std::ios::out | std::ios::trunc)`.
3. [`Config::Load`](src/Config.cpp) reads `.\XtremeTool.ini` via Win32 `GetPrivateProfileIntW` across `[MEMORY_SETTINGS]`, `[MISC]`, and `[SPAWN_SETTINGS]` and returns a populated [`Config`](src/Config.h) struct.
4. [`Game::DetectEdition`](src/Game.cpp) queries `GetModuleBaseNameA` on `GetModuleHandleW(nullptr)` to check for `GTA5_Enhanced.exe` (`Edition::Enhanced`) or `GTA5.exe` (`Edition::Legacy`), falling back to a byte-pattern probe (`kHeapSizePattern` vs. `kLegacyHeapSizePattern`) if the user renamed the game executable.
5. [`Game::Initialize`](src/Game.cpp) validates `pedLimit` (capping at `340` on Enhanced to prevent main-thread stack overflow), converts MB values to 64-bit byte counts (`<< 20`), ensures `poolBytes < heapBytes`, and runs all edition-specific pattern scans and memory patches.

---

## 3. Where to Find Every Function & Symbol

| File | Symbol | Line Reference | Responsibility |
| :--- | :--- | :--- | :--- |
| [`src/dllmain.cpp`](src/dllmain.cpp) | `DllMain` | `L14-L33` | DLL entry point; invokes `Game::Initialize()` on `DLL_PROCESS_ATTACH`. |
| [`src/Logger.h`](src/Logger.h) / [`src/Logger.cpp`](src/Logger.cpp) | `Logger::Init` | `Logger.cpp#L5-L7` | Opens and truncates `XtremeTool.log`. |
| [`src/Logger.h`](src/Logger.h) / [`src/Logger.cpp`](src/Logger.cpp) | `Logger::Log` | `Logger.cpp#L9-L13` | Writes a line and flushes to `XtremeTool.log`. |
| [`src/Config.h`](src/Config.h) / [`src/Config.cpp`](src/Config.cpp) | `Config::Load` | `Config.cpp#L4-L45` | Parses `.\XtremeTool.ini` using `GetPrivateProfileIntW`. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::GetMainModuleRegion` | `Pattern.cpp#L10-L20` | Queries `GetModuleHandleW(nullptr)` and `GetModuleInformation` for the host EXE base and image size. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::FindPatternRange` | `Pattern.cpp#L22-L49` | Scans `[startAddr, endAddr)` using raw byte pattern + `'x'`/`'?'` mask. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::FindPattern` | `Pattern.cpp#L51-L57` | Scans the entire main executable module for a byte signature. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::PatchBytes` | `Pattern.cpp#L59-L65` | Unlocks memory via `VirtualProtect(..., PAGE_EXECUTE_READWRITE)`, copies bytes, and restores protection. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteU32` / `AddU32` | `Pattern.cpp#L67-L74` | Writes or adds a 32-bit immediate at the target address. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteMovEaxImm32` | `Pattern.cpp#L76-L81` | Writes 5-byte `B8 <imm32>` (`mov eax, imm32`). |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteMovRaxImm64Nop3` | `Pattern.cpp#L83-L92` | Writes 13-byte `48 B8 <imm64> 90 90 90` (`movabs rax, imm64; nop; nop; nop`). |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::AllocateTrampolineNear` | `Pattern.cpp#L94-L128` | Allocates a `PAGE_EXECUTE_READWRITE` page within `±1.75 GB` of `anchorAddr` via `VirtualAlloc`. |
| [`src/Pattern.h`](src/Pattern.h) / [`src/Pattern.cpp`](src/Pattern.cpp) | `Memory::WriteIndirectJump64` | `Pattern.cpp#L130-L155` | Emits `FF 25 <disp32>` pointing to `[trampoline + 0x80]` so no game `.text` bytes are corrupted. |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::DetectEdition` | `Game.cpp#L26-L47` | Detects `GTA5_Enhanced.exe` vs `GTA5.exe` by module filename with byte-pattern fallback. |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::Patch*` (Enhanced) | `Game.cpp#L62-L560` | All 20 patch functions for `GTA5_Enhanced.exe` (`v1.0.811.8` – `v1.0.1158.13+`). |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::PatchLegacy*` (Legacy) | `Game.cpp#L571-L880` | All 15 patch functions for `GTA5.exe` (`v1.0.3095.0` – `v1.0.3889.0+`). |
| [`src/Game.h`](src/Game.h) / [`src/Game.cpp`](src/Game.cpp) | `Game::Initialize` | `Game.cpp#L885-L1057` | Unified entrypoint that configures logging, loads `.ini`, and dispatches Enhanced or Legacy patches. |

---

## 4. Stability Fixes Over Original Binaries

1. **Restored 64-Bit Trampolines on Legacy (`GTA5.exe`) & Safe Pointer Slots on Both Editions**:
   - In the original *XtremeTool 1.1 Legacy* binary (`Config::Initialize` at `0x18000490e`), the author hardcoded `use64BitHeapSize = 0`, `use64BitPoolSize = 0`, and `use64BitResourceSize = 0` at the end of config parsing because Legacy's 64-bit patch functions wrote their 8-byte trampoline target address to `[jmpSite - 0x1FF]` (and Enhanced wrote to `[match - 0xFFF]`), corrupting unrelated instructions in `.text`.
   - Our [`Memory::AllocateTrampolineNear`](src/Pattern.cpp) reserves a `VirtualAlloc` page within `±1.75 GB` of the hook site, and [`Memory::WriteIndirectJump64`](src/Pattern.cpp) stores the 8-byte target address at `trampoline + 0x80` (inside our own allocated page), eliminating `.text` corruption and allowing 64-bit heaps to stay enabled on both Legacy and Enhanced.
2. **Missing `VirtualProtect` in Legacy 32-Bit Heap/Pool/Resource & Pathfind Patches**:
   - Original *XtremeTool 1.1 Legacy* wrote directly to `*reinterpret_cast<uint32_t*>(match + offset)` in `SetHeapSize`, `SetPoolSize`, and `SetResourceSize` without calling `VirtualProtect`, and passed `size = 3` to `VirtualProtect` in `SkipPedsPathfindPass` while writing 5 bytes (`4D 31 D2 90 90`). All writes in **XtremeFork-Universal** go through [`Memory::PatchBytes`](src/Pattern.cpp) with full byte-length protection.
3. **64-Bit Overflow Fix for `>= 4096 MB` Heaps**:
   - Both original binaries computed `heapSize << 20` in 32-bit signed `int` before sign-extending via `movsxd`. All MB-to-byte conversions now use `static_cast<uint64_t>(size) << 20`.

---

## 5. Upstream Binary VMA Maps

### A. Upstream *XtremeTool Enhanced v1.2* (`GTA5_Enhanced.exe`, Image Base `0x180000000`)

| Upstream VMA | Upstream Function | `XtremeFork-Universal` Implementation |
| :--- | :--- | :--- |
| `0x180007290` | `DllMain` | [`DllMain`](src/dllmain.cpp#L14-L33) |
| `0x1800050d0` | `Game::Initialize` | [`Game::Initialize`](src/Game.cpp#L885-L1057) |
| `0x18000592f` | `SetHeapSize32` | [`Game::PatchMainHeapSize32`](src/Game.cpp#L62-L73) |
| `0x180005a17` | `SetHeapSize64` | [`Game::PatchMainHeapSize64`](src/Game.cpp#L75-L112) |
| `0x180005cef` | `SetPoolSize32` | [`Game::PatchPoolHeapSize32`](src/Game.cpp#L123-L134) |
| `0x180005dad` | `SetPoolSize64` | [`Game::PatchPoolHeapSize64`](src/Game.cpp#L136-L169) |
| `0x180006081` | `SetResourceSize32` | [`Game::PatchResourceHeapSize32`](src/Game.cpp#L180-L191) |
| `0x18000613f` | `SetResourceSize64` | [`Game::PatchResourceHeapSize64`](src/Game.cpp#L193-L226) |
| `0x180006413` | `SetPedMemBudget` | [`Game::PatchPedMemoryBudget`](src/Game.cpp#L231-L264) |
| `0x1800068c8` | `SkipPedsPathfindPass` | [`Game::PatchSkipPedsPathfindPass`](src/Game.cpp#L271-L286) |
| `0x1800069e8` | `SetPedSpawnAmbientRate` | [`Game::PatchPedSpawnAmbientMultiplier`](src/Game.cpp#L291-L303) |
| `0x180006a8a` | `SetPedSpawnScenarioRate` | [`Game::PatchPedSpawnScenarioMultiplier`](src/Game.cpp#L305-L317) |
| `0x180006b2c` | `SetVehSpawnAmbientRate` | [`Game::PatchVehSpawnAmbientMultiplier`](src/Game.cpp#L319-L331) |
| `0x180006bce` | `SetVehSpawnParkedRate` | [`Game::PatchVehSpawnParkedMultiplier`](src/Game.cpp#L333-L345) |
| `0x180006c70` | `SetMaxPeds` | [`Game::PatchMaxPeds`](src/Game.cpp#L350-L362) |
| `0x180006cfe` | `SetMaxVehAmbient` | [`Game::PatchMaxVehAmbient`](src/Game.cpp#L364-L376) |
| `0x180006d8c` | `SetMaxVehParked` | [`Game::PatchMaxVehParked`](src/Game.cpp#L378-L390) |
| `0x180006e1a` | `SetPedPoolLimit` | [`Game::PatchPedPoolLimit`](src/Game.cpp#L395-L425) |
| `0x180006f6c` | `LimitFix1` | [`Game::PatchLimitFixInstance1`](src/Game.cpp#L427-L463) |
| `0x1800070e8` | `LimitFix2` | [`Game::PatchLimitFixInstance2`](src/Game.cpp#L465-L523) |
| `0x180007312` | `LimitFix3` | [`Game::PatchLimitFixInstance3`](src/Game.cpp#L525-L538) |
| `0x1800073a0` | `LimitFix4` | [`Game::PatchLimitFixInstance4`](src/Game.cpp#L540-L560) |

### B. Upstream *XtremeTool 1.1 Legacy* (`GTA5.exe`, Image Base `0x180000000`)

| Upstream VMA | Upstream Function | `XtremeFork-Universal` Implementation |
| :--- | :--- | :--- |
| `0x180007330` | `DllMain` | [`DllMain`](src/dllmain.cpp#L14-L33) |
| `0x180005b50` | `Game::Initialize` | [`Game::Initialize`](src/Game.cpp#L885-L1057) |
| `0x180005d5d` | `SetHeapSize32` (Legacy) | [`Game::PatchLegacyMainHeapSize32`](src/Game.cpp#L571-L582) |
| `0x180005e1a` | `SetHeapSize64` (Legacy) | [`Game::PatchLegacyMainHeapSize64`](src/Game.cpp#L584-L621) |
| `0x180006129` | `SetPoolSize32` (Legacy) | [`Game::PatchLegacyPoolHeapSize32`](src/Game.cpp#L632-L643) |
| `0x1800061e7` | `SetPoolSize64` (Legacy) | [`Game::PatchLegacyPoolHeapSize64`](src/Game.cpp#L645-L678) |
| `0x1800064c7` | `SetResourceSize32` (Legacy) | [`Game::PatchLegacyResourceHeapSize32`](src/Game.cpp#L689-L700) |
| `0x180006585` | `SetResourceSize64` (Legacy) | [`Game::PatchLegacyResourceHeapSize64`](src/Game.cpp#L702-L735) |
| `0x180006865` | `SetPedMemSize` (Legacy) | [`Game::PatchLegacyPedMemoryBudget`](src/Game.cpp#L742-L754) |
| `0x180006ce6` | `SkipPedsPathfindPass` (Legacy) | [`Game::PatchLegacySkipPedsPathfindPass`](src/Game.cpp#L761-L776) |
| `0x180006e2d` | `SetPedSpawnAmbientRate` (Legacy) | [`Game::PatchLegacyPedSpawnAmbientMultiplier`](src/Game.cpp#L781-L793) |
| `0x180006ed2` | `SetPedSpawnScenarioRate` (Legacy) | [`Game::PatchLegacyPedSpawnScenarioMultiplier`](src/Game.cpp#L795-L807) |
| `0x180006f77` | `SetVehSpawnAmbientRate` (Legacy) | [`Game::PatchLegacyVehSpawnAmbientMultiplier`](src/Game.cpp#L809-L821) |
| `0x18000701c` | `SetVehSpawnParkedRate` (Legacy) | [`Game::PatchLegacyVehSpawnParkedMultiplier`](src/Game.cpp#L823-L835) |
| `0x1800070c1` | `SetMaxPeds` (Legacy) | [`Game::PatchLegacyMaxPeds`](src/Game.cpp#L840-L852) |
| `0x180007152` | `SetMaxVehAmbient` (Legacy) | [`Game::PatchLegacyMaxVehAmbient`](src/Game.cpp#L854-L866) |
| `0x1800071e3` | `SetMaxVehParked` (Legacy) | [`Game::PatchLegacyMaxVehParked`](src/Game.cpp#L868-L880) |

---

## 6. Complete Memory Signature Reference

### A. GTA V Enhanced (`GTA5_Enhanced.exe`, Builds `v1.0.811.8` – `v1.0.1158.13+`)

| Function | Byte Pattern | Mask | Offset | Patch Operation |
| :--- | :--- | :--- | :--- | :--- |
| `PatchMainHeapSize32/64` | `BA ?? ?? ?? ?? 45 31 C0 41 B1 01 E8 ?? ?? ?? ?? 48 8D 0D` | `x????xxxxxxx????xxx` | `+0x01` (32b) / `+0x00` (64b) | 32b: `WriteU32(heapBytes)` at `+1`; 64b: `FF 25` + `90 90` (8B) at `+0` -> trampoline (`movabs rdx, imm64; xor r8d, r8d`) |
| `PatchPoolHeapSize32/64` | `48 ?? ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? ?? ?? 41 B8 00 00 01 00 41 B9 ?? ?? ?? ?? E8` | `x??????x??????xxxxxxxx????x` | `+0x16` (32b) / `+0x14` (64b) | 32b: `WriteU32(poolBytes)` at `+0x16`; 64b: `FF 25` (6B) at `+0x14` -> trampoline (`movabs r9, imm64`) |
| `PatchResourceHeapSize32/64` | `48 ?? ?? ?? ?? ?? ?? BA 00 20 00 00 41 B8 ?? ?? ?? ??` | `x??????xxxxxxxx????` | `+0x0E` (32b) / `+0x0C` (64b) | 32b: `WriteU32(resourceBytes)` at `+0x0E`; 64b: `FF 25` (6B) at `+0x0C` -> trampoline (`movabs r8, imm64`) |
| `PatchPedMemoryBudget` | `05 60 E3 16 00` | `xxxxx` | `+0x01` (all matches) | `WriteU32(pedMemBytes)` on every match in `.text` |
| `PatchSkipPedsPathfindPass` | `48 89 44 24 30 89 5C 24 20 48 8D 0D` | `xxxxxxxxxxxx` | `+0x00` | `31 DB 90 90 90` (`xor ebx, ebx; nop; nop; nop`, 5B) |
| `PatchPedSpawnAmbientMultiplier` | `48 8D 51 28 48 83 C1 2C E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(mult * 100)> 90 90 90` (13B) |
| `PatchPedSpawnScenarioMultiplier` | `48 8D 51 20 48 83 C1 24 E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(mult * 100)> 90 90 90` (13B) |
| `PatchVehSpawnAmbientMultiplier` | `48 8D 51 4C 48 83 C1 50 E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(mult * 100)> 90 90 90` (13B) |
| `PatchVehSpawnParkedMultiplier` | `48 8D 51 58 48 83 C1 5C E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(mult * 100)> 90 90 90` (13B) |
| `PatchMaxPeds` | `48 8D 51 30 48 83 C1 34 E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(pedLimit - 10)> 90 90 90` (13B) |
| `PatchMaxVehAmbient` | `48 8D 51 68 48 83 C1 6C E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(vehLimit - 10)> 90 90 90` (13B) |
| `PatchMaxVehParked` | `48 8D 51 74 48 83 C1 78 E8` | `xxxxxxxxx` | `+0x00` | `48 B8 <int64(vehLimit - 20)> 90 90 90` (13B) |
| `PatchPedPoolLimit` (1 & 2) | `E8 ?? ?? ?? ?? 48 89 F9 BA ...` / `B9 30 00 00 00 E8 ...` | `x????xxxx????xxxxxxx????xx?` | `+0x13` / `+0x19` | `B8 <uint32(pedLimit)>` (`mov eax, imm32`, 5B) |
| `PatchLimitFixInstance1` | `B8 48 10 00 00 E8 ?? ?? ?? ?? 48 29 C4 48 8B 05 ?? ?? ?? ?? 48 63 70 5C` | `xxxxxx????xxxxxx????xxxx` | `+0x01` & `add rsp` `+0x03` | `AddU32((pedLimit - 256) * 16)` |
| `PatchLimitFixInstance2` | `41 56 56 57 53 48 81 EC 28 0C 00` | `xxxxxxxxxxx` | `+0x08` & inner offsets | `AddU32` with `(pedLimit - 256) * 12` and `* 8` |
| `PatchLimitFixInstance3` | `BA 56 44 A5 8D 41 B8 01 00 00 00` | `xxxxxxxxxxx` | `+0x0B` | `B8 <uint32(pedLimit)>` (5B) |
| `PatchLimitFixInstance4` | `75 EF EB 20 8B 40 04 83 F8 FF` | `xxxxxxxxxx` | `-0x0C` | `B8 80 A9 03 00 90 90 90` (`mov eax, 240000; nop*3`, 8B) |

### B. GTA V Legacy (`GTA5.exe`, Builds `v1.0.3095.0` – `v1.0.3889.0+`)

| Function | Byte Pattern | Mask | Offset | Patch Operation |
| :--- | :--- | :--- | :--- | :--- |
| `PatchLegacyMainHeapSize32/64` | `48 8D 0D ?? ?? ?? ?? 41 B1 01 45 33 C0 BA` | `xxx????xxxxxxx` | `+0x0E` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xE`; 64b: `FF 25` + `90 90` (8B) at `+0xA` -> trampoline (`movabs rdx, imm64; xor r8d, r8d`) |
| `PatchLegacyPoolHeapSize32/64` | `83 C8 04 48 8D 0D ?? ?? ?? ?? 41 B8` | `xxxxxx????xx` | `+0x0C` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xC`; 64b: `FF 25` (6B) at `+0xA` -> trampoline (`movabs r8, imm64`) |
| `PatchLegacyResourceHeapSize32/64` | `83 C8 20 48 8D 0D ?? ?? ?? ?? 41 B8` | `xxxxxx????xx` | `+0x0C` (32b) / `+0x0A` (64b) | 32b: `WriteU32(bytes)` at `+0xC`; 64b: `FF 25` (6B) at `+0xA` -> trampoline (`movabs r8, imm64`) |
| `PatchLegacyPedMemoryBudget` | `41 BE 60 E3 16 00` | `xxxxxx` | `+0x02` | `WriteU32(pedMemBytes)` |
| `PatchLegacySkipPedsPathfindPass` | `48 8B 05 ?? ?? ?? ?? 4C 8B C7 48 ?? ?? ?? 30` | `xxx????xxxx???x` | `+0x0A` | `4D 31 D2 90 90` (`xor r10, r10; nop; nop`, 5B) |
| `PatchLegacyPedSpawnAmbientMultiplier` | `48 83 C1 2C F3 44 0F 59 D6 E8` | `xxxxxxxxxx` | `+0x09` | `B8 <uint32(mult * 100)>` (`mov eax, imm32`, 5B) |
| `PatchLegacyPedSpawnScenarioMultiplier` | `48 8D 51 20 48 83 C1 24 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(mult * 100)>` (5B) |
| `PatchLegacyVehSpawnAmbientMultiplier` | `48 8D 51 4C 48 83 C1 50 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(mult * 100)>` (5B) |
| `PatchLegacyVehSpawnParkedMultiplier` | `48 8D 51 58 48 83 C1 5C E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(mult * 100)>` (5B) |
| `PatchLegacyMaxPeds` | `F3 0F 59 C2 F3 0F 2C D8 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxPeds)>` (min `1000`, or `pedLimit - 10` if higher) |
| `PatchLegacyMaxVehAmbient` | `48 8D 51 68 48 83 C1 6C E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxVehAmbient)>` (min `1000`, or `vehLimit - 10` if higher) |
| `PatchLegacyMaxVehParked` | `48 8D 51 74 48 83 C1 78 E8` | `xxxxxxxxx` | `+0x08` | `B8 <uint32(maxVehParked)>` (min `1000`, or `vehLimit - 20` if higher) |
