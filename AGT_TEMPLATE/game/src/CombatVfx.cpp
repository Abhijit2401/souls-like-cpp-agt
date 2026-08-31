#include "pch.h"
#include "CombatVfx.h"
#include "platform/opengl/gl_shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

void CombatVfx::initialise()
{

    std::vector<engine::mesh::vertex> particle_quad_vertices
    {
        { {-0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f} },
        { { 0.5f, -0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f} },
        { { 0.5f,  0.5f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f} },
        { {-0.5f,  0.5f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f} },
    };
    const std::vector<uint32_t> particle_quad_indices{ 0, 1, 2, 0, 2, 3 };
    m_particle_quad = engine::mesh::create(particle_quad_vertices, particle_quad_indices);
    m_material = engine::material::create(1.0f, glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), 0.0f);

    const int num_particles = 46;
    for (int i = 0; i < num_particles; i++)
    {
        ParticleSeed seed;
        seed.t = (float)i / (float)(num_particles - 1);
        seed.jitter_perp = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.16f;
        seed.jitter_along = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.05f;
        seed.size = 0.045f + ((float)rand() / (float)RAND_MAX) * 0.05f;
        m_particles.push_back(seed);
    }
    m_aoe_ring_shape = engine::SlashEffect::create(0.85f, 1.05f, 360.0f, 24, 0.03f);
}

void CombatVfx::render_billboard_particle(const engine::ref<engine::shader>& mesh_shader,
    const glm::vec3& world_position, float size, const glm::vec3& colour, float alpha)
{
    glm::vec3 world_up(0.f, 1.f, 0.f);
    glm::vec3 to_camera = m_camera_position - world_position;
    if (glm::length(to_camera) < 0.001f) to_camera = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 billboard_normal = glm::normalize(to_camera);
    glm::vec3 billboard_right = glm::normalize(glm::cross(world_up, billboard_normal));
    glm::vec3 billboard_up = glm::cross(billboard_normal, billboard_right);

    glm::mat4 particle_transform(1.0f);
    particle_transform[0] = glm::vec4(billboard_right * size, 0.0f);
    particle_transform[1] = glm::vec4(billboard_up * size, 0.0f);
    particle_transform[2] = glm::vec4(billboard_normal, 0.0f);
    particle_transform[3] = glm::vec4(world_position, 1.0f);

    m_material->set_ambient(colour);
    m_material->set_transparency(alpha);
    m_material->submit(mesh_shader);
    engine::renderer::submit(mesh_shader, m_particle_quad, particle_transform);
}

void CombatVfx::render_particle_sweep(const engine::ref<engine::shader>& mesh_shader,
    const glm::vec3& start_point, const glm::vec3& end_point,
    float sweep_progress, const glm::vec3& colour, float alpha_scale)
{
    glm::vec3 line_vec = end_point - start_point;
    if (glm::length(line_vec) < 0.001f) return;

    glm::vec3 world_up(0.f, 1.f, 0.f);
    glm::vec3 perp_dir = glm::cross(glm::normalize(line_vec), world_up);
    if (glm::length(perp_dir) < 0.01f) perp_dir = glm::vec3(1.f, 0.f, 0.f);
    perp_dir = glm::normalize(perp_dir);

    const float lead_in = 0.06f;  // fraction of the sweep spent fading a particle in just ahead of the sweep point
    const float trail_out = 0.4f; // fraction of the sweep spent fading a particle out behind it

    for (const auto& seed : m_particles)
    {
        float dist_from_sweep = sweep_progress - seed.t;
        if (dist_from_sweep <= -lead_in || dist_from_sweep >= trail_out) continue;

        float alpha = (dist_from_sweep < 0.0f)
            ? (dist_from_sweep + lead_in) / lead_in
            : 1.0f - (dist_from_sweep / trail_out);
        if (alpha <= 0.0f) continue;

        float t = glm::clamp(seed.t + seed.jitter_along, 0.0f, 1.0f);
        glm::vec3 particle_pos = glm::mix(start_point, end_point, t) + perp_dir * seed.jitter_perp;

        render_billboard_particle(mesh_shader, particle_pos, seed.size, colour, alpha * alpha_scale);
    }
}

