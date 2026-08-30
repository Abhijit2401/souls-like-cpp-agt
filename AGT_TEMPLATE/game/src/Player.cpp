#include "pch.h"
#include "Player.h"
#include "engine/core/input.h"
#include "engine/key_codes.h"
#include <glm/gtc/matrix_transform.hpp> 

player::player()
{
    m_animation_timer = 0.0f;
    m_speed = 0.f;

    //CAMERA SETUP
    m_camera_yaw = 0.f;
    m_camera_pitch = 20.f;
    m_camera_distance = 6.f;
    s_mouse_sensitivity = 0.5f;

    //INTIAL CHARACTER STATS
    m_max_health = 100.0f;
    m_health = m_max_health;
    m_max_stamina = 100.0f;
    m_stamina = m_max_stamina;
    m_attack_damage = 20.0f;
    m_run_speed = 4.0f;
    m_num_potions = 0; // Starts with no potions (can only be bought from upgrade pause menu

    m_current_state = PlayerState::Idle;

    m_camera_forward = glm::vec3(0.f);
    m_camera_right = glm::vec3(0.f);
    m_camera_up = glm::vec3(0.f);

    m_dash_angle = 0.0f;
}

player::~player() {}

void player::initialise(engine::ref<engine::game_object> object)
{
    m_object = object;
    m_object->set_forward(glm::vec3(0.f, 0.f, -1.f));
    m_object->set_position(glm::vec3(0.f, 0.09f, 10.f)); // Starting spawn position

    //PLAYER ANIMATION INDEX
    m_anim_attack = 0;
    m_anim_idle = 3;
    m_anim_walk = 4;
    m_anim_dash = 2;

    m_object->animated_mesh()->set_default_animation(m_anim_idle);
    m_object->animated_mesh()->switch_animation(m_anim_idle);
    m_current_state = PlayerState::Idle;

    m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));
    m_object->set_rotation_amount(0.0f);
}

bool player::is_attacking() const {
    return m_current_state == PlayerState::Attacking;
}

//Handles incoming damage
void player::take_damage(float amount)
{
    // Rolling/Dodging/Dashing or whatever u want to call it to implement the dark souls dodge effect which causes invulnerability
    if (m_current_state == PlayerState::Rolling) return;

    m_health -= amount;
    if (m_health <= 0.0f) {
        m_health = 0.0f;
        m_current_state = PlayerState::Dead;
        m_object->animated_mesh()->switch_animation(m_anim_dash);
    }
}

//Handles healing using the potion
void player::heal(float amount)
{
    if (m_current_state == PlayerState::Dead) return;

    m_health += amount;
    if (m_health > m_max_health) m_health = m_max_health;
}

//If the player uses the potion
bool player::use_potion()
{
    //Heal 50 HP if we have potions and the player isnt at full HP
    if (m_num_potions > 0 && m_health < m_max_health)
    {
        m_num_potions--;
        heal(50.0f);
        return true;
    }
    return false;
}

