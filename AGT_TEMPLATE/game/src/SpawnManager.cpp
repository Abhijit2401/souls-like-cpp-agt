#include "pch.h"
#include "SpawnManager.h"
#include "Player.h"
#include <cstdlib>

void SpawnManager::initialise(engine::ref<engine::skinned_mesh> berzerker_mesh,
    std::vector<engine::ref<engine::texture_2d>> berzerker_textures,
    engine::ref<engine::skinned_mesh> priest_mesh,
    std::vector<engine::ref<engine::texture_2d>> priest_textures,
    float spawn_radius,
    float safe_radius)
{
    m_berzerker_mesh = berzerker_mesh;
    m_berzerker_textures = berzerker_textures;
    m_priest_mesh = priest_mesh;
    m_priest_textures = priest_textures;
    m_spawn_radius = spawn_radius;
    m_safe_radius = safe_radius;
}

void SpawnManager::spawn_enemy(const player& player, std::vector<engine::ref<engine::game_object>>& game_objects)
{
    if (!m_berzerker_mesh) return;

    engine::ref<engine::skinned_mesh> unique_mesh = engine::skinned_mesh::create("assets/models/animated/berzerker/berzerker.fbx");
    unique_mesh->switch_root_movement(false);
    unique_mesh->set_textures(m_berzerker_textures);

    engine::game_object_properties enemy_props;
    enemy_props.animated_mesh = unique_mesh;
    enemy_props.scale = glm::vec3(0.15f);
    enemy_props.textures = m_berzerker_textures;
    enemy_props.type = 0;
    enemy_props.mass = 10.0f; // heavy enough that a dashing player (mass 80) doesn't fling them around
    enemy_props.friction = 1.0f;
    enemy_props.restitution = 0.0f;
    enemy_props.bounding_shape = glm::vec3(0.5f, 0.9f, 0.5f);

    glm::vec3 spawn_pos;
    bool valid_position = false;
    int attempts = 0;

    // Loop to ensure enemy doesn't spawn right on top of player or within a certain radius
    while (!valid_position && attempts < 10)
    {
        float rX = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
        float rZ = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
        spawn_pos = glm::vec3(rX, 5.0f, rZ);

        if (player.object()) {
            float dist_to_player = glm::distance(spawn_pos, player.object()->position());
            if (dist_to_player > m_safe_radius) {
                valid_position = true;
            }
        }
        else {
            valid_position = true;
        }
        attempts++;
    }

    enemy_props.position = spawn_pos;

    engine::ref<engine::game_object> warrior = engine::game_object::create(enemy_props);
    warrior->set_angular_factor_lock(true);
    warrior->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    m_warriors.push_back(warrior);
    game_objects.push_back(warrior);

    Enemy enemy_logic;
    if (player.object()) {
        enemy_logic.initialise(warrior, enemy_props.position, player.object());
    }
    m_enemies.push_back(enemy_logic);
}

void SpawnManager::spawn_priest(const player& player, std::vector<engine::ref<engine::game_object>>& game_objects)
{
    if (!m_priest_mesh) return;

    engine::ref<engine::skinned_mesh> unique_mesh = engine::skinned_mesh::create("assets/models/animated/priest/priest.fbx");
    unique_mesh->switch_root_movement(false);
    unique_mesh->set_textures(m_priest_textures);

    engine::game_object_properties priest_props;
    priest_props.animated_mesh = unique_mesh;
    priest_props.scale = glm::vec3(0.15f);
    priest_props.textures = m_priest_textures;
    priest_props.type = 0;
    priest_props.mass = 6.0f; // lighter than a warrior, but still not so light it slides on a bump
    priest_props.friction = 1.0f;
    priest_props.restitution = 0.0f;
    priest_props.bounding_shape = glm::vec3(0.5f, 0.9f, 0.5f);

    float rX = ((float)rand() / (float)RAND_MAX * 50.0f) - 25.0f;
    float rZ = ((float)rand() / (float)RAND_MAX * 50.0f) - 25.0f;
    priest_props.position = glm::vec3(rX, 5.0f, rZ);

    engine::ref<engine::game_object> priest_obj = engine::game_object::create(priest_props);
    priest_obj->set_angular_factor_lock(true);
    priest_obj->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    m_priest_objects.push_back(priest_obj);
    game_objects.push_back(priest_obj);

    Priest priest_logic;
    if (player.object()) {
        priest_logic.initialise(priest_obj, priest_props.position, player.object());
    }
    m_priests.push_back(priest_logic);
}

