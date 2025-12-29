# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## Build Commands

```bash
# Build with CMake
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run the application
./goobert [/path/to/media]
```

## Dependencies

- C++20 compiler (GCC 10+, Clang 11+)
- Qt6 (Widgets, OpenGLWidgets, Network)
- libmpv
- CMake 3.16+

## Architecture

Goobert is a video wall application that displays multiple videos in a configurable grid layout using Qt6 and libmpv.

**UI Layout:**
```
MainWindow (QMainWindow)
├── ToolBar (top)
│   └── Start/Stop, Fullscreen, Playback controls, Volume
├── ConfigPanel (collapsible)
│   └── Grid size, Source path, Filter input
└── QSplitter (horizontal)
    ├── SidePanel (left, 250-400px)
    │   └── QTabWidget
    │       ├── MonitorWidget (cell status table)
    │       └── PlaylistWidget (drag & drop playlist management)
    └── Video Wall (right, expandable)
        └── QGridLayout
            └── GridCell[] (QFrame containers)
                └── MpvWidget (QOpenGLWidget + libmpv)
```

**Key classes:**
- `MpvWidget` - OpenGL widget with direct libmpv integration, OSC support for fullscreen
- `GridCell` - Container for MpvWidget, handles mouse events, tracks position and state
- `MainWindow` - Grid lifecycle, fullscreen modes, synchronized playback, watchdog
- `ToolBar` - Playback controls, volume, start/stop buttons
- `SidePanel` - Tabbed container for Monitor and Playlist widgets
- `MonitorWidget` - Real-time cell status table with context menu
- `PlaylistWidget` - Drag & drop playlist management per cell
- `ConfigPanel` - Grid size, source directory, filter configuration
- `FileScanner` - Recursive media file discovery with AND filter support
- `Config` - Singleton settings manager (QSettings)
- `KeyMap` - Singleton keyboard shortcut manager

**Signal flow:**
```
UI Widget → MainWindow → GridCell → MpvWidget → libmpv
                ↓
         Status Updates ← MpvWidget
                ↓
         MonitorWidget / PlaylistWidget
```

**Video rendering:**
- Each MpvWidget creates its own `mpv_render_context`
- OSC (on-screen controller) enabled in tile fullscreen mode
- Mouse events forwarded to mpv for OSC interaction

## Commit Guidelines

- Do NOT add "Generated with Claude Code" footer
- Do NOT add "Co-Authored-By: Claude"
- Keep commit messages clean and concise
