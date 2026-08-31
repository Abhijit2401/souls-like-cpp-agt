#pragma once
#include <engine.h>

class Firebolt
{
public:
    Firebolt(const glm::vec3& start_position, const glm::vec3& direction, float speed, float damage);

    void on_update(const engine::timestep& time_step);

    // Returns the damage dealt (and deactivates) if this frame's position is close enough to
    // player_position, otherwise 0.
    float check_collision(const glm::vec3& player_position);

    glm::vec3 position() const { return m_position; }
    glm::vec3 direction() const { return m_direction; }
    bool is_active() const { return m_active; }

private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
    float m_speed;
    float m_damage;
    float m_age = 0.0f;
    bool m_active = true;
};
