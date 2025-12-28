use egui::{Ui, RichText, Color32};
use std::path::Path;

use crate::grid_cell::GridCell;

pub struct ControlPanel {
    pub source_dir: String,
    pub rows: usize,
    pub cols: usize,
    pub volume: i64,
    pub is_running: bool,
    pub selected_path: String,
    pub log_message: String,
}

impl ControlPanel {
    pub fn new(source_dir: String, rows: usize, cols: usize, volume: i64) -> Self {
        Self {
            source_dir,
            rows,
            cols,
            volume,
            is_running: false,
            selected_path: String::new(),
            log_message: "Ready".to_string(),
        }
    }

    pub fn log(&mut self, message: &str) {
        let timestamp = chrono::Local::now().format("%H:%M:%S");
        self.log_message = format!("[{}] {}", timestamp, message);
    }

    pub fn ui(&mut self, ui: &mut Ui) -> ControlPanelResponse {
        let mut response = ControlPanelResponse::default();

        ui.horizontal(|ui| {
            // Grid configuration
            ui.label("Grid");
            ui.add(egui::DragValue::new(&mut self.cols).range(1..=10).prefix(""));
            ui.label("×");
            ui.add(egui::DragValue::new(&mut self.rows).range(1..=10).prefix(""));

            ui.add_space(12.0);

            // Source directory
            ui.label("Source");
            let source_response = ui.add(
                egui::TextEdit::singleline(&mut self.source_dir)
                    .desired_width(300.0)
            );

            if ui.button("...").clicked() {
                if let Some(path) = rfd::FileDialog::new()
                    .set_directory(&self.source_dir)
                    .pick_folder()
                {
                    self.source_dir = path.to_string_lossy().to_string();
                }
            }

            ui.add_space(16.0);

            // Start/Stop buttons
            ui.add_enabled_ui(!self.is_running, |ui| {
                if ui.button("▶ Start").clicked() {
                    response.start = true;
                }
            });

            ui.add_enabled_ui(self.is_running, |ui| {
                if ui.button("■ Stop").clicked() {
                    response.stop = true;
                }
            });

            if ui.button("⛶ Fullscreen").clicked() {
                response.fullscreen = true;
            }

            ui.add_space(16.0);

            // Playback controls
            if ui.button("⏮").clicked() {
                response.prev = true;
            }
            if ui.button("⏯").clicked() {
                response.play_pause = true;
            }
            if ui.button("⏭").clicked() {
                response.next = true;
            }
            if ui.button("🔀").clicked() {
                response.shuffle = true;
            }
            if ui.button("🔇").clicked() {
                response.mute = true;
            }

            // Volume slider
            ui.label("Vol");
            if ui.add(egui::Slider::new(&mut self.volume, 0..=100).show_value(false)).changed() {
                response.volume_changed = Some(self.volume);
            }
        });

        // Status bar
        ui.horizontal(|ui| {
            ui.label(RichText::new(&self.selected_path).color(Color32::GRAY));
            ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                ui.label(RichText::new(&self.log_message).color(Color32::DARK_GRAY));
            });
        });

        response
    }

    pub fn cell_table(&self, ui: &mut Ui, cells: &[GridCell]) {
        egui::ScrollArea::vertical()
            .max_height(150.0)
            .show(ui, |ui| {
                egui::Grid::new("cell_status_grid")
                    .num_columns(3)
                    .striped(true)
                    .show(ui, |ui| {
                        ui.label(RichText::new("Cell").strong());
                        ui.label(RichText::new("Status").strong());
                        ui.label(RichText::new("File").strong());
                        ui.end_row();

                        for cell in cells {
                            let state = cell.state();
                            let status = if state.paused { "PAUSE" } else { "PLAY " };
                            let pos = format_time(state.position);
                            let dur = format_time(state.duration);

                            let filename = Path::new(&state.path)
                                .file_name()
                                .map(|n| n.to_string_lossy().to_string())
                                .unwrap_or_default();

                            let cell_text = if cell.is_selected() {
                                RichText::new(cell.cell_id()).strong().color(Color32::LIGHT_BLUE)
                            } else {
                                RichText::new(cell.cell_id())
                            };

                            ui.label(cell_text);
                            ui.label(format!("{} {}/{}", status, pos, dur));
                            ui.label(&filename);
                            ui.end_row();
                        }
                    });
            });
    }
}

#[derive(Default)]
pub struct ControlPanelResponse {
    pub start: bool,
    pub stop: bool,
    pub fullscreen: bool,
    pub play_pause: bool,
    pub next: bool,
    pub prev: bool,
    pub shuffle: bool,
    pub mute: bool,
    pub volume_changed: Option<i64>,
}

fn format_time(seconds: f64) -> String {
    if seconds < 0.0 {
        return "--:--".to_string();
    }

    let total_secs = seconds as u64;
    let hours = total_secs / 3600;
    let minutes = (total_secs % 3600) / 60;
    let secs = total_secs % 60;

    if hours > 0 {
        format!("{}:{:02}:{:02}", hours, minutes, secs)
    } else {
        format!("{:02}:{:02}", minutes, secs)
    }
}