void player::on_update(const engine::timestep& time_step)
{
    if (m_current_state == PlayerState::Dead) return;

    //STAMINA REGEN
    //Only regenerate if not rolling or attacking just like in souls games
    if (m_current_state != PlayerState::Rolling && m_current_state != PlayerState::Attacking)
    {
        m_stamina += m_stamina_regen * (float)time_step;
        if (m_stamina > m_max_stamina) m_stamina = m_max_stamina;
    }

    //CAMERA CALCULATION
    auto [mouse_delta_x, mouse_delta_y] = engine::input::mouse_position();
    m_camera_yaw += mouse_delta_x * s_mouse_sensitivity;
    m_camera_pitch += mouse_delta_y * s_mouse_sensitivity;

    // Clamps the camera pitch so it doesn't look too far up or down as requested to fix from feedback from first milestone
    if (m_camera_pitch > 30.f) m_camera_pitch = 30.f;
    if (m_camera_pitch < -89.f) m_camera_pitch = -89.f;

    // Calculates the new camera direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(m_camera_yaw)) * cos(glm::radians(m_camera_pitch));
    front.y = sin(glm::radians(m_camera_pitch));
    front.z = sin(glm::radians(m_camera_yaw)) * cos(glm::radians(m_camera_pitch));

    if (glm::length(front) > 0.001f) m_camera_forward = glm::normalize(front);
    m_camera_right = glm::normalize(glm::cross(m_camera_forward, glm::vec3(0.f, 1.f, 0.f)));
    m_camera_up = glm::normalize(glm::cross(m_camera_right, m_camera_forward));

    //STATE MACHINE

    //ATTACKING STATE
    if (m_current_state == PlayerState::Attacking)
    {
        m_animation_timer -= (float)time_step;
        m_object->animated_mesh()->on_update(time_step);
        m_object->set_offset(glm::vec3(0.f, -9.0f, 0.f)); // Keep visual offset

        if (m_animation_timer < 0.0f)
        {
            // Exit attack state
            m_object->animated_mesh()->switch_root_movement(false);
            m_object->animated_mesh()->switch_animation(m_anim_idle);
            m_current_state = PlayerState::Idle;
        }
    }
    //ROLLING STATE (Invulnerability)
    else if (m_current_state == PlayerState::Rolling)
    {
        m_animation_timer -= (float)time_step;

        float dash_speed = m_run_speed * 3.0f;
        glm::vec3 current_vel = m_object->velocity();
        //Applies a constant velocity in the locked roll direction just like in every souls game
        m_object->set_velocity(glm::vec3(m_locked_roll_direction.x * dash_speed, current_vel.y, m_locked_roll_direction.z * dash_speed));

        m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));
        m_object->set_rotation_amount(m_dash_angle);

        m_object->set_offset(glm::vec3(0.f, -9.0f, 0.f));
        m_object->animated_mesh()->on_update(time_step * 3.0f); //Speed up its animation

        if (m_animation_timer < 0.0f)
        {
            //Exit rolling state
            m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f)); //Stop horizontal movement
            m_object->animated_mesh()->switch_root_movement(false);
            m_object->animated_mesh()->switch_animation(m_anim_idle);
            m_current_state = PlayerState::Idle;
        }
    }
    //IDLE/WALKING STATE
    else
    {
        m_object->set_offset(glm::vec3(0.f, -9.0f, 0.f));
        m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));

        //Get 2D (ground plane) camera directions
        glm::vec3 cam_forward_2d = glm::normalize(glm::vec3(m_camera_forward.x, 0.f, m_camera_forward.z));
        glm::vec3 cam_right_2d = glm::normalize(glm::vec3(m_camera_right.x, 0.f, m_camera_right.z));

        //Calculate raw input movement direction (WASD)
        glm::vec3 move_direction(0.f);
        if (engine::input::key_pressed(engine::key_codes::KEY_W)) move_direction += cam_forward_2d;
        if (engine::input::key_pressed(engine::key_codes::KEY_S)) move_direction -= cam_forward_2d;
        if (engine::input::key_pressed(engine::key_codes::KEY_A)) move_direction -= cam_right_2d;
        if (engine::input::key_pressed(engine::key_codes::KEY_D)) move_direction += cam_right_2d;

        if (glm::length(move_direction) > 0.f)
        {
            m_speed = m_run_speed;
            m_object->set_forward(glm::normalize(move_direction));

            if (m_current_state != PlayerState::Walking)
            {
                m_object->animated_mesh()->switch_animation(m_anim_walk);
                m_current_state = PlayerState::Walking;
            }

            //Rotates the player model to face the direction of movement
            float theta = atan2(m_object->forward().x, m_object->forward().z);
            m_object->set_rotation_amount(theta);
        }
        else
        {
            //Stops movement if no keys are pressed
            m_speed = 0.f;
            if (m_current_state != PlayerState::Idle)
            {
                m_object->animated_mesh()->switch_animation(m_anim_idle);
                m_current_state = PlayerState::Idle;
            }
        }

        //Checks if attack key space bar is pressed
        if (engine::input::key_pressed(engine::key_codes::KEY_SPACE))
            attack();

        //Checks input for roll input key which is L shift
        bool shift_down = engine::input::key_pressed(engine::key_codes::KEY_LEFT_SHIFT);
        if (shift_down && !m_shift_pressed)
        {
            roll();
        }
        m_shift_pressed = shift_down; //Updates tracking variable for roll trigger

        //Applies final velocity vector (movement + gravity)
        glm::vec3 current_y_velocity = m_object->velocity();
        glm::vec3 velocity_vector = glm::vec3(0.f);
        if (m_speed > 0.f) velocity_vector = m_object->forward() * m_speed;
        m_object->set_velocity(glm::vec3(velocity_vector.x, current_y_velocity.y, velocity_vector.z));

        //Fixes rotation in case the forward vector was updated
        float theta = atan2(m_object->forward().x, m_object->forward().z);
        m_object->set_rotation_axis(glm::vec3(0.f, 1.f, 0.f));
        m_object->set_rotation_amount(theta);
        m_object->animated_mesh()->on_update(time_step);
    }
}