SpawnManager::UpdateEvents SpawnManager::update(const engine::timestep& time_step, player& player_ref)
{
    UpdateEvents events;

    // --- Enemy horde: AI, player-attack hit detection, respawn, soul drops ---
    for (size_t i = 0; i < m_enemies.size(); i++)
    {
        m_enemies[i].on_update(time_step);

        if (player_ref.is_attacking()) {
            float dist = glm::distance(player_ref.object()->position(), m_warriors[i]->position());
            if (dist < 3.0f) {
                glm::vec3 to_enemy = glm::normalize(m_warriors[i]->position() - player_ref.object()->position());
                if (glm::dot(player_ref.object()->forward(), to_enemy) > 0.5f) {
                    if (!m_enemies[i].is_dead()) {
                        m_enemies[i].take_damage(player_ref.get_damage() * (float)time_step);
                    }
                }
            }
        }

        if (m_enemies[i].check_hit_player()) {
            player_ref.take_damage(10.0f);
        }

        // Respawn mechanic: a killed enemy that has vanished re-appears outside the safe zone
        if (m_enemies[i].has_vanished())
        {
            glm::vec3 spawn_pos;
            bool valid = false;
            int attempts = 0;
            while (!valid && attempts < 10) {
                float rX = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
                float rZ = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
                spawn_pos = glm::vec3(rX, 5.0f, rZ);
                if (glm::distance(spawn_pos, player_ref.object()->position()) > m_safe_radius) valid = true;
                attempts++;
            }
            m_enemies[i].initialise(m_warriors[i], spawn_pos, player_ref.object());
        }

        // Drops a random set number of souls
        if (m_enemies[i].is_dead() && !m_enemies[i].souls_dropped())
        {
            int num_souls = (rand() % 5) + 1;
            glm::vec3 drop_center = m_warriors[i]->position();
            for (int s = 0; s < num_souls; s++) {
                float rX = ((float)rand() / (float)RAND_MAX * 3.0f) - 1.5f;
                float rZ = ((float)rand() / (float)RAND_MAX * 3.0f) - 1.5f;
                events.soul_drop_positions.push_back(glm::vec3(drop_center.x + rX, 0.5f, drop_center.z + rZ));
            }
            m_enemies[i].set_souls_dropped(true);
        }
    }

    // --- Priest mages: AI, projectile requests, player-attack hit detection, soul drops ---
    for (size_t i = 0; i < m_priests.size(); i++)
    {
        m_priests[i].on_update(time_step);

        if (m_priests[i].should_spawn_projectile()) {
            events.firebolt_spawn_positions.push_back(m_priest_objects[i]->position() + glm::vec3(0.f, 1.4f, 0.f));
        }

        if (player_ref.is_attacking()) {
            float dist = glm::distance(player_ref.object()->position(), m_priest_objects[i]->position());
            if (dist < 3.0f) {
                glm::vec3 to_enemy = glm::normalize(m_priest_objects[i]->position() - player_ref.object()->position());
                if (glm::dot(player_ref.object()->forward(), to_enemy) > 0.5f) {
                    m_priests[i].take_damage(player_ref.get_damage() * (float)time_step);
                }
            }
        }

        if (m_priests[i].is_dead() && !m_priests[i].souls_dropped())
        {
            int num_souls = (rand() % 6) + 3;
            glm::vec3 drop_center = m_priest_objects[i]->position();
            for (int s = 0; s < num_souls; s++) {
                float rX = ((float)rand() / (float)RAND_MAX * 2.0f) - 1.0f;
                float rZ = ((float)rand() / (float)RAND_MAX * 2.0f) - 1.0f;
                events.soul_drop_positions.push_back(glm::vec3(drop_center.x + rX, 0.5f, drop_center.z + rZ));
            }
            m_priests[i].set_souls_dropped(true);
        }
    }

    return events;
}

void SpawnManager::render(const engine::ref<engine::shader>& mesh_shader) const
{
    for (size_t i = 0; i < m_enemies.size(); i++)
    {
        if (!m_enemies[i].has_vanished())
        {
            glm::mat4 warrior_transform(1.0f);
            m_warriors[i]->transform(warrior_transform);
            engine::renderer::submit(mesh_shader, warrior_transform, m_warriors[i]);
        }
    }

    for (size_t i = 0; i < m_priests.size(); i++)
    {
        if (!m_priests[i].has_vanished())
        {
            glm::mat4 priest_transform(1.0f);
            m_priest_objects[i]->transform(priest_transform);
            engine::renderer::submit(mesh_shader, priest_transform, m_priest_objects[i]);
        }
    }
}
