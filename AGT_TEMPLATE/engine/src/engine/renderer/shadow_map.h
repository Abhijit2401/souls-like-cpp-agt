#pragma once
#include "engine/core.h"

namespace engine
{
	/// \brief A depth-only render target for shadow mapping: one samplable depth texture, no
	/// colour attachment at all (a shadow pass only ever needs depth). Kept as its own minimal
	/// class rather than folded into engine::framebuffer, which always assumes a colour
	/// attachment - a shadow map's requirements are different enough that reusing it would mean
	/// threading a "sometimes no colour" case through code that doesn't otherwise need it.
	class shadow_map
	{
	public:
		shadow_map(uint32_t size);
		~shadow_map();

		shadow_map(const shadow_map&) = delete;
		shadow_map& operator=(const shadow_map&) = delete;

		/// \brief Binds this framebuffer and sets the viewport to size x size.
		void bind() const;
		/// \brief Binds the default framebuffer (the window) - does NOT restore any previous viewport.
		static void unbind();

		uint32_t depth_texture() const { return m_depth_texture; }
		uint32_t size() const { return m_size; }
		/// \brief True if creation succeeded - callers should skip using this (and leave shadows
		/// off) if false rather than assume it works.
		bool is_valid() const { return m_valid; }

		static ref<shadow_map> create(uint32_t size);

	private:
		uint32_t m_fbo = 0;
		uint32_t m_depth_texture = 0;
		uint32_t m_size;
		bool m_valid = false;
	};
}
