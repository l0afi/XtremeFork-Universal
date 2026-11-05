<div align="center">

# ⚡ XtremeTool Universal
### *High-Density Traffic, Crowd & Memory Unlocker for GTA V (Enhanced & Legacy)*

[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-0078D6?style=for-the-badge&logo=windows&logoColor=white)](#)
[![Edition](https://img.shields.io/badge/GTA%20V-Enhanced%20%26%20Legacy-2ea44f?style=for-the-badge)](#)
[![Runtime](https://img.shields.io/badge/API-ScriptHookV%20(.asi)-FF6F00?style=for-the-badge)](#)
[![Prebuilt](https://img.shields.io/badge/Binary-XtremeTool.asi%20Included-8A2BE2?style=for-the-badge)](#)

<p align="center">
  <b>One universal <code>.asi</code> plugin for both <i>GTA V Enhanced</i> (<code>GTA5_Enhanced.exe</code>) and <i>GTA V Legacy</i> (<code>GTA5.exe</code>).</b><br/>
  Boost pedestrian and traffic density, expand memory heaps beyond 4 GB, and raise entity pool limits with a single config file.
</p>

---

[✨ Features](#-features) •
[🛠️ Fixes & Improvements](#️-fixes--improvements-for-current-gta-v) •
[📦 Installation](#-installation) •
[⚙️ Configuration](#️-configuration-xtremetoolini) •
[🔨 Building](#-building-from-source)

</div>

> [!IMPORTANT]
> ### 🤖 AI Disclaimer
> This repository—including its C++20 source code, universal edition detection, stability fixes, pre-built `.asi` binary, and documentation—was generated, reverse-engineered, and reconstructed with the assistance of an **AI coding agent (Antigravity)** based on the original *XtremeTool Enhanced v1.2* and *XtremeTool Legacy v1.1* releases by **Xtreme** on [GTA5-Mods.com](https://www.gta5-mods.com/misc/xtremetool-enhanced). Always back up your game folder before installing memory-patching mods.

---

## ✨ Features

* 🌍 **Universal Edition Support** — Automatically detects whether you are launching **GTA V Enhanced** (`GTA5_Enhanced.exe`) or **GTA V Legacy** (`GTA5.exe`) and applies the right patches on the fly.
* 🚶 **Custom Crowd & Traffic Multipliers** — Independently control how quickly walking pedestrians, scenario pedestrians, moving traffic, and parked cars spawn around you.
* 🧠 **64-Bit Memory Unlocker** — Break past the game's built-in 2 GB / 4 GB memory limits for the Main Heap, Pool Heap, and Resource Heap so heavy modlists don't crash with out-of-memory errors.
* 📈 **Higher Pedestrian & Vehicle Caps** — Raise active pedestrian and vehicle limits directly from [`XtremeTool.ini`](dist/Put%20in%20main%20folder/XtremeTool.ini).
* ⚡ **CPU Benchmark Mode** — Optionally freeze pedestrian pathfinding (`SKIP_PEDS_PATHFIND_PASS`) to test maximum crowd rendering performance without AI CPU bottlenecks.

---

## 🛠️ Fixes & Improvements for Current GTA V

Several stability bugs in the original releases prevented certain features from working properly on modern versions of GTA V. This universal version fixes them out of the box:

### 1. Re-Enabled 64-Bit Memory Unlocks on GTA V Legacy
In the original *XtremeTool 1.1 Legacy* release, the 64-bit memory toggles (`USE_64BIT_HEAP_SIZE`, `USE_64BIT_POOL_SIZE`, and `USE_64BIT_RESOURCE_SIZE`) were silently disabled in code because their memory jumps overwrote nearby game instructions and crashed on launch. This build uses a safe memory allocator that never touches surrounding game code, restoring full 64-bit heap support on both Legacy and Enhanced.

### 2. Fixed Crashes When Setting Memory Above 4 GB
Previously, setting `HEAP_SIZE`, `POOL_SIZE`, or `RESOURCE_SIZE` to **4096 MB (4 GB) or higher** caused a number overflow that passed a zero or negative size to the game engine, immediately crashing at the loading screen. All memory calculations now use full 64-bit math.

### 3. Crash-Safe Memory Patching Across Game Updates
Earlier builds wrote to parts of the game executable without always unlocking read-only memory protection first, or relied on fixed memory offsets that could break after a title update. Every patch in this version dynamically scans for the right game code and safely unlocks memory permissions before applying changes.

### 4. Fixed Startup Stack Corruption
The configuration loader and logger were rewritten to avoid large memory buffers on the main game thread during startup, preventing random crashes when loading alongside other ScriptHookV plugins.

---

## 📦 Installation

A pre-compiled **`XtremeTool.asi`** binary is ready to use in the [`dist/Put in main folder/`](dist/Put%20in%20main%20folder/) directory.

1. Make sure you have **[ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)** (`ScriptHookV.dll` and an ASI Loader such as `dinput8.dll`) installed in your main GTA V folder.
2. Open [`dist/Put in main folder/`](dist/Put%20in%20main%20folder/) and copy both files into your main GTA V folder (next to `GTA5_Enhanced.exe` or `GTA5.exe`):
   * [`XtremeTool.asi`](dist/Put%20in%20main%20folder/XtremeTool.asi)
   * [`XtremeTool.ini`](dist/Put%20in%20main%20folder/XtremeTool.ini)
3. *(Optional)* Open `XtremeTool.ini` in any text editor to customize your traffic multipliers and memory budgets.
4. Launch the game! A `XtremeTool.log` file will be created in your game folder confirming which edition was detected and which settings were applied.

---

## ⚙️ Configuration (`XtremeTool.ini`)

All settings live in [`XtremeTool.ini`](dist/Put%20in%20main%20folder/XtremeTool.ini):

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

If you want to modify the code and compile your own `XtremeTool.asi`:

### Option A: Windows (Visual Studio / CMake)
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### Option B: Cross-Compile (macOS / Linux via Zig or MinGW-w64)
```bash
zig c++ -target x86_64-windows-gnu -shared -std=c++20 -O2 -static -s \
  -Iinc -Isrc src/dllmain.cpp src/Config.cpp src/Logger.cpp src/Pattern.cpp src/Game.cpp \
  -lpsapi -o "dist/Put in main folder/XtremeTool.asi"
```

> 💡 **For Developers & Reverse Engineers:** Deep technical documentation—including assembly patches, memory signatures, VMA maps, and architecture notes for both Enhanced and Legacy—is located in [`AGENTS.md`](AGENTS.md).

---

## 📜 Credits

* **Xtreme** — Original creator of [XtremeTool Enhanced](https://www.gta5-mods.com/misc/xtremetool-enhanced) and [XtremeTool Legacy](https://www.gta5-mods.com/misc/xtremetool-legacy).
* **Alexander Blade** — Creator of [ScriptHookV](http://www.dev-c.com/gtav/scripthookv/).
