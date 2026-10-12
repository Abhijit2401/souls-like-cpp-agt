#pragma once
#include <engine.h>
#include <vector>
#include "Player.h"
#include "SpawnManager.h"
#include "Boss.h"
#include "Firebolt.h"
#include "AttackArc.h"
#include <map>

class CombatVfx
{
public:
    void initialise();
    void on_update(float dt);
    void render(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& camera_position,
        const player& player,
        const SpawnManager& spawn_manager,
        const Boss& boss_logic,
        const engine::ref<engine::game_object>& boss_object,
        const std::vector<Firebolt>& firebolts);

    //Spawns sparks and a quick flash where something got hit
    void spawn_hit_burst(const glm::vec3& position, const glm::vec3& colour, int spark_count, float speed, float flash_size);

private:
    struct ParticleSeed
    {
        float t;
        float jitter_perp;   // fixed sideways offset off the line, for a rough/organic look
        float jitter_along;  // fixed offset along the line
        float size;          // billboard quad size
    };

    struct Spark
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 colour;
        float life;
        float max_life;
        float size;
    };

    struct ImpactFlash
    {
        glm::vec3 position;
        glm::vec3 colour;
        float life;
        float max_life;
        float size;
    };

    //Particle helpers
    void render_billboard_particle(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& world_position, float size, const glm::vec3& colour, float alpha);
    void render_particle_sweep(const engine::ref<engine::shader>& mesh_shader,
        const glm::vec3& start_point, const glm::vec3& end_point,
        float sweep_progress, const glm::vec3& colour, float alpha_scale);
    void render_firebolt(const engine::ref<engine::shader>& mesh_shader, const Firebolt& bolt);
    void render_hit_effects(const engine::ref<engine::shader>& mesh_shader);

    //Draws the see through attack indicator at weapon height tilted the same way as the swing.
    //It fills up as the attack charges (fill_progress 0 to 1) then flashes when it hits (flash 0 to 1)
    void render_arc_indicator(const engine::ref<engine::shader>& mesh_shader, const AttackArc& arc,
        float fill_progress, float flash, const glm::vec3& colour, float base_alpha, float thickness);

    //Meshes for the indicator shape with a radius of 1, saved per angle so they only get made once
    struct ArcMeshes
    {
        engine::ref<engine::mesh> face;  //Flat pie slice for the top and bottom
        engine::ref<engine::mesh> wall;  //Curved outer wall
        engine::ref<engine::mesh> sides; //Straight sides (empty for a full circle)
    };
    const ArcMeshes& arc_meshes(float half_angle_degrees);
    void submit_arc_mesh(const engine::ref<engine::shader>& mesh_shader, const engine::ref<engine::mesh>& mesh,
        const AttackArc& arc, const glm::mat4& local_transform, const glm::vec3& colour, float alpha);

    glm::vec3 m_camera_position{ 0.f }; //Set at the start of render() for the billboard particles

    std::vector<ParticleSeed> m_particles;
    std::vector<Spark> m_sparks;
    std::vector<ImpactFlash> m_flashes;
    engine::ref<engine::mesh> m_particle_quad;
    engine::ref<engine::material> m_material;
    std::map<int, ArcMeshes> m_arc_meshes;
};
