use std::collections::HashSet;
use std::path::Path;
use walkdir::WalkDir;

const VIDEO_EXTENSIONS: &[&str] = &[
    "mp4", "mkv", "avi", "mov", "wmv", "flv", "webm", "m4v", "mpeg", "mpg", "3gp", "ts",
];

const IMAGE_EXTENSIONS: &[&str] = &[
    "jpg", "jpeg", "png", "gif", "bmp", "webp", "tiff", "tif",
];

pub struct FileScanner {
    video_exts: HashSet<&'static str>,
    image_exts: HashSet<&'static str>,
}

impl Default for FileScanner {
    fn default() -> Self {
        Self::new()
    }
}

impl FileScanner {
    pub fn new() -> Self {
        Self {
            video_exts: VIDEO_EXTENSIONS.iter().copied().collect(),
            image_exts: IMAGE_EXTENSIONS.iter().copied().collect(),
        }
    }

    pub fn scan<P: AsRef<Path>>(&self, path: P) -> Vec<String> {
        let path = path.as_ref();

        if !path.exists() {
            log::warn!("Path does not exist: {}", path.display());
            return Vec::new();
        }

        WalkDir::new(path)
            .follow_links(true)
            .into_iter()
            .filter_map(Result::ok)
            .filter(|entry| entry.file_type().is_file())
            .filter(|entry| self.is_media_file(entry.path()))
            .map(|entry| entry.path().to_string_lossy().into_owned())
            .collect()
    }

    fn is_media_file(&self, path: &Path) -> bool {
        path.extension()
            .and_then(|ext| ext.to_str())
            .map(|ext| ext.to_lowercase())
            .map(|ext| self.is_video_ext(&ext) || self.is_image_ext(&ext))
            .unwrap_or(false)
    }

    fn is_video_ext(&self, ext: &str) -> bool {
        self.video_exts.contains(ext)
    }

    fn is_image_ext(&self, ext: &str) -> bool {
        self.image_exts.contains(ext)
    }

    pub fn is_image<P: AsRef<Path>>(&self, path: P) -> bool {
        path.as_ref()
            .extension()
            .and_then(|ext| ext.to_str())
            .map(|ext| self.is_image_ext(&ext.to_lowercase()))
            .unwrap_or(false)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_video_extensions() {
        let scanner = FileScanner::new();
        assert!(scanner.is_media_file(Path::new("video.mp4")));
        assert!(scanner.is_media_file(Path::new("video.MKV")));
        assert!(scanner.is_media_file(Path::new("video.WebM")));
    }

    #[test]
    fn test_image_extensions() {
        let scanner = FileScanner::new();
        assert!(scanner.is_media_file(Path::new("image.jpg")));
        assert!(scanner.is_media_file(Path::new("image.PNG")));
        assert!(scanner.is_image(Path::new("image.gif")));
    }

    #[test]
    fn test_non_media_files() {
        let scanner = FileScanner::new();
        assert!(!scanner.is_media_file(Path::new("document.txt")));
        assert!(!scanner.is_media_file(Path::new("script.py")));
    }
}
