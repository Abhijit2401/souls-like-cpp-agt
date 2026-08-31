#pragma once
#include "engine/core.h"

namespace engine
{
	/// \brief A minimal offscreen render target: one colour texture (optionally HDR-ish
	/// RGBA16F, for bloom's bright-pass to have headroom above 1.0) plus an optional
	/// depth/stencil renderbuffer. Not behind an abstract interface - this engine only ever
	/// targets OpenGL (see bullet_manager, text_manager for the same pragmatic precedent of
	/// calling GL directly rather than adding an unused abstraction layer).
	class framebuffer
	{
	public:
		framebuffer(uint32_t width, uint32_t height, bool with_depth, bool hdr);
		~framebuffer();

		framebuffer(const framebuffer&) = delete;
		framebuffer& operator=(const framebuffer&) = delete;

		/// \brief Binds this framebuffer and sets the viewport to its size.
		void bind() const;
		/// \brief Binds the default framebuffer (the window) - does NOT restore any previous viewport.
		static void unbind();

		uint32_t colour_texture() const { return m_colour_texture; }
		uint32_t width() const { return m_width; }
		uint32_t height() const { return m_height; }
		/// \brief True if framebuffer creation succeeded - callers should skip using this
		/// framebuffer (and fall back to normal rendering) if false rather than assume it works.
		bool is_valid() const { return m_valid; }

		static ref<framebuffer> create(uint32_t width, uint32_t height, bool with_depth, bool hdr);

	private:
		uint32_t m_fbo = 0;
		uint32_t m_colour_texture = 0;
		uint32_t m_depth_renderbuffer = 0;
		uint32_t m_width;
		uint32_t m_height;
		bool m_with_depth;
		bool m_hdr;
		bool m_valid = false;
	};
}
