#include "pch.h"
#include "CombatVfx.h"
#include "platform/opengl/gl_shader.h"
#include "engine/renderer/renderer_api.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdlib>

namespace
{
    float random01() { return (float)rand() / (float)RAND_MAX; }
}

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
        seed.jitter_perp = (random01() - 0.5f) * 0.16f;
        seed.jitter_along = (random01() - 0.5f) * 0.05f;
        seed.size = 0.045f + random01() * 0.05f;
        m_particles.push_back(seed);
    }
}

void CombatVfx::spawn_hit_burst(const glm::vec3& position, const glm::vec3& colour, int spark_count, float speed, float flash_size)
{
    for (int i = 0; i < spark_count; i++)
    {
        //Random direction but mostly upwards so the sparks dont just go into the floor
        glm::vec3 dir(random01() * 2.f - 1.f, random01() * 1.2f - 0.2f, random01() * 2.f - 1.f);
        if (glm::length(dir) < 0.001f) dir = glm::vec3(0.f, 1.f, 0.f);
        dir = glm::normalize(dir);

        Spark spark;
        spark.position = position;
        spark.velocity = dir * speed * (0.4f + random01() * 0.8f);
        //Mixes some white in so each spark is a slightly different shade
        spark.colour = glm::mix(colour, glm::vec3(1.0f), random01() * 0.5f);
        spark.max_life = 0.25f + random01() * 0.35f;
        spark.life = spark.max_life;
        spark.size = 0.07f + random01() * 0.09f;
        m_sparks.push_back(spark);
    }

    ImpactFlash flash;
    flash.position = position;
    flash.colour = glm::mix(colour, glm::vec3(1.0f), 0.5f);
    flash.max_life = 0.14f;
    flash.life = flash.max_life;
    flash.size = flash_size;
    m_flashes.push_back(flash);
}

