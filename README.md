<div align="center">
# ⚡ XtremeFork-Universal
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
