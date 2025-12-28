use directories::ProjectDirs;
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;
use std::sync::OnceLock;

static CONFIG: OnceLock<Config> = OnceLock::new();

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    #[serde(default)]
    pub playback: PlaybackConfig,
    #[serde(default)]
    pub grid: GridConfig,
    #[serde(default)]
    pub paths: PathsConfig,
    #[serde(default)]
    pub skipper: SkipperConfig,
    #[serde(default)]
    pub seek: SeekConfig,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PlaybackConfig {
    #[serde(default = "default_loop_count")]
    pub loop_count: u32,
    #[serde(default = "default_volume")]
    pub default_volume: u32,
    #[serde(default = "default_image_duration")]
    pub image_display_duration: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct GridConfig {
    #[serde(default = "default_rows")]
    pub default_rows: u32,
    #[serde(default = "default_cols")]
    pub default_cols: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PathsConfig {
    #[serde(default = "default_media_path")]
    pub default_media_path: String,
    #[serde(default = "default_screenshot_path")]
    pub screenshot_path: String,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SkipperConfig {
    #[serde(default)]
    pub enabled: bool,
    #[serde(default = "default_skip_percent")]
    pub skip_percent: f64,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SeekConfig {
    #[serde(default = "default_seek_amount")]
    pub amount_seconds: u32,
}

// Default value functions
fn default_loop_count() -> u32 { 5 }
fn default_volume() -> u32 { 30 }
fn default_image_duration() -> f64 { 2.5 }
fn default_rows() -> u32 { 3 }
fn default_cols() -> u32 { 3 }
fn default_media_path() -> String {
    dirs::video_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .to_string_lossy()
        .to_string()
}
fn default_screenshot_path() -> String {
    dirs::picture_dir()
        .unwrap_or_else(|| PathBuf::from("."))
        .to_string_lossy()
        .to_string()
}
fn default_skip_percent() -> f64 { 0.33 }
fn default_seek_amount() -> u32 { 30 }

impl Default for PlaybackConfig {
    fn default() -> Self {
        Self {
            loop_count: default_loop_count(),
            default_volume: default_volume(),
            image_display_duration: default_image_duration(),
        }
    }
}

impl Default for GridConfig {
    fn default() -> Self {
        Self {
            default_rows: default_rows(),
            default_cols: default_cols(),
        }
    }
}

impl Default for PathsConfig {
    fn default() -> Self {
        Self {
            default_media_path: default_media_path(),
            screenshot_path: default_screenshot_path(),
        }
    }
}

impl Default for SkipperConfig {
    fn default() -> Self {
        Self {
            enabled: false,
            skip_percent: default_skip_percent(),
        }
    }
}

impl Default for SeekConfig {
    fn default() -> Self {
        Self {
            amount_seconds: default_seek_amount(),
        }
    }
}

impl Default for Config {
    fn default() -> Self {
        Self {
            playback: PlaybackConfig::default(),
            grid: GridConfig::default(),
            paths: PathsConfig::default(),
            skipper: SkipperConfig::default(),
            seek: SeekConfig::default(),
        }
    }
}

impl Config {
    pub fn instance() -> &'static Config {
        CONFIG.get_or_init(|| Config::load())
    }

    fn config_path() -> Option<PathBuf> {
        ProjectDirs::from("", "", "goobert")
            .map(|dirs| dirs.config_dir().join("goobert.toml"))
    }

    fn load() -> Config {
        if let Some(path) = Self::config_path() {
            if path.exists() {
                if let Ok(content) = fs::read_to_string(&path) {
                    if let Ok(config) = toml::from_str(&content) {
                        return config;
                    }
                }
            }
        }
        Config::default()
    }

    #[allow(dead_code)]
    pub fn save(&self) -> anyhow::Result<()> {
        if let Some(path) = Self::config_path() {
            if let Some(parent) = path.parent() {
                fs::create_dir_all(parent)?;
            }
            let content = toml::to_string_pretty(self)?;
            fs::write(path, content)?;
        }
        Ok(())
    }
}