void CombatVfx::on_update(float dt)
{
    const float gravity = 7.0f;
    const float drag = 2.5f;
    for (auto& spark : m_sparks)
    {
        spark.velocity.y -= gravity * dt;
        spark.velocity *= glm::max(0.0f, 1.0f - drag * dt);
        spark.position += spark.velocity * dt;
        spark.life -= dt;
    }
    for (auto& flash : m_flashes) flash.life -= dt;

    m_sparks.erase(std::remove_if(m_sparks.begin(), m_sparks.end(), [](const Spark& s) { return s.life <= 0.0f; }), m_sparks.end());
    m_flashes.erase(std::remove_if(m_flashes.begin(), m_flashes.end(), [](const ImpactFlash& f) { return f.life <= 0.0f; }), m_flashes.end());
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

void CombatVfx::render_hit_effects(const engine::ref<engine::shader>& mesh_shader)
{
    for (const auto& flash : m_flashes)
    {
        float t = 1.0f - flash.life / flash.max_life; //Goes from 0 to 1 over the flash
        render_billboard_particle(mesh_shader, flash.position, flash.size * (0.5f + 0.7f * t), flash.colour, 0.9f * (1.0f - t));
        //Bright white centre
        render_billboard_particle(mesh_shader, flash.position, flash.size * 0.35f * (1.0f - t), glm::vec3(1.0f), 1.0f - t);
    }
    for (const auto& spark : m_sparks)
    {
        float remaining = spark.life / spark.max_life;
        render_billboard_particle(mesh_shader, spark.position, spark.size * (0.4f + 0.6f * remaining), spark.colour, remaining);
    }
}

const CombatVfx::ArcMeshes& CombatVfx::arc_meshes(float half_angle_degrees)
{
    const bool full_circle = half_angle_degrees >= 180.0f;
    const int key = full_circle ? 180 : (int)std::round(half_angle_degrees);
    auto found = m_arc_meshes.find(key);
    if (found != m_arc_meshes.end()) return found->second;

    const float half_arc = glm::radians((float)key);
    const int segments = glm::max(8, key / 4);
    const glm::vec3 up(0.f, 1.f, 0.f);
    auto dir_at = [&](int i) { float a = -half_arc + 2.f * half_arc * (float)i / (float)segments; return glm::vec3(sin(a), 0.f, cos(a)); };

    //Flat pie slice shape from the centre out to the curved edge
    std::vector<engine::mesh::vertex> face_vertices{ { glm::vec3(0.f), up, { 0.5f, 0.f } } };
    std::vector<uint32_t> face_indices;
    for (int i = 0; i <= segments; i++)
    {
        face_vertices.push_back({ dir_at(i), up, { (float)i / segments, 1.f } });
        if (i > 0) { face_indices.push_back(0); face_indices.push_back(i); face_indices.push_back(i + 1); }
    }

    //Curved outer wall
    std::vector<engine::mesh::vertex> wall_vertices;
    std::vector<uint32_t> wall_indices;
    for (int i = 0; i <= segments; i++)
    {
        glm::vec3 d = dir_at(i);
        wall_vertices.push_back({ d + glm::vec3(0.f, -0.5f, 0.f), d, { (float)i / segments, 0.f } });
        wall_vertices.push_back({ d + glm::vec3(0.f, 0.5f, 0.f), d, { (float)i / segments, 1.f } });
        if (i > 0)
        {
            uint32_t b = (uint32_t)(i - 1) * 2;
            wall_indices.insert(wall_indices.end(), { b, b + 2, b + 3, b, b + 3, b + 1 });
        }
    }

    ArcMeshes meshes;
    meshes.face = engine::mesh::create(face_vertices, face_indices);
    meshes.wall = engine::mesh::create(wall_vertices, wall_indices);

    //The two straight sides of the cone (not needed for a full circle)
    if (!full_circle)
    {
        std::vector<engine::mesh::vertex> side_vertices;
        std::vector<uint32_t> side_indices;
        for (int side = 0; side < 2; side++)
        {
            glm::vec3 d = dir_at(side == 0 ? 0 : segments);
            glm::vec3 n = glm::cross(up, d);
            uint32_t b = (uint32_t)side_vertices.size();
            side_vertices.push_back({ glm::vec3(0.f, -0.5f, 0.f), n, { 0.f, 0.f } });
            side_vertices.push_back({ d + glm::vec3(0.f, -0.5f, 0.f), n, { 1.f, 0.f } });
            side_vertices.push_back({ d + glm::vec3(0.f, 0.5f, 0.f), n, { 1.f, 1.f } });
            side_vertices.push_back({ glm::vec3(0.f, 0.5f, 0.f), n, { 0.f, 1.f } });
            side_indices.insert(side_indices.end(), { b, b + 1, b + 2, b, b + 2, b + 3 });
        }
        meshes.sides = engine::mesh::create(side_vertices, side_indices);
    }

    return m_arc_meshes.emplace(key, meshes).first->second;
}

void CombatVfx::submit_arc_mesh(const engine::ref<engine::shader>& mesh_shader, const engine::ref<engine::mesh>& mesh,
    const AttackArc& arc, const glm::mat4& local_transform, const glm::vec3& colour, float alpha)
{
    if (!mesh || alpha <= 0.001f) return;

    //Moves it to weapon height, faces it the same way as the attack and tilts it to match the swing
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, arc.centre());
    transform = glm::rotate(transform, arc.yaw(), glm::vec3(0.f, 1.f, 0.f));
    transform = glm::rotate(transform, glm::radians(arc.roll_degrees), glm::vec3(0.f, 0.f, 1.f));
    transform = transform * local_transform;

    m_material->set_ambient(colour);
    m_material->set_transparency(alpha);
    m_material->submit(mesh_shader);
    engine::renderer::submit(mesh_shader, mesh, transform);
}

