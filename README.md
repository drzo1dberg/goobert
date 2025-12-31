# Goobert - Video Wall Application

High-performance video wall application built with Qt6 and libmpv.

## Features

### Video Wall
- Configurable NxM grid layouts (1x1 to 10x10)
- Hardware-accelerated OpenGL rendering via libmpv
- Mixed media support (videos, images, GIFs)
- Auto-loop, shuffle, and watchdog auto-restart
- Filename filter with AND logic (space-separated terms)
- Zoom-to-cursor with mouse wheel

### Playback Control
- Synchronized play/pause/next across all cells
- Frame-by-frame stepping (B/Shift+B)
- Seek navigation (5s with C/V, 2min with Shift+C/V)
- Per-cell and global volume control
- Loop toggle per cell with visual indicator

### Display Modes
- Grid view with collapsible side panel
- Global fullscreen (Tab)
- Tile fullscreen with mpv OSC (double-click or F)

### UI Components
- **ToolBar** - Unified control bar with grid config, source, filter, playback controls, volume
- **SidePanel** - Collapsible panel with Monitor and Playlist tabs
- **MonitorWidget** - Real-time cell status table with context menu
- **PlaylistWidget** - Per-cell playlist management
- **PlaylistPicker** - Quick file search and selection (Y key)

### Professional Features
- Screenshot capture with clipboard copy
- Video rotation and zoom
- Skipper mode (auto-seek to percentage on new files)
- INI-based configuration
- One-handed keyboard layout (left hand on QWERTY)

### Statistics & Analytics
- SQLite-based watch time tracking per file
- Comprehensive web dashboard with modern visualizations
- Session tracking with hourly/daily/weekly/monthly trends
- Skip and loop event logging with heatmaps
- File type breakdown (video vs image)
- Completion rate tracking (full watch, partial, skipped)
- Concurrent cell usage analytics
- Favorites system for bookmarking files
- Directory-level statistics
- CSV export for external analysis

### Settings Dialog
- General: Grid size, paths, watchdog interval
- Playback: Seek steps, volume, loop count, image duration
- Keyboard: Full shortcut customization with key capture
- Statistics: Enable/disable tracking, view top files, export/clear data

## Requirements

- Linux (Debian, Ubuntu, Arch, Fedora) or macOS
- C++20 compiler (GCC 10+, Clang 11+)
- CMake 3.16+
- Qt6 (Widgets, OpenGLWidgets, Network)
- libmpv

## Installation

### Debian/Ubuntu

```bash
sudo apt install build-essential cmake qt6-base-dev libqt6opengl6-dev libmpv-dev pkg-config

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
sudo dnf install cmake qt6-qtbase-devel mpv-libs-devel
```

### macOS

```bash
brew install cmake qt@6 mpv
```

## Usage

```bash
./goobert                    # Use default from config
./goobert /path/to/media     # Custom directory
```

1. Set grid size (cols x rows) in toolbar
2. Enter or browse media source directory
3. Optional: Enter filter terms (space-separated, AND logic)
4. Click **Start**

## Keyboard Shortcuts (One-Handed Layout)

All shortcuts accessible with left hand on QWERTY keyboard.

### Global Controls
| Key | Action |
|-----|--------|
| Space | Play/Pause all |
| Tab | Toggle fullscreen |
| Escape | Exit fullscreen |
| ` (backtick) | Toggle mute |
| 1 / 2 | Volume down/up |
| 6 | Panic reset (stop all) |

### Navigation & Playback
| Key | Action |
|-----|--------|
| W/A/S/D | Navigate grid selection |
| E | Next all cells |
| Q | Shuffle all |
| X | Shuffle then next |

### Selected Cell
| Key | Action |
|-----|--------|
| F | Tile fullscreen selected |
| G | Toggle pause selected |
| R | Toggle loop |
| Shift+R | Rotate 90° |
| Y | Open playlist picker |
| 3 / 4 | Prev/Next in playlist |
| C / V | Seek -5s / +5s |
| Shift+C / Shift+V | Seek -2min / +2min |
| B | Frame step forward |
| Shift+B | Frame step backward |
| Z / Shift+Z | Zoom in/out |
| T | Screenshot |

### Mouse Controls
| Action | Effect |
|--------|--------|
| Left click | Select cell |
| Double-click | Tile fullscreen |
| Right click | Toggle pause (cell) |
| Middle click | Toggle loop (cell) |
| Shift+Middle | Reset zoom |
| Forward button | Next video |
| Shift+Forward | Previous video |
| Scroll wheel | Zoom (to cursor) |
| Horizontal scroll | Seek |

## Statistics Dashboard

Goobert includes a web-based statistics dashboard for viewing detailed analytics.

### Setup

```bash
# Install Python dependencies
cd dashboard
pip install flask flask-cors

# Run dashboard
python3 app.py
```

Open `http://localhost:5000` in your browser.

### Dashboard Features

- **Summary Stats**: Real elapsed time, accumulated watch time, parallelism factor
- **Time Distribution**: Hourly and daily watch patterns
- **Timeline**: 30-day activity history
- **Content Analysis**: Completion rates, file type breakdown
- **Skip Analysis**: Skip position heatmap, skip type distribution
- **Session Analysis**: Session length distribution, weekly/monthly trends
- **Top Files**: Most watched files with skip/loop counts
- **Favorites**: Bookmarked files
- **Directory Stats**: Watch time by folder

### Launcher Script

```bash
# Start both Goobert and the dashboard
./scripts/start-with-dashboard.sh
```

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
├── ToolBar (grid config, source, filter, playback, volume)
└── QSplitter
    ├── SidePanel (collapsible)
    │   ├── MonitorWidget (cell status table)
    │   └── PlaylistWidget (per-cell playlists)
    └── Video Wall (QGridLayout)
        └── GridCell[] (QFrame)
            ├── MpvWidget (QOpenGLWidget + libmpv)
            └── Loop Indicator (QLabel overlay)
```

## Project Structure

```
src/
├── main.cpp              # Entry point
├── mainwindow.cpp/h      # Grid lifecycle, input handling, fullscreen
├── mpvwidget.cpp/h       # libmpv OpenGL integration
├── gridcell.cpp/h        # Video cell container with overlays
├── toolbar.cpp/h         # Unified toolbar with all controls
├── sidepanel.cpp/h       # Tabbed container for Monitor/Playlist
├── monitorwidget.cpp/h   # Real-time cell status table
├── playlistwidget.cpp/h  # Per-cell playlist management
├── playlistpicker.cpp/h  # Quick file search dialog
├── filescanner.cpp/h     # Recursive media file scanner
├── config.cpp/h          # Singleton settings manager (QSettings)
├── keymap.cpp/h          # Centralized keyboard shortcut mapping
├── statsmanager.cpp/h    # SQLite statistics tracking singleton
├── settingsdialog.cpp/h  # Settings dialog with 4 tabs
└── theme.h               # Centralized UI theme constants

dashboard/
├── app.py                # Flask web server with REST API
├── templates/
│   └── index.html        # Dashboard UI with Chart.js visualizations
└── requirements.txt      # Python dependencies

scripts/
└── start-with-dashboard.sh  # Launch both app and dashboard
```

## License

**GPL-3.0** - Free for commercial and personal use.

This project uses [libmpv](https://mpv.io/) which is licensed under GPL. Therefore, Goobert is also distributed under the GNU General Public License v3.0.

See [LICENSE](LICENSE) for full text.

## Author

**drzo1dberg** - [@drzo1dberg](https://github.com/drzo1dberg)
