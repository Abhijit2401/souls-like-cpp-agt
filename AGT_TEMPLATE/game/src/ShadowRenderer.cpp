#include "pch.h"
#include "ShadowRenderer.h"
#include "platform/opengl/gl_shader.h"
#include <glm/gtc/matrix_transform.hpp>

void ShadowRenderer::initialise(uint32_t resolution)
{
    m_shadow_map = engine::shadow_map::create(resolution);
    if (!m_shadow_map->is_valid())
        LOG_CORE_ERROR("ShadowRenderer: shadow map failed to initialise - shadows will stay off");
}

void ShadowRenderer::render(const glm::vec3& light_direction, const glm::vec3& focus_position,
    const std::function<void(const engine::ref<engine::shader>&)>& draw_casters)
{
    if (!is_available()) return;

    glm::vec3 dir = glm::normalize(light_direction);
    glm::vec3 light_pos = focus_position - dir * m_distance_back;
    glm::vec3 up(0.f, 1.f, 0.f);
    glm::mat4 light_view = glm::lookAt(light_pos, focus_position, up);
    glm::mat4 light_projection = glm::ortho(-m_frustum_half_size, m_frustum_half_size,
        -m_frustum_half_size, m_frustum_half_size, 1.0f, m_distance_back + m_frustum_half_size);
    m_light_space_matrix = light_projection * light_view;

    auto shadow_shader = engine::renderer::shaders_library()->get("shadow_depth");

    engine::render_command::disable_culling();

    m_shadow_map->bind();
    engine::render_command::clear_depth();

    auto gl_shadow_shader = std::static_pointer_cast<engine::gl_shader>(shadow_shader);
    gl_shadow_shader->bind();
    gl_shadow_shader->set_uniform("u_view_projection", m_light_space_matrix);

    draw_casters(shadow_shader);

    engine::shadow_map::unbind();
    engine::render_command::enable_culling();
    engine::render_command::resize_viewport(0, 0, engine::application::window().width(), engine::application::window().height());
}

void ShadowRenderer::bind_for_sampling(const engine::ref<engine::shader>& mesh_shader, uint32_t texture_slot)
{
    auto gl_mesh_shader = std::static_pointer_cast<engine::gl_shader>(mesh_shader);

    if (!is_available())
    {
        gl_mesh_shader->set_uniform("shadows_on", false);
        return;
    }

    engine::render_command::bind_texture(m_shadow_map->depth_texture(), texture_slot);
    gl_mesh_shader->set_uniform("shadow_map", (int)texture_slot);
    gl_mesh_shader->set_uniform("u_light_view_projection", m_light_space_matrix);
    gl_mesh_shader->set_uniform("shadows_on", true);
}
