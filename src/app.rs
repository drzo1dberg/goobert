use eframe::egui;
use rand::seq::SliceRandom;
use std::sync::Arc;
use std::time::{Duration, Instant};

use crate::config::Config;
use crate::file_scanner::FileScanner;
use crate::grid_cell::GridCell;
use crate::keymap::{Action, KeyMap};
use crate::ui::{ControlPanel, ControlPanelResponse};
use crate::video_renderer::VideoRenderer;

pub struct GoobertApp {
    config: &'static Config,
    keymap: KeyMap,
    control_panel: ControlPanel,
    cells: Vec<GridCell>,
    selected_row: Option<usize>,
    selected_col: Option<usize>,
    is_fullscreen: bool,
    is_tile_fullscreen: bool,
    fullscreen_cell: Option<(usize, usize)>,
    last_update: Instant,
    video_renderer: Option<VideoRenderer>,
    gl: Option<Arc<glow::Context>>,
    render_initialized: bool,
}

impl GoobertApp {
    pub fn new(cc: &eframe::CreationContext<'_>, source_dir: Option<String>) -> Self {
        // Configure dark theme
        let mut style = (*cc.egui_ctx.style()).clone();
        style.visuals = egui::Visuals::dark();
        cc.egui_ctx.set_style(style);

        let config = Config::instance();
        let source = source_dir.unwrap_or_else(|| config.paths.default_media_path.clone());

        // Get the glow context if available
        let gl = cc.gl.clone();

        Self {
            config,
            keymap: KeyMap::new(),
            control_panel: ControlPanel::new(
                source,
                config.grid.default_rows as usize,
                config.grid.default_cols as usize,
                config.playback.default_volume as i64,
            ),
            cells: Vec::new(),
            selected_row: None,
            selected_col: None,
            is_fullscreen: false,
            is_tile_fullscreen: false,
            fullscreen_cell: None,
            last_update: Instant::now(),
            video_renderer: None,
            gl,
            render_initialized: false,
        }
    }

    fn init_video_renderer(&mut self) {
        if self.video_renderer.is_some() {
            return;
        }

        if let Some(gl) = &self.gl {
            self.video_renderer = Some(VideoRenderer::new(gl.clone()));
            log::info!("Video renderer initialized");
        } else {
            log::warn!("No GL context available for video rendering");
        }
    }

    fn start_grid(&mut self) {
        let scanner = FileScanner::new();
        let files = scanner.scan(&self.control_panel.source_dir);

        if files.is_empty() {
            self.control_panel.log("No media files found!");
            return;
        }

        self.control_panel.log(&format!("Found {} files", files.len()));

        // Clear existing cells
        self.stop_grid();

        // Initialize video renderer if not done
        self.init_video_renderer();

        let rows = self.control_panel.rows;
        let cols = self.control_panel.cols;
        let cell_count = rows * cols;

        // Create FBOs for video rendering
        if let Some(renderer) = &mut self.video_renderer {
            // Start with a reasonable default size, will be resized on first render
            renderer.create_fbos(cell_count, 640, 480);
        }

        // Create grid cells
        let mut rng = rand::thread_rng();

        for row in 0..rows {
            for col in 0..cols {
                let mut cell = GridCell::new(row, col);

                if let Err(e) = cell.initialize() {
                    log::error!("Failed to initialize cell [{},{}]: {}", row, col, e);
                    continue;
                }

                // Initialize render context for this cell
                if let Err(e) = cell.init_render_context() {
                    log::error!("Failed to init render context for cell [{},{}]: {}", row, col, e);
                }

                // Shuffle files for this cell
                let mut shuffled = files.clone();
                shuffled.shuffle(&mut rng);
                cell.set_playlist(shuffled);
                cell.set_volume(self.control_panel.volume);
                cell.play();

                self.cells.push(cell);
            }
        }

        self.control_panel.is_running = true;
        self.control_panel.log(&format!("Started {}x{} grid", cols, rows));
        self.render_initialized = true;

        // Auto-select first cell
        if !self.cells.is_empty() {
            self.select_cell(0, 0);
        }
    }

    fn stop_grid(&mut self) {
        for cell in &self.cells {
            cell.stop();
        }
        self.cells.clear();
        self.selected_row = None;
        self.selected_col = None;
        self.control_panel.is_running = false;
        self.control_panel.log("Stopped");
        self.render_initialized = false;

        // Clean up FBOs
        if let Some(renderer) = &mut self.video_renderer {
            renderer.cleanup();
        }
    }

