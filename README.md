# tkde-app — KDE Plasma in Termux, Fully Integrated

<p align="center">
  <img src="https://img.shields.io/badge/version-2.1.0-blue?style=flat-square" alt="Version 2.1.0">
  <img src="https://img.shields.io/badge/platform-Termux%2BAndroid%20ARM64-red?style=flat-square" alt="Platform">
  <img src="https://img.shields.io/badge/C%2B%2B-20-purple?style=flat-square" alt="C++20">
  <img src="https://img.shields.io/badge/license-GPL--3.0-green?style=flat-square" alt="GPL v3">
  <img src="https://img.shields.io/badge/GPU-Mali--G72-orange?style=flat-square" alt="Mali GPU">
</p>

> **What is this?** A single compiled C++ binary (`tkde-app`) that gives you a full keyboard-navigable TUI to manage every aspect of running KDE Plasma desktop inside Termux on Android — from display servers to GPU drivers to app installation to theme switching.

---

## Table of Contents

1. [Features](#-features)
2. [Screenshots](#-screenshots)
3. [Architecture](#-architecture)
4. [Requirements](#-requirements)
5. [Installation](#-installation)
6. [Usage](#-usage)
7. [Configuration](#-configuration)
8. [GPU Acceleration](#-gpu-acceleration)
9. [Building from Source](#-building-from-source)
10. [Keyboard Controls](#-keyboard-controls)
11. [Project Structure](#-project-structure)
12. [Credits](#-credits)

---

## ✨ Features

### Display Manager
- **TigerVNC** — Start/stop VNC server with one command
- **Termux:X11** — Launch native X11 display with hardware acceleration
- **GPU/CPU modes** — Toggle hardware-accelerated vs software rendering
- **Display forwarding** — Forward your session to any remote IP address
- **Real-time status** — Live VNC + X11 status in the status bar

### GPU Monitor
- **Auto-detection** — Detects your GPU model and core count automatically
- **Live stats** — Real-time GPU utilization and frequency monitoring
- **Driver switching** — Switch between VirGL, Zink, and Turnip without restarting
- **Benchmarking** — Built-in GPU load test
- **Supported drivers**: VirGL, VirGL+ANGLE, VirGL+Vulkan, Zink, Zink+VirGL, Turnip, Mali Native

### KDE Plasma Manager
- **Session control** — Start, stop, and restart your KDE Plasma session
- **Theme switcher** — One-command switch between Catppuccin, Nord, Dracula, and Tokyo Night
- **Compositor control** — Enable/disable KWin compositor on the fly
- **Component management** — View and restart individual KDE components (KRunner, Baloo, Klipper, etc.)
- **D-Bus integration** — Full D-Bus control via `qdbus`
- **Font DPI control** — Adjust display scaling from the app

### App Store
- **25+ apps** across 5 categories: Internet, Multimedia, Development, Utility, System
- **Native + Distro** — Installs both native Termux packages and proot-distro apps
- **Wine support** — Install Wine (stock or Hangover) to run Windows executables
- **One-click install** — Press Enter to install, status shown in-app
- **Category browsing** — Filter by category, see installed/not-installed state

### Distro Manager
- **Proot-distro integration** — Manage Debian, Ubuntu, Arch Linux, Fedora
- **User management** — Add users to distros with password setup
- **Turnip support** — Install Turnip (Adreno GPU driver) inside distros
- **Desktop environment install** — Install KDE, XFCE, LXQt inside any distro
- **Audio configuration** — Configure PulseAudio for proot-distro sessions
- **Disk usage** — View and clean up distro cache

### Shell Manager
- **Zsh themes** — Install td_zsh (custom), Pure, or Powerlevel10k
- **Nerd Fonts** — 0xProto, FiraCode, JetBrains Mono, Hack
- **Neovim distributions** — NvChad, LazyVim, or stock Neovim
- **Shell completions** — Auto-installed for vncstart, tx11start, distro, etc.
- **Terminal utilities** — htop, neofetch, ranger, starship

### Termux Bridge
- **Battery monitoring** — Percentage, charging status, temperature
- **Wake lock** — Keep CPU alive during long sessions
- **Clipboard** — Read/write system clipboard from the app
- **Notifications** — Send Termux notifications from within the app
- **PulseAudio** — Start/stop/configure audio daemon
- **Volume control** — Adjust music, alarm, notification stream volumes

### TUI (Terminal User Interface)
- **100% keyboard-driven** — No mouse required
- **Color-coded status bar** — GPU model, battery, VNC/X11 status at a glance
- **Animated navigation** — j/kvim-style navigation
- **Scrollable lists** — Browse large app/driver lists
- **No root required** — Runs entirely within Termux sandbox

---

## 🖥 Screenshots

> TODO: Add screenshots of the TUI running on device

| Main Menu | GPU Monitor | App Store |
|:---:|:---:|:---:|
| Display, GPU, KDE, Apps, Distro, Shell | Mali-G72 stats, driver switching | Browse & install apps |

---

## 🏗 Architecture

```
tkde-app/
├── include/           # Header files (public API)
│   ├── logger.hpp     # Logging system with color output
│   ├── config.hpp     # Config file parser (reads termux-desktop config)
│   ├── display.hpp    # VNC + Termux:X11 control
│   ├── gpu.hpp        # GPU detection & driver management
│   ├── kde.hpp        # KDE Plasma / KWin / D-Bus integration
│   ├── termux.hpp     # Termux API bridge
│   ├── distro.hpp     # Proot-distro management
│   ├── shell.hpp      # Shell + font + neovim setup
│   ├── appstore.hpp   # App catalog & installer
│   └── ui.hpp         # NCurses TUI framework
├── src/
│   ├── core/          # Logger, Config, Distro, Shell managers
│   ├── display/       # Display server control
│   ├── gpu/           # GPU monitoring & driver switching
│   ├── kde/           # KDE D-Bus commands, theme install
│   ├── termux/        # Termux API (battery, clipboard, etc.)
│   ├── appstore/      # App catalog & package installer
│   └── ui/            # NCurses screen, widget renderers
├── config/
│   └── app.conf.in    # Config template
├── CMakeLists.txt     # Build system
└── README.md
```

### Module Overview

| Module | Language | Lines | Purpose |
|--------|----------|-------|---------|
| `tkde_core` | C++20 | ~800 | Logger, config, distro, shell |
| `tkde_kde` | C++20 | ~700 | KDE Plasma integration |
| `tkde_display` | C++20 | ~400 | VNC + Termux:X11 |
| `tkde_appstore` | C++20 | ~600 | App catalog |
| `tkde-app` | C++20 | ~450 | Main + TUI |

**Total: ~2,950 lines of C++20**

---

## 📋 Requirements

### Runtime
- **Termux** on Android (aarch64 / ARM64)
- **Bash** shell
- **TigerVNC** (`pkg install tigervnc`)
- **Termux:X11** (optional, for native X11 display)
- **PulseAudio** (`pkg install pulseaudio`)
- **proot-distro** (optional, for distro support)

### Build
- **cmake** (`pkg install cmake`)
- **clang++** (Termux's LLVM-based C++ compiler)
- **ncurses** (`pkg install ncurses`)
- **C++20** compatible standard library

---

## 📦 Installation

### Pre-built (Recommended)

```bash
# The binary is already compiled and installed at:
~/.local/bin/tkde-app

# Just make sure it's executable
chmod +x ~/.local/bin/tkde-app

# Run it!
tkde-app
```

### Manual Installation

```bash
# Clone the repo
git clone https://github.com/Unnho/tkde-app.git
cd tkde-app

# Build
cmake -B build -S . \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-std=c++20 -stdlib=libc++"

cmake --build build

# Install
cp build/tkde-app ~/.local/bin/
chmod +x ~/.local/bin/tkde-app
```

---

## 🚀 Usage

```bash
# TUI mode (default) — full keyboard-navigable interface
tkde-app

# Headless mode — daemon with periodic status logging
tkde-app -t

# Background daemon
tkde-app -d

# Show help
tkde-app -h
```

### First Run

1. Launch `tkde-app` — you'll see the main menu
2. Status bar shows: your battery %, GPU model, and VNC/X11 state
3. Navigate with `j`/`k` (vim-style), press `Enter` to select
4. Press `q` to go back, `q` again to exit

---

## ⚙ Configuration

### Automatic Config Reading

`tkde-app` automatically reads your existing **termux-desktop** configuration:

```bash
# Your existing config — tkde-app reads this
cat /data/data/com.termux/files/usr/etc/termux-desktop/configuration.conf
```

### App-specific Config

```bash
# tkde-app stores its own config at:
~/.config/tkde-app/config.conf

# Logs are at:
~/.cache/tkde-app/tkde-app.log
```

### Key Configuration Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `GPU_NAME` | Detected GPU | `mali` |
| `termux_hw_answer` | GPU acceleration method | `virgl_angle` |
| `gui_mode` | Display mode | `termux_x11` |
| `de_name` | Desktop environment | `plasma` |
| `de_startup` | DE startup command | `startplasma-x11` |
| `chosen_shell_name` | Default shell | `zsh` |

---

## 🎮 GPU Acceleration

### How it Works

Termux uses **VirGL** (virtual GPU) to provide OpenGL rendering:

```
Your App → virgl_test_server_android → Mali-G72 (host GPU)
            ↑ this runs as a subprocess
Xvnc/Termux:X11 ← Vulkan/Wrapper ICD ← virpipe translation layer
```

### Supported Drivers

| Driver | Best For | Requires |
|--------|----------|----------|
| **VirGL** | General use, Mali GPUs | `virgl_test_server_android` |
| **VirGL+ANGLE** | Better GLES support | `angle-android` |
| **VirGL+Vulkan** | Vulkan apps over GL | ANGLE + virgl |
| **Zink** | OpenGL over Vulkan | Vulkan ICD |
| **Zink+VirGL** | Combined approach | VirGL + Zink |
| **Turnip** | Adreno GPUs (Snapdragon) | Proot-distro |

### DRM / Native GPU

> ⚠️ **Hardware limitation**: This device (Exynos 9610 / Mali-G72 MP3) has **no DRM subsystem** (`CONFIG_DRM is not set` in the kernel). `/dev/dri/` does not exist. GPU acceleration works through **VirGL software translation**, not native DRM.

To switch drivers in the app:
1. Go to **GPU** view
2. Select a driver (e.g., "Switch to VirGL")
3. Press **Enter**
4. The environment variables are set instantly — no restart needed

---

## 🔨 Building from Source

### Prerequisites

```bash
pkg install cmake clang ncurses
```

### Full Build

```bash
git clone https://github.com/Unnho/tkde-app.git
cd tkde-app
mkdir build && cd build

cmake .. \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-std=c++20 -stdlib=libc++"

cmake --build . -- -j$(nproc)

# Result: build/tkde-app (ARM64 ELF binary)
strip -s build/tkde-app  # Reduce from ~3.2MB to ~205KB
```

### Build Options

```bash
# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Release build (optimized)
cmake .. -DCMAKE_BUILD_TYPE=Release
```

### Dependencies Tree

```
tkde-app
├── tkde_core (logger, config, distro, shell)
├── tkde_kde  (KDE D-Bus, themes, compositor)
├── tkde_display (VNC, Termux:X11)
├── tkde_appstore (app catalog)
└── tkde_ui   (ncurses TUI)
    └── ncurses (termux package)
```

---

## ⌨ Keyboard Controls

| Key | Action |
|-----|--------|
| `j` / `↓` | Move down in list |
| `k` / `↑` | Move up in list |
| `Enter` | Select / Execute |
| `g` | Go to first item |
| `G` | Go to last item |
| `r` | Refresh current view |
| `q` | Go back / Quit |

---

## 📁 Project Structure

```
tkde-app/
├── include/              # C++ headers
│   ├── logger.hpp        # Thread-safe logger with color
│   ├── config.hpp        # Config file parser + typed structs
│   ├── display.hpp       # VNC + X11 display management
│   ├── gpu.hpp           # GPU info + driver switching
│   ├── kde.hpp           # KDE Plasma integration
│   ├── termux.hpp        # Termux API bridge
│   ├── distro.hpp        # Proot-distro management
│   ├── shell.hpp         # Shell + font + neovim
│   ├── appstore.hpp      # App catalog
│   └── ui.hpp           # NCurses widget system
├── src/
│   ├── core/
│   │   ├── logger.cpp    # Logger implementation
│   │   ├── config.cpp     # Config file reader/writer
│   │   ├── distro.cpp    # Proot-distro management
│   │   └── shell.cpp      # Shell + font setup
│   ├── display/
│   │   └── display.cpp   # VNC + X11 control via script calls
│   ├── gpu/
│   │   └── gpu.cpp       # GPU probing + driver switching
│   ├── kde/
│   │   └── kde.cpp       # KDE D-Bus + theme management
│   ├── termux/
│   │   └── termux.cpp    # Termux API calls
│   ├── appstore/
│   │   └── appstore.cpp  # App catalog + install logic
│   └── ui/
│       └── screen.cpp    # NCurses screen + widget renderers
├── config/
│   └── app.conf.in       # CMake config template
├── CMakeLists.txt        # Build system
├── LICENSE               # GPL-3.0
└── README.md             # This file
```

---

## 🎨 Themes Supported

The KDE Manager supports installing these Plasma themes:

| Theme | Style | Source |
|-------|-------|--------|
| **Catppuccin** | Soft, pastel | github.com/catppuccin/kde |
| **Nord** | Arctic, blue-gray | github.com/Elena--/nord-kde |
| **Dracula** | Dark purple | github.com/dracula/kde |
| **Tokyo Night** | Dark blue | github.com/Enoch-Chang/TokyoNight-KDE |

---

## 🤝 Credits

- **[sabamdarif/termux-desktop](https://github.com/sabamdarif/termux-desktop)** — The incredible project this app integrates with. All display scripts, GPU configuration, distro setup, and styling come from sabamdarif's work.
- **Mali-G72** — The GPU in your Exynos 9610 device
- **VirGL** — Virtual GPU rendering for Android
- **NDK r29** — Used to compile the original `virgl_test_server_android`

---

## 📄 License

**GPL-3.0** — See [LICENSE](LICENSE)

---

<p align="center">
  Built with ❤️ on Termux (Android / ARM64)<br>
  Version 2.1.0 · May 2026
</p>