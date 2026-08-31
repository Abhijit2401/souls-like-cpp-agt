#include "pch.h"
#include "Firebolt.h"

Firebolt::Firebolt(const glm::vec3& start_position, const glm::vec3& direction, float speed, float damage)
    : m_position(start_position), m_direction(direction), m_speed(speed), m_damage(damage)
{
    if (glm::length(m_direction) > 0.001f)
        m_direction = glm::normalize(m_direction);
    else
        m_direction = glm::vec3(0.f, 0.f, -1.f);
}

void Firebolt::on_update(const engine::timestep& time_step)
{
    if (!m_active) return;

    m_position += m_direction * m_speed * (float)time_step;
    m_age += (float)time_step;

    // Straigh line shot with a lifetime cap so a missed firebolt doesn't fly forever
    if (m_age > 4.0f) m_active = false;
}

float Firebolt::check_collision(const glm::vec3& player_position)
{
    if (!m_active) return 0.0f;

    if (glm::distance(m_position, player_position) < 1.0f)
    {
        m_active = false;
        return m_damage;
    }
    return 0.0f;
}
