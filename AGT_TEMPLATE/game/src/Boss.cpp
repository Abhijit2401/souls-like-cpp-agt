#include "pch.h"
#include "Boss.h"

Boss::Boss() {}
Boss::~Boss() {}

void Boss::initialise(engine::ref<engine::game_object> object,
    glm::vec3 position,
    engine::ref<engine::game_object> player_target)
{
    m_object = object;
    m_player_target = player_target;

    //Sets initial position to the floor height just like iwht the others (its flying btw cos its meant to be a flying boss even on all the naimations)
    glm::vec3 correct_position = position;
    correct_position.y = 0.09f;
    m_object->set_position(correct_position);

    //Applies the fix offset immediately
    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));
    m_object->set_velocity(glm::vec3(0.0f, -5.0f, 0.0f));

    // DEVIL BOSS ANIMATION INDEX
    m_anim_idle = 3;
    m_anim_walk = 5; //its a flying animation
    m_anim_attack = 2; //it stomps the greound in front of it
    m_anim_death = 1;

    m_object->animated_mesh()->set_default_animation(m_anim_idle);
    m_object->animated_mesh()->switch_animation(m_anim_idle);

    m_state = State::Idle;
}

//checks if the damage signal was set during the attack and then resets it
bool Boss::check_hit_player() {
    if (m_damage_signal) {
        m_damage_signal = false;
        return true;
    }
    return false;
}

void Boss::on_update(const engine::timestep& time_step)
{
    //Prevents sinking through the floor, but stops respawning when dead.
    if (m_state != State::Dead)
    {
        if (m_object->position().y < 0.09f) {
            glm::vec3 current_pos = m_object->position();
            m_object->set_position(glm::vec3(current_pos.x, 0.09f, current_pos.z));
        }
    }

    //Lifts the mesh out of the ground
    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));

    //Lock rotation so he doesn't tilt
    m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));

    //DEAD STATE
    if (m_state == State::Dead) {
        m_object->animated_mesh()->on_update(time_step);
        //Keeps the offset applied even when dead so he doesn't sink into the floor instantly
        m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));

        if (!m_vanished) {
            m_death_timer += (float)time_step;

            // Boss death animation is about 2 seconds i think
            if (m_death_timer > 2.0f) {
                m_vanished = true;
                // Move underground to make it disappear
                m_object->set_position(glm::vec3(0.0f, -100.0f, 0.0f));
            }
        }
        return;
    }

    //AI LOGIC
    //Calculates distance to player for state transitions
    float dist = glm::distance(m_object->position(), m_player_target->position());
    float anim_speed = 1.0f;

    switch (m_state)
    {
    case State::Idle:
        //Transitions to chasing if player enters detection radius
        if (dist < m_detection_radius) {
            m_state = State::Chasing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
        }
        break;

    case State::Chasing:
        //Transitions to attacking if player is within attack range
        if (dist <= m_attack_range) {
            m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f)); //stops movement
            m_state = State::Attacking;
            m_object->animated_mesh()->switch_animation(m_anim_attack);
            m_attack_cooldown = 0.0f;
            m_damage_dealt = false;
            m_damage_signal = false;
        }
        else {
            chase_player((float)time_step); //continues chasing
        }
        break;

    case State::Attacking:
        anim_speed = 0.8f; //Boss' attacks are heavier and slower than the bezerker's one
        attack_player((float)time_step);
        break;
    }

    //Updates the animated mesh with current animation speed to avoid mismatch
    m_object->animated_mesh()->on_update(time_step * anim_speed);
}

//Handles movement toward the player
void Boss::chase_player(float dt)
{
    glm::vec3 dir = m_player_target->position() - m_object->position();
    dir.y = 0.0f; //Flattens movement to ground plane
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);

    m_object->set_forward(dir);
    float current_y = m_object->velocity().y;
    m_object->set_velocity(glm::vec3(dir.x * m_speed, current_y, dir.z * m_speed));

    // Calculate to face the player as done with the others
    float theta = atan2(m_object->forward().x, m_object->forward().z);
    m_object->set_rotation_amount(theta);
}

//Attack sequence, damage dealt and cooldowns
void Boss::attack_player(float dt)
{
    m_attack_cooldown += dt;

    //Checks for damage window in the middle of the animation
    if (m_attack_cooldown > 1.0f && m_attack_cooldown < 1.5f && !m_damage_dealt)
    {
        glm::vec3 to_player = m_player_target->position() - m_object->position();
        //Slightly larger hit check for the big boss
        if (glm::length(to_player) <= m_attack_range + 1.0f)
        {
            m_damage_dealt = true;
            m_damage_signal = true; // Signals to example_layer that damage should be applied
        }
    }

    if (m_attack_cooldown >= 3.0f) //Longer recovery time for boss
    {
        m_state = State::Chasing;
        m_object->animated_mesh()->switch_animation(m_anim_walk);
    }
}

//Handles taking damage and transitioning to dead state
void Boss::take_damage(float amount)
{
    m_health -= amount;
    if (m_health <= 0 && m_state != State::Dead)
    {
        m_state = State::Dead;
        m_object->set_velocity(glm::vec3(0.f)); //stops all movement
        m_object->animated_mesh()->switch_animation(m_anim_death);
        m_death_timer = 0.0f;
    }
}
