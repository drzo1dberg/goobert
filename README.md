# Goobert (Rust)

High-performance video wall application written in Rust.

## Features

- Configurable grid layouts (1×1 to 10×10)
- Hardware-accelerated video playback via libmpv
- Keyboard shortcuts for all operations
- Shuffle and loop functionality
- Frame stepping and seeking
- Screenshot capture
- Dark theme UI

## Dependencies

- Rust 1.70+
- libmpv
- OpenGL 3.0+

### Debian/Ubuntu

```bash
sudo apt install libmpv-dev pkg-config
```

### Arch Linux

```bash
sudo pacman -S mpv
```

## Build

```bash
cargo build --release
```

## Run

```bash
# Default media directory
./target/release/goobert

# Custom media directory
./target/release/goobert /path/to/media
```

## Keyboard Shortcuts

### Global
| Key | Action |
|-----|--------|
| Space / P | Play/Pause all |
| U / I | Volume up/down |
| M | Mute all |
| Shift+N | Next all |
| Q | Shuffle all |
| F11 | Fullscreen |
| Esc | Exit fullscreen |

### Navigation
| Key | Action |
|-----|--------|
| W/A/S/D | Navigate grid |

### Selected Cell
| Key | Action |
|-----|--------|
| F | Fullscreen cell |
| V / C | Seek forward/backward |
| N / B | Frame step forward/backward |
| L | Toggle loop |
| Z / Shift+Z | Zoom in/out |
| Ctrl+R | Rotate |
| T | Screenshot |

## Configuration

Config file: `~/.config/goobert/goobert.toml`

```toml
[playback]
loop_count = 5
default_volume = 30
image_display_duration = 2.5

[grid]
default_rows = 3
default_cols = 3

[paths]
default_media_path = "/home/user/Videos"
screenshot_path = "/home/user/Pictures"

[skipper]
enabled = false
skip_percent = 0.33

[seek]
amount_seconds = 30
```

## Project Structure

```
src/
├── main.rs           # Entry point
├── app.rs            # Main application state
├── config.rs         # Configuration management
├── keymap.rs         # Keyboard shortcuts
├── file_scanner.rs   # Media file discovery
├── mpv_player.rs     # MPV wrapper
├── grid_cell.rs      # Video cell management
└── ui/
    ├── mod.rs
    └── control_panel.rs
```

## License

MIT
