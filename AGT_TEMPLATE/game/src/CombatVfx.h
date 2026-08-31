#pragma once
#include <engine.h>
#include <vector>
#include "Player.h"
#include "SpawnManager.h"
#include "Boss.h"
#include "Firebolt.h"
#include "SlashEffect.h"

class CombatVfx
{
public:
    void initialise();
    void render(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& camera_position,
        const player& player,
        const SpawnManager& spawn_manager,
        const Boss& boss_logic,
        const engine::ref<engine::game_object>& boss_object,
        const std::vector<Firebolt>& firebolts);

private:
    struct ParticleSeed
    {
        float t;
        float jitter_perp;   // fixed sideways offset off the line, for a rough/organic look
        float jitter_along;  // fixed offset along the line
        float size;          // billboard quad size
    };

    // Shared particle-trail primitive
    void render_billboard_particle(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& world_position, float size, const glm::vec3& colour, float alpha);
    void render_particle_sweep(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& start_point, const glm::vec3& end_point,
        float sweep_progress, const glm::vec3& colour, float alpha_scale);
    void render_attacker_line_vfx(const engine::ref<engine::shader>& mesh_shader,
        const engine::ref<engine::game_object>& attacker, float progress,
        const glm::vec3& colour, float alpha_scale, float reach, float height, float half_length,
        float swing_angle_degrees);
    void render_firebolt(const engine::ref<engine::shader>& mesh_shader, const Firebolt& bolt);

    glm::vec3 m_camera_position{ 0.f }; // set at the top of render(), read by the helpers above

    std::vector<ParticleSeed> m_particles;
    engine::ref<engine::mesh> m_particle_quad;
    engine::ref<engine::material> m_material;
    engine::ref<engine::SlashEffect> m_aoe_ring_shape;
};
