# Release v1.0.0-alpha: XtremeTool Universal

Initial alpha release of **XtremeTool Universal** — an open-source, modern C++20 recreation of XtremeTool supporting both **GTA V Enhanced** (`GTA5_Enhanced.exe`) and **GTA V Legacy** (`GTA5.exe`) through a single, unified `.asi` binary.

---

## 🌟 What's New

### 🌍 Universal Architecture
- **Automatic Edition Detection**: Detects whether the game process is running `GTA5_Enhanced.exe` or `GTA5.exe` at startup, with a fallback pattern scan if the executable is renamed.
- **Unified Configuration**: Use one single `XtremeTool.ini` config file regardless of which GTA V edition you are playing.

### 🚗 Density & Population Controls
- **Adjustable Pedestrian Spawn Rates**: Custom multipliers for both ambient walking pedestrians and scenario peds.
- **Adjustable Vehicle Spawn Rates**: Custom multipliers for active traffic vehicles and parked roadside/lot vehicles.
- **Expanded Entity Limits**:
  - Raise max pedestrian pools and active spawned limits (`PED_LIMIT`).
  - Raise max ambient and parked vehicle limits (`VEH_LIMIT`).
- **Benchmarking Mode**: Option to toggle `SKIP_PEDS_PATHFIND_PASS` to bypass pedestrian pathfinding logic for high-entity FPS benchmarking.

### 🧠 64-Bit Memory Allocator & Stability Fixes
- **Break the 2GB / 4GB Barrier**: Safely enables 64-bit allocation sizes for the Main Heap (`HEAP_SIZE`), Pool Heap (`POOL_SIZE`), and Resource Heap (`RESOURCE_SIZE`).
- **Safe Trampoline Allocator**: Fixes a critical bug from original legacy releases where 64-bit jumps overwrote surrounding game code in `.text`. Trampolines and pointer slots are now safely allocated in dedicated memory pages within RIP-relative reach.
- **64-Bit Math Overflow Protection**: Arithmetic on heap sizes now uses 64-bit integers to prevent truncation and negative allocation sizes when configuring heaps $\ge$ 4096 MB.
- **Memory Protection Wrap**: All patch writes safely acquire `PAGE_EXECUTE_READWRITE` permissions before modifying memory.

---

## 📦 Included Assets & Files

- `XtremeTool.asi` — Pre-compiled Windows x64 binary (zero external runtime dependencies).
- `XtremeTool.ini` — Default configuration template.
- `ReadMe_Install.txt` & `Documentation_Config.txt` — Quick-start and config documentation.

---

## 🚀 Installation

1. Ensure **[ScriptHookV](http://www.dev-c.com/gtav/scripthookv/)** (`ScriptHookV.dll` + ASI loader) is installed in your main game directory.
2. Copy `XtremeTool.asi` and `XtremeTool.ini` from the release package (or `dist/Put in main folder/`) into your main GTA V folder:
   - For **GTA V Enhanced**: folder containing `GTA5_Enhanced.exe`.
   - For **GTA V Legacy**: folder containing `GTA5.exe`.
3. Launch GTA V. Check `XtremeTool.log` in your game folder to verify successful initialization.

---

## ⚠️ Notes & Known Issues

- **Alpha Release**: Please back up your game directory and save files before installing.
- **Vehicle Pool in `gameconfig.xml`**: When setting `VEH_LIMIT` higher than vanilla, ensure your `gameconfig.xml` vehicle pool sizes match your configured limit.
- **AI-Assisted Reconstruction**: This project was reconstructed and reverse-engineered with the assistance of an AI coding agent based on the original work by **Xtreme**.
