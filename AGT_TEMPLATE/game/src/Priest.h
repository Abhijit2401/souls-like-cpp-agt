#pragma once
#include <engine.h>

class Priest
{
public:
    Priest();
    ~Priest();

    //Initialises the priest object, position, and player target
    void initialise(engine::ref<engine::game_object> object,
        glm::vec3 position,
        engine::ref<engine::game_object> player_target);

    //AI LOGIC AND ANIMATION
    void on_update(const engine::timestep& time_step);

    engine::ref<engine::game_object> object() const { return m_object; }

    //HEALTH AND STATE
    void take_damage(float amount);
    bool is_dead() const { return m_health <= 0.0f; }

    //Signals to the layer when a projectile needs to be spawned
    bool should_spawn_projectile();

    //HEALTH PERCENT
    float get_health_percent() const { return m_health / 80.0f; }

    //Soul drop status for one-time drop on death
    bool souls_dropped() const { return m_souls_dropped; }
    void set_souls_dropped(bool dropped) { m_souls_dropped = dropped; }
    // Check if the priest has disappeared
    bool has_vanished() const { return m_vanished; }

    bool is_telegraphing_attack() const { return m_state == State::Attacking && m_attack_cooldown < 1.0f; }
    float get_telegraph_progress() const { return glm::clamp(m_attack_cooldown / 1.0f, 0.f, 1.f); }

private:
    //Helper to rotate the priest to face the player
    void face_player(float dt);

    engine::ref<engine::game_object> m_object;
    engine::ref<engine::game_object> m_player_target;

    //PRIEST STATS
    float m_health = 80.0f;
    float m_speed = 1.5f;
    float m_flee_speed = 1.0f;

    float m_attack_range = 15.0f; //Ranegd attack range (distance from player)
    float m_detection_radius = 25.0f;
    float m_flee_radius = 5.0f; // Distance from the player htat causes the priest to flee to a safe area

    enum class State { Idle, Chasing, Attacking, Fleeing, Dead };
    State m_state = State::Idle;

    //PRIEST ANIMATION INDEX
    uint32_t m_anim_idle = 0;
    uint32_t m_anim_walk = 1;
    uint32_t m_anim_attack = 2;
    uint32_t m_anim_death = 3;

    // Attack/holy projectile control
    float m_attack_cooldown = 0.0f;
    bool m_spawn_signal = false; //Signals a projectile drop in the layer
    bool m_has_spawned_this_attack = false;

    //DEAD STATE LOGIC 
    bool m_souls_dropped = false;
    float m_death_timer = 0.0f;
    bool m_vanished = false;
};
