#pragma once
#include <engine.h>
#include "Player.h"

class HolyProjectile
{
public:
    HolyProjectile();
    ~HolyProjectile();

    //Initialises the projectile with a game object
    void initialise(engine::ref<engine::game_object> object);
    //Updates the projectiles movement and rotation
    void on_update(const engine::timestep& time_step);

    //Returns damage amount if it hits the player, otherwise the damage is 0
    float check_collision(engine::ref<engine::game_object> player);

    engine::ref<engine::game_object> object() const { return m_object; }
    bool is_active() const { return m_active; }
    void set_active(bool active) { m_active = active; }

private:
    engine::ref<engine::game_object> m_object;
    bool m_active = true;
    float m_speed = 3.0f; //Speed for the falling holy holographic projectile
    float m_damage = 15.0f;
};