    fn select_cell(&mut self, row: usize, col: usize) {
        // Deselect previous
        if let Some(cell) = self.selected_cell_mut() {
            cell.set_selected(false);
        }

        self.selected_row = Some(row);
        self.selected_col = Some(col);

        // Select new
        if let Some(cell) = self.selected_cell_mut() {
            cell.set_selected(true);
            self.control_panel.selected_path = cell.current_file().to_string();
        }
    }

    fn selected_cell(&self) -> Option<&GridCell> {
        let row = self.selected_row?;
        let col = self.selected_col?;
        self.get_cell(row, col)
    }

    fn selected_cell_mut(&mut self) -> Option<&mut GridCell> {
        let row = self.selected_row?;
        let col = self.selected_col?;
        self.get_cell_mut(row, col)
    }

    fn get_cell(&self, row: usize, col: usize) -> Option<&GridCell> {
        let cols = self.control_panel.cols;
        let index = row * cols + col;
        self.cells.get(index)
    }

    fn get_cell_mut(&mut self, row: usize, col: usize) -> Option<&mut GridCell> {
        let cols = self.control_panel.cols;
        let index = row * cols + col;
        self.cells.get_mut(index)
    }

    fn navigate_selection(&mut self, col_delta: i32, row_delta: i32) {
        if self.cells.is_empty() {
            return;
        }

        let rows = self.control_panel.rows;
        let cols = self.control_panel.cols;

        let (current_row, current_col) = match (self.selected_row, self.selected_col) {
            (Some(r), Some(c)) => (r as i32, c as i32),
            _ => {
                self.select_cell(0, 0);
                return;
            }
        };

        let mut new_row = current_row + row_delta;
        let mut new_col = current_col + col_delta;

        // Wrap around
        if new_col < 0 {
            new_col = cols as i32 - 1;
        } else if new_col >= cols as i32 {
            new_col = 0;
        }

        if new_row < 0 {
            new_row = rows as i32 - 1;
        } else if new_row >= rows as i32 {
            new_row = 0;
        }

        self.select_cell(new_row as usize, new_col as usize);
    }

    fn handle_action(&mut self, action: Action) {
        match action {
            Action::PauseAll => self.play_pause_all(),
            Action::NextAll => self.next_all_if_not_looping(),
            Action::PrevAll => self.prev_all(),
            Action::ShuffleAll => self.shuffle_all(),
            Action::ShuffleThenNextAll => {
                self.shuffle_all();
                self.next_all_if_not_looping();
            }
            Action::FullscreenGlobal => self.toggle_fullscreen(),
            Action::ExitFullscreen => self.exit_fullscreen(),
            Action::VolumeUp => self.volume_up(),
            Action::VolumeDown => self.volume_down(),
            Action::MuteAll => self.mute_all(),
            Action::NavigateUp => self.navigate_selection(0, -1),
            Action::NavigateDown => self.navigate_selection(0, 1),
            Action::NavigateLeft => self.navigate_selection(-1, 0),
            Action::NavigateRight => self.navigate_selection(1, 0),
            Action::FullscreenSelected => self.toggle_tile_fullscreen(),
            Action::SeekForward => self.seek_selected(5.0),
            Action::SeekBackward => self.seek_selected(-5.0),
            Action::FrameStepForward => {
                if let Some(cell) = self.selected_cell() {
                    cell.frame_step();
                }
            }
            Action::FrameStepBackward => {
                if let Some(cell) = self.selected_cell() {
                    cell.frame_back_step();
                }
            }
            Action::ToggleLoop => {
                if let Some(cell) = self.selected_cell() {
                    cell.toggle_loop();
                }
            }
            Action::ZoomIn => {
                if let Some(cell) = self.selected_cell() {
                    cell.zoom_in();
                }
            }
            Action::ZoomOut => {
                if let Some(cell) = self.selected_cell() {
                    cell.zoom_out();
                }
            }
            Action::Rotate => {
                if let Some(cell) = self.selected_cell_mut() {
                    cell.rotate();
                }
            }
            Action::Screenshot => {
                if let Some(cell) = self.selected_cell() {
                    cell.screenshot();
                    self.control_panel.log("Screenshot taken");
                }
            }
        }
    }

    fn play_pause_all(&mut self) {
        for cell in &self.cells {
            cell.toggle_pause();
        }
    }

    fn next_all_if_not_looping(&mut self) {
        for cell in &mut self.cells {
            cell.next_if_not_looping();
        }
    }

