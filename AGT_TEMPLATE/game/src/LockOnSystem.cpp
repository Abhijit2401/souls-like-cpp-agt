#include "pch.h"
#include "LockOnSystem.h"
#include "platform/opengl/gl_shader.h"
#include <glm/gtc/matrix_transform.hpp>

void LockOnSystem::initialise()
{
    m_ring_shape = engine::SlashEffect::create(0.85f, 1.05f, 360.0f, 24, 0.03f);
    m_ring_material = engine::material::create(1.0f, glm::vec3(1.0f, 0.25f, 0.2f), glm::vec3(1.0f, 0.25f, 0.2f), glm::vec3(1.0f), 0.0f);
}

engine::ref<engine::game_object> LockOnSystem::find_closest_target_to_screen_center(
    const engine::perspective_camera& camera,
    const SpawnManager& spawn_manager, const Boss& boss_logic,
    const engine::ref<engine::game_object>& boss_object) const
{
    engine::ref<engine::game_object> best;
    float best_dist2 = 1.0e9f;

    auto consider = [&](const engine::ref<engine::game_object>& obj)
    {
        if (!obj) return;
        glm::vec3 pos = obj->position() + glm::vec3(0.f, 1.2f, 0.f);
        glm::vec4 clip = camera.projection_matrix() * camera.view_matrix() * glm::vec4(pos, 1.0f);
        if (clip.w <= 0.0f) return;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < 0.0f || ndc.z > 1.0f) return;
        if (glm::abs(ndc.x) > 1.0f || glm::abs(ndc.y) > 1.0f) return;
        float dist2 = ndc.x * ndc.x + ndc.y * ndc.y;
        if (dist2 < best_dist2)
        {
            best_dist2 = dist2;
            best = obj;
        }
    };

    const auto& enemies = spawn_manager.enemies();
    const auto& warriors = spawn_manager.warrior_objects();
    for (size_t i = 0; i < enemies.size(); i++)
        if (!enemies[i].is_dead()) consider(warriors[i]);

    const auto& priests = spawn_manager.priests();
    const auto& priest_objects = spawn_manager.priest_objects();
    for (size_t i = 0; i < priests.size(); i++)
        if (!priests[i].is_dead()) consider(priest_objects[i]);

    if (!boss_logic.is_dead()) consider(boss_object);

    return best;
}

void LockOnSystem::on_update(const engine::timestep& time_step, bool middle_mouse_down,
    const engine::perspective_camera& camera,
    const SpawnManager& spawn_manager, const Boss& boss_logic,
    const engine::ref<engine::game_object>& boss_object)
{
    if (middle_mouse_down && !m_middle_mouse_was_down)
    {
        if (m_locked_on)
        {
            m_locked_on = false;
            m_locked_target = nullptr;
        }
        else
        {
            m_locked_target = find_closest_target_to_screen_center(camera, spawn_manager, boss_logic, boss_object);
            m_locked_on = (m_locked_target != nullptr);
        }
    }
    m_middle_mouse_was_down = middle_mouse_down;

    if (m_locked_on)
    {
        bool target_alive = false;
        if (m_locked_target == boss_object)
        {
            target_alive = !boss_logic.is_dead();
        }
        else
        {
            const auto& enemies = spawn_manager.enemies();
            const auto& warriors = spawn_manager.warrior_objects();
            for (size_t i = 0; i < warriors.size() && !target_alive; i++)
                if (warriors[i] == m_locked_target) target_alive = !enemies[i].is_dead();

            if (!target_alive)
            {
                const auto& priests = spawn_manager.priests();
                const auto& priest_objects = spawn_manager.priest_objects();
                for (size_t i = 0; i < priest_objects.size() && !target_alive; i++)
                    if (priest_objects[i] == m_locked_target) target_alive = !priests[i].is_dead();
            }
        }

        if (!target_alive)
        {
            m_locked_on = false;
            m_locked_target = nullptr;
        }
        else
        {
            m_ring_spin += (float)time_step * 2.0f;
        }
    }
}

void LockOnSystem::render_reticle(const engine::ref<engine::shader>& mesh_shader, const glm::vec3& camera_position)
{
    if (!m_locked_on || !m_locked_target) return;

    glm::vec3 ring_pos = m_locked_target->position() + glm::vec3(0.0f, 1.3f, 0.0f);
    glm::vec3 to_camera = camera_position - ring_pos;
    if (glm::length(to_camera) < 0.001f) to_camera = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 billboard_normal = glm::normalize(to_camera);
    glm::vec3 world_up(0.f, 1.f, 0.f);
    glm::vec3 billboard_right = glm::normalize(glm::cross(world_up, billboard_normal));
    glm::vec3 billboard_up = glm::cross(billboard_normal, billboard_right);

    glm::mat4 ring_basis(1.0f);
    ring_basis[0] = glm::vec4(billboard_right, 0.0f);
    ring_basis[1] = glm::vec4(billboard_up, 0.0f);
    ring_basis[2] = glm::vec4(billboard_normal, 0.0f);
    ring_basis[3] = glm::vec4(ring_pos, 1.0f);

    glm::mat4 ring_transform = ring_basis * glm::rotate(glm::mat4(1.0f), m_ring_spin, glm::vec3(0.f, 0.f, 1.f));

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
    engine::render_command::disable_culling();
    m_ring_material->set_transparency(0.85f);
    m_ring_material->submit(mesh_shader);
    engine::renderer::submit(mesh_shader, m_ring_shape->mesh(), ring_transform);
    engine::render_command::enable_culling();
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
}
