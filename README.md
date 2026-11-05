<div align="center">

# ⚡ XtremeFork-Universal

**High-Density Traffic, Crowd & Memory Unlocker for GTA V (Enhanced & Legacy)**

[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?style=for-the-badge&logo=windows&logoColor=white)](#)
[![Enhanced](https://img.shields.io/badge/GTA%20V%20Enhanced-v1.0.811.8%E2%80%931158.13+-2ea44f?style=for-the-badge)](#)
[![Legacy](https://img.shields.io/badge/GTA%20V%20Legacy-v1.0.3095.0%E2%80%933889.0+-1f6feb?style=for-the-badge)](#)
[![Runtime](https://img.shields.io/badge/API-ScriptHookV%20(.asi)-FF6F00?style=for-the-badge)](#)
[![Experimental Build](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fapi.github.com%2Frepos%2Fl0afi%2FXtremeFork-Universal%2Freleases%2Ftags%2Fexperimental&query=%24.name&label=&color=8A2BE2&style=for-the-badge)](https://github.com/l0afi/XtremeFork-Universal/releases/tag/experimental)

One universal `.asi` plugin for both **GTA V Enhanced** (`GTA5_Enhanced.exe`) and **GTA V Legacy** (`GTA5.exe`).  
Boost pedestrian and traffic density, expand memory heaps beyond 4 GB, and raise entity pool limits with a single config file.

[Features](#-features) •
[Fixes & Improvements](#-fixes--improvements-for-current-gta-v) •
[Installation](#-installation) •
[Configuration](#-configuration-xtremetoolini) •
[Building](#-building-from-source)

</div>

---

> [!IMPORTANT]
> ### 🤖 AI Disclaimer
> **XtremeFork-Universal**—including its C++20 source code, universal edition detection, stability fixes, automated CI build pipeline, and documentation—was generated, reverse-engineered, and reconstructed with the assistance of an **AI coding agent (Antigravity)** based on the original *XtremeTool Enhanced v1.2* and *XtremeTool Legacy v1.1* releases by **Xtreme** on [GTA5-Mods.com](https://www.gta5-mods.com/misc/xtremetool-enhanced). Always back up your game folder before installing memory-patching mods.

---

## ✨ Features

* 🌍 **Universal Edition Support** — Automatically detects whether you are launching **GTA V Enhanced** (`GTA5_Enhanced.exe`, builds `1.0.811.8` – `1.0.1158.13+`) or **GTA V Legacy** (`GTA5.exe`, builds `1.0.3095.0` – `1.0.3889.0+`) and applies the right patches on the fly.
* 🚶 **Custom Crowd & Traffic Multipliers** — Independently control how quickly walking pedestrians, scenario pedestrians, moving traffic, and parked cars spawn around you.
* 🧠 **64-Bit Memory Unlocker** — Break past the game's built-in 2 GB / 4 GB memory limits for the Main Heap, Pool Heap, and Resource Heap so heavy modlists don't crash with out-of-memory errors.
* 📈 **Higher Pedestrian & Vehicle Caps** — Raise active pedestrian and vehicle limits directly from `XtremeTool.ini`.
* ⚡ **CPU Benchmark Mode** — Optionally freeze pedestrian pathfinding (`SKIP_PEDS_PATHFIND_PASS`) to test maximum crowd rendering performance without AI CPU bottlenecks.

---

## 🛠️ Fixes & Improvements for Current GTA V

Several stability bugs in the original releases prevented certain features from working properly on modern versions of GTA V. **XtremeFork-Universal** fixes them out of the box:

### 1. Re-Enabled 64-Bit Memory Unlocks on GTA V Legacy
In the original *XtremeTool 1.1 Legacy* release, the 64-bit memory toggles (`USE_64BIT_HEAP_SIZE`, `USE_64BIT_POOL_SIZE`, and `USE_64BIT_RESOURCE_SIZE`) were silently disabled in code because their memory jumps overwrote nearby game instructions and crashed on launch. **XtremeFork-Universal** uses a safe memory allocator that never touches surrounding game code, restoring full 64-bit heap support on both Legacy and Enhanced.

### 2. Fixed Crashes When Setting Memory Above 4 GB
Previously, setting `HEAP_SIZE`, `POOL_SIZE`, or `RESOURCE_SIZE` to **4096 MB (4 GB) or higher** caused a number overflow that passed a zero or negative size to the game engine, immediately crashing at the loading screen. All memory calculations now use full 64-bit math.

### 3. Crash-Safe Memory Patching Across Game Updates
Earlier builds wrote to parts of the game executable without always unlocking read-only memory protection first, or relied on fixed memory offsets that could break after a title update. Every patch in **XtremeFork-Universal** dynamically scans for the right game code and safely unlocks memory permissions before applying changes.

### 4. Fixed Startup Stack Corruption
The configuration loader and logger were rewritten to avoid large memory buffers on the main game thread during startup, preventing random crashes when loading alongside other ScriptHookV plugins.

---

## 📦 Installation

Pre-compiled, ready-to-use binaries are automatically published on every build:

* 🧪 **Latest Experimental Build:** Download the latest rolling pre-release [`XtremeFork-Universal-Windows-x64.zip`](https://github.com/l0afi/XtremeFork-Universal/releases/tag/experimental) from the **[Experimental Release Tag](https://github.com/l0afi/XtremeFork-Universal/releases/tag/experimental)** (showing the latest build commit hash).
* 🏷️ **Stable / Tagged Releases:** Available under the **[Releases Tab](https://github.com/l0afi/XtremeFork-Universal/releases)**.

1. Make sure you have **[ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)** (`ScriptHookV.dll` and an ASI Loader such as `dinput8.dll`) installed in your main GTA V folder.
2. Download the latest release package (`XtremeFork-Universal-Windows-x64.zip`) from **[Releases](https://github.com/l0afi/XtremeFork-Universal/releases)**.
3. Extract the contents directly into your main GTA V folder (where `GTA5_Enhanced.exe` or `GTA5.exe` is located):
   * `XtremeTool.asi`
   * `XtremeTool.ini`
4. *(Optional)* Open `XtremeTool.ini` in any text editor to customize your traffic multipliers and memory budgets.
5. Launch the game! A `XtremeTool.log` file will be created in your game directory confirming which edition was detected and which settings were applied.

---

## ⚙️ Configuration (`XtremeTool.ini`)

All settings are configured inside `XtremeTool.ini`:

### 🌆 Spawn Multipliers
> Controls **how fast** peds and vehicles spawn around the player. Combine with `PED_LIMIT` and `VEH_LIMIT` to increase the total number of active entities on screen.

| Setting | Default | What It Does |
| :--- | :---: | :--- |
| `PED_SPAWN_AMBIENT_MULTIPLIER` | `3.0` | Multiplier for walking pedestrians on sidewalks. |
| `PED_SPAWN_SCENARIO_MULTIPLIER` | `3.0` | Multiplier for pedestrians doing stationary activities. |
| `VEH_SPAWN_AMBIENT_MULTIPLIER` | `3.0` | Multiplier for active driving traffic. |
| `VEH_SPAWN_PARKED_MULTIPLIER` | `2.0` | Multiplier for parked cars along streets and lots. |

### 💾 Memory & Heap Sizes
> Values are in **Megabytes (MB)**. Always keep `POOL_SIZE` smaller than `HEAP_SIZE`.

| Setting | Default | What It Does |
| :--- | :---: | :--- |
| `USE_64BIT_HEAP_SIZE` | `1` | Set to `1` when `HEAP_SIZE` is over `2048` MB (2 GB). |
| `HEAP_SIZE` | `2000` | Main game memory heap in MB *(Enhanced default: `1280` MB)*. |
| `USE_64BIT_POOL_SIZE` | `0` | Set to `1` when `POOL_SIZE` is over `4096` MB (4 GB). |
| `POOL_SIZE` | `1000` | Entity pool heap in MB *(Enhanced default: `688` MB)*. |
| `USE_64BIT_RESOURCE_SIZE` | `1` | Set to `1` when `RESOURCE_SIZE` is over `4096` MB (4 GB). |
| `RESOURCE_SIZE` | `4500` | Streaming resource heap in MB *(Enhanced default: `3500` MB)*. |
| `PED_MEM_SIZE` | `250` | Memory budget for pedestrian models in MB (increase for addon peds). |
| `VEH_MEM_SIZE` | `500` | Memory budget for vehicle models in MB *(Enhanced only)*. |

### 🚗 Pool Limits & Performance
> [!WARNING]
> When raising `VEH_LIMIT`, make sure your `gameconfig.xml` (`Vehicle` and `CVehicle` pools) matches the same number so the game has enough vehicle slots allocated.

| Setting | Default | Edition Support | What It Does |
| :--- | :---: | :---: | :--- |
| `PED_LIMIT` | `340` | **Enhanced & Legacy** | Raises max active pedestrians (up to `340` on Enhanced without editing `gameconfig.xml`; unlocks active ped cap on Legacy). |
| `VEH_LIMIT` | `500` | **Enhanced & Legacy** | Raises max active ambient and parked vehicles (requires matching `Vehicle` & `CVehicle` pool sizes in `gameconfig.xml`). |
| `SKIP_PEDS_PATHFIND_PASS` | `0` | **Enhanced & Legacy** | Set to `1` to freeze pedestrian movement AI for high-FPS crowd benchmarking. |

---

## 🔨 Building from Source

If you want to modify the code and compile your own binary:

### Option A: Windows (Visual Studio / CMake)
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Option B: Cross-Compile (macOS / Linux via Zig or MinGW-w64)
```bash
zig c++ -target x86_64-windows-gnu -shared -std=c++20 -O2 -static -s \
  -Iinc -Isrc src/dllmain.cpp src/Config.cpp src/Logger.cpp src/Pattern.cpp src/Game.cpp \
  -lpsapi -o "build/XtremeTool.asi"
```

> 💡 **For Developers & Reverse Engineers:** Deep technical documentation—including supported GTA V build numbers, assembly patches, memory signatures, VMA maps, and architecture notes for both Enhanced and Legacy—is located in [`AGENTS.md`](AGENTS.md).

---

## 📜 Credits

* **Xtreme** — Original creator of [XtremeTool Enhanced](https://www.gta5-mods.com/misc/xtremetool-enhanced) and [XtremeTool Legacy](https://www.gta5-mods.com/misc/xtremetool-legacy).
* **Alexander Blade** — Creator of [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/).
