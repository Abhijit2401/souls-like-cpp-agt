#include "pch.h"
#include "framebuffer.h"
#include "glad/glad.h"

engine::framebuffer::framebuffer(uint32_t width, uint32_t height, bool with_depth, bool hdr)
	: m_width(width), m_height(height), m_with_depth(with_depth), m_hdr(hdr)
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenTextures(1, &m_colour_texture);
	glBindTexture(GL_TEXTURE_2D, m_colour_texture);
	// RGBA16F for the HDR-ish scene target (bloom needs values that can go above 1.0 to have
	// anything to extract), plain RGBA8 for the smaller extract/blur passes.
	GLenum internal_format = hdr ? GL_RGBA16F : GL_RGBA8;
	GLenum data_type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, data_type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colour_texture, 0);

	if (with_depth)
	{
		glGenRenderbuffers(1, &m_depth_renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth_renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, (GLsizei)width, (GLsizei)height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depth_renderbuffer);
	}

	m_valid = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	if (!m_valid)
		LOG_CORE_ERROR("framebuffer: incomplete ({0}x{1}, depth={2}, hdr={3}) - caller should fall back to normal rendering", width, height, with_depth, hdr);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

engine::framebuffer::~framebuffer()
{
	if (m_depth_renderbuffer) glDeleteRenderbuffers(1, &m_depth_renderbuffer);
	if (m_colour_texture) glDeleteTextures(1, &m_colour_texture);
	if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
}

void engine::framebuffer::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0, (GLsizei)m_width, (GLsizei)m_height);
}

void engine::framebuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

engine::ref<engine::framebuffer> engine::framebuffer::create(uint32_t width, uint32_t height, bool with_depth, bool hdr)
{
	return std::make_shared<framebuffer>(width, height, with_depth, hdr);
}
