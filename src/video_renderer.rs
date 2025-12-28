use glow::HasContext;
use std::sync::Arc;

/// Manages video frame buffers for rendering MPV output to egui textures
pub struct VideoRenderer {
    gl: Arc<glow::Context>,
    fbos: Vec<FrameBuffer>,
}

/// A framebuffer with associated texture for video rendering
pub struct FrameBuffer {
    pub fbo: glow::Framebuffer,
    pub texture: glow::Texture,
    pub texture_id: egui::TextureId,
    pub width: u32,
    pub height: u32,
}

impl VideoRenderer {
    pub fn new(gl: Arc<glow::Context>) -> Self {
        Self {
            gl,
            fbos: Vec::new(),
        }
    }

    /// Create FBOs for a given number of cells
    pub fn create_fbos(&mut self, count: usize, width: u32, height: u32) {
        // Clean up existing FBOs
        self.cleanup();

        for _ in 0..count {
            if let Some(fb) = self.create_fbo(width, height) {
                self.fbos.push(fb);
            }
        }

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

            // Unbind
            self.gl.bind_framebuffer(glow::FRAMEBUFFER, None);
            self.gl.bind_texture(glow::TEXTURE_2D, None);

            // Get the native texture handle for egui
            let native_texture = glow::NativeTexture(texture.0);
            let texture_id = egui::TextureId::User(native_texture.0.get() as u64);

            Some(FrameBuffer {
                fbo,
                texture,
                texture_id,
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

    /// Get the egui texture ID for a cell
    pub fn get_texture_id(&self, index: usize) -> Option<egui::TextureId> {
        self.fbos.get(index).map(|fb| fb.texture_id)
    }

    /// Resize FBOs to a new size
    pub fn resize(&mut self, width: u32, height: u32) {
        let count = self.fbos.len();
        if count > 0 {
            let first = &self.fbos[0];
            if first.width != width || first.height != height {
                self.create_fbos(count, width, height);
            }
        }
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
