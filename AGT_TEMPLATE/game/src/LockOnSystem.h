#pragma once
#include <engine.h>
#include "SpawnManager.h"
#include "Boss.h"
#include "SlashEffect.h"

class LockOnSystem
{
public:
    void initialise();

    void on_update(const engine::timestep& time_step, bool middle_mouse_down,
        const engine::perspective_camera& camera,
        const SpawnManager& spawn_manager, const Boss& boss_logic,
        const engine::ref<engine::game_object>& boss_object);

    engine::ref<engine::game_object> find_closest_target_to_screen_center(
        const engine::perspective_camera& camera,
        const SpawnManager& spawn_manager, const Boss& boss_logic,
        const engine::ref<engine::game_object>& boss_object) const;

    bool is_locked() const { return m_locked_on; }
    engine::ref<engine::game_object> target() const { return m_locked_target; }
    glm::vec3 target_position() const { return m_locked_target ? m_locked_target->position() : glm::vec3(0.f); }

    // Spinning billboarded ring around the current target
    void render_reticle(const engine::ref<engine::shader>& mesh_shader, const glm::vec3& camera_position);

private:
    bool m_locked_on = false;
    engine::ref<engine::game_object> m_locked_target{};
    bool m_middle_mouse_was_down = false;
    float m_ring_spin = 0.0f;

    engine::ref<engine::SlashEffect> m_ring_shape{};
    engine::ref<engine::material> m_ring_material{};
};
