use anyhow::{anyhow, Result};
use libmpv2::{events::Event, Mpv};
use std::collections::HashSet;
use std::path::Path;

use crate::config::Config;

#[derive(Debug, Clone, Default)]
pub struct PlayerState {
    pub path: String,
    pub position: f64,
    pub duration: f64,
    pub paused: bool,
    pub volume: i64,
    pub muted: bool,
    pub loop_file: bool,
}

pub struct MpvPlayer {
    mpv: Mpv,
    playlist: Vec<String>,
    playlist_index: usize,
    seen_files: HashSet<String>,
    skipper_enabled: bool,
    skip_percent: f64,
    rotation: i64,
}

impl MpvPlayer {
    pub fn new() -> Result<Self> {
        let mpv = Mpv::new().map_err(|e| anyhow!("Failed to create MPV: {:?}", e))?;
        let config = Config::instance();

        // Helper to set properties with error conversion
        macro_rules! set_prop {
            ($name:expr, $val:expr) => {
                mpv.set_property($name, $val)
                    .map_err(|e| anyhow!("Failed to set {}: {:?}", $name, e))?
            };
        }

        // Configure MPV options
        set_prop!("terminal", false);
        set_prop!("vo", "null");
        set_prop!("keep-open", false);
        set_prop!("idle", true);
        set_prop!("input-default-bindings", false);
        set_prop!("osc", false);

        // High-quality settings
        set_prop!("hwdec", "auto-safe");

        // Playback settings from config
        set_prop!("loop-file", config.playback.loop_count as i64);
        set_prop!("image-display-duration", config.playback.image_display_duration);
        set_prop!("volume", config.playback.default_volume as i64);

        // Screenshot settings
        set_prop!("screenshot-directory", config.paths.screenshot_path.as_str());
        set_prop!("screenshot-template", "%f-%P");
        set_prop!("screenshot-format", "png");

        Ok(Self {
            mpv,
            playlist: Vec::new(),
            playlist_index: 0,
            seen_files: HashSet::new(),
            skipper_enabled: config.skipper.enabled,
            skip_percent: config.skipper.skip_percent,
            rotation: 0,
        })
    }

    pub fn load_playlist(&mut self, files: Vec<String>) {
        self.playlist = files;
        self.playlist_index = 0;

        if let Some(first) = self.playlist.first() {
            self.load_file(first);
        }
    }

    pub fn load_file<P: AsRef<Path>>(&self, path: P) {
        let path_str = path.as_ref().to_string_lossy();
        if let Err(e) = self.mpv.command("loadfile", &[&path_str]) {
            log::error!("Failed to load file: {:?}", e);
        }
    }

    pub fn play(&self) {
        let _ = self.mpv.set_property("pause", false);
    }

    pub fn pause(&self) {
        let _ = self.mpv.set_property("pause", true);
    }

    pub fn toggle_pause(&self) {
        let _ = self.mpv.command("cycle", &["pause"]);
    }

    pub fn stop(&self) {
        let _ = self.mpv.command("stop", &[]);
    }

    pub fn next(&mut self) {
        if self.playlist.is_empty() {
            return;
        }

        self.playlist_index = (self.playlist_index + 1) % self.playlist.len();
        if let Some(path) = self.playlist.get(self.playlist_index).cloned() {
            self.load_file(&path);
        }
    }

    pub fn prev(&mut self) {
        if self.playlist.is_empty() {
            return;
        }

        self.playlist_index = if self.playlist_index == 0 {
            self.playlist.len() - 1
        } else {
            self.playlist_index - 1
        };

        if let Some(path) = self.playlist.get(self.playlist_index).cloned() {
            self.load_file(&path);
        }
    }

    pub fn shuffle(&mut self) {
        use rand::seq::SliceRandom;
        let mut rng = rand::thread_rng();
        self.playlist.shuffle(&mut rng);
    }

    pub fn seek(&self, seconds: f64) {
        let _ = self.mpv.command("seek", &[&seconds.to_string(), "relative"]);
    }

    pub fn seek_absolute(&self, seconds: f64) {
        let _ = self.mpv.command("seek", &[&seconds.to_string(), "absolute"]);
    }

