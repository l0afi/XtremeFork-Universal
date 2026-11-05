# Release v1.0.0-beta.1: XtremeFork-Universal

Welcome to the first beta release of **XtremeFork-Universal**!

I created this project to unite and modernize the original *XtremeTool Enhanced* and *XtremeTool Legacy* mods by Xtreme into a single, unified C++20 `.asi` plugin. Whether you are running **GTA V Enhanced** (`GTA5_Enhanced.exe`) or **GTA V Legacy** (`GTA5.exe`), this single build automatically detects your game edition at startup and applies the right memory patches, density multipliers, and pool caps without needing separate binaries.

---

## 🌟 What I've Built & Included in this Release

### 🌍 One Plugin for Both Game Editions
- **Automatic Edition Detection**: You no longer need to worry about which edition of XtremeTool to download. The plugin identifies whether the game process is running `GTA5_Enhanced.exe` (builds `1.0.811.8` – `1.0.1158.13+`) or `GTA5.exe` (builds `1.0.3095.0` – `1.0.3889.0+`) at boot, with a fallback pattern scan if your executable is renamed.
- **Unified Config**: Both editions now share the exact same `XtremeTool.ini` file.

### 🚗 High-Density Population & Spawn Multipliers
- **Independent Multipliers**: You can independently dial in spawn rates for walking ambient pedestrians, scenario peds doing activities, active moving traffic, and parked cars.
- **Expanded Limits**: I increased the active pedestrian and vehicle caps so the game can spawn larger crowds without choking entity limits.
- **Benchmarking Mode**: If you want to benchmark pure rendering performance without CPU pathfinding overhead, set `SKIP_PEDS_PATHFIND_PASS = 1` to freeze pedestrian navigation logic.

### 🧠 64-Bit Memory Unlocker & Stability Fixes
- **Broke the 2 GB / 4 GB Heap Limit**: The plugin unlocks 64-bit allocation sizes for the Main Heap, Pool Heap, and Resource Heap on both Enhanced and Legacy versions.
- **Fixed the Legacy 64-Bit Trampoline Crash**: In older versions of the legacy mod, 64-bit heap hooks were disabled because the jump trampolines wrote pointers into surrounding game `.text`, causing immediate crashes. I implemented a safe near-memory page allocator that keeps all hook pointers in isolated memory.
- **Fixed Arithmetic Overflows on Large Heaps**: I converted all heap calculation arithmetic to full 64-bit integers, preventing memory overflows and crashes when configuring heaps of 4096 MB (4 GB) or larger.
- **Memory Protection Wrappers**: Every memory write is now wrapped in proper `VirtualProtect` calls across the exact modified byte lengths.

---

## 📦 What's Inside the Download (`XtremeFork-Universal-Windows-x64.zip`)

- `XtremeTool.asi` — The pre-compiled Windows x64 plugin (built with MSVC C++20, statically linked with zero extra DLL dependencies).
- `XtremeTool.ini` — Default configuration template ready for your tweaks.
- `ReadMe_Install.txt` & `Documentation_Config.txt` — Quick-start and config documentation.

---

## 🚀 How to Install

1. Make sure you have **[ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)** (`ScriptHookV.dll` and an ASI loader like `dinput8.dll`) installed in your GTA V root directory.
2. Download `XtremeFork-Universal-Windows-x64.zip` from the assets section below.
3. Extract `XtremeTool.asi` and `XtremeTool.ini` directly into your main GTA V directory (where `GTA5_Enhanced.exe` or `GTA5.exe` lives).
4. Launch the game! You can check `XtremeTool.log` in your game directory to see which edition was detected and verify that your custom settings loaded cleanly.

---

## ⚠️ Notes & Feedback

- **Beta Status**: While I have verified stability and addressed known memory corruptions from earlier releases, please make sure you back up your game files and saves before loading heavy modlists.
- **Vehicle Pool Warning**: If you set `VEH_LIMIT` higher than vanilla, remember to match the pool size in your `gameconfig.xml` (`Vehicle` and `CVehicle` pools) so the engine reserves enough space.
- **Feedback**: If you run into any issues, crashes, or pattern-match errors on newer game updates, please open an issue on the repository and include your `XtremeTool.log`!