    fn prev_all(&mut self) {
        for cell in &mut self.cells {
            cell.prev();
        }
    }

    fn shuffle_all(&mut self) {
        for cell in &mut self.cells {
            cell.shuffle();
        }
    }

    fn mute_all(&mut self) {
        for cell in &self.cells {
            cell.toggle_mute();
        }
    }

    fn volume_up(&mut self) {
        self.control_panel.volume = (self.control_panel.volume + 5).min(100);
        self.set_volume_all(self.control_panel.volume);
        self.control_panel.log(&format!("Volume: {}%", self.control_panel.volume));
    }

    fn volume_down(&mut self) {
        self.control_panel.volume = (self.control_panel.volume - 5).max(0);
        self.set_volume_all(self.control_panel.volume);
        self.control_panel.log(&format!("Volume: {}%", self.control_panel.volume));
    }

    fn set_volume_all(&self, volume: i64) {
        for cell in &self.cells {
            cell.set_volume(volume);
        }
    }

    fn seek_selected(&self, seconds: f64) {
        if let Some(cell) = self.selected_cell() {
            cell.seek_relative(seconds);
        }
    }

    fn toggle_fullscreen(&mut self) {
        self.is_fullscreen = !self.is_fullscreen;
    }

    fn exit_fullscreen(&mut self) {
        if self.is_tile_fullscreen {
            self.is_tile_fullscreen = false;
            self.fullscreen_cell = None;
        }
        self.is_fullscreen = false;
    }

    fn toggle_tile_fullscreen(&mut self) {
        if self.is_tile_fullscreen {
            self.is_tile_fullscreen = false;
            self.fullscreen_cell = None;
        } else if let (Some(row), Some(col)) = (self.selected_row, self.selected_col) {
            self.is_tile_fullscreen = true;
            self.fullscreen_cell = Some((row, col));
            self.is_fullscreen = true;
        }
    }

    fn handle_control_panel_response(&mut self, response: ControlPanelResponse) {
        if response.start {
            self.start_grid();
        }
        if response.stop {
            self.stop_grid();
        }
        if response.fullscreen {
            self.toggle_fullscreen();
        }
        if response.play_pause {
            self.play_pause_all();
        }
        if response.next {
            self.next_all_if_not_looping();
        }
        if response.prev {
            self.prev_all();
        }
        if response.shuffle {
            self.shuffle_all();
        }
        if response.mute {
            self.mute_all();
        }
        if let Some(volume) = response.volume_changed {
            self.set_volume_all(volume);
        }
    }

    fn update_cells(&mut self) {
        for cell in &mut self.cells {
            cell.update();
        }
    }

    fn render_videos(&mut self, ctx: &egui::Context) {
        if !self.render_initialized || self.cells.is_empty() {
            return;
        }

        let renderer = match &mut self.video_renderer {
            Some(r) => r,
            None => return,
        };

        // Render each cell's video to its FBO and update egui texture
        for index in 0..self.cells.len() {
            let fbo_id = renderer.get_fbo_id(index);
            let fbo_size = renderer.get_fbo(index).map(|f| (f.width, f.height));

            if let (Some(fbo_id), Some((width, height))) = (fbo_id, fbo_size) {
                // Render MPV frame to the FBO
                if let Some(cell) = self.cells.get_mut(index) {
                    if cell.render(fbo_id, width as i32, height as i32) {
                        cell.report_swap();
                    }
                }

                // Update egui texture from FBO
                renderer.update_egui_texture(index, ctx);
            }
        }

        // Restore default framebuffer
        renderer.bind_default_fbo();
    }
}

impl eframe::App for GoobertApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // Update cells periodically
        let now = Instant::now();
        if now.duration_since(self.last_update) > Duration::from_millis(16) {
            self.update_cells();
            self.render_videos(ctx);
            self.last_update = now;
        }

        // Handle keyboard input
        ctx.input(|i| {
            for event in &i.events {
                if let egui::Event::Key { key, pressed: true, modifiers, .. } = event {
                    if let Some(action) = self.keymap.get_action(*key, *modifiers) {
                        self.handle_action(action);
                    }
                }
            }
        });

        // Handle fullscreen
        ctx.send_viewport_cmd(egui::ViewportCommand::Fullscreen(self.is_fullscreen));

        // Main UI
        if !self.is_fullscreen || !self.is_tile_fullscreen {
            egui::TopBottomPanel::bottom("control_panel")
                .resizable(true)
                .default_height(150.0)
                .show(ctx, |ui| {
                    let response = self.control_panel.ui(ui);
                    self.handle_control_panel_response(response);

                    ui.separator();
                    self.control_panel.cell_table(ui, &self.cells);
                });
        }

        // Video wall area
        egui::CentralPanel::default()
            .frame(egui::Frame::none().fill(egui::Color32::from_rgb(10, 10, 10)))
            .show(ctx, |ui| {
                if self.cells.is_empty() {
                    ui.centered_and_justified(|ui| {
                        ui.label(
                            egui::RichText::new("Configure grid and click Start")
                                .size(24.0)
                                .color(egui::Color32::GRAY),
                        );
                    });
                } else {
                    self.render_grid(ui);
                }
            });

        // Request continuous updates for video playback
        ctx.request_repaint();
    }
}

