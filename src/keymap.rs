use egui::{Key, Modifiers};
use std::collections::HashMap;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Action {
    // Global actions
    PauseAll,
    NextAll,
    PrevAll,
    ShuffleAll,
    ShuffleThenNextAll,
    FullscreenGlobal,
    ExitFullscreen,
    VolumeUp,
    VolumeDown,
    MuteAll,

    // Navigation
    NavigateUp,
    NavigateDown,
    NavigateLeft,
    NavigateRight,

    // Selected cell actions
    FullscreenSelected,
    SeekForward,
    SeekBackward,
    FrameStepForward,
    FrameStepBackward,
    ToggleLoop,
    ZoomIn,
    ZoomOut,
    Rotate,
    Screenshot,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct KeyBinding {
    pub key: Key,
    pub modifiers: Modifiers,
}

impl KeyBinding {
    pub const fn new(key: Key, modifiers: Modifiers) -> Self {
        Self { key, modifiers }
    }

    pub const fn simple(key: Key) -> Self {
        Self::new(key, Modifiers::NONE)
    }

    pub const fn with_shift(key: Key) -> Self {
        Self::new(key, Modifiers::SHIFT)
    }

    pub const fn with_ctrl(key: Key) -> Self {
        Self::new(key, Modifiers::CTRL)
    }
}

pub struct KeyMap {
    bindings: HashMap<KeyBinding, Action>,
    descriptions: HashMap<Action, &'static str>,
}

impl Default for KeyMap {
    fn default() -> Self {
        Self::new()
    }
}

impl KeyMap {
    pub fn new() -> Self {
        let mut keymap = Self {
            bindings: HashMap::new(),
            descriptions: HashMap::new(),
        };
        keymap.setup_default_bindings();
        keymap
    }

    fn setup_default_bindings(&mut self) {
        use Action::*;
        use Key::*;

        // Pause/Play - multiple bindings
        self.bind(KeyBinding::simple(Space), PauseAll);
        self.bind(KeyBinding::simple(P), PauseAll);

        // Volume
        self.bind(KeyBinding::simple(U), VolumeUp);
        self.bind(KeyBinding::simple(I), VolumeDown);
        self.bind(KeyBinding::simple(M), MuteAll);

        // Navigation - WASD
        self.bind(KeyBinding::simple(W), NavigateUp);
        self.bind(KeyBinding::simple(S), NavigateDown);
        self.bind(KeyBinding::simple(A), NavigateLeft);
        self.bind(KeyBinding::simple(D), NavigateRight);

        // Seek
        self.bind(KeyBinding::simple(V), SeekForward);
        self.bind(KeyBinding::simple(C), SeekBackward);

        // Screenshot
        self.bind(KeyBinding::simple(T), Screenshot);

        // Frame stepping
        self.bind(KeyBinding::simple(B), FrameStepBackward);
        self.bind(KeyBinding::simple(N), FrameStepForward);

        // Grid controls
        self.bind(KeyBinding::with_shift(N), NextAll);
        self.bind(KeyBinding::simple(Q), ShuffleAll);
        self.bind(KeyBinding::simple(X), ShuffleThenNextAll);

        // Fullscreen
        self.bind(KeyBinding::simple(F11), FullscreenGlobal);
        self.bind(KeyBinding::simple(Escape), ExitFullscreen);
        self.bind(KeyBinding::simple(F), FullscreenSelected);

        // Selected cell controls
        self.bind(KeyBinding::simple(L), ToggleLoop);
        self.bind(KeyBinding::simple(Z), ZoomIn);
        self.bind(KeyBinding::with_shift(Z), ZoomOut);
        self.bind(KeyBinding::with_ctrl(R), Rotate);

        // Descriptions
        self.descriptions.insert(PauseAll, "Pause/Play all cells");
        self.descriptions.insert(NextAll, "Next video (all cells)");
        self.descriptions.insert(PrevAll, "Previous video (all cells)");
        self.descriptions.insert(ShuffleAll, "Shuffle all playlists");
        self.descriptions.insert(ShuffleThenNextAll, "Shuffle then next");
        self.descriptions.insert(FullscreenGlobal, "Toggle fullscreen");
        self.descriptions.insert(ExitFullscreen, "Exit fullscreen");
        self.descriptions.insert(VolumeUp, "Volume up");
        self.descriptions.insert(VolumeDown, "Volume down");
        self.descriptions.insert(MuteAll, "Mute all");
        self.descriptions.insert(NavigateUp, "Navigate up");
        self.descriptions.insert(NavigateDown, "Navigate down");
        self.descriptions.insert(NavigateLeft, "Navigate left");
        self.descriptions.insert(NavigateRight, "Navigate right");
        self.descriptions.insert(FullscreenSelected, "Fullscreen selected");
        self.descriptions.insert(SeekForward, "Seek forward 5s");
        self.descriptions.insert(SeekBackward, "Seek backward 5s");
        self.descriptions.insert(FrameStepForward, "Frame step forward");
        self.descriptions.insert(FrameStepBackward, "Frame step backward");
        self.descriptions.insert(ToggleLoop, "Toggle loop");
        self.descriptions.insert(ZoomIn, "Zoom in");
        self.descriptions.insert(ZoomOut, "Zoom out");
        self.descriptions.insert(Rotate, "Rotate video");
        self.descriptions.insert(Screenshot, "Take screenshot");
    }

    fn bind(&mut self, binding: KeyBinding, action: Action) {
        self.bindings.insert(binding, action);
    }

    pub fn get_action(&self, key: Key, modifiers: Modifiers) -> Option<Action> {
        // Normalize modifiers (ignore non-essential ones)
        let normalized = Modifiers {
            alt: modifiers.alt,
            ctrl: modifiers.ctrl,
            shift: modifiers.shift,
            mac_cmd: false,
            command: modifiers.command,
        };

        self.bindings.get(&KeyBinding::new(key, normalized)).copied()
    }

    pub fn get_description(&self, action: Action) -> &'static str {
        self.descriptions.get(&action).unwrap_or(&"Unknown action")
    }

    pub fn generate_help_text(&self) -> String {
        let mut lines = vec!["Keyboard Shortcuts:".to_string(), String::new()];

        let mut action_keys: HashMap<Action, Vec<String>> = HashMap::new();
        for (binding, action) in &self.bindings {
            let key_str = format_key_binding(binding);
            action_keys.entry(*action).or_default().push(key_str);
        }

        lines.push("Global:".to_string());
        for action in [
            Action::PauseAll,
            Action::VolumeUp,
            Action::VolumeDown,
            Action::MuteAll,
            Action::NextAll,
            Action::ShuffleAll,
            Action::FullscreenGlobal,
        ] {
            if let Some(keys) = action_keys.get(&action) {
                lines.push(format!(
                    "  {} - {}",
                    keys.join("/"),
                    self.get_description(action)
                ));
            }
        }

        lines.push(String::new());
        lines.push("Navigation: WASD".to_string());

        lines.push(String::new());
        lines.push("Selected Cell:".to_string());
        for action in [
            Action::FullscreenSelected,
            Action::SeekForward,
            Action::SeekBackward,
            Action::FrameStepForward,
            Action::FrameStepBackward,
            Action::Screenshot,
            Action::ToggleLoop,
            Action::ZoomIn,
            Action::ZoomOut,
            Action::Rotate,
        ] {
            if let Some(keys) = action_keys.get(&action) {
                lines.push(format!(
                    "  {} - {}",
                    keys.join("/"),
                    self.get_description(action)
                ));
            }
        }

        lines.join("\n")
    }
}

fn format_key_binding(binding: &KeyBinding) -> String {
    let mut parts = Vec::new();

    if binding.modifiers.ctrl {
        parts.push("Ctrl");
    }
    if binding.modifiers.shift {
        parts.push("Shift");
    }
    if binding.modifiers.alt {
        parts.push("Alt");
    }

    let key_name = match binding.key {
        Key::Space => "Space",
        Key::Escape => "Esc",
        Key::F11 => "F11",
        _ => return format!("{}{:?}",
            if parts.is_empty() { String::new() } else { parts.join("+") + "+" },
            binding.key
        ),
    };

    if parts.is_empty() {
        key_name.to_string()
    } else {
        format!("{}+{}", parts.join("+"), key_name)
    }
}
