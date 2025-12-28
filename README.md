# 🎬 Goobert - The Professional Video Wall

**High-performance, hardware-accelerated video wall application for Qt6 and libmpv**

[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Qt6](https://img.shields.io/badge/Qt-6-green.svg)](https://www.qt.io/)
[![MPV](https://img.shields.io/badge/libmpv-latest-orange.svg)](https://mpv.io/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## 🚀 Overview

**Goobert** is a modern C++ video wall application designed for professionals who need reliable, high-performance multi-display video playback. Perfect for digital signage, video installations, live events, streaming setups, and creative displays.

### Why Goobert?

- ⚡ **Native Performance** - Direct libmpv integration with hardware-accelerated OpenGL rendering
- 🎯 **Zero Latency** - Smooth playback across unlimited grid configurations
- 🎨 **Professional UI** - Clean, dark interface designed for production environments
- 🔧 **Fully Configurable** - Control every aspect through INI configuration files
- 🖱️ **Intuitive Controls** - Mouse and keyboard shortcuts for efficient operation
- 📦 **Cross-Platform** - Runs on Linux with minimal dependencies
- 🆓 **Open Source** - MIT licensed, free for commercial and personal use

---

## ✨ Features

### 🎬 Video Wall Engine
- **Configurable Grid Layouts** - Create any NxM grid configuration (1×1 to 10×10+)
- **Mixed Media Support** - Play videos, images, and animated GIFs simultaneously
- **Hardware Acceleration** - Leverages GPU for smooth 4K+ playback
- **Auto-Loop & Shuffle** - Configurable loop counts and intelligent playlist shuffling
- **Image Slideshow** - Automatic timing for static images in your wall

### 🎮 Advanced Playback Control
- **Synchronized Controls** - Play, pause, next, previous across all cells or individually
- **Frame-Stepping** - Precise frame-by-frame navigation with mouse wheel
- **Seek Navigation** - Side-scroll for quick 30-second jumps (perfect for MX Master mice!)
- **Volume Management** - Global and per-cell volume control
- **Mute Toggle** - Instant audio muting across the wall

### 🖥️ Display Modes
- **Grid View** - Standard multi-cell layout with controls
- **Global Fullscreen** - Maximize the entire wall for presentations
- **Tile Fullscreen** - Double-click any cell to focus it
- **Adaptive Layouts** - Responsive grid sizing with minimal borders

### 🎯 Professional Features
- **Cell Selection** - Click to select and control individual cells
- **Status Monitoring** - Real-time playback status for each cell
- **File Browser** - Quick media directory selection
- **Skipper Mode** - Auto-skip to configured percentage for video previews
- **Loop Control** - Per-file or per-cell loop customization
- **Video Transforms** - Rotation and zoom controls

### ⚙️ Configuration System
- **INI-Based Settings** - Simple, human-readable configuration files
- **Persistent Settings** - Remember your preferences across sessions
- **Customizable Defaults** - Set default grid size, volume, paths, and more
- **Hot-Reload** - Changes apply without restart

---

## 📋 System Requirements

### Minimum Requirements
- **OS**: Linux (Debian, Ubuntu, Arch, Fedora, etc.)
- **CPU**: Any modern x86_64 processor
- **GPU**: OpenGL 3.0+ support
- **RAM**: 2GB (4GB+ recommended for large grids)
- **Disk**: 100MB for application

### Software Dependencies
- **C++20 Compiler** - GCC 10+, Clang 11+, or equivalent
- **CMake** 3.16 or later
- **Qt6** - Widgets, OpenGLWidgets, Network modules
- **libmpv** - MPV media player library
- **OpenGL** - 3.0 or higher

---

## 🛠️ Installation

### Quick Install (Debian/Ubuntu)

```bash
# Install dependencies
sudo apt install build-essential cmake qt6-base-dev qt6-opengl-dev libmpv-dev pkg-config

# Clone and build
git clone https://github.com/yourusername/goobert-cpp.git
cd goobert-cpp
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./goobert
```

### Other Distributions

**Arch Linux:**
```bash
sudo pacman -S cmake qt6-base mpv
```

**Fedora:**
```bash
sudo dnf install cmake qt6-qtbase-devel qt6-qtopengl-devel mpv-libs-devel
```

**openSUSE:**
```bash
sudo zypper install cmake qt6-base-devel libmpv-devel
```

### System-Wide Installation

```bash
# From build directory
sudo make install

# Goobert is now available system-wide
goobert
```

---

## 🎯 Quick Start Guide

### 1. Launch Goobert

```bash
# Use default media directory (~/Videos)
./goobert

# Specify a custom media directory
./goobert /path/to/your/media

# Show help
./goobert --help
```

### 2. Configure Your Grid

1. **Set Grid Size** - Use the spinboxes to choose rows × columns (e.g., 3×3, 2×4)
2. **Select Media Source** - Browse to your media directory or enter path directly
3. **Click "Start"** - Grid initializes and begins playback automatically

### 3. Control Playback

- **⏯ Play/Pause** - Toggle playback across all cells
- **⏭ Next/Previous** - Navigate through playlists
- **🔀 Shuffle** - Randomize all playlists
- **🔇 Mute** - Silence all audio
- **Volume Slider** - Adjust global volume (0-100%)

### 4. Advanced Controls

- **Click a Cell** - Select for individual control
- **Double-Click** - Enter tile fullscreen mode
- **Horizontal Scroll** - Seek ±30 seconds (configurable)
- **Vertical Scroll** - Step frame-by-frame forward/backward
- **Middle Mouse** - Toggle loop on selected cell
- **Back Button** - Shuffle then skip to next

---

## ⚙️ Configuration

Goobert stores settings in `~/.config/goobert/goobert.ini`

### Configuration File Example

```ini
[playback]
loop_count=5                    # Loop each file 5 times
default_volume=30               # Start at 30% volume
image_display_duration=2.5      # Show images for 2.5 seconds

[grid]
default_rows=3                  # 3×3 grid by default
default_cols=3

[paths]
default_media_path=/mnt/media   # Your media directory

[skipper]
enabled=true                    # Auto-skip on file load
skip_percent=0.33               # Skip to 33% of duration

[seek]
amount_seconds=30               # Seek by 30 seconds with side-scroll
```

### Copy Example Config

```bash
cp goobert.ini.example ~/.config/goobert/goobert.ini
# Edit with your preferred settings
nano ~/.config/goobert/goobert.ini
```

---

## 🎮 Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `F11` | Toggle global fullscreen |
| `Escape` | Exit fullscreen / Deselect cell |
| `Double-Click` | Focus selected cell (tile fullscreen) |
| `Horizontal Scroll` | Seek ±30 seconds in selected cell |
| `Vertical Scroll` | Frame step forward/backward |
| `Middle Click` | Toggle loop-file on selected cell |
| `Mouse Back Button` | Shuffle all → Next all |

---

## 🏗️ Architecture

### Component Overview

```
┌─────────────────────────────────────────┐
│           MainWindow (QMainWindow)      │
│  ┌─────────────────────────────────┐   │
│  │   Video Wall (QGridLayout)      │   │
│  │  ┌────────┬────────┬────────┐   │   │
│  │  │ Cell   │ Cell   │ Cell   │   │   │
│  │  │ (MPV)  │ (MPV)  │ (MPV)  │   │   │
│  │  └────────┴────────┴────────┘   │   │
│  └─────────────────────────────────┘   │
│  ┌─────────────────────────────────┐   │
│  │    ControlPanel (QWidget)       │   │
│  │  [Grid] [Source] [▶] [Vol] ...  │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### Key Classes

- **`MpvWidget`** - OpenGL widget with libmpv render context (one per cell)
- **`GridCell`** - Container for MpvWidget, tracks position and state
- **`MainWindow`** - Grid orchestrator, handles input and fullscreen
- **`ControlPanel`** - UI controls and status monitoring
- **`FileScanner`** - Recursive media file discovery
- **`Config`** - Singleton configuration manager (Qt QSettings)

### Signal/Slot Flow

```
User Action → ControlPanel → MainWindow → GridCell → MpvWidget → libmpv
                                ↓
                          Status Updates ← MpvWidget
                                ↓
                          ControlPanel (Monitor Table)
```

---

## 🎨 Use Cases

### Digital Signage
- Retail displays with synchronized product videos
- Restaurant menus with rotating content
- Corporate lobbies with multi-screen presentations

### Live Events
- Concert visuals across multiple screens
- Conference backgrounds and sponsor displays
- Theater and stage production backdrops

### Creative Installations
- Art galleries with synchronized video art
- Museums with interactive multi-screen exhibits
- Public spaces with ambient video walls

### Streaming & Production
- OBS virtual camera multi-angle displays
- Live stream backgrounds with dynamic content
- Video production monitoring walls

### Personal Use
- Home media centers with multi-screen ambiance
- Security camera monitoring grids
- Photo slideshow walls at events

---

## 🔧 Development

### Build from Source

```bash
git clone https://github.com/yourusername/goobert-cpp.git
cd goobert-cpp

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build with all CPU cores
make -j$(nproc)

# Run tests (if available)
ctest

# Install
sudo make install
```

### Project Structure

```
goobert-cpp/
├── CMakeLists.txt           # Build configuration
├── goobert.ini.example      # Example config file
├── README.md                # This file
├── CLAUDE.md                # Developer instructions
└── src/
    ├── main.cpp             # Application entry point
    ├── mainwindow.{cpp,h}   # Main window & grid management
    ├── mpvwidget.{cpp,h}    # libmpv OpenGL integration
    ├── gridcell.{cpp,h}     # Video cell container
    ├── controlpanel.{cpp,h} # UI controls
    ├── filescanner.{cpp,h}  # Media file scanner
    ├── config.{cpp,h}       # Configuration manager
    └── mpvcontroller.{cpp,h}# MPV IPC (optional)
```

### Code Quality
- **Modern C++20** - Uses latest language features
- **Qt Best Practices** - Signal/slot architecture, RAII, parent-child cleanup
- **Thread-Safe** - Proper Qt event loop integration
- **Memory Safe** - No manual memory leaks, proper resource cleanup

---

## 🐛 Troubleshooting

### MPV Not Found
```bash
# Ensure libmpv is installed
pkg-config --modversion mpv

# If missing, install:
sudo apt install libmpv-dev  # Debian/Ubuntu
```

### Qt6 Not Found
```bash
# Verify Qt6 installation
qmake6 --version

# Install Qt6 base packages:
sudo apt install qt6-base-dev qt6-opengl-dev
```

### OpenGL Errors
```bash
# Check OpenGL support
glxinfo | grep "OpenGL version"

# Update graphics drivers if needed
```

### Black Screen / No Video
- Check that media files are in supported formats (mp4, mkv, avi, webm, etc.)
- Verify file permissions on media directory
- Check MPV log output for codec errors

---

## 📜 License

**MIT License** - Free for commercial and personal use.

See [LICENSE](LICENSE) for full text.

---

## 👤 Author

**drzo1dberg**
GitHub: [@drzo1dberg](https://github.com/drzo1dberg)

---

## 🙏 Acknowledgments

- **MPV Project** - For the excellent media player library
- **Qt Project** - For the powerful GUI framework
- **Open Source Community** - For continuous inspiration

---

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/yourusername/goobert-cpp/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/goobert-cpp/discussions)

---

## 🗺️ Roadmap

- [ ] Windows and macOS support
- [ ] Audio visualization modes
- [ ] Network stream support (RTSP, HTTP)
- [ ] Playlist management UI
- [ ] Video effects and filters
- [ ] Remote control API
- [ ] Web-based control panel

---

**Made with ❤️ and C++20**
