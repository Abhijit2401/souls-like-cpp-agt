#pragma once
#include <engine.h>
#include "glm/gtx/rotate_vector.hpp"

class player
{
public:
    player();
    ~player();
    void initialise(engine::ref<engine::game_object> object);
    void on_update(const engine::timestep& time_step);

    engine::ref<engine::game_object> object() const { return m_object; }
    void turn(float angle);
    void update_camera(engine::perspective_camera& camera); //Positions and targets the camera

    void attack(); //Starts the attack sequence
    void roll();   //Starts the dash/roll sequence

    //STATS
    // Takes damage, includes roll immunity
    void take_damage(float amount);

    //Heal function
    void heal(float amount);

    float get_health() const { return m_health; }
    float get_max_health() const { return m_max_health; }
    bool is_attacking() const;
    bool is_rolling() const { return m_current_state == PlayerState::Rolling; }

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
    void set_mouse_sensitivity(float sensitivity) { s_mouse_sensitivity = sensitivity; }
    float get_mouse_sensitivity() const { return s_mouse_sensitivity; }

    int get_debug_anim_index() const { return m_anim_dash; }

private:
    float m_speed{ 0.f };
    float m_animation_timer; // Used to track attack/roll duration
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
    uint32_t m_anim_dash;

    //Roll/Dash stats
    glm::vec3 m_locked_roll_direction{ 0.f };
    float m_dash_angle = 0.0f;
    bool m_shift_pressed = false; //Tracks shift state to only trigger roll on press

    //CHAR STATS
    float m_health = 100.0f;
    float m_max_health = 100.0f;
    float m_stamina = 100.0f;
    float m_max_stamina = 100.0f;
    float m_stamina_regen = 18.0f; //amount of stamina gained per second

    float m_attack_damage = 20.0f;
    float m_run_speed = 4.0f;

    //POTION INVENTORY AMOUNT
    int m_num_potions = 0;
};
