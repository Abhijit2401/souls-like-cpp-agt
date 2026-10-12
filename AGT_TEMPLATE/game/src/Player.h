#pragma once
#include <engine.h>
#include "glm/gtx/rotate_vector.hpp"
#include "AttackArc.h"

class player
{
public:
    player();
    ~player();
    void initialise(engine::ref<engine::game_object> object);
    void on_update(const engine::timestep& time_step, bool has_lock_on_target = false, const glm::vec3& lock_on_target_position = glm::vec3(0.f));

    engine::ref<engine::game_object> object() const { return m_object; }
    void turn(float angle);
    void update_camera(engine::perspective_camera& camera, const engine::timestep& time_step);
    void attack(); //Starts the attack sequence
    void roll(const glm::vec3& direction);

    //STATS
    // Takes damage, includes roll immunity
    void take_damage(float amount);

    //Heal function
    void heal(float amount);

    float get_health() const { return m_health; }
    float get_max_health() const { return m_max_health; }
    bool is_attacking() const;
    float get_attack_progress() const;
    //Area the current swing covers based on where the player is and is facing
    AttackArc get_attack_arc() const;
    //Goes from 0 to 1 from the start of the swing until the sword hits (used for the indicator)
    float get_attack_windup_progress() const;
    bool has_attack_connected() const { return m_attack_hit_fired; }
    //Returns true once per swing on the frame the sword hits so damage only gets applied once
    bool consume_attack_hit();
    bool is_rolling() const { return m_current_state == PlayerState::Rolling; }
    bool is_dead() const { return m_current_state == PlayerState::Dead; }
    //True once the death animation is finished so the defeat screen can show
    bool is_death_sequence_finished() const { return is_dead() && m_death_timer >= m_death_duration + m_death_linger; }
    //Transform used for rendering the player which includes the roll flip
    glm::mat4 render_transform() const;
    void respawn();
    void toggle_invincible() { m_invincible = !m_invincible; }
    bool is_invincible() const { return m_invincible; }

    //POTIONS
    int get_potions() const { return m_num_potions; }
    void add_potion(int amount) { m_num_potions += amount; }
    bool use_potion(); //Consumes one potion and heals, returns true if used

    //Getter for upgrades in the pause menu
    float get_damage() const { return m_attack_damage; }
    float get_speed() const { return m_run_speed; }
    float get_stamina() const { return m_stamina; }
    float get_max_stamina() const { return m_max_stamina; }

    //Setters for the upgrades
    void increase_damage(float amount) { m_attack_damage += amount; }
    void increase_speed(float amount) { m_run_speed += amount; }
    void increase_max_health(float amount) { m_max_health += amount; m_health = m_max_health; }
    void increase_max_stamina(float amount) { m_max_stamina += amount; }

    //Camera/Control settings
    void set_mouse_sensitivity(float sensitivity) { s_mouse_sensitivity = glm::clamp(sensitivity, 0.05f, 2.0f); }
    float get_mouse_sensitivity() const { return s_mouse_sensitivity; }
    void trigger_camera_shake(float duration, float magnitude);
private:
    float m_speed{ 0.f };
    float m_animation_timer; // Used to track attack/roll duration
    float m_attack_duration = 1.0f;
    float m_attack_hit_fraction = 0.4f; //How far into the attack animation the sword hits
    bool m_attack_hit_fired = false;
    bool m_attack_hit_pending = false;
    float m_attack_arc_radius = 2.8f;
    float m_attack_arc_half_angle = 70.0f;
    float m_attack_arc_height = 0.8f; //How high the sword is from the feet
    float m_attack_arc_roll = 15.0f;  //Slightly diagonal to match the knights swing
    engine::ref<engine::game_object> m_object;

    //CAMERA SETTINGS
    float m_camera_yaw{ 0.f };
    float m_camera_pitch{ 20.f };
    float m_camera_distance{ 4.f };

    glm::vec3 m_camera_forward{ 0.f };
    glm::vec3 m_camera_right{ 0.f };
    glm::vec3 m_camera_up{ 0.f };

    float s_mouse_sensitivity{ 0.5f };

    //FSM STATES
    enum class PlayerState { Idle, Walking, Attacking, Rolling, Dead };
    PlayerState m_current_state;

    //ANIMATION INDEXES
    uint32_t m_anim_idle;
    uint32_t m_anim_walk;
    uint32_t m_anim_attack;
    uint32_t m_anim_death;

    glm::vec3 m_last_movement_direction{ 0.f, 0.f, -1.f };
    //Roll stats (the knight model has no roll animation so the flip is done in code in render_transform)
    glm::vec3 m_locked_roll_direction{ 0.f };
    float m_dash_angle = 0.0f;
    float m_roll_duration = 0.55f; //Also how long the invincibility lasts
    float m_roll_pivot_height = 0.55f; //Point the flip rotates around, about the middle of the body (the physics position is at the feet)
    float m_roll_hop_height = 0.2f;    //Small hop in the middle of the roll so it doesn't go into the ground

    //Death sequence
    float m_death_timer = 0.0f;
    float m_death_duration = 2.0f; //Gets set to the death animation length when the player dies
    float m_death_linger = 0.75f;  //Extra wait on the last frame before the defeat screen shows
    bool m_shift_pressed = false; //Tracks shift state to only trigger roll on press

    //CAMERA SHAKE
    float m_shake_duration = 0.0f;
    float m_shake_time_left = 0.0f;
    float m_shake_magnitude = 0.0f;

    //CHAR STATS
    float m_health = 100.0f;
    float m_max_health = 100.0f;
    float m_stamina = 100.0f;
    float m_max_stamina = 100.0f;
    float m_stamina_regen = 18.0f;

    float m_attack_damage = 20.0f;
    float m_run_speed = 4.0f;

    float m_acceleration = 20.0f;
    float m_deceleration = 25.0f;
    float m_base_fov = 45.0f;
    float m_current_fov = 45.0f;
    float m_fov_lerp_speed = 8.0f;

    //POTION INVENTORY AMOUNT
    int m_num_potions = 0;
    bool m_invincible = false;
};
