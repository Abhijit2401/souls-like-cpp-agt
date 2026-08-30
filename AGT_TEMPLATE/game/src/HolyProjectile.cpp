#include "pch.h"
#include "HolyProjectile.h"

HolyProjectile::HolyProjectile() {}
HolyProjectile::~HolyProjectile() {}

//Sets up the projectile's initial state
void HolyProjectile::initialise(engine::ref<engine::game_object> object)
{
    m_object = object;

    //Falls down on spawn
    m_object->set_forward(glm::vec3(0.0f, -1.0f, 0.0f));

    //Spins like a thrown weapon from the sky
    //(I wasnt able to make it rotate vertically for some reason or it was just bugging out so ive decided to spin it horizontally instead)
    m_object->set_rotation_axis(glm::vec3(0.0f, 4.0f, 4.0f));
    m_object->set_rotation_amount(0.0f);

    m_active = true;
}

void HolyProjectile::on_update(const engine::timestep& time_step)
{
    if (!m_active) return;

    float dt = (float)time_step;

    //Fall down
    glm::vec3 pos = m_object->position();
    pos.y -= m_speed * dt;
    m_object->set_position(pos);
    //Spin amount
    float current_rot = m_object->rotation_amount();
    m_object->set_rotation_amount(current_rot + 3.0f * dt);
    //Deactivates if it touches the floor
    if (pos.y <= 0.0f) {
        m_active = false;
    }
}

//checks for collision with the player and returns damage if hit
float HolyProjectile::check_collision(engine::ref<engine::game_object> player)
{
    if (!m_active) return 0.0f;

    float dist = glm::distance(m_object->position(), player->position());

    //Increased hit radius slightly since its spinning so the hitbox would be weird and too small
    if (dist < 1.2f) {
        m_active = false; //Deactivates on hit
        return m_damage;
    }
    return 0.0f;
}
