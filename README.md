# Goobert

A video wall application for MPV. Watch multiple videos simultaneously in a synchronized grid layout.

![Python](https://img.shields.io/badge/python-3.10+-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

## Features

- **Grid Layout** - Configurable NxM grid (default 3x3)
- **Synchronized Controls** - Play/pause, next, previous, shuffle, volume for all tiles
- **Fullscreen Modes** - Global fullscreen or single-tile fullscreen
- **File Management** - Rename files directly, paths auto-update in all playlists
- **Pop-out Player** - Open any tile in a separate fullscreen mpv window
- **Clip Export** - Mark start/end points and export clips with ffmpeg
- **Loop Markers** - Toggle infinite loop on any tile
- **Dark UI** - Sleek dark theme, controls below the video wall

## Screenshot

<!-- TODO: Add screenshot -->
```
┌────────────────────────────────────────────┐
│  ┌──────┐ ┌──────┐ ┌──────┐               │
│  │ vid1 │ │ vid2 │ │ vid3 │               │
│  └──────┘ └──────┘ └──────┘               │
│  ┌──────┐ ┌──────┐ ┌──────┐               │
│  │ vid4 │ │ vid5 │ │ vid6 │               │
│  └──────┘ └──────┘ └──────┘               │
│  ┌──────┐ ┌──────┐ ┌──────┐               │
│  │ vid7 │ │ vid8 │ │ vid9 │               │
│  └──────┘ └──────┘ └──────┘               │
├────────────────────────────────────────────┤
│ Grid 3×3  Source [~/Videos]  ▶ Start  ■ ⛶ │
│ [Monitor: Cell | Status | Filename]        │
│ /path/to/current/file.mp4        [Ready]   │
└────────────────────────────────────────────┘
```

## Requirements

- Python 3.10+
- mpv (with IPC support)
- Tkinter (usually included with Python)
- ffmpeg (for clip export)
- X11 or XWayland

### Install dependencies

```bash
# Debian/Ubuntu
sudo apt install mpv python3-tk ffmpeg

# Arch
sudo pacman -S mpv tk ffmpeg

# Fedora
sudo dnf install mpv python3-tkinter ffmpeg
```

## Installation

```bash
# Clone the repository
git clone https://github.com/drzo1dberg/goobert.git
cd goobert

# Run directly
python3 goobert.py

# Or make it executable
chmod +x goobert.py
./goobert.py

# Optional: Create a symlink
sudo ln -s $(pwd)/goobert.py /usr/local/bin/goobert
```

## Usage

```bash
# Start with default directory (~/Videos)
goobert

# Start with a specific directory
goobert /path/to/media

# Show help
goobert --help

# Broadcast commands to all running instances
goobert --broadcast shuffle
goobert --broadcast next
```

## Keyboard Shortcuts

### In any tile (mpv window)

| Key | Action |
|-----|--------|
| `g` | Set clip start point |
| `G` | Set clip end point |
| `Ctrl+g` | Export clip to file |
| `Middle Mouse` | Toggle infinite loop |
| `N` | Next (all tiles) |
| `S` | Shuffle (all tiles) |
| `x` | Shuffle + Next (all tiles) |
| `z` / `Shift+z` | Zoom in/out |
| `Ctrl+r` | Rotate video |
| `Scroll` | Frame step |

### Global

| Key | Action |
|-----|--------|
| `F11` | Toggle fullscreen |
| `Escape` | Exit fullscreen |
| `Double-click` | Tile fullscreen |

## Output Directory

Clips and screenshots are saved to:
```
~/.local/share/goobert/output/
├── screenshots/
└── clips/
```

Override with environment variable:
```bash
export MPV_GRID_OUTPUT_DIR=/path/to/custom/dir
```

## Configuration

Currently configured via the GUI. Future versions may support a config file.

Default settings:
- Volume: 30%
- Grid: 3×3
- Image duration: 5 seconds

## How it works

Goobert embeds multiple mpv instances into a single Tkinter window using X11 window embedding (`--wid`). Each tile runs its own mpv process with IPC enabled, allowing synchronized control from the main application.

Embedded Lua scripts handle:
- Grid synchronization (playlist-next, shuffle across all tiles)
- Loop toggle with visual indicator
- Clip export via ffmpeg
- Random start position (33% into each video)

## License

MIT License - see [LICENSE](LICENSE)

## Contributing

Pull requests welcome! Please open an issue first to discuss major changes.

## Author

[drzo1dberg](https://github.com/drzo1dberg)