void CombatVfx::render_attacker_line_vfx(const engine::ref<engine::shader>& mesh_shader,
    const engine::ref<engine::game_object>& attacker, float progress,
    const glm::vec3& colour, float alpha_scale, float reach, float height, float half_length,
    float swing_angle_degrees)
{
    if (!attacker) return;

    glm::vec3 forward = attacker->forward();
    if (glm::length(forward) < 0.001f) forward = glm::vec3(0.f, 0.f, -1.f);
    forward = glm::normalize(forward);

    glm::vec3 world_up(0.f, 1.f, 0.f);
    glm::vec3 right = glm::normalize(glm::cross(world_up, forward));
    glm::vec3 up = glm::cross(forward, right);

    glm::vec3 centre = attacker->position() + forward * reach + glm::vec3(0.f, height, 0.f);
    float angle_radians = glm::radians(swing_angle_degrees);
    glm::vec3 top_corner_dir = glm::normalize(up * cos(angle_radians) + right * sin(angle_radians));
    glm::vec3 start_point = centre + top_corner_dir * half_length;
    glm::vec3 end_point = centre - top_corner_dir * half_length;

    render_particle_sweep(mesh_shader, start_point, end_point, progress, colour, alpha_scale);
}

void CombatVfx::render_firebolt(const engine::ref<engine::shader>& mesh_shader, const Firebolt& bolt)
{
    const glm::vec3 fire_colour(1.0f, 0.5f, 0.1f);
    const int trail_count = 5;
    for (int i = 0; i <= trail_count; i++)
    {
        float t = (float)i / (float)trail_count;
        glm::vec3 particle_pos = bolt.position() - bolt.direction() * (t * 0.7f);
        float size = glm::mix(0.16f, 0.05f, t);
        float alpha = glm::mix(0.95f, 0.0f, t);
        render_billboard_particle(mesh_shader, particle_pos, size, fire_colour, alpha);
    }
}

