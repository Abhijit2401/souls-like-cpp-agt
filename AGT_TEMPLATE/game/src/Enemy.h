#pragma once
#include <engine.h>

//Frward declaration instead of include to prevent infinite loop
class player;

class Enemy
{
public:
    Enemy();
    ~Enemy();

    //Initialises the enemy object, position, and player target which is used for respawn logic
    void initialise(engine::ref<engine::game_object> object,
        glm::vec3 position,
        engine::ref<engine::game_object> player_target);

    //Updates the enemy AI logic and animation
    void on_update(const engine::timestep& time_step);

    engine::ref<engine::game_object> object() const { return m_object; }

    //HEALTH AND STATES
    void take_damage(float amount);
    bool is_dead() const { return m_health <= 0.0f; }

    //Returns true if damage should be applied to player this frame
    bool check_hit_player();

    //Helper for health bar
    float get_health_percent() const { return m_health / 100.0f; }
    //  Soul drop status for one-time drop on death
    bool souls_dropped() const { return m_souls_dropped; }
    void set_souls_dropped(bool dropped) { m_souls_dropped = dropped; }
    // Checks if the enemy has disappeared after the death animation
    bool has_vanished() const { return m_vanished; }

private:
    //AI MOVEMENT AND ATTACK
    void chase_player(float dt);
    void attack_player(float dt);

    engine::ref<engine::game_object> m_object;
    engine::ref<engine::game_object> m_player_target;

    float m_health = 100.0f;

    float m_speed = 0.6f; //Base speed

    float m_attack_range = 2.0f;
    float m_detection_radius = 15.0f;
    float m_damage = 10.0f;

    enum class State { Idle, Chasing, Attacking, Dead };
    State m_state = State::Idle;

    //BEZERKER ANIMATION INDEXES
    uint32_t m_anim_idle = 1;
    uint32_t m_anim_walk = 4;
    uint32_t m_anim_attack = 0;
    uint32_t m_anim_death = 2;

    float m_attack_cooldown = 0.0f;
    bool m_damage_dealt = false;
    bool m_damage_signal = false;
    bool m_souls_dropped = false;
    float m_death_timer = 0.0f;
    bool m_vanished = false;
};
