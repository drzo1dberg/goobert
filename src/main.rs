mod app;
mod config;
mod file_scanner;
mod grid_cell;
mod keymap;
mod mpv_player;
mod ui;

use app::GoobertApp;

const VERSION: &str = env!("CARGO_PKG_VERSION");

fn main() -> eframe::Result<()> {
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info")).init();

    log::info!("Goobert {} starting...", VERSION);

    // Parse command line arguments
    let source_dir = std::env::args().nth(1);

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_title(format!("Goobert {}", VERSION))
            .with_inner_size([1500.0, 900.0])
            .with_min_inner_size([800.0, 600.0]),
        ..Default::default()
    };

    eframe::run_native(
        "Goobert",
        options,
        Box::new(|cc| Ok(Box::new(GoobertApp::new(cc, source_dir)))),
    )
}