impl GoobertApp {
    fn render_grid(&mut self, ui: &mut egui::Ui) {
        let rows = self.control_panel.rows;
        let cols = self.control_panel.cols;

        let available = ui.available_size();
        let cell_width = available.x / cols as f32;
        let cell_height = available.y / rows as f32;

        egui::Grid::new("video_grid")
            .num_columns(cols)
            .spacing([2.0, 2.0])
            .show(ui, |ui| {
                for row in 0..rows {
                    for col in 0..cols {
                        let cell_size = egui::vec2(cell_width - 4.0, cell_height - 4.0);

                        if self.is_tile_fullscreen {
                            if let Some((fr, fc)) = self.fullscreen_cell {
                                if row != fr || col != fc {
                                    continue;
                                }
                            }
                        }

                        let (rect, response) = ui.allocate_exact_size(
                            cell_size,
                            egui::Sense::click(),
                        );

                        // Draw cell background
                        let is_selected = self.selected_row == Some(row)
                            && self.selected_col == Some(col);

                        let bg_color = if is_selected {
                            egui::Color32::from_rgb(30, 50, 80)
                        } else {
                            egui::Color32::from_rgb(20, 20, 20)
                        };

                        ui.painter().rect_filled(rect, 0.0, bg_color);

                        // Draw video frame if available
                        let cell_index = row * cols + col;
                        if let Some(renderer) = &self.video_renderer {
                            if let Some(texture_id) = renderer.get_texture_id(cell_index) {
                                ui.painter().image(
                                    texture_id,
                                    rect,
                                    egui::Rect::from_min_max(
                                        egui::pos2(0.0, 0.0),
                                        egui::pos2(1.0, 1.0),
                                    ),
                                    egui::Color32::WHITE,
                                );
                            }
                        }

                        // Draw cell info
                        if let Some(cell) = self.get_cell(row, col) {
                            let state = cell.state();
                            let filename = std::path::Path::new(&state.path)
                                .file_name()
                                .map(|n| n.to_string_lossy().to_string())
                                .unwrap_or_else(|| "No file".to_string());

                            // Truncate filename
                            let display_name = if filename.len() > 30 {
                                format!("{}...", &filename[..27])
                            } else {
                                filename
                            };

                            // Draw semi-transparent overlay for text
                            let text_bg = egui::Rect::from_min_size(
                                rect.min,
                                egui::vec2(rect.width(), 20.0),
                            );
                            ui.painter().rect_filled(
                                text_bg,
                                0.0,
                                egui::Color32::from_rgba_unmultiplied(0, 0, 0, 180),
                            );

                            ui.painter().text(
                                rect.min + egui::vec2(5.0, 5.0),
                                egui::Align2::LEFT_TOP,
                                &display_name,
                                egui::FontId::proportional(12.0),
                                egui::Color32::WHITE,
                            );

                            // Show status
                            let status = if state.paused { "⏸" } else { "▶" };
                            ui.painter().text(
                                rect.max - egui::vec2(5.0, 5.0),
                                egui::Align2::RIGHT_BOTTOM,
                                status,
                                egui::FontId::proportional(14.0),
                                egui::Color32::WHITE,
                            );
                        }

                        // Handle clicks
                        if response.clicked() {
                            self.select_cell(row, col);
                        }

                        if response.double_clicked() {
                            self.select_cell(row, col);
                            self.toggle_tile_fullscreen();
                        }

                        if response.secondary_clicked() {
                            if let Some(cell) = self.get_cell(row, col) {
                                cell.toggle_pause();
                            }
                        }

                        if response.middle_clicked() {
                            if let Some(cell) = self.get_cell(row, col) {
                                cell.toggle_loop();
                            }
                        }
                    }
                    ui.end_row();
                }
            });
    }
}
