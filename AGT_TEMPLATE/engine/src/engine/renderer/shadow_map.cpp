#include "pch.h"
#include "shadow_map.h"
#include "glad/glad.h"

engine::shadow_map::shadow_map(uint32_t size)
	: m_size(size)
{
	glGenFramebuffers(1, &m_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

	glGenTextures(1, &m_depth_texture);
	glBindTexture(GL_TEXTURE_2D, m_depth_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)size, (GLsizei)size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// Outside the shadow frustum reads as maximum depth (1.0) - i.e. "nothing there is closer
	// than the far plane", so sampling past the frustum's edge reads as unshadowed instead of
	// producing a hard shadowed border around the covered area.
	float border_colour[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_colour);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depth_texture, 0);

	// No colour attachment at all - tell GL not to expect one.
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	m_valid = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	if (!m_valid)
		LOG_CORE_ERROR("shadow_map: incomplete ({0}x{0}) - shadows will stay off", size);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

engine::shadow_map::~shadow_map()
{
	if (m_depth_texture) glDeleteTextures(1, &m_depth_texture);
	if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
}

void engine::shadow_map::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	glViewport(0, 0, (GLsizei)m_size, (GLsizei)m_size);
}

void engine::shadow_map::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

engine::ref<engine::shadow_map> engine::shadow_map::create(uint32_t size)
{
	return std::make_shared<shadow_map>(size);
}
