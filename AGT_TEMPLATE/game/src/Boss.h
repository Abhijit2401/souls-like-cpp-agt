#pragma once
#include <engine.h>
#include "Player.h" 

class Boss
{
public:
    Boss();
    ~Boss();

    //Initialises the boss object, position, and player target
    void initialise(engine::ref<engine::game_object> object,
        glm::vec3 position,
        engine::ref<engine::game_object> player_target);

    //Updates the boss AI logic and animation
    void on_update(const engine::timestep& time_step);

    engine::ref<engine::game_object> object() const { return m_object; }

    //HEALTH AND STATE
    void take_damage(float amount);
    bool is_dead() const { return m_health <= 0.0f; }
    //Checks if the boss attack connected this frame
    bool check_hit_player();

    //Helper for the Boss HUD health bar
    float get_health_percent() const { return m_health / m_max_health; }
    //Checks if the boss has disappeared
    bool has_vanished() const { return m_vanished; }

private:
    //AI MOVEMENT AND ATTACK FUNCTIONS
    void chase_player(float dt);
    void attack_player(float dt);

    engine::ref<engine::game_object> m_object;
    engine::ref<engine::game_object> m_player_target;

    //BOSS STATS
    float m_health = 500.0f;
    float m_max_health = 500.0f;
    float m_speed = 1.5f;
    float m_damage = 20.0f; // Higher damage than usual enemies

    float m_attack_range = 3.5f; //larger range for big boss
    float m_detection_radius = 30.0f; // Detects you from way further away so hes basically always chasing

    enum class State { Idle, Chasing, Attacking, Dead };
    State m_state = State::Idle;

    // DEVIL ATTACK ANIMATIONS 
    uint32_t m_anim_attack = 0;
    uint32_t m_anim_death = 1;
    uint32_t m_anim_idle = 2;
    uint32_t m_anim_walk = 3;

    float m_attack_cooldown = 0.0f;
    bool m_damage_dealt = false;
    bool m_damage_signal = false;

    float m_death_timer = 0.0f;
    bool m_vanished = false;
};
