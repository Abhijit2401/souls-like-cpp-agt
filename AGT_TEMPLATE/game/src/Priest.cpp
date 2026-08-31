#include "pch.h"
#include "Priest.h"

Priest::Priest() {}
Priest::~Priest() {}

void Priest::initialise(engine::ref<engine::game_object> object,
    glm::vec3 position,
    engine::ref<engine::game_object> player_target)
{
    m_object = object;
    m_player_target = player_target;

    // Physics Position
    m_object->set_position(glm::vec3(position.x, 0.09f, position.z));

    // Visual Offset
    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));
    m_object->set_velocity(glm::vec3(0.0f, -5.0f, 0.0f));

    //RESETS STATS (For initialisation and respawning logic)
    m_health = 80.0f;
    m_spawn_signal = false;
    m_has_spawned_this_attack = false;
    m_vanished = false;
    m_souls_dropped = false;
    m_death_timer = 0.0f;

    // PRIEST ANIMATION INDEX
    m_anim_idle = 4;
    m_anim_walk = 3;
    m_anim_attack = 0;
    m_anim_death = 1;

    // Set initial animation
    m_object->animated_mesh()->set_default_animation(m_anim_idle);
    m_object->animated_mesh()->switch_animation(m_anim_idle);

    m_state = State::Idle;
}

bool Priest::should_spawn_projectile() {
    if (m_spawn_signal) {
        m_spawn_signal = false;
        return true;
    }
    return false;
}

void Priest::on_update(const engine::timestep& time_step)
{

    if (m_state != State::Dead) {
        if (m_object->position().y < 0.09f) {
            glm::vec3 current_pos = m_object->position();
            m_object->set_position(glm::vec3(current_pos.x, 0.09f, current_pos.z));
        }
    }
    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));
    m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));
    if (m_state == State::Dead) {
        m_object->animated_mesh()->on_update(time_step);

        if (!m_vanished) {
            m_death_timer += (float)time_step;
            float anim_duration = 3.0f;
            if (m_object->animated_mesh()->animations().size() > m_anim_death) {
                anim_duration = (float)m_object->animated_mesh()->animations().at(m_anim_death)->mDuration / 30.0f;
            }
            if (m_death_timer > anim_duration) {
                m_vanished = true;
                m_object->set_position(glm::vec3(0.0f, -100.0f, 0.0f));
            }
        }
        return;
    }
    float dist = glm::distance(m_object->position(), m_player_target->position());

    switch (m_state)
    {
    case State::Idle:
        if (dist < m_detection_radius) {
            m_state = State::Chasing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
        }
        break;

    case State::Chasing:
        if (dist < m_flee_radius) {
            m_state = State::Fleeing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
        }
        else if (dist <= m_attack_range) {
            m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f));
            m_state = State::Attacking;
            m_object->animated_mesh()->switch_animation(m_anim_attack);
            m_attack_cooldown = 0.0f;
            m_spawn_signal = false;
            m_has_spawned_this_attack = false;
        }
        else {
            face_player((float)time_step);
            glm::vec3 dir = glm::normalize(m_player_target->position() - m_object->position());
            dir.y = 0.0f;
            float current_y = m_object->velocity().y;
            m_object->set_velocity(glm::vec3(dir.x * m_speed, current_y, dir.z * m_speed));
        }
        break;

    case State::Fleeing:
        // Return to Chasing if player is far enough away
        if (dist > m_flee_radius * 1.5f) {
            m_state = State::Chasing;
        }
        // Move away from the player
        else {
            face_player((float)time_step);
            glm::vec3 dir = glm::normalize(m_object->position() - m_player_target->position());
            dir.y = 0.0f;
            float current_y = m_object->velocity().y;
            m_object->set_velocity(glm::vec3(dir.x * m_flee_speed, current_y, dir.z * m_flee_speed));
        }
        break;

    case State::Attacking:
        if (dist < m_flee_radius) {
            m_state = State::Fleeing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
            break;
        }

        m_attack_cooldown += (float)time_step;
        if (m_attack_cooldown > 1.0f && !m_has_spawned_this_attack) {
            m_spawn_signal = true;
            m_has_spawned_this_attack = true;
        }

        if (m_attack_cooldown > 3.0f) {
            m_state = State::Chasing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
        }
        break;
    }
    float playback_speed = (m_state == State::Fleeing) ? 0.8f : 1.0f;
    m_object->animated_mesh()->on_update(time_step * playback_speed);
}

void Priest::face_player(float dt)
{
    glm::vec3 dir = m_player_target->position() - m_object->position();
    dir.y = 0.0f;
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);
    m_object->set_forward(dir);

    float theta = atan2(m_object->forward().x, m_object->forward().z);
    m_object->set_rotation_amount(theta);
}

// Handles damage and transition to Dead state
void Priest::take_damage(float amount)
{
    m_health -= amount;
    if (m_health <= 0 && m_state != State::Dead)
    {
        m_state = State::Dead;
        m_object->set_velocity(glm::vec3(0.f)); // Stop movement
        m_object->animated_mesh()->switch_animation(m_anim_death);
        m_death_timer = 0.0f;
    }
}
