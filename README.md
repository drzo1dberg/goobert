# Goobert (C++ Version)

A video wall application for MPV, built with Qt6 and libmpv.

![C++](https://img.shields.io/badge/C++-20-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6-green.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)

## Features

- **Grid Layout** - Configurable NxM grid (default 3x3)
- **Native Performance** - Direct libmpv integration via OpenGL
- **Synchronized Controls** - Play/pause, next, previous, shuffle, volume
- **Fullscreen Modes** - Global fullscreen or single-tile fullscreen
- **Dark UI** - Sleek dark theme with controls below the video wall

## Requirements

- C++20 compiler (GCC 10+, Clang 11+)
- CMake 3.16+
- Qt6 (Widgets, OpenGLWidgets, Network)
- libmpv
- OpenGL

### Install dependencies

```bash
# Debian/Ubuntu
sudo apt install build-essential cmake qt6-base-dev libmpv-dev

# Arch
sudo pacman -S cmake qt6-base mpv

# Fedora
sudo dnf install cmake qt6-qtbase-devel mpv-libs-devel
```

## Building

### With CMake (recommended)

```bash
cd goobert-cpp
mkdir build && cd build
cmake ..
make -j$(nproc)
./goobert
```

### With qmake

```bash
cd goobert-cpp
qmake6 goobert.pro  # or just 'qmake' on some systems
make -j$(nproc)
./goobert
```

### Install

```bash
sudo make install  # installs to /usr/local/bin
```

## Usage

```bash
# Start with default directory (~/Videos)
./goobert

# Start with a specific directory
./goobert /path/to/media

# Show help
./goobert --help

# Show version
./goobert --version
```

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `F11` | Toggle fullscreen |
| `Escape` | Exit fullscreen |
| `Double-click` | Tile fullscreen |

## Project Structure

```
goobert-cpp/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp           # Entry point
    ├── mainwindow.cpp/h   # Main application window
    ├── mpvwidget.cpp/h    # MPV OpenGL widget
    ├── mpvcontroller.cpp/h # MPV IPC controller
    ├── gridcell.cpp/h     # Grid cell container
    ├── controlpanel.cpp/h # Bottom control panel
    └── filescanner.cpp/h  # Media file scanner
```

## Architecture

The C++ version uses:
- **Qt6** for the GUI framework
- **libmpv** with OpenGL rendering for video playback
- Each grid cell contains an `MpvWidget` that renders video via `mpv_render_context`
- Control synchronization happens at the application level

## License

MIT License - see [LICENSE](../LICENSE)

## Author

[drzo1dberg](https://github.com/drzo1dberg)

## See Also

- [Python Version](../) - Single-file Python implementation