void player::turn(float angle) {
    m_object->set_forward(glm::rotate(m_object->forward(), angle, glm::vec3(0.f, 1.f, 0.f)));
}

//Updates the 3D camera position and view matrix
void player::update_camera(engine::perspective_camera& camera) {
    if (!m_object) return;
    //Camera targets player's neck height (player_pos + 1m up)
    glm::vec3 player_pos = m_object->position() + glm::vec3(0.f, 1.f, 0.f);
    //Camera position is behind the player and is offset by distance and direction to mimic a souls like camera
    glm::vec3 cam_pos = player_pos - m_camera_forward * m_camera_distance;

    //FLOOR CLAMP FIX AS REQUESTED
    //Ensures camera never goes below 0.5 meters
    if (cam_pos.y < 0.5f) {
        cam_pos.y = 0.5f;
    }

    camera.set_view_matrix(cam_pos, player_pos);
}

//Initiates the attack sequence
void player::attack() {
    if (m_stamina < 15.0f) return; //Checks stamina cost
    m_stamina -= 15.0f;

    m_object->animated_mesh()->switch_root_movement(true); // Allow animation to move root bone
    m_object->animated_mesh()->switch_animation(m_anim_attack);
    m_current_state = PlayerState::Attacking;
    m_speed = 0.0f; //Stops horizontal movement
    m_object->set_velocity(glm::vec3(0.f, m_object->velocity().y, 0.f));

    //Calculates attack duration from animation data
    float anim_dur = 1.0f;
    if (m_object->animated_mesh()->animations().size() > m_anim_attack)
        anim_dur = (float)m_object->animated_mesh()->animations().at(m_anim_attack)->mDuration / 30.0f;

    m_animation_timer = anim_dur; //Sets timer for state exit
}

//Initiates the Roll/Dash sequence
void player::roll() {
    if (m_stamina < 20.0f) return; //Checks stamina cost
    m_stamina -= 20.0f;

    m_object->animated_mesh()->switch_root_movement(true);
    m_object->animated_mesh()->switch_animation(m_anim_dash);

    m_current_state = PlayerState::Rolling;
    m_locked_roll_direction = m_object->forward(); //Locks the roll direction at start

    // Defaults roll direction if standing still
    if (glm::length(m_locked_roll_direction) < 0.01f)
        m_locked_roll_direction = glm::vec3(0, 0, -1);

    m_speed = 0.0f;

    m_dash_angle = atan2(m_locked_roll_direction.x, m_locked_roll_direction.z);

    glm::vec3 v = m_object->velocity();
    m_object->set_velocity(glm::vec3(v.x, 2.0f, v.z)); //Adds small vertical jump to roll to give it some distinction

    m_animation_timer = 0.5f; //Invulnerability period
}
