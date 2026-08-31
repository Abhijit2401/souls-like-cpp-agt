#pragma once
#include <engine.h>
#include "Enemy.h"
#include "Priest.h"

class player;

class SpawnManager
{
public:
    struct UpdateEvents
    {
        std::vector<glm::vec3> soul_drop_positions;
        std::vector<glm::vec3> firebolt_spawn_positions;
    };

    void initialise(engine::ref<engine::skinned_mesh> berzerker_mesh,
        std::vector<engine::ref<engine::texture_2d>> berzerker_textures,
        engine::ref<engine::skinned_mesh> priest_mesh,
        std::vector<engine::ref<engine::texture_2d>> priest_textures,
        float spawn_radius,
        float safe_radius);

    void spawn_enemy(const player& player, std::vector<engine::ref<engine::game_object>>& game_objects);
    void spawn_priest(const player& player, std::vector<engine::ref<engine::game_object>>& game_objects);

    UpdateEvents update(const engine::timestep& time_step, player& player_ref);

    void render(const engine::ref<engine::shader>& mesh_shader) const;

    const std::vector<Enemy>& enemies() const { return m_enemies; }
    const std::vector<engine::ref<engine::game_object>>& warrior_objects() const { return m_warriors; }
    const std::vector<Priest>& priests() const { return m_priests; }
    const std::vector<engine::ref<engine::game_object>>& priest_objects() const { return m_priest_objects; }

private:
    engine::ref<engine::skinned_mesh> m_berzerker_mesh;
    std::vector<engine::ref<engine::texture_2d>> m_berzerker_textures;
    engine::ref<engine::skinned_mesh> m_priest_mesh;
    std::vector<engine::ref<engine::texture_2d>> m_priest_textures;

    float m_spawn_radius = 30.0f;
    float m_safe_radius = 10.0f;

    std::vector<engine::ref<engine::game_object>> m_warriors;
    std::vector<Enemy> m_enemies;
    std::vector<engine::ref<engine::game_object>> m_priest_objects;
    std::vector<Priest> m_priests;
};
