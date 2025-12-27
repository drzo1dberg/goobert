#!/usr/bin/env python3
"""
Goobert - Video Wall for MPV

Watch multiple videos at once in a synchronized grid layout.
Each cell runs an independent mpv instance embedded into a single window.

Features:
- Configurable NxM grid layout
- Synchronized playback controls (play/pause, next, shuffle, volume)
- Global and per-tile fullscreen modes
- File renaming with automatic playlist updates
- Pop-out fullscreen player for individual tiles
- Clip export with ffmpeg
- Keyboard shortcuts via embedded Lua scripts

Usage:
  goobert [SOURCE_DIR]
  goobert --broadcast next|shuffle

Requirements:
- Python 3.10+
- mpv with IPC support
- Tkinter
- X11 (Wayland works via XWayland)

Repository: https://github.com/drzo1dberg/goobert
License: MIT
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import random
import shlex
import socket as pysocket
import subprocess
import sys
import tempfile
import time
from contextlib import suppress
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from shutil import rmtree
from threading import Thread
from typing import TYPE_CHECKING, Any

if TYPE_CHECKING:
    import tkinter as tk

# ============================
# File types
# ============================

__version__ = "1.0.0"
__author__ = "drzo1dberg"
__repo__ = "https://github.com/drzo1dberg/goobert"

VIDEO_EXTS = frozenset({
    "mkv", "mp4", "avi", "mov", "m4v", "flv", "wmv", "mpg", "mpeg", "ts", "ogv", "webm"
})
IMAGE_EXTS = frozenset({
    "jpg", "jpeg", "png", "webp", "avif", "bmp", "tif", "tiff", "gif"
})
MEDIA_EXTS = VIDEO_EXTS | IMAGE_EXTS


def get_default_source() -> str:
    """Get default media source directory."""
    # Try common media locations
    for path in [
        Path.home() / "Videos",
        Path.home() / "Movies",
        Path.home() / "Media",
        Path("/media"),
        Path.home(),
    ]:
        if path.exists():
            return str(path)
    return str(Path.home())


def get_output_dir() -> Path:
    """Get output directory for exports (screenshots, clips)."""
    # Use XDG data dir or fallback
    xdg_data = os.environ.get("XDG_DATA_HOME", str(Path.home() / ".local" / "share"))
    output_dir = Path(xdg_data) / "goobert" / "output"
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


# ============================
# Enums
# ============================

class FullscreenMode(Enum):
    """Fullscreen state modes."""
    NORMAL = auto()
    GLOBAL = auto()
    TILE = auto()


# ============================
# Embedded MPV input bindings
# ============================

@dataclass(frozen=True, slots=True)
class InputBinding:
    """Represents an MPV key binding."""
    key: str
    command: str
    comment: str | None = None

    def render(self) -> str:
        line = f"{self.key} {self.command}".rstrip()
        if self.comment:
            line = f"{line}  # {self.comment}"
        return line


def default_input_bindings() -> list[InputBinding]:
    """Return default MPV input bindings.

    Important: override dangerous defaults (ESC/q) so embedded tiles
    are not accidentally killed.
    """
    return [
        InputBinding("ESC", "ignore", "prevent closing embedded tile"),
        InputBinding("q", "ignore", "prevent quitting embedded tile"),
        InputBinding("Q", "ignore", "prevent quitting embedded tile"),
        InputBinding("ctrl+r", 'cycle_values video-rotate "90" "180" "270" "0"'),
        InputBinding("WHEEL_DOWN", "osd-msg frame-step"),
        InputBinding("WHEEL_UP", "osd-msg frame-back-step"),
        InputBinding("MBTN_MID", "script-binding toggle-custom-loop"),
        InputBinding("MBTN_BACK", "script-binding chained-next-then-shuffle"),
        # Broadcast helpers (replaces /usr/local/bin/*)
        InputBinding("N", 'run "mpv-grid-broadcast" "next"'),
        InputBinding("S", 'run "mpv-grid-broadcast" "shuffle"'),
        InputBinding("z", "add video-zoom 0.1"),
        InputBinding("Shift+z", "add video-zoom -0.1"),
        InputBinding("WHEEL_LEFT", "seek -30"),
        InputBinding("WHEEL_RIGHT", "seek 30"),
        # built-in select.lua binding (depends on build/options)
        InputBinding("y", "script-binding select/select-playlist"),
        InputBinding("x", "script-binding chained-next-then-shuffle"),
    ]


# ============================
# Embedded Lua scripts
# ============================

LUA_CHAINED_NEXT_SHUFFLE = r'''-- chained-next-shuffle.lua
-- Key: trigger "shuffle" broadcast, then after a delay "next" broadcast

local mp = require("mp")

local DELAY = 0.2 -- seconds between shuffle and next

local function chained_next_then_shuffle()
    mp.commandv("run", "mpv-grid-broadcast", "shuffle")
    mp.add_timeout(DELAY, function()
        mp.commandv("run", "mpv-grid-broadcast", "next")
    end)
end

mp.add_key_binding(nil, "chained-next-then-shuffle", chained_next_then_shuffle)
'''

LUA_GRID_SYNC = r'''-- grid-sync.lua
-- Handle broadcast messages inside each mpv instance

local mp = require("mp")

local function handle_grid_sync_next()
    local loop_val = mp.get_property("loop-file", "no")
    if loop_val == "inf" then
        mp.osd_message("grid-sync-next: skipped (loop-file=inf)")
        return
    end
    mp.commandv("playlist-next", "force")
    mp.osd_message("grid-sync-next: playlist-next")
end

local function handle_grid_sync_shuffle()
    mp.commandv("playlist-shuffle")
    mp.osd_message("grid-sync-shuffle: playlist-shuffle")
end

mp.register_script_message("grid-sync-next", handle_grid_sync_next)
mp.register_script_message("grid-sync-shuffle", handle_grid_sync_shuffle)
'''

LUA_SKIPPER = r'''-- skipper.lua
-- skips once to a percentage on file load

local mp = require("mp")

local PERCENT = 0.33
local seen = {}

local function skip_once()
    local path = mp.get_property("path", "")
    if path == "" then return end
    if seen[path] then return end

    local dur = mp.get_property_number("duration", 0)
    if not dur or dur <= 0 then return end

    seen[path] = true
    local target = dur * PERCENT
    mp.commandv("seek", tostring(target), "absolute", "exact")
    mp.osd_message(string.format("start@%d%%", math.floor(PERCENT * 100)))
end

mp.register_event("file-loaded", function()
    mp.add_timeout(0.15, skip_once)
end)
'''

LUA_TOGGLE_LOOP = r'''-- toggle-loop.lua
-- Middle mouse button: toggle loop-file + visual marking (title + persistent OSD label)

local mp = require("mp")

local default_loop = "no"
local state = "default"
local base_title = nil

local overlay = mp.create_osd_overlay("ass-events")

local function update_title()
    if not base_title then
        base_title = mp.get_property("title", mp.get_property("media-title", "mpv"))
    end
    if state == "inf" then
        mp.set_property("title", base_title .. " [∞]")
    else
        mp.set_property("title", base_title)
    end
end

local function update_overlay()
    if state == "inf" then
        overlay.data = "{\\an2\\fs60\\bord0\\shad0\\1c&HFFFFFF&}**"
        overlay:update()
    else
        overlay:remove()
    end
end

mp.register_event("file-loaded", function()
    default_loop = mp.get_property("loop-file", "no")
    state = (default_loop == "inf") and "inf" or "default"
    base_title = mp.get_property("title", mp.get_property("media-title", "mpv"))
    update_title()
    update_overlay()
end)

local function toggle_loop_file()
    if state == "default" then
        mp.set_property("loop-file", "inf")
        state = "inf"
        mp.osd_message("loop-file=inf")
    else
        local val = default_loop or "no"
        mp.set_property("loop-file", val)
        state = "default"
        mp.osd_message("loop-file=" .. val)
    end
    update_title()
    update_overlay()
end

mp.add_key_binding(nil, "toggle-custom-loop", toggle_loop_file)
'''

LUA_CLIP_EXPORT = r'''-- clip-export.lua
-- Export video clips using ffmpeg
-- Usage: "g" to set start, "G" to set end, "Ctrl+g" to export

local mp = require("mp")
local msg = require("mp.msg")
local utils = require("mp.utils")

-- Configuration (can be overridden via script-opts)
local options = {
    fps = -1,           -- -1 = original fps
    width = -1,         -- -1 = original width
    height = -1,        -- -1 = original height
    format = "mp4",     -- output format
    output_dir = "",    -- empty = use MPV_GRID_OUTPUT_DIR env or ~/Videos
}

-- Try to read options
pcall(function() require("mp.options").read_options(options, "clip-export") end)

-- Get output directory
local function get_output_dir()
    if options.output_dir ~= "" then
        return options.output_dir
    end
    local env_dir = os.getenv("MPV_GRID_OUTPUT_DIR")
    if env_dir and env_dir ~= "" then
        return env_dir
    end
    local home = os.getenv("HOME") or os.getenv("USERPROFILE") or "/tmp"
    return home .. "/Videos/mpv-clips"
end

local start_time = -1
local end_time = -1

local function get_source_path()
    local path = mp.get_property("path", "")
    if path == "" then return nil end
    local wd = mp.get_property("working-directory", "")
    if wd ~= "" and not path:match("^/") then
        path = wd .. "/" .. path
    end
    return path
end

local function file_exists(name)
    local f = io.open(name, "r")
    if f then f:close() return true end
    return false
end

local function get_output_filename()
    local out_dir = get_output_dir()
    os.execute('mkdir -p "' .. out_dir .. '"')

    local filename = mp.get_property("filename/no-ext") or "clip"
    for i = 0, 999 do
        local fn = string.format("%s/%s_%03d.%s", out_dir, filename, i, options.format)
        if not file_exists(fn) then
            return fn
        end
    end
    return nil
end

local function export_clip(with_subs)
    if start_time < 0 or end_time < 0 or start_time >= end_time then
        mp.osd_message("Set start (g) and end (G) first")
        return
    end

    local source = get_source_path()
    if not source then
        mp.osd_message("No file loaded")
        return
    end

    local output = get_output_filename()
    if not output then
        mp.osd_message("Could not create output file")
        return
    end

    local duration = end_time - start_time
    mp.osd_message(string.format("Exporting %.1fs clip...", duration))

    local args = {
        "ffmpeg", "-y",
        "-ss", tostring(start_time),
        "-i", source,
        "-t", tostring(duration),
        "-c:v", "libx264",
        "-c:a", "aac",
        "-preset", "fast",
        output
    }

    mp.command_native_async({
        name = "subprocess",
        args = args,
        capture_stderr = true,
    }, function(success, result)
        if success and result.status == 0 then
            mp.osd_message("Saved: " .. output, 3)
            msg.info("Clip saved: " .. output)
        else
            mp.osd_message("Export failed", 2)
            msg.error("Export failed: " .. (result.stderr or "unknown error"))
        end
    end)
end

local function set_start()
    start_time = mp.get_property_number("time-pos", -1)
    if start_time >= 0 then
        mp.osd_message(string.format("Start: %.2fs", start_time))
    end
end

local function set_end()
    end_time = mp.get_property_number("time-pos", -1)
    if end_time >= 0 then
        mp.osd_message(string.format("End: %.2fs (duration: %.2fs)",
            end_time, math.max(0, end_time - start_time)))
    end
end

mp.add_key_binding("g", "clip-set-start", set_start)
mp.add_key_binding("G", "clip-set-end", set_end)
mp.add_key_binding("Ctrl+g", "clip-export", function() export_clip(false) end)
'''

EMBEDDED_LUA_SCRIPTS: dict[str, str] = {
    "grid-sync.lua": LUA_GRID_SYNC,
    "toggle-loop.lua": LUA_TOGGLE_LOOP,
    "chained-next-shuffle.lua": LUA_CHAINED_NEXT_SHUFFLE,
    "skipper.lua": LUA_SKIPPER,
    "clip-export.lua": LUA_CLIP_EXPORT,
}


# ============================
# Runtime assets (written at startup)
# ============================

@dataclass(slots=True)
class RuntimeAssets:
    """Manages runtime assets written to temp directory."""
    root: Path
    scripts_dir: Path
    bin_dir: Path
    input_conf: Path
    lua_paths: dict[str, Path]
    broadcast_helper: Path

    @classmethod
    def create(
        cls,
        base_tmpdir: Path,
        bindings: list[InputBinding],
        lua_sources: dict[str, str],
        main_script_path: Path,
        python_exe: Path,
    ) -> RuntimeAssets:
        assets_root = base_tmpdir / "mpv-assets"
        scripts_dir = assets_root / "scripts"
        bin_dir = assets_root / "bin"
        scripts_dir.mkdir(parents=True, exist_ok=True)
        bin_dir.mkdir(parents=True, exist_ok=True)

        input_conf = assets_root / "input.conf"
        input_text = "\n".join(b.render() for b in bindings) + "\n"
        input_conf.write_text(input_text, encoding="utf-8")

        lua_paths = {
            filename: scripts_dir / filename
            for filename in lua_sources
        }
        for filename, content in lua_sources.items():
            lua_paths[filename].write_text(f"{content.rstrip()}\n", encoding="utf-8")

        broadcast_helper = bin_dir / "mpv-grid-broadcast"
        helper_script = (
            "#!/bin/sh\n"
            f"exec {shlex.quote(str(python_exe))} {shlex.quote(str(main_script_path))} --broadcast \"$@\"\n"
        )
        broadcast_helper.write_text(helper_script, encoding="utf-8")
        broadcast_helper.chmod(0o755)

        return cls(
            root=assets_root,
            scripts_dir=scripts_dir,
            bin_dir=bin_dir,
            input_conf=input_conf,
            lua_paths=lua_paths,
            broadcast_helper=broadcast_helper,
        )


# ============================
# Fullscreen State Management
# ============================

CellKey = tuple[int, int]
GridInfo = dict[str, Any]


@dataclass(slots=True)
class FullscreenState:
    """Centralized state for fullscreen management."""
    mode: FullscreenMode = FullscreenMode.NORMAL
    active_tile: CellKey | None = None
    saved_sash_pos: int | None = None
    side_visible: bool = True
    saved_grid_info: dict[CellKey, GridInfo | None] = field(default_factory=dict)
    saved_pause_states: dict[CellKey, bool] = field(default_factory=dict)
    saved_mute_states: dict[CellKey, bool] = field(default_factory=dict)
    tile_forced_global: bool = False

    def reset(self) -> None:
        """Reset to normal state."""
        self.mode = FullscreenMode.NORMAL
        self.active_tile = None
        self.side_visible = True
        self.saved_grid_info.clear()
        self.saved_pause_states.clear()
        self.saved_mute_states.clear()
        self.tile_forced_global = False

    @property
    def is_fullscreen(self) -> bool:
        return self.mode in (FullscreenMode.GLOBAL, FullscreenMode.TILE)


# ============================
# MPV IPC communication
# ============================

class MPVController:
    """Controller for MPV IPC communication via Unix socket."""

    __slots__ = ("socket_path", "_last_error")

    def __init__(self, socket_path: str) -> None:
        self.socket_path = socket_path
        self._last_error: str | None = None

    def send_command(self, *args: Any, timeout: float = 1.0) -> dict[str, Any]:
        try:
            with pysocket.socket(pysocket.AF_UNIX, pysocket.SOCK_STREAM) as sock:
                sock.settimeout(timeout)
                sock.connect(self.socket_path)
                cmd = json.dumps({"command": list(args)}) + "\n"
                sock.send(cmd.encode("utf-8"))
                response = sock.recv(4096).decode("utf-8")
                return json.loads(response)
        except Exception as e:
            self._last_error = str(e)
            return {"error": str(e)}

    def get_property(self, prop: str) -> Any:
        return self.send_command("get_property", prop).get("data")

    def set_property(self, prop: str, value: Any) -> dict[str, Any]:
        return self.send_command("set_property", prop, value)

    def cycle(self, prop: str) -> dict[str, Any]:
        return self.send_command("cycle", prop)

    def playlist_next(self, *, force: bool = False) -> dict[str, Any]:
        if force:
            return self.send_command("playlist-next", "force")
        return self.send_command("playlist-next")

    def playlist_prev(self) -> dict[str, Any]:
        return self.send_command("playlist-prev")

    def playlist_shuffle(self) -> dict[str, Any]:
        return self.send_command("playlist-shuffle")

    def script_message(self, name: str, *args: str) -> dict[str, Any]:
        return self.send_command("script-message", name, *args)

    def grid_sync_next(self) -> dict[str, Any]:
        return self.script_message("grid-sync-next")

    def grid_sync_shuffle(self) -> dict[str, Any]:
        return self.script_message("grid-sync-shuffle")

    def script_binding(self, name: str) -> dict[str, Any]:
        return self.send_command("script-binding", name)

    def toggle_custom_loop(self) -> dict[str, Any]:
        return self.script_binding("toggle-custom-loop")

    def chained_next_shuffle(self) -> dict[str, Any]:
        return self.script_binding("chained-next-then-shuffle")

    @property
    def last_error(self) -> str | None:
        return self._last_error


# ============================
# Broadcast mode (CLI)
# ============================

def send_ipc_command(socket_path: str, command: list[str], timeout: float = 0.5) -> bool:
    """Send IPC command to an MPV socket."""
    try:
        with pysocket.socket(pysocket.AF_UNIX, pysocket.SOCK_STREAM) as sock:
            sock.settimeout(timeout)
            sock.connect(socket_path)
            payload = json.dumps({"command": command}) + "\n"
            sock.send(payload.encode("utf-8"))
        return True
    except Exception:
        return False


def broadcast(action: str, sock_glob: str | None = None) -> int:
    """Broadcast action to all MPV grid sockets."""
    action = action.strip().lower()
    sock_glob = sock_glob or os.environ.get("MPV_GRID_SOCKET_GLOB", "/tmp/mpv-grid-*.sock")

    match action:
        case "next":
            cmd = ["script-message", "grid-sync-next"]
        case "shuffle":
            cmd = ["script-message", "grid-sync-shuffle"]
        case _:
            raise SystemExit(f"Unknown broadcast action: {action!r} (expected: next|shuffle)")

    return sum(
        send_ipc_command(p, cmd)
        for p in sorted(glob.glob(sock_glob))
    )


# ============================
# File scanner
# ============================

def scan_files(source_path: str) -> tuple[list[Path], list[Path], list[Path]]:
    """Scan directory for media files, returning (all, videos, images)."""
    source = Path(source_path)
    if not source.exists():
        return [], [], []

    all_files: list[Path] = []
    videos: list[Path] = []
    images: list[Path] = []

    files = source.rglob("*") if source.is_dir() else [source]
    for f in sorted(files):
        if not f.is_file():
            continue
        ext = f.suffix.lstrip(".").lower()
        all_files.append(f)
        if ext in VIDEO_EXTS:
            videos.append(f)
        if ext in IMAGE_EXTS:
            images.append(f)

    return all_files, videos, images


# ============================
# MPV config (pure options)
# ============================

@dataclass(slots=True)
class MPVConfig:
    """MPV configuration options."""
    volume: int = 30
    loop_file: str = "no"
    image_duration: int = 5
    screenshot_dir: str = ""

    def __post_init__(self) -> None:
        if not self.screenshot_dir:
            self.screenshot_dir = str(get_output_dir() / "screenshots")
            Path(self.screenshot_dir).mkdir(parents=True, exist_ok=True)

    def base_mpv_opts(self) -> list[str]:
        return [
            "--no-config",
            "--no-shuffle",
            f"--volume={self.volume}",
            "--keep-open=yes",
            f"--loop-file={self.loop_file}",
            f"--screenshot-directory={self.screenshot_dir}",
            "--idle=yes",
            "--force-window=yes",
            "--focus-on=never",
            f"--image-display-duration={self.image_duration}",
            "--no-border",
            "--no-window-dragging",
        ]


# ============================
# GUI
# ============================

def run_gui(initial_source: str | None = None) -> None:
    import tkinter as tk
    from tkinter import filedialog, messagebox, scrolledtext, simpledialog, ttk

    class MPVGridApp:
        def __init__(self, root: tk.Tk, initial_source: str | None = None) -> None:
            self.root = root
            self.root.title(f"Goobert {__version__}")
            self.root.geometry("1500x900")
            self.initial_source = initial_source or get_default_source()

            self.config = MPVConfig()
            self.processes: list[subprocess.Popen[bytes]] = []
            self.controllers: dict[CellKey, MPVController] = {}
            self.playlists: dict[CellKey, list[Path]] = {}

            self.tmpdir = Path(tempfile.mkdtemp(prefix="mpv-grid-"))
            self.grid_id = f"{os.getpid()}-{int(time.time())}"

            main_script = Path(__file__).resolve()
            python_exe = Path(sys.executable).resolve()

            self.assets = RuntimeAssets.create(
                base_tmpdir=self.tmpdir,
                bindings=default_input_bindings(),
                lua_sources=EMBEDDED_LUA_SCRIPTS,
                main_script_path=main_script,
                python_exe=python_exe,
            )

            # Wall embedding
            self.wall_frame: tk.Frame | None = None
            self.cell_frames: dict[CellKey, tk.Frame] = {}
            self.current_rows = 0
            self.current_cols = 0

            # Monitor state
            self.monitor_items: dict[CellKey, str] = {}
            self.last_paths: dict[CellKey, str] = {}
            self._monitor_job: str | None = None

            # Fullscreen state (centralized)
            self.fs_state = FullscreenState()

            self._setup_ui()
            self.root.protocol("WM_DELETE_WINDOW", self._on_close)

            # Keybindings with safe wrappers
            self.root.bind("<F11>", self._safe_toggle_fullscreen)
            self.root.bind("<Escape>", self._safe_exit_fullscreen)

        # ========== Enhanced Logging ==========

        def _log(self, message: str, level: str = "INFO") -> None:
            """Update status bar with latest message."""
            if level == "DEBUG":
                return  # Skip debug messages in sleek UI
            timestamp = time.strftime("%H:%M:%S")
            display = f"[{timestamp}] {message}"
            if hasattr(self, "status_var"):
                self.status_var.set(display)

        # ========== Safe Event Handlers ==========

        def _safe_toggle_fullscreen(self, event: tk.Event | None = None) -> None:
            """Safe wrapper for fullscreen toggle."""
            try:
                self._toggle_fullscreen()
            except Exception as e:
                self._log(f"Fullscreen toggle error: {e}", "ERROR")
                self._emergency_reset()

        def _safe_exit_fullscreen(self, event: tk.Event | None = None) -> None:
            """Safe wrapper for exit fullscreen."""
            try:
                if self.fs_state.is_fullscreen:
                    self._exit_fullscreen()
            except Exception as e:
                self._log(f"Exit fullscreen error: {e}", "ERROR")
                self._emergency_reset()

        # ========== Fullscreen Management (Improved) ==========

        def _toggle_fullscreen(self) -> None:
            """Toggle between normal and global fullscreen."""
            if self.fs_state.mode == FullscreenMode.NORMAL:
                self._enter_fullscreen()
            else:
                self._exit_fullscreen()

        def _enter_fullscreen(self) -> None:
            """Enter global fullscreen mode with robust state management."""
            if self.fs_state.mode == FullscreenMode.GLOBAL:
                return  # Already in global fullscreen

            self._log("Entering global fullscreen...", "DEBUG")

            # Save current state
            if self.fs_state.mode == FullscreenMode.NORMAL:
                with suppress(Exception):
                    self.fs_state.saved_sash_pos = self.pw.sashpos(0)

            # Hide side panel
            if self.fs_state.side_visible:
                with suppress(Exception):
                    self.pw.forget(self.side)
                    self.fs_state.side_visible = False

            # Enter fullscreen
            try:
                self.root.attributes("-fullscreen", True)
            except Exception:
                with suppress(Exception):
                    self.root.state("zoomed")

            # Update state
            self.fs_state.mode = FullscreenMode.GLOBAL

            # Update UI
            if hasattr(self, "fullscreen_btn"):
                self.fullscreen_btn.config(text="⛶ Exit FS")

            self._show_overlay(True)
            self._log("Fullscreen ON")

        def _exit_fullscreen(self) -> None:
            """Exit fullscreen mode (both global and tile) with atomic cleanup."""
            if self.fs_state.mode == FullscreenMode.NORMAL:
                return  # Already in normal mode

            self._log(f"Exiting fullscreen (mode: {self.fs_state.mode.name})...", "DEBUG")

            # Exit tile fullscreen first if active
            if self.fs_state.mode == FullscreenMode.TILE:
                self._exit_tile_fullscreen_internal()

            # Exit window fullscreen
            with suppress(Exception):
                self.root.attributes("-fullscreen", False)

            # Restore side panel
            if not self.fs_state.side_visible:
                with suppress(Exception):
                    self.pw.add(self.side, weight=2)
                    self.fs_state.side_visible = True

                    # Restore sash position
                    if self.fs_state.saved_sash_pos is not None:
                        self.root.update_idletasks()
                        with suppress(Exception):
                            self.pw.sashpos(0, self.fs_state.saved_sash_pos)

            # Update state
            old_mode = self.fs_state.mode
            self.fs_state.mode = FullscreenMode.NORMAL
            self.fs_state.tile_forced_global = False

            # Update UI
            if hasattr(self, "fullscreen_btn"):
                self.fullscreen_btn.config(text="⛶ Fullscreen")

            self._show_overlay(False)
            self._log("Fullscreen OFF")

        def _toggle_tile_fullscreen(self) -> None:
            """Toggle tile fullscreen for selected cell."""
            if self.fs_state.mode == FullscreenMode.TILE:
                self._exit_tile_fullscreen()
            elif cell := self._selected_cell():
                self._enter_tile_fullscreen(cell)
            else:
                self._log("No cell selected for tile fullscreen", "WARN")

        def _enter_tile_fullscreen(self, cell: CellKey) -> None:
            """Enter tile fullscreen mode with atomic state changes."""
            if cell not in self.cell_frames:
                self._log(f"Cell {cell} not found", "ERROR")
                return

            self._log(f"Entering tile fullscreen for {cell}...", "DEBUG")

            try:
                # Enter global fullscreen if not already
                if self.fs_state.mode == FullscreenMode.NORMAL:
                    self._enter_fullscreen()
                    self.fs_state.tile_forced_global = True

                # Save grid state for ALL cells
                self.fs_state.saved_grid_info = {
                    rc: self._save_frame_grid_info(frame)
                    for rc, frame in self.cell_frames.items()
                }

                # Save and modify MPV states (pause/mute non-selected)
                self.fs_state.saved_pause_states.clear()
                self.fs_state.saved_mute_states.clear()

                for rc, ctrl in self.controllers.items():
                    if rc == cell:
                        continue

                    # Save states
                    p = ctrl.get_property("pause")
                    m = ctrl.get_property("mute")
                    self.fs_state.saved_pause_states[rc] = bool(p) if p is not None else False
                    self.fs_state.saved_mute_states[rc] = bool(m) if m is not None else False

                    # Pause and mute
                    ctrl.set_property("pause", True)
                    ctrl.set_property("mute", True)

                # Unmute selected tile
                if sel_ctrl := self.controllers.get(cell):
                    sel_ctrl.set_property("mute", False)

                # Hide all frames except selected
                for rc, frame in self.cell_frames.items():
                    if rc != cell:
                        frame.grid_remove()

                # Expand selected frame to fill entire grid
                sel_frame = self.cell_frames[cell]
                sel_frame.grid(
                    row=0, column=0,
                    rowspan=max(1, self.current_rows),
                    columnspan=max(1, self.current_cols),
                    sticky="nsew", padx=0, pady=0
                )

                # Update state
                self.fs_state.mode = FullscreenMode.TILE
                self.fs_state.active_tile = cell

                # Update UI
                if hasattr(self, "tile_fs_btn"):
                    self.tile_fs_btn.config(text="Exit Tile")

                self._show_overlay(True)
                self._log(f"Tile FS: {cell}")

            except Exception as e:
                self._log(f"Error entering tile fullscreen: {e}", "ERROR")
                self._emergency_reset()
                raise

        def _exit_tile_fullscreen(self) -> None:
            """Exit tile fullscreen mode (public interface)."""
            if self.fs_state.mode != FullscreenMode.TILE:
                return

            try:
                self._exit_tile_fullscreen_internal()

                # Exit global fullscreen if it was forced by tile mode
                if self.fs_state.tile_forced_global:
                    self._exit_fullscreen()

            except Exception as e:
                self._log(f"Error exiting tile fullscreen: {e}", "ERROR")
                self._emergency_reset()

        def _exit_tile_fullscreen_internal(self) -> None:
            """Internal tile fullscreen exit (called by exit_fullscreen)."""
            if self.fs_state.mode != FullscreenMode.TILE:
                return

            self._log("Exiting tile fullscreen...", "DEBUG")

            # Restore grid layout for all cells
            for rc, frame in self.cell_frames.items():
                if saved_info := self.fs_state.saved_grid_info.get(rc):
                    self._restore_frame_grid_info(frame, saved_info)
                else:
                    frame.grid()

            # Restore MPV states (pause/mute)
            for rc, ctrl in self.controllers.items():
                if rc == self.fs_state.active_tile:
                    continue

                if rc in self.fs_state.saved_pause_states:
                    ctrl.set_property("pause", self.fs_state.saved_pause_states[rc])
                if rc in self.fs_state.saved_mute_states:
                    ctrl.set_property("mute", self.fs_state.saved_mute_states[rc])

            # Clear tile state
            self.fs_state.active_tile = None
            self.fs_state.saved_grid_info.clear()
            self.fs_state.saved_pause_states.clear()
            self.fs_state.saved_mute_states.clear()

            # Update UI
            if hasattr(self, "tile_fs_btn"):
                self.tile_fs_btn.config(text="Tile FS")

            self._log("Tile FS OFF")

        def _save_frame_grid_info(self, frame: tk.Frame) -> GridInfo | None:
            """Save grid info for a frame with robust error handling."""
            try:
                info = frame.grid_info()
                return {
                    "row": int(info.get("row", 0)),
                    "column": int(info.get("column", 0)),
                    "rowspan": int(info.get("rowspan", 1)),
                    "columnspan": int(info.get("columnspan", 1)),
                    "sticky": str(info.get("sticky", "nsew")),
                    "padx": self._parse_padding(info.get("padx", 1)),
                    "pady": self._parse_padding(info.get("pady", 1)),
                }
            except Exception as e:
                self._log(f"Could not save frame grid info: {e}", "WARN")
                return None

        def _restore_frame_grid_info(self, frame: tk.Frame, info: GridInfo) -> None:
            """Restore grid info for a frame."""
            with suppress(Exception):
                frame.grid(**info)

        @staticmethod
        def _parse_padding(pad_value: Any) -> int:
            """Parse padding value safely."""
            if isinstance(pad_value, int):
                return pad_value
            with suppress(ValueError, TypeError):
                return int(pad_value)
            return 1

        def _emergency_reset(self) -> None:
            """Emergency reset when fullscreen state becomes inconsistent."""
            self._log("EMERGENCY RESET triggered!", "ERROR")

            with suppress(Exception):
                self.root.attributes("-fullscreen", False)

            if not self.fs_state.side_visible:
                with suppress(Exception):
                    self.pw.add(self.side, weight=2)

            # Show all cell frames
            for rc, frame in self.cell_frames.items():
                with suppress(Exception):
                    if saved := self.fs_state.saved_grid_info.get(rc):
                        self._restore_frame_grid_info(frame, saved)
                    else:
                        frame.grid()

            # Unmute and unpause all
            for ctrl in self.controllers.values():
                with suppress(Exception):
                    ctrl.set_property("mute", False)
                    ctrl.set_property("pause", False)

            # Reset all state
            self.fs_state.reset()

            # Update UI
            if hasattr(self, "fullscreen_btn"):
                self.fullscreen_btn.config(text="⛶ Fullscreen")
            if hasattr(self, "tile_fs_btn"):
                self.tile_fs_btn.config(text="Tile FS")

            self._show_overlay(False)
            self._log("Emergency reset done")

        # ========== Pop-out Fullscreen Player ==========

        def _popout_fullscreen_selected(self) -> None:
            """Open selected tile in separate fullscreen mpv process."""
            if (cell := self._selected_cell()) is None:
                return
            if (ctrl := self.controllers.get(cell)) is None:
                return

            p = self._resolve_local_path(ctrl)
            if not p or not p.exists():
                self._log("Current item is not a local file", "WARN")
                return

            # Pause the embedded tile
            ctrl.set_property("pause", True)

            mpv_opts = [
                "--no-config",
                f"--input-conf={self.assets.input_conf}",
                *[f"--script={pp}" for pp in self.assets.lua_paths.values()],
                "--fullscreen",
                "--keep-open=yes",
                f"--volume={self.config.volume}",
            ]

            env = os.environ.copy()
            env["PATH"] = f"{self.assets.bin_dir}:{env.get('PATH', '')}"

            try:
                subprocess.Popen(
                    ["mpv", *mpv_opts, str(p)],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    env=env,
                )
                self._log(f"Pop-out fullscreen: {p.name}")
            except Exception as e:
                self._log(f"Pop-out failed: {e}", "ERROR")

        # ========== Formatting Helpers ==========

        @staticmethod
        def _fmt_time(seconds: float | None) -> str:
            if seconds is None:
                return "--:--"
            try:
                s = int(seconds)
            except (ValueError, TypeError):
                return "--:--"
            if s < 0:
                return "--:--"
            h, rem = divmod(s, 3600)
            m, sec = divmod(rem, 60)
            return f"{h}:{m:02d}:{sec:02d}" if h else f"{m:02d}:{sec:02d}"

        def _status_line(
            self, path: str, pos: float | None, dur: float | None, paused: bool | None
        ) -> str:
            state = "PAUSE" if paused else "PLAY "
            base = Path(path).name if path else "-"
            pos_s = self._fmt_time(pos)
            dur_s = self._fmt_time(dur) if dur and dur > 0 else "--:--"
            if dur and dur > 0 and pos is not None and pos >= 0:
                pct = f"{int((pos / dur) * 100):3d}%"
            else:
                pct = "  -%"
            return f"{state} {pos_s}/{dur_s} {pct}  {base}"

        @staticmethod
        def _mpv_ok(resp: dict[str, Any]) -> bool:
            return isinstance(resp, dict) and resp.get("error") == "success"

        def _resolve_local_path(self, ctrl: MPVController) -> Path | None:
            if not (p := ctrl.get_property("path")) or not isinstance(p, str):
                return None
            if "://" in p:
                return None
            pp = Path(p)
            if pp.is_absolute():
                return pp
            if (wd := ctrl.get_property("working-directory")) and isinstance(wd, str):
                return (Path(wd) / pp).resolve()
            return pp.resolve()

        # ========== UI Setup ==========

        def _setup_ui(self) -> None:
            # Main layout: wall on top, controls below
            self.pw = ttk.Panedwindow(self.root, orient=tk.VERTICAL)
            self.pw.pack(fill=tk.BOTH, expand=True)

            # Video wall (top, takes most space)
            wall_container = tk.Frame(self.pw, bg="#0a0a0a")
            self.pw.add(wall_container, weight=8)
            self.wall_frame = wall_container

            # Bottom panel (sleek toolbar style)
            self.side = tk.Frame(self.pw, bg="#1a1a1a")
            self.pw.add(self.side, weight=1)

            # Overlay (fullscreen controls)
            self.overlay = tk.Frame(self.wall_frame, bg="#000000", bd=0)
            for txt, cmd in [
                ("Exit (Esc)", self._safe_exit_fullscreen),
                ("Exit Tile", self._exit_tile_fullscreen),
                ("Reset", self._emergency_reset),
            ]:
                tk.Button(
                    self.overlay, text=txt, command=cmd,
                    bg="#222", fg="#aaa", bd=0, padx=8, pady=4,
                    activebackground="#333", activeforeground="#fff"
                ).pack(side=tk.LEFT, padx=2, pady=4)
            self.overlay.place_forget()

            self._setup_bottom_panel(self.side)

        def _show_overlay(self, show: bool) -> None:
            if not self.wall_frame:
                return
            if show:
                self.overlay.place(x=8, y=8)
                with suppress(Exception):
                    self.overlay.lift()
            else:
                self.overlay.place_forget()

        def _setup_bottom_panel(self, parent: tk.Frame) -> None:
            frame_style = {"bg": "#1a1a1a", "bd": 0, "highlightthickness": 0}
            label_style = {"bg": "#1a1a1a", "fg": "#ccc"}
            btn_style = {
                "bg": "#2a2a2a", "fg": "#ddd", "bd": 0, "padx": 10, "pady": 4,
                "activebackground": "#3a3a3a", "activeforeground": "#fff"
            }
            small_btn = {**btn_style, "padx": 6}

            # ─── Row 1: Config + Main Controls ───
            row1 = tk.Frame(parent, **frame_style)
            row1.pack(fill=tk.X, pady=(4, 2), padx=8)

            # Grid config
            tk.Label(row1, text="Grid", **label_style).pack(side=tk.LEFT)
            self.cols_var = tk.StringVar(value="3")
            self.rows_var = tk.StringVar(value="3")
            for var, lbl in [(self.cols_var, "×"), (self.rows_var, "")]:
                e = tk.Entry(row1, textvariable=var, width=3, bg="#2a2a2a", fg="#fff",
                             insertbackground="#fff", bd=0, highlightthickness=1,
                             highlightbackground="#333", highlightcolor="#555")
                e.pack(side=tk.LEFT, padx=2)
                if lbl:
                    tk.Label(row1, text=lbl, **label_style).pack(side=tk.LEFT)

            tk.Frame(row1, width=12, **frame_style).pack(side=tk.LEFT)

            # Source path
            tk.Label(row1, text="Source", **label_style).pack(side=tk.LEFT, padx=(0, 4))
            self.source_var = tk.StringVar(value=self.initial_source)
            tk.Entry(
                row1, textvariable=self.source_var, width=40,
                bg="#2a2a2a", fg="#fff", insertbackground="#fff", bd=0,
                highlightthickness=1, highlightbackground="#333", highlightcolor="#555"
            ).pack(side=tk.LEFT, padx=2)
            tk.Button(row1, text="…", command=self._browse_source, **small_btn).pack(side=tk.LEFT, padx=2)

            tk.Frame(row1, width=16, **frame_style).pack(side=tk.LEFT)

            # Main buttons
            self.start_btn = tk.Button(row1, text="▶ Start", command=self._start_grid, **btn_style)
            self.start_btn.pack(side=tk.LEFT, padx=2)

            self.stop_btn = tk.Button(row1, text="■ Stop", command=self._stop_all, state=tk.DISABLED, **btn_style)
            self.stop_btn.pack(side=tk.LEFT, padx=2)

            self.fullscreen_btn = tk.Button(row1, text="⛶ Fullscreen", command=self._toggle_fullscreen, **btn_style)
            self.fullscreen_btn.pack(side=tk.LEFT, padx=2)

            # Spacer
            tk.Frame(row1, **frame_style).pack(side=tk.LEFT, fill=tk.X, expand=True)

            # Playback controls (right side)
            for txt, cmd in [
                ("⏮", self._prev_all), ("⏯", self._toggle_pause_all),
                ("⏭", self._next_all), ("🔀", self._shuffle_all), ("🔇", self._mute_all)
            ]:
                tk.Button(row1, text=txt, command=cmd, **small_btn).pack(side=tk.LEFT, padx=1)

            # Volume
            tk.Label(row1, text="Vol", **label_style).pack(side=tk.LEFT, padx=(8, 2))
            self.volume_var = tk.IntVar(value=self.config.volume)
            vol_scale = ttk.Scale(
                row1, from_=0, to=100, orient=tk.HORIZONTAL,
                variable=self.volume_var, command=self._set_volume_all, length=80
            )
            vol_scale.pack(side=tk.LEFT)

            # ─── Row 2: Monitor + Actions ───
            row2 = tk.Frame(parent, **frame_style)
            row2.pack(fill=tk.BOTH, expand=True, pady=(2, 4), padx=8)

            # Monitor (compact)
            mon_frame = tk.Frame(row2, **frame_style)
            mon_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

            cols = ("cell", "status", "path")
            self.monitor = ttk.Treeview(mon_frame, columns=cols, show="headings", height=4)
            self.monitor.heading("cell", text="Cell")
            self.monitor.heading("status", text="Status")
            self.monitor.heading("path", text="Path")
            self.monitor.column("cell", width=50, anchor="center")
            self.monitor.column("status", width=280, anchor="w")
            self.monitor.column("path", width=400, anchor="w")

            # Dark theme for treeview
            tree_style = ttk.Style()
            tree_style.configure("Treeview", background="#1e1e1e", foreground="#ccc",
                                 fieldbackground="#1e1e1e", rowheight=20)
            tree_style.configure("Treeview.Heading", background="#2a2a2a", foreground="#aaa")
            tree_style.map("Treeview", background=[("selected", "#3a3a3a")])

            self.monitor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

            scrollbar = ttk.Scrollbar(mon_frame, orient=tk.VERTICAL, command=self.monitor.yview)
            self.monitor.configure(yscrollcommand=scrollbar.set)
            scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

            self.monitor.bind("<<TreeviewSelect>>", self._on_monitor_select)
            self.monitor.bind("<Double-1>", lambda e: self._toggle_tile_fullscreen())

            # Action buttons (right side, vertical)
            actions = tk.Frame(row2, **frame_style)
            actions.pack(side=tk.RIGHT, fill=tk.Y, padx=(8, 0))

            self.tile_fs_btn = tk.Button(actions, text="Tile FS", command=self._toggle_tile_fullscreen, **small_btn)
            self.tile_fs_btn.pack(fill=tk.X, pady=1)
            tk.Button(actions, text="Pop-out", command=self._popout_fullscreen_selected, **small_btn).pack(fill=tk.X, pady=1)
            tk.Button(actions, text="Rename", command=self._rename_selected_current, **small_btn).pack(fill=tk.X, pady=1)
            tk.Button(actions, text="Copy", command=self._copy_selected_path, **small_btn).pack(fill=tk.X, pady=1)

            # ─── Row 3: Status bar ───
            row3 = tk.Frame(parent, bg="#111")
            row3.pack(fill=tk.X, side=tk.BOTTOM)

            self.selected_path_var = tk.StringVar(value="")
            tk.Label(row3, textvariable=self.selected_path_var, bg="#111", fg="#666",
                     anchor="w", padx=8, pady=2).pack(side=tk.LEFT, fill=tk.X, expand=True)

            self.status_var = tk.StringVar(value="Ready")
            tk.Label(row3, textvariable=self.status_var, bg="#111", fg="#555",
                     anchor="e", padx=8, pady=2).pack(side=tk.RIGHT)

            # Hidden scrolled text for logging (minimal)
            self.status_text = scrolledtext.ScrolledText(parent, height=1, state=tk.DISABLED,
                                                         bg="#111", fg="#444", bd=0)
            # Don't pack - only used internally for log storage

        def _browse_source(self) -> None:
            if path := filedialog.askdirectory(initialdir=self.source_var.get()):
                self.source_var.set(path)

        # ========== Wall Grid Building ==========

        def _build_wall_grid(self, rows: int, cols: int) -> None:
            assert self.wall_frame is not None

            self.current_rows = rows
            self.current_cols = cols

            # Destroy all cell frames, keep overlay
            for w in list(self.wall_frame.winfo_children()):
                if w is not self.overlay:
                    w.destroy()
            self.cell_frames.clear()

            for r in range(rows):
                self.wall_frame.grid_rowconfigure(r, weight=1, uniform="wall")
            for c in range(cols):
                self.wall_frame.grid_columnconfigure(c, weight=1, uniform="wall")

            for r in range(rows):
                for c in range(cols):
                    cell = tk.Frame(
                        self.wall_frame,
                        bg="black",
                        highlightthickness=1,
                        highlightbackground="#202020",
                    )
                    cell.grid(row=r, column=c, sticky="nsew", padx=1, pady=1)
                    self.cell_frames[(r, c)] = cell

            self.root.update_idletasks()

        # ========== Grid Lifecycle ==========

        def _start_grid(self) -> None:
            try:
                cols = int(self.cols_var.get())
                rows = int(self.rows_var.get())
                source = self.source_var.get()

                self._log(f"Starting {cols}x{rows} wall…")
                self._log(f"Scanning {source}…")

                all_files, videos, images = scan_files(source)
                if not all_files:
                    messagebox.showerror("Error", f"No media files found in {source}")
                    return

                self._log(f"Found {len(all_files)} files ({len(videos)} videos, {len(images)} images)")

                self._build_wall_grid(rows, cols)

                # Reset fullscreen state
                self.fs_state.reset()

                # Clear monitor
                for item in self.monitor.get_children():
                    self.monitor.delete(item)
                self.monitor_items.clear()
                self.last_paths.clear()
                self.playlists.clear()

                for row in range(rows):
                    for col in range(cols):
                        playlist = all_files.copy()
                        random.shuffle(playlist)

                        cell = self.cell_frames[(row, col)]
                        wid = cell.winfo_id()

                        self._spawn_cell(row, col, playlist, wid)

                self.start_btn.config(state=tk.DISABLED)
                self.stop_btn.config(state=tk.NORMAL)

                self._log(f"Wall started with {len(self.processes)} tiles")
                Thread(target=self._monitor_loop, daemon=True).start()
                self._start_monitor_updates()

            except Exception as e:
                messagebox.showerror("Error", str(e))
                self._log(f"Error: {e}", "ERROR")

        def _spawn_cell(self, row: int, col: int, playlist: list[Path], wid: int) -> None:
            socket_path = f"/tmp/mpv-grid-{self.grid_id}-{row}-{col}.sock"
            Path(socket_path).unlink(missing_ok=True)

            pl_file = self.tmpdir / f"cell-{row}-{col}.m3u"
            pl_file.write_text("\n".join(str(p) for p in playlist) + "\n", encoding="utf-8")

            # Detect Wayland
            is_wayland = "WAYLAND_DISPLAY" in os.environ

            mpv_opts = [
                *self.config.base_mpv_opts(),
                f"--input-conf={self.assets.input_conf}",
                *[f"--script={p}" for p in self.assets.lua_paths.values()],
            ]

            # Force XWayland for --wid embedding on Wayland
            if is_wayland:
                mpv_opts.extend([
                    "--gpu-context=x11egl",
                    "--x11-name=mpv-grid-cell",
                ])

            cmd = [
                "mpv",
                *mpv_opts,
                f"--input-ipc-server={socket_path}",
                f"--wid={wid}",
                f"--playlist={pl_file}",
            ]

            env = os.environ.copy()
            env["PATH"] = f"{self.assets.bin_dir}:{env.get('PATH', '')}"
            env["MPV_GRID_SOCKET_GLOB"] = f"/tmp/mpv-grid-{self.grid_id}-*.sock"

            proc = subprocess.Popen(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                env=env,
            )

            self.processes.append(proc)
            self.controllers[(row, col)] = MPVController(socket_path)
            self.playlists[(row, col)] = playlist

            item_id = self.monitor.insert("", "end", values=(f"{row},{col}", "starting…", ""))
            self.monitor_items[(row, col)] = item_id

            self._log(f"Tile [{row},{col}] started")

        def _monitor_loop(self) -> None:
            while self.processes:
                alive = sum(1 for p in self.processes if p.poll() is None)
                if alive == 0:
                    self.root.after(0, lambda: self._log("All tiles closed"))
                    self.root.after(0, lambda: self.start_btn.config(state=tk.NORMAL))
                    self.root.after(0, lambda: self.stop_btn.config(state=tk.DISABLED))
                    break
                time.sleep(2)

        def _stop_all(self) -> None:
            self._log("Stopping all tiles…")

            if self._monitor_job is not None:
                with suppress(Exception):
                    self.root.after_cancel(self._monitor_job)
                self._monitor_job = None

            for proc in self.processes:
                try:
                    proc.terminate()
                    proc.wait(timeout=2)
                except Exception:
                    with suppress(Exception):
                        proc.kill()

            self.processes.clear()
            self.controllers.clear()
            self.playlists.clear()

            # Clear wall (keep overlay)
            if self.wall_frame is not None:
                for w in list(self.wall_frame.winfo_children()):
                    if w is not self.overlay:
                        w.destroy()
            self.cell_frames.clear()

            # Reset fullscreen state
            self.fs_state.reset()
            self._show_overlay(False)

            # Reset UI to normal
            with suppress(Exception):
                self.root.attributes("-fullscreen", False)

            if not self.fs_state.side_visible:
                with suppress(Exception):
                    self.pw.add(self.side, weight=2)
                    self.fs_state.side_visible = True

            self.start_btn.config(state=tk.NORMAL)
            self.stop_btn.config(state=tk.DISABLED)

            if hasattr(self, "fullscreen_btn"):
                self.fullscreen_btn.config(text="⛶ Fullscreen")
            if hasattr(self, "tile_fs_btn"):
                self.tile_fs_btn.config(text="Tile FS")

            self._log("Stopped")

        def _cleanup(self) -> None:
            rmtree(self.tmpdir, ignore_errors=True)

        def _on_close(self) -> None:
            if self.processes:
                if messagebox.askokcancel("Quit", "Stop all tiles and quit?"):
                    self._stop_all()
                    self._cleanup()
                    self.root.destroy()
            else:
                self._cleanup()
                self.root.destroy()

        # ========== Live Monitor Refresh ==========

        def _start_monitor_updates(self) -> None:
            if self._monitor_job is not None:
                with suppress(Exception):
                    self.root.after_cancel(self._monitor_job)
                self._monitor_job = None
            self._refresh_monitor()

        def _refresh_monitor(self) -> None:
            """Batch update for better performance."""
            for (row, col), ctrl in list(self.controllers.items()):
                if not (item := self.monitor_items.get((row, col))):
                    continue

                path = ctrl.get_property("path") or ""
                pos = ctrl.get_property("time-pos")
                dur = ctrl.get_property("duration")
                paused = ctrl.get_property("pause")

                try:
                    pos_f = float(pos) if pos is not None else None
                except (ValueError, TypeError):
                    pos_f = None
                try:
                    dur_f = float(dur) if dur is not None else None
                except (ValueError, TypeError):
                    dur_f = None

                status = self._status_line(path, pos_f, dur_f, bool(paused) if paused is not None else None)
                filename = Path(path).name if path else ""

                # Store full path for copy/rename
                if path:
                    self.last_paths[(row, col)] = path

                self.monitor.item(item, values=(f"{row},{col}", status, filename))

            self._monitor_job = self.root.after(1000, self._refresh_monitor)

        def _on_monitor_select(self, _evt: tk.Event | None = None) -> None:
            if not (sel := self.monitor.selection()):
                self.selected_path_var.set("")
                return
            if cell := self._selected_cell():
                if path := self.last_paths.get(cell):
                    self.selected_path_var.set(path)

        def _copy_selected_path(self) -> None:
            if not (cell := self._selected_cell()):
                return
            if not (p := self.last_paths.get(cell)):
                return
            self.root.clipboard_clear()
            self.root.clipboard_append(p)
            self._log(f"Copied: {Path(p).name}")

        def _selected_cell(self) -> CellKey | None:
            if not (sel := self.monitor.selection()):
                return None
            item = sel[0]
            for k, v in self.monitor_items.items():
                if v == item:
                    return k
            return None

        # ========== Rename (Current File) ==========

        def _update_playlist_entry_noncurrent(self, ctrl: MPVController, idx: int, new_path: Path) -> None:
            ctrl.send_command("playlist-remove", idx)
            resp = ctrl.send_command("loadfile", str(new_path), "insert-at", idx)
            if self._mpv_ok(resp):
                return

            ctrl.send_command("loadfile", str(new_path), "append")
            cnt = ctrl.get_property("playlist-count")
            with suppress(ValueError, TypeError):
                new_idx = int(cnt) - 1
                ctrl.send_command("playlist-move", new_idx, idx)

        def _replace_current_entry(self, ctrl: MPVController, idx: int, new_path: Path) -> None:
            pos = ctrl.get_property("time-pos")
            paused = bool(ctrl.get_property("pause") or False)
            try:
                pos_f = float(pos) if pos is not None else 0.0
            except (ValueError, TypeError):
                pos_f = 0.0

            resp = ctrl.send_command("loadfile", str(new_path), "insert-at", idx + 1)
            if not self._mpv_ok(resp):
                return

            ctrl.send_command("playlist-play-index", idx + 1)

            deadline = time.time() + 2.0
            while time.time() < deadline:
                if p := ctrl.get_property("path"):
                    if isinstance(p, str):
                        with suppress(Exception):
                            if Path(p).resolve() == new_path.resolve():
                                break
                time.sleep(0.05)

            ctrl.set_property("pause", True)
            ctrl.set_property("time-pos", pos_f)
            ctrl.set_property("pause", paused)

            ctrl.send_command("playlist-remove", idx)

        def _rename_selected_current(self) -> None:
            if (cell := self._selected_cell()) is None:
                return

            if (ctrl := self.controllers.get(cell)) is None:
                return

            old_path = self._resolve_local_path(ctrl)
            if not old_path or not old_path.exists() or not old_path.is_file():
                messagebox.showerror("Rename", "Current item is not a local file.")
                return

            new_name = simpledialog.askstring(
                "Rename current file",
                f"Rename:\n{old_path}\n\nNew name (basename; extension optional):",
                initialvalue=old_path.stem,
            )
            if not new_name:
                return
            if "/" in new_name or "\\" in new_name:
                messagebox.showerror("Rename", "Invalid name (must not contain path separators).")
                return

            target_name = new_name if Path(new_name).suffix else new_name + old_path.suffix
            new_path = old_path.with_name(target_name)
            if new_path.exists():
                messagebox.showerror("Rename", f"Target already exists:\n{new_path}")
                return

            try:
                old_path.rename(new_path)
            except Exception as e:
                messagebox.showerror("Rename", f"Rename failed:\n{e}")
                return

            for cell_k, pl in self.playlists.items():
                try:
                    idx = pl.index(old_path)
                except ValueError:
                    continue

                pl[idx] = new_path

                if not (c := self.controllers.get(cell_k)):
                    continue

                cur = c.get_property("playlist-pos")
                try:
                    cur_i = int(cur)
                except (ValueError, TypeError):
                    cur_i = -999

                if idx == cur_i:
                    self._replace_current_entry(c, idx, new_path)
                else:
                    self._update_playlist_entry_noncurrent(c, idx, new_path)

            self._log(f"Renamed: {old_path.name} -> {new_path.name}")

        # ========== Basic Controls ==========

        def _toggle_pause_all(self) -> None:
            for c in self.controllers.values():
                c.cycle("pause")
            self._log("Play/Pause toggled (all)")

        def _next_all(self) -> None:
            for c in self.controllers.values():
                c.playlist_next()
            self._log("Next (all)")

        def _prev_all(self) -> None:
            for c in self.controllers.values():
                c.playlist_prev()
            self._log("Prev (all)")

        def _shuffle_all(self) -> None:
            for c in self.controllers.values():
                c.playlist_shuffle()
            self._log("Shuffle (all)")

        def _mute_all(self) -> None:
            for c in self.controllers.values():
                c.cycle("mute")
            self._log("Mute toggled (all)")

        def _set_volume_all(self, value: str) -> None:
            vol = int(float(value))
            for c in self.controllers.values():
                c.set_property("volume", vol)

    root = tk.Tk()
    _app = MPVGridApp(root, initial_source)
    root.mainloop()


# ============================
# Main
# ============================

def main() -> None:
    parser = argparse.ArgumentParser(
        prog="goobert",
        description="Goobert - Video Wall for MPV",
        epilog=f"Repository: {__repo__}",
    )
    parser.add_argument(
        "source",
        nargs="?",
        default=None,
        help="Source directory containing media files",
    )
    parser.add_argument(
        "--broadcast",
        choices=["next", "shuffle"],
        help="Broadcast action to all running mpv-grid instances",
    )
    parser.add_argument(
        "--glob",
        dest="sock_glob",
        default=None,
        help="Socket glob pattern for broadcast (default: auto-detect)",
    )
    parser.add_argument(
        "--version",
        action="version",
        version=f"%(prog)s {__version__}",
    )
    args = parser.parse_args()

    if args.broadcast:
        n = broadcast(args.broadcast, args.sock_glob)
        print(f"Sent to {n} instance(s)")
        return

    run_gui(args.source)


if __name__ == "__main__":
    main()