void CombatVfx::render_arc_indicator(const engine::ref<engine::shader>& mesh_shader, const AttackArc& arc,
    float fill_progress, float flash, const glm::vec3& colour, float base_alpha, float thickness)
{
    const ArcMeshes& meshes = arc_meshes(arc.half_angle_degrees);
    const float half_thickness = thickness * 0.5f;

    //Draws the whole shape (top, bottom, outer wall and sides) out to the given radius
    auto slab = [&](float radius, float face_alpha, float wall_alpha, float side_alpha, const glm::vec3& c)
    {
        if (radius <= 0.001f) return;
        glm::mat4 top = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.f, half_thickness, 0.f)), glm::vec3(radius, 1.f, radius));
        glm::mat4 bottom = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.f, -half_thickness, 0.f)), glm::vec3(radius, 1.f, radius));
        glm::mat4 walls = glm::scale(glm::mat4(1.0f), glm::vec3(radius, thickness, radius));
        submit_arc_mesh(mesh_shader, meshes.face, arc, top, c, face_alpha);
        submit_arc_mesh(mesh_shader, meshes.face, arc, bottom, c, face_alpha);
        submit_arc_mesh(mesh_shader, meshes.wall, arc, walls, c, wall_alpha);
        submit_arc_mesh(mesh_shader, meshes.sides, arc, walls, c, side_alpha);
    };

    if (flash > 0.0f)
    {
        //When the attack hits the whole shape flashes bright then fades out
        const glm::vec3 flash_colour = glm::mix(colour, glm::vec3(1.0f, 0.97f, 0.9f), 0.55f);
        slab(arc.radius, 0.55f * flash * base_alpha, flash * base_alpha, 0.8f * flash * base_alpha, flash_colour);
        return;
    }

    float fill = glm::clamp(fill_progress, 0.0f, 1.0f);
    //Faint full size shape so you can see the whole attack area, with brighter edges
    slab(arc.radius, 0.12f * base_alpha, 0.6f * base_alpha, 0.45f * base_alpha, colour);
    //Fills up from the attacker outwards and the attack lands when it reaches the edge
    slab(arc.radius * fill, 0.22f * base_alpha, 0.9f * base_alpha, 0.0f, glm::mix(colour, glm::vec3(1.0f), 0.2f));
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
    //Turns off depth writing for the see through effects so overlapping ones dont hide each other
    engine::renderer_api::disable_depth_mask();

    const glm::vec3 telegraph_colour(1.0f, 0.15f, 0.1f);

    //PLAYER SWING INDICATOR (blue, fills up until the sword hits then flashes)
    if (player.is_attacking())
    {
        const glm::vec3 holo_colour(0.3f, 0.85f, 1.0f);
        const AttackArc swing = player.get_attack_arc();
        if (!player.has_attack_connected())
            render_arc_indicator(mesh_shader, swing, player.get_attack_windup_progress(), 0.0f, holo_colour, 0.7f, 0.1f);
        else
            render_arc_indicator(mesh_shader, swing, 1.0f, 1.0f - player.get_attack_progress(), holo_colour, 0.7f, 0.1f);
    }

    //BERZERKER ATTACK INDICATORS (red)
    for (const auto& enemy : spawn_manager.enemies())
    {
        if (enemy.is_telegraphing_attack())
            render_arc_indicator(mesh_shader, enemy.get_attack_arc(), enemy.get_telegraph_progress(), 0.0f, telegraph_colour, 1.0f, 0.14f);
        else if (enemy.is_striking_attack())
            render_arc_indicator(mesh_shader, enemy.get_attack_arc(), 1.0f, 1.0f - enemy.get_strike_progress(), telegraph_colour, 1.0f, 0.14f);
    }

    //PRIEST charge up effect above the head before shooting a firebolt
    const auto& priests_for_vfx = spawn_manager.priests();
    const auto& priest_objects_for_vfx = spawn_manager.priest_objects();
    for (size_t i = 0; i < priests_for_vfx.size(); i++)
    {
        if (priests_for_vfx[i].is_telegraphing_attack())
        {
            glm::vec3 p = priest_objects_for_vfx[i]->position() + glm::vec3(0.f, 1.6f, 0.f);
            render_particle_sweep(mesh_shader, p - glm::vec3(0.25f, 0.f, 0.f), p + glm::vec3(0.25f, 0.f, 0.f), priests_for_vfx[i].get_telegraph_progress(), glm::vec3(1.0f, 0.5f, 0.1f), 0.7f);
        }
    }

    //Firebolts (priest ranged shots already in flight)
    for (const auto& bolt : firebolts)
        if (bolt.is_active())
            render_firebolt(mesh_shader, bolt);

    //BOSS ATTACK INDICATORS (red for the swing, orange ring for the ground slam)
    if (!boss_logic.is_dead() && (boss_logic.is_telegraphing_attack() || boss_logic.is_striking_attack()))
    {
        bool aoe = boss_logic.get_current_attack_type() == Boss::AttackType::AOE;
        glm::vec3 boss_colour = aoe ? glm::vec3(1.0f, 0.45f, 0.05f) : telegraph_colour;
        float thickness = aoe ? 0.3f : 0.22f;

        if (boss_logic.is_telegraphing_attack())
            render_arc_indicator(mesh_shader, boss_logic.get_attack_arc(), boss_logic.get_telegraph_progress(), 0.0f, boss_colour, 1.0f, thickness);
        else
            render_arc_indicator(mesh_shader, boss_logic.get_attack_arc(), 1.0f, 1.0f - boss_logic.get_strike_progress(), boss_colour, 1.0f, thickness);
    }

    render_hit_effects(mesh_shader);

    engine::renderer_api::enable_depth_mask();
    engine::render_command::enable_culling();
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
}