    pub fn frame_step(&self) {
        let _ = self.mpv.command("frame-step", &[]);
    }

    pub fn frame_back_step(&self) {
        let _ = self.mpv.command("frame-back-step", &[]);
    }

    pub fn set_volume(&self, volume: i64) {
        let _ = self.mpv.set_property("volume", volume);
    }

    pub fn toggle_mute(&self) {
        let _ = self.mpv.command("cycle", &["mute"]);
    }

    pub fn mute(&self) {
        let _ = self.mpv.set_property("mute", true);
    }

    pub fn unmute(&self) {
        let _ = self.mpv.set_property("mute", false);
    }

    pub fn toggle_loop(&self) {
        let current = self.get_string_property("loop-file");
        let new_value = if current == "inf" { "no" } else { "inf" };
        let _ = self.mpv.set_property("loop-file", new_value);
    }

    pub fn is_loop_file(&self) -> bool {
        self.get_string_property("loop-file") == "inf"
    }

    pub fn rotate(&mut self) {
        self.rotation = (self.rotation + 90) % 360;
        let _ = self.mpv.set_property("video-rotate", self.rotation);
    }

    pub fn zoom_in(&self) {
        let zoom = self.get_f64_property("video-zoom");
        let _ = self.mpv.set_property("video-zoom", zoom + 0.1);
    }

    pub fn zoom_out(&self) {
        let zoom = self.get_f64_property("video-zoom");
        let _ = self.mpv.set_property("video-zoom", zoom - 0.1);
    }

    pub fn screenshot(&self) {
        let _ = self.mpv.command("screenshot", &[]);
    }

    pub fn update_playlist_path(&mut self, old_path: &str, new_path: &str) {
        for path in &mut self.playlist {
            if path == old_path {
                *path = new_path.to_string();
            }
        }
    }

    fn get_string_property(&self, name: &str) -> String {
        self.mpv.get_property::<String>(name).unwrap_or_default()
    }

    fn get_f64_property(&self, name: &str) -> f64 {
        self.mpv.get_property::<f64>(name).unwrap_or(0.0)
    }

    fn get_i64_property(&self, name: &str) -> i64 {
        self.mpv.get_property::<i64>(name).unwrap_or(0)
    }

    fn get_bool_property(&self, name: &str) -> bool {
        self.mpv.get_property::<bool>(name).unwrap_or(false)
    }

    pub fn poll_state(&self) -> PlayerState {
        PlayerState {
            path: self.get_string_property("path"),
            position: self.get_f64_property("time-pos"),
            duration: self.get_f64_property("duration"),
            paused: self.get_bool_property("pause"),
            volume: self.get_i64_property("volume"),
            muted: self.get_bool_property("mute"),
            loop_file: self.is_loop_file(),
        }
    }

    pub fn process_events(&mut self) {
        let mut file_loaded = false;
        let mut file_ended = false;

        loop {
            match self.mpv.wait_event(0.0) {
                Some(Ok(Event::FileLoaded)) => file_loaded = true,
                Some(Ok(Event::EndFile(_))) => file_ended = true,
                Some(Err(e)) => log::warn!("MPV event error: {:?}", e),
                None => break,
                _ => {}
            }
        }

        if file_loaded {
            self.on_file_loaded();
        }
        if file_ended {
            self.next();
        }
    }

    fn on_file_loaded(&mut self) {
        let path = self.get_string_property("path");

        if self.skipper_enabled && !path.is_empty() && !self.seen_files.contains(&path) {
            self.seen_files.insert(path);

            let duration = self.get_f64_property("duration");
            if duration > 0.0 {
                let target = duration * self.skip_percent;
                self.seek_absolute(target);
            }
        }
    }

    pub fn current_file(&self) -> String {
        self.get_string_property("path")
    }

    pub fn position(&self) -> f64 {
        self.get_f64_property("time-pos")
    }

    pub fn duration(&self) -> f64 {
        self.get_f64_property("duration")
    }

    pub fn is_paused(&self) -> bool {
        self.get_bool_property("pause")
    }
}

impl Drop for MpvPlayer {
    fn drop(&mut self) {
        self.stop();
    }
}
