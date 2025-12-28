use crate::mpv_player::{MpvPlayer, PlayerState};
use anyhow::Result;

pub struct GridCell {
    pub row: usize,
    pub col: usize,
    player: Option<MpvPlayer>,
    selected: bool,
    state: PlayerState,
    render_initialized: bool,
}

impl GridCell {
    pub fn new(row: usize, col: usize) -> Self {
        Self {
            row,
            col,
            player: None,
            selected: false,
            state: PlayerState::default(),
            render_initialized: false,
        }
    }

    pub fn initialize(&mut self) -> Result<()> {
        self.player = Some(MpvPlayer::new()?);
        Ok(())
    }

    /// Initialize the render context (must be called after GL context is available)
    pub fn init_render_context(&mut self) -> Result<()> {
        if let Some(player) = &mut self.player {
            if !self.render_initialized {
                player.init_render_context()?;
                self.render_initialized = true;
            }
        }
        Ok(())
    }

    pub fn has_render_context(&self) -> bool {
        self.render_initialized
    }

    /// Check if the player needs to render a new frame
    pub fn needs_render(&self) -> bool {
        self.player
            .as_ref()
            .map(|p| p.needs_render())
            .unwrap_or(false)
    }

    /// Render current frame to the given FBO
    pub fn render(&mut self, fbo: i32, width: i32, height: i32) -> bool {
        if let Some(player) = &mut self.player {
            player.render(fbo, width, height)
        } else {
            false
        }
    }

    /// Report that the frame has been displayed
    pub fn report_swap(&self) {
        if let Some(player) = &self.player {
            player.report_swap();
        }
    }

    pub fn set_playlist(&mut self, files: Vec<String>) {
        if let Some(player) = &mut self.player {
            player.load_playlist(files);
        }
    }

    pub fn play(&self) {
        if let Some(player) = &self.player {
            player.play();
        }
    }

    pub fn pause(&self) {
        if let Some(player) = &self.player {
            player.pause();
        }
    }

    pub fn toggle_pause(&self) {
        if let Some(player) = &self.player {
            player.toggle_pause();
        }
    }

    pub fn stop(&self) {
        if let Some(player) = &self.player {
            player.stop();
        }
    }

    pub fn next(&mut self) {
        if let Some(player) = &mut self.player {
            player.next();
        }
    }

    pub fn next_if_not_looping(&mut self) {
        if let Some(player) = &mut self.player {
            if !player.is_loop_file() {
                player.next();
            }
        }
    }

    pub fn prev(&mut self) {
        if let Some(player) = &mut self.player {
            player.prev();
        }
    }

    pub fn shuffle(&mut self) {
        if let Some(player) = &mut self.player {
            player.shuffle();
        }
    }

    pub fn set_volume(&self, volume: i64) {
        if let Some(player) = &self.player {
            player.set_volume(volume);
        }
    }

    pub fn toggle_mute(&self) {
        if let Some(player) = &self.player {
            player.toggle_mute();
        }
    }

    pub fn mute(&self) {
        if let Some(player) = &self.player {
            player.mute();
        }
    }

    pub fn unmute(&self) {
        if let Some(player) = &self.player {
            player.unmute();
        }
    }

    pub fn toggle_loop(&self) {
        if let Some(player) = &self.player {
            player.toggle_loop();
        }
    }

    pub fn is_loop_file(&self) -> bool {
        self.player
            .as_ref()
            .map(|p| p.is_loop_file())
            .unwrap_or(false)
    }

    pub fn frame_step(&self) {
        if let Some(player) = &self.player {
            player.frame_step();
        }
    }

    pub fn frame_back_step(&self) {
        if let Some(player) = &self.player {
            player.frame_back_step();
        }
    }

    pub fn seek_relative(&self, seconds: f64) {
        if let Some(player) = &self.player {
            player.seek(seconds);
        }
    }

    pub fn rotate(&mut self) {
        if let Some(player) = &mut self.player {
            player.rotate();
        }
    }

    pub fn zoom_in(&self) {
        if let Some(player) = &self.player {
            player.zoom_in();
        }
    }

    pub fn zoom_out(&self) {
        if let Some(player) = &self.player {
            player.zoom_out();
        }
    }

    pub fn screenshot(&self) {
        if let Some(player) = &self.player {
            player.screenshot();
        }
    }

    pub fn update_playlist_path(&mut self, old_path: &str, new_path: &str) {
        if let Some(player) = &mut self.player {
            player.update_playlist_path(old_path, new_path);
        }
    }

    pub fn set_selected(&mut self, selected: bool) {
        self.selected = selected;
    }

    pub fn is_selected(&self) -> bool {
        self.selected
    }

    pub fn update(&mut self) {
        if let Some(player) = &mut self.player {
            player.process_events();
            self.state = player.poll_state();
        }
    }

    pub fn state(&self) -> &PlayerState {
        &self.state
    }

    pub fn current_file(&self) -> &str {
        &self.state.path
    }

    pub fn position(&self) -> f64 {
        self.state.position
    }

    pub fn duration(&self) -> f64 {
        self.state.duration
    }

    pub fn is_paused(&self) -> bool {
        self.state.paused
    }

    pub fn cell_id(&self) -> String {
        format!("{},{}", self.row, self.col)
    }
}
