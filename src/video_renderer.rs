use egui::{ColorImage, TextureHandle, TextureOptions};
use glow::HasContext;
use std::sync::Arc;

/// Manages video frame buffers for rendering MPV output
pub struct VideoRenderer {
    gl: Arc<glow::Context>,
    fbos: Vec<FrameBuffer>,
    textures: Vec<Option<TextureHandle>>,
    pixel_buffer: Vec<u8>,
}

/// A framebuffer with associated texture for video rendering
pub struct FrameBuffer {
    pub fbo: glow::Framebuffer,
    pub texture: glow::Texture,
    pub width: u32,
    pub height: u32,
}

impl VideoRenderer {
    pub fn new(gl: Arc<glow::Context>) -> Self {
        Self {
            gl,
            fbos: Vec::new(),
            textures: Vec::new(),
            pixel_buffer: Vec::new(),
        }
    }

    /// Create FBOs for a given number of cells
    pub fn create_fbos(&mut self, count: usize, width: u32, height: u32) {
        // Clean up existing FBOs
        self.cleanup();

        for _ in 0..count {
            if let Some(fb) = self.create_fbo(width, height) {
                self.fbos.push(fb);
                self.textures.push(None);
            }
        }

        // Allocate pixel buffer for reading
        self.pixel_buffer = vec![0u8; (width * height * 4) as usize];

        log::info!("Created {} FBOs at {}x{}", self.fbos.len(), width, height);
    }

    fn create_fbo(&self, width: u32, height: u32) -> Option<FrameBuffer> {
        unsafe {
            // Create texture
            let texture = self.gl.create_texture().ok()?;
            self.gl.bind_texture(glow::TEXTURE_2D, Some(texture));
            self.gl.tex_image_2d(
                glow::TEXTURE_2D,
                0,
                glow::RGBA8 as i32,
                width as i32,
                height as i32,
                0,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                None,
            );
            self.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MIN_FILTER,
                glow::LINEAR as i32,
            );
            self.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_MAG_FILTER,
                glow::LINEAR as i32,
            );
            self.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_WRAP_S,
                glow::CLAMP_TO_EDGE as i32,
            );
            self.gl.tex_parameter_i32(
                glow::TEXTURE_2D,
                glow::TEXTURE_WRAP_T,
                glow::CLAMP_TO_EDGE as i32,
            );

            // Create FBO
            let fbo = self.gl.create_framebuffer().ok()?;
            self.gl.bind_framebuffer(glow::FRAMEBUFFER, Some(fbo));
            self.gl.framebuffer_texture_2d(
                glow::FRAMEBUFFER,
                glow::COLOR_ATTACHMENT0,
                glow::TEXTURE_2D,
                Some(texture),
                0,
            );

            // Check FBO completeness
            let status = self.gl.check_framebuffer_status(glow::FRAMEBUFFER);
            if status != glow::FRAMEBUFFER_COMPLETE {
                log::error!("FBO incomplete: {}", status);
                self.gl.delete_framebuffer(fbo);
                self.gl.delete_texture(texture);
                return None;
            }

            // Clear the FBO to dark gray initially
            self.gl.clear_color(0.1, 0.1, 0.1, 1.0);
            self.gl.clear(glow::COLOR_BUFFER_BIT);

            // Unbind
            self.gl.bind_framebuffer(glow::FRAMEBUFFER, None);
            self.gl.bind_texture(glow::TEXTURE_2D, None);

            Some(FrameBuffer {
                fbo,
                texture,
                width,
                height,
            })
        }
    }

    /// Get FBO info for a cell index
    pub fn get_fbo(&self, index: usize) -> Option<&FrameBuffer> {
        self.fbos.get(index)
    }

    /// Get the OpenGL FBO ID for a cell (for MPV to render to)
    pub fn get_fbo_id(&self, index: usize) -> Option<i32> {
        self.fbos.get(index).map(|fb| fb.fbo.0.get() as i32)
    }

    /// Read pixels from FBO and update egui texture
    pub fn update_egui_texture(&mut self, index: usize, ctx: &egui::Context) {
        let fbo = match self.fbos.get(index) {
            Some(fb) => fb,
            None => return,
        };

        let width = fbo.width as usize;
        let height = fbo.height as usize;

        // Read pixels from FBO
        unsafe {
            self.gl.bind_framebuffer(glow::READ_FRAMEBUFFER, Some(fbo.fbo));
            self.gl.read_pixels(
                0, 0,
                width as i32, height as i32,
                glow::RGBA,
                glow::UNSIGNED_BYTE,
                glow::PixelPackData::Slice(&mut self.pixel_buffer),
            );
            self.gl.bind_framebuffer(glow::READ_FRAMEBUFFER, None);
        }

        // Flip the image vertically (OpenGL has origin at bottom-left)
        let row_size = width * 4;
        let mut flipped = vec![0u8; self.pixel_buffer.len()];
        for y in 0..height {
            let src_start = y * row_size;
            let dst_start = (height - 1 - y) * row_size;
            flipped[dst_start..dst_start + row_size]
                .copy_from_slice(&self.pixel_buffer[src_start..src_start + row_size]);
        }

        // Create egui image
        let image = ColorImage::from_rgba_unmultiplied([width, height], &flipped);

        // Update or create texture
        if let Some(Some(handle)) = self.textures.get_mut(index) {
            handle.set(image, TextureOptions::LINEAR);
        } else if index < self.textures.len() {
            let handle = ctx.load_texture(
                format!("video_{}", index),
                image,
                TextureOptions::LINEAR,
            );
            self.textures[index] = Some(handle);
        }
    }

    /// Get egui texture ID for a cell
    pub fn get_texture_id(&self, index: usize) -> Option<egui::TextureId> {
        self.textures.get(index)?.as_ref().map(|h| h.id())
    }

    /// Clean up all FBOs
    pub fn cleanup(&mut self) {
        unsafe {
            for fb in &self.fbos {
                self.gl.delete_framebuffer(fb.fbo);
                self.gl.delete_texture(fb.texture);
            }
        }
        self.fbos.clear();
        self.textures.clear();
    }

    /// Bind the default framebuffer
    pub fn bind_default_fbo(&self) {
        unsafe {
            self.gl.bind_framebuffer(glow::FRAMEBUFFER, None);
        }
    }

    pub fn fbo_count(&self) -> usize {
        self.fbos.len()
    }
}

impl Drop for VideoRenderer {
    fn drop(&mut self) {
        self.cleanup();
    }
}