void CombatVfx::render(const engine::ref<engine::shader>& mesh_shader,
    const glm::vec3& camera_position,
    const player& player,
    const SpawnManager& spawn_manager,
    const Boss& boss_logic,
    const engine::ref<engine::game_object>& boss_object,
    const std::vector<Firebolt>& firebolts)
{
    m_camera_position = camera_position;

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
    engine::render_command::disable_culling();

    if (player.is_attacking())
    {
        float progress = player.get_attack_progress();

        glm::vec3 character_forward = player.object()->forward();
        if (glm::length(character_forward) < 0.001f) character_forward = glm::vec3(0.f, 0.f, -1.f);
        character_forward = glm::normalize(character_forward);

        glm::vec3 slash_centre = player.object()->position()
            + character_forward * 1.1f
            + glm::vec3(0.0f, 1.5f, 0.0f);

        glm::vec3 world_up(0.f, 1.f, 0.f);
        glm::vec3 character_right = glm::normalize(glm::cross(world_up, character_forward));
        glm::vec3 character_up = glm::cross(character_forward, character_right);

        const float half_length = 1.1f;

        for (int stroke_index = 0; stroke_index < 2; stroke_index++)
        {
            float phase_start = stroke_index * 0.5f;
            bool in_phase = progress >= phase_start && progress < phase_start + 0.5f;
            if (!in_phase) continue;
            float phase_progress = (progress - phase_start) / 0.5f;
            float right_sign = (stroke_index == 0) ? 1.0f : -1.0f;
            glm::vec3 top_corner_dir = glm::normalize(character_right * right_sign + character_up);
            glm::vec3 start_point = slash_centre + top_corner_dir * half_length;
            glm::vec3 end_point = slash_centre - top_corner_dir * half_length;

            render_particle_sweep(mesh_shader, start_point, end_point, phase_progress, glm::vec3(1.0f), 0.9f);
        }
    }

    //Enemy/priest/boss attack telegraph (red) then strike (white) - same particle technique as the player's attack
    const glm::vec3 telegraph_colour(0.95f, 0.15f, 0.1f);
    const glm::vec3 strike_colour(1.0f, 1.0f, 1.0f);

    const auto& enemies_for_vfx = spawn_manager.enemies();
    const auto& warriors_for_vfx = spawn_manager.warrior_objects();
    for (size_t i = 0; i < enemies_for_vfx.size(); i++)
    {
        if (enemies_for_vfx[i].is_telegraphing_attack())
            render_attacker_line_vfx(mesh_shader, warriors_for_vfx[i], enemies_for_vfx[i].get_telegraph_progress(), telegraph_colour, 0.6f, 1.0f, 1.3f, 0.9f, enemies_for_vfx[i].get_attack_swing_angle());
        else if (enemies_for_vfx[i].is_striking_attack())
            render_attacker_line_vfx(mesh_shader, warriors_for_vfx[i], enemies_for_vfx[i].get_strike_progress(), strike_colour, 0.9f, 1.0f, 1.3f, 0.9f, enemies_for_vfx[i].get_attack_swing_angle());
    }

    const auto& priests_for_vfx = spawn_manager.priests();
    const auto& priest_objects_for_vfx = spawn_manager.priest_objects();
    for (size_t i = 0; i < priests_for_vfx.size(); i++)
    {
        if (priests_for_vfx[i].is_telegraphing_attack())
        {
            glm::vec3 p = priest_objects_for_vfx[i]->position() + glm::vec3(0.f, 1.6f, 0.f);
            render_particle_sweep(mesh_shader, p - glm::vec3(0.25f, 0.f, 0.f), p + glm::vec3(0.25f, 0.f, 0.f), priests_for_vfx[i].get_telegraph_progress(), telegraph_colour, 0.6f);
        }
    }

    //Firebolts (priest ranged shots already in flight)
    for (const auto& bolt : firebolts)
        if (bolt.is_active())
            render_firebolt(mesh_shader, bolt);

    if (!boss_logic.is_dead() && (boss_logic.is_telegraphing_attack() || boss_logic.is_striking_attack()))
    {
        bool telegraphing = boss_logic.is_telegraphing_attack();
        float progress = telegraphing ? boss_logic.get_telegraph_progress() : boss_logic.get_strike_progress();
        glm::vec3 colour = telegraphing ? telegraph_colour : strike_colour;
        float alpha_scale = telegraphing ? 0.6f : 0.95f;

        if (boss_logic.get_current_attack_type() == Boss::AttackType::AOE)
        {
            float radius = telegraphing ? glm::mix(0.6f, boss_logic.get_aoe_radius(), progress) : boss_logic.get_aoe_radius() * 1.05f;
            glm::mat4 ring_transform(1.0f);
            ring_transform = glm::translate(ring_transform, boss_object->position() + glm::vec3(0.f, 0.15f, 0.f));
            ring_transform = glm::rotate(ring_transform, glm::radians(90.0f), glm::vec3(1.f, 0.f, 0.f));
            ring_transform = glm::scale(ring_transform, glm::vec3(radius));

            m_material->set_ambient(colour);
            m_material->set_transparency(alpha_scale);
            m_material->submit(mesh_shader);
            engine::renderer::submit(mesh_shader, m_aoe_ring_shape->mesh(), ring_transform);
        }
        else
        {
            render_attacker_line_vfx(mesh_shader, boss_object, progress, colour, alpha_scale, 1.8f, 1.6f, 1.8f, boss_logic.get_attack_swing_angle());
        }
    }

    engine::render_command::enable_culling();
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
}
