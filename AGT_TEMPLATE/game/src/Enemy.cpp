#include "pch.h"
#include "Enemy.h"
#include "Player.h" 

Enemy::Enemy() {}
Enemy::~Enemy() {}

void Enemy::initialise(engine::ref<engine::game_object> object,
    glm::vec3 position,
    engine::ref<engine::game_object> player_target)
{
    m_object = object;
    m_player_target = player_target;

    //RESET PHYSICAL STATE
    glm::vec3 correct_position = position;
    correct_position.y = 0.09f; //Fixes the spawn height
    m_object->set_position(correct_position);

    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));
    m_object->set_velocity(glm::vec3(0.0f, -5.0f, 0.0f));

    //RESETS ENEMY STATS which we need to properly do respawns when its initialised
    m_health = 100.0f;
    m_souls_dropped = false;  //Allows souls ot be dropped on death again
    m_vanished = false;
    m_death_timer = 0.0f;
    m_damage_dealt = false;
    m_damage_signal = false;

    //Random speed variation so they don't all move the same which is why some are way slower than others
    float random_variation = ((float)rand() / (float)RAND_MAX) * 0.3f;
    m_speed = 0.3f + random_variation;

    // BERZERKER ANIMATION INDEX
    m_anim_attack = 0;
    m_anim_idle = 3;
    m_anim_walk = 4;
    m_anim_death = 1;

    //reset animation
    m_object->animated_mesh()->set_default_animation(m_anim_idle);
    m_object->animated_mesh()->switch_animation(m_anim_idle);

    //randomises their animation start time
    float random_time_offset = ((float)rand() / (float)RAND_MAX) * 2.0f;
    m_object->animated_mesh()->on_update(random_time_offset);

    m_state = State::Idle;
}

//checks if the damage signal was set during the attack adn then resets it
bool Enemy::check_hit_player() {
    if (m_damage_signal) {
        m_damage_signal = false;
        return true;
    }
    return false;
}

void Enemy::on_update(const engine::timestep& time_step)
{
    // Prevents the dead body from snapping back to the floor
    if (m_state != State::Dead) {
        if (m_object->position().y < 0.09f) {
            glm::vec3 current_pos = m_object->position();
            m_object->set_position(glm::vec3(current_pos.x, 0.09f, current_pos.z));
        }
    }

    //Always apply visual offset and lock rotation
    m_object->set_offset(glm::vec3(0.0f, -9.0f, 0.0f));
    m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));

    // DEAD STATE
    if (m_state == State::Dead) {
        m_object->animated_mesh()->on_update(time_step);

        // Wait for death animation to finish then vanishes
        if (!m_vanished) {
            m_death_timer += (float)time_step;

            //Calculatest eh animation duration
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

    //ALIVE AI LOGIC
    float dist = glm::distance(m_object->position(), m_player_target->position());

    //Animation speed based on move speed to make it match
    float anim_speed = m_speed * 1.2f;

    switch (m_state)
    {
    case State::Idle:
        anim_speed = 0.6f;
        //transitions to chasing if player enters detection radius
        if (dist < m_detection_radius) {
            m_state = State::Chasing;
            m_object->animated_mesh()->switch_animation(m_anim_walk);
        }
        break;

    case State::Chasing:
        //Transitions to attacking if player is within attack range
        if (dist <= m_attack_range) {
            m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f)); //Stops movement
            m_state = State::Attacking;
            m_object->animated_mesh()->switch_animation(m_anim_attack);
            m_attack_cooldown = 0.0f;
            m_damage_dealt = false;
            m_damage_signal = false;
        }
        //Transitions back to idle if player gets too far away
        else if (dist > m_detection_radius * 1.5f) {
            m_state = State::Idle;
            m_object->animated_mesh()->switch_animation(m_anim_idle);
            m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f));
        }
        else {
            chase_player((float)time_step); //Otehrwise continues chasing
        }
        break;

    case State::Attacking:
        anim_speed = 1.0f; //Attack speed
        attack_player((float)time_step);
        break;
    }

    //Updates the animated mesh with current animation speed
    m_object->animated_mesh()->on_update(time_step * anim_speed);
}

//Handles movement and rotation toward the player
void Enemy::chase_player(float dt)
{
    glm::vec3 dir = m_player_target->position() - m_object->position();
    dir.y = 0.0f;//Keeps movement on the surface
    if (glm::length(dir) > 0.01f) dir = glm::normalize(dir);
    else dir = glm::vec3(0, 0, 1);//if stationary default forward

    m_object->set_forward(dir);
    float current_y = m_object->velocity().y;
    m_object->set_velocity(glm::vec3(dir.x * m_speed, current_y, dir.z * m_speed));

    //Calculates rotation to face the player
    float theta = atan2(m_object->forward().x, m_object->forward().z);
    m_object->set_rotation_amount(theta);
}

//Handles the attack sequence, damage dealt and cooldown
void Enemy::attack_player(float dt)
{
    m_attack_cooldown += dt;

    // Check for damage window in the middle of the attack animation
    if (m_attack_cooldown > 1.5f && m_attack_cooldown < 2.0f && !m_damage_dealt)
    {
        glm::vec3 to_player = m_player_target->position() - m_object->position();
        float dist = glm::length(to_player);
        if (dist > 0.01f) to_player = glm::normalize(to_player);

        float dot_prod = glm::dot(m_object->forward(), to_player);

        //Check distance AND if the player is in front (dot product check)
        if (dist <= m_attack_range + 1.5f && dot_prod > 0.5f)
        {
            m_damage_dealt = true;
            m_damage_signal = true; // Signal to example_layer that damage should be applied of this is all true
        }
    }

    if (m_attack_cooldown >= 2.5f) //End of attack recovery
    {
        m_state = State::Chasing;
        m_object->animated_mesh()->switch_animation(m_anim_walk);
    }
}

//Handles taking damage and transitioning to the dead state
void Enemy::take_damage(float amount)
{
    m_health -= amount;
    if (m_health <= 0 && m_state != State::Dead)
    {
        m_state = State::Dead;
        m_object->set_velocity(glm::vec3(0.f)); //Stops all movement
        m_object->animated_mesh()->switch_animation(m_anim_death);
        m_death_timer = 0.0f;
    }
}
