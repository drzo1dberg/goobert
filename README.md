# Goobert - Video Wall Application

High-performance video wall application built with Qt6 and libmpv.

## Features

### Video Wall
- Configurable NxM grid layouts (1x1 to 10x10)
- Hardware-accelerated OpenGL rendering
- Mixed media support (videos, images, GIFs)
- Auto-loop, shuffle, and watchdog auto-restart
- Filename filter with AND logic

### Playback Control
- Synchronized play/pause/next across all cells
- Frame-by-frame stepping with mouse wheel
- Seek navigation with horizontal scroll
- Per-cell and global volume control
- Loop toggle per cell

### Display Modes
- Grid view with status panel
- Global fullscreen (F11)
- Tile fullscreen with mpv OSC (double-click cell)

### UI Components
- **ToolBar** - Playback controls, volume slider
- **ConfigPanel** - Grid size, source path, filter
- **MonitorWidget** - Real-time cell status table
- **PlaylistWidget** - Drag & drop playlist management

### Professional Features
- Screenshot capture with clipboard copy
- Video rotation and zoom
- Skipper mode (auto-seek to percentage)
- INI-based configuration

## Requirements

- Linux (Debian, Ubuntu, Arch, Fedora)
- C++20 compiler (GCC 10+, Clang 11+)
- CMake 3.16+
- Qt6 (Widgets, OpenGLWidgets, Network)
- libmpv

## Installation

### Debian/Ubuntu

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-opengl-dev libmpv-dev pkg-config

git clone https://github.com/drzo1dberg/goobert.git
cd goobert
mkdir build && cd build
cmake ..
make -j$(nproc)
./goobert
```

### Arch Linux

```bash
sudo pacman -S cmake qt6-base mpv
```

### Fedora

```bash
sudo dnf install cmake qt6-qtbase-devel qt6-qtopengl-devel mpv-libs-devel
```

## Usage

```bash
./goobert                    # Use ~/Videos
./goobert /path/to/media     # Custom directory
```

1. Set grid size (rows x columns)
2. Select media source directory
3. Optional: Enter filter terms (space-separated, AND logic)
4. Click Start

## Keyboard Shortcuts

### Global
| Key | Action |
|-----|--------|
| Space / P | Play/Pause all |
| E | Next all |
| Q | Shuffle all |
| M | Mute all |
| U / I | Volume up/down |
| F / F11 | Toggle fullscreen |
| Escape | Exit fullscreen |

### Selected Cell
| Key | Action |
|-----|--------|
| W/A/S/D | Navigate selection |
| V / C | Seek forward/backward |
| N / B | Frame step forward/backward |
| L | Toggle loop |
| , / . | Previous/Next in playlist |
| T | Screenshot |
| R | Rotate 90° |
| +/- | Zoom in/out |

### Mouse
| Action | Effect |
|--------|--------|
| Left click | Select cell |
| Double-click | Tile fullscreen |
| Right click | Toggle pause |
| Middle click | Toggle loop |
| Horizontal scroll | Seek |
| Vertical scroll | Frame step |

## Configuration

Settings stored in `~/.config/goobert/goobert.ini`:

```ini
[playback]
loop_count=5
default_volume=30
image_display_duration=2.5

[grid]
default_rows=3
default_cols=3

[paths]
default_media_path=/mnt/media
screenshot_path=/mnt/screenshots

[skipper]
enabled=true
skip_percent=0.33

[seek]
amount_seconds=30
```

## Architecture

```
MainWindow
├── ToolBar (playback controls)
├── ConfigPanel (grid/source settings)
└── QSplitter
    ├── SidePanel
    │   ├── MonitorWidget (status table)
    │   └── PlaylistWidget (drag & drop)
    └── Video Wall
        └── GridCell[] → MpvWidget (libmpv)
```

## Project Structure

```
src/
├── main.cpp
├── mainwindow.cpp/h      # Grid management, input handling
├── mpvwidget.cpp/h       # libmpv OpenGL integration
├── gridcell.cpp/h        # Video cell container
├── toolbar.cpp/h         # Playback controls
├── sidepanel.cpp/h       # Tabbed side panel
├── monitorwidget.cpp/h   # Cell status monitor
├── playlistwidget.cpp/h  # Playlist drag & drop
├── configpanel.cpp/h     # Configuration UI
├── filescanner.cpp/h     # Media file scanner
├── config.cpp/h          # Settings manager
└── keymap.cpp/h          # Keyboard shortcuts
```

## License

**GPL-3.0** - Free for commercial and personal use.

This project uses [libmpv](https://mpv.io/) which is licensed under GPL. Therefore, Goobert is also distributed under the GNU General Public License v3.0 to comply with libmpv's licensing terms.

See [LICENSE](LICENSE) for full text.

## Author

**drzo1dberg** - [@drzo1dberg](https://github.com/drzo1dberg)
