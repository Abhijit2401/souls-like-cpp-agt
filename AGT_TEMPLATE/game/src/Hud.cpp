#include "pch.h"
#include "Hud.h"
#include "platform/opengl/gl_shader.h"
#include <sstream>
#include <iomanip>

void Hud::initialise(engine::ref<engine::text_manager> text_manager)
{
    m_text_manager = text_manager;
}

void Hud::render_main_menu(const engine::ref<engine::shader>& text_shader,
    const engine::ref<engine::texture_2d>& intro_texture,
    const engine::ref<engine::mesh>& quad_mesh,
    int menu_selection,
    float mouse_sensitivity,
    float music_volume)
{
    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("has_texture", true);
    intro_texture->bind();
    glm::mat4 transform(1.0f);
    engine::renderer::submit(text_shader, quad_mesh, transform);
    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("has_texture", false);

    float width = (float)engine::application::window().width();
    float height = (float)engine::application::window().height();

    m_text_manager->render_text(text_shader, "medievalsoulsgameforadvancedgamesproject", 100.f, height / 2.f + 100.f, 1.0f, glm::vec4(1.f, 1.f, 0.f, 1.f));

    // Main menu selection text setup
    if (menu_selection == 0)
    {
        m_text_manager->render_text(text_shader, "> START", 100.f, height / 2.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
        m_text_manager->render_text(text_shader, "  SENSITIVITY", 100.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        m_text_manager->render_text(text_shader, "  VOLUME", 100.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
    }
    else if (menu_selection == 1)
    {
        m_text_manager->render_text(text_shader, "  START", 100.f, height / 2.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        m_text_manager->render_text(text_shader, "> SENSITIVITY", 100.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
        m_text_manager->render_text(text_shader, "  VOLUME", 100.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
    }
    else
    {
        m_text_manager->render_text(text_shader, "  START", 100.f, height / 2.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        m_text_manager->render_text(text_shader, "  SENSITIVITY", 100.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        m_text_manager->render_text(text_shader, "> VOLUME", 100.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
    }

    // Mouse sensitivity value
    std::stringstream sensitivity_calc;
    sensitivity_calc << std::fixed << std::setprecision(2) << mouse_sensitivity;
    std::string sens_text = "< " + sensitivity_calc.str() + " >";
    m_text_manager->render_text(text_shader, sens_text, 350.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));

    // Music volume value
    std::stringstream vol_calc;
    vol_calc << std::fixed << std::setprecision(2) << music_volume;
    std::string vol_text = "< " + vol_calc.str() + " >";
    m_text_manager->render_text(text_shader, vol_text, 350.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));

    // Controls list
    m_text_manager->render_text(text_shader, "Controls:", 10.f, height - 25.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "WASD: Move | L-Shift: Dash | R: Health Potion", 10.f, height - 50.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "Mouse: Move Camera", 10.f, height - 75.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "Space: Attack | E: Toggle Upgrade Menu", 10.f, height - 100.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "P: Spawn Enemy", 10.f, height - 125.f, 0.5f, glm::vec4(1.f));
}

void Hud::render_game_hud(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count)
{
    // HP red text
    std::stringstream hp_ss;
    hp_ss << "HP: " << (int)player.get_health() << "/" << (int)player.get_max_health();
    m_text_manager->render_text(text_shader, hp_ss.str(), 10.f, 25.f, 0.5f, glm::vec4(1.f, 0.2f, 0.2f, 1.f));

    // Stamina green text
    std::stringstream stm_ss;
    stm_ss << "Stamina: " << (int)player.get_stamina() << "/" << (int)player.get_max_stamina();
    m_text_manager->render_text(text_shader, stm_ss.str(), 10.f, 50.f, 0.5f, glm::vec4(0.f, 1.f, 0.f, 1.f));

    // Souls white text
    std::string soul_text = "Soul Fragments: " + std::to_string(soul_count);
    m_text_manager->render_text(text_shader, soul_text, 10.f, 75.f, 0.5f, glm::vec4(1.f, 1.f, 1.f, 1.f));

    // Potions blue text
    std::stringstream pot_ss;
    pot_ss << "Potions: " << player.get_potions();
    m_text_manager->render_text(text_shader, pot_ss.str(), 10.f, 100.f, 0.5f, glm::vec4(0.f, 1.f, 1.f, 1.f));
}

void Hud::render_floating_health_bar(const engine::ref<engine::shader>& text_shader,
    const engine::perspective_camera& camera,
    const glm::vec3& world_position,
    float health_percent,
    const glm::vec4& colour)
{
    glm::vec3 pos = world_position;
    pos.y += 2.5f;
    glm::vec4 clip_space = camera.projection_matrix() * camera.view_matrix() * glm::vec4(pos, 1.0f);
    if (clip_space.w <= 0.0f) return;

    glm::vec3 ndc = glm::vec3(clip_space) / clip_space.w;
    if (ndc.z < 0.0f || ndc.z > 1.0f) return;

    float screen_w = (float)engine::application::window().width();
    float screen_h = (float)engine::application::window().height();
    float x = (ndc.x + 1.0f) / 2.0f * screen_w;
    float y = (ndc.y + 1.0f) / 2.0f * screen_h;

    const int bars = 10;
    int fill = (int)(health_percent * bars);
    std::string bar = "[";
    for (int b = 0; b < bars; b++) bar += (b < fill) ? "|" : " ";
    bar += "]";

    m_text_manager->render_text(text_shader, bar, x - 30, y, 0.3f, colour);
}

void Hud::render_boss_health_bar(const engine::ref<engine::shader>& text_shader, float health_percent)
{
    const int bars = 20;
    int fill = (int)(health_percent * bars);
    std::string bar = "[";
    for (int b = 0; b < bars; b++) bar += (b < fill) ? "|" : " ";
    bar += "]";

    float screen_w = (float)engine::application::window().width();
    float screen_h = (float)engine::application::window().height();
    m_text_manager->render_text(text_shader, "BOSS: DEVIL", screen_w / 2 - 100, screen_h - 40, 0.5f, glm::vec4(1.f, 0.5f, 0.f, 1.f));
    m_text_manager->render_text(text_shader, bar, screen_w / 2 - 150, screen_h - 70, 0.5f, glm::vec4(1.f, 0.f, 0.f, 1.f));
}

void Hud::render_pause_menu(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    m_text_manager->render_text(text_shader, "Pause Upgrade Menu", w / 2 - 150, h / 2 + 100, 1.0f, glm::vec4(1.f, 1.f, 0.f, 1.f));

    std::stringstream souls;
    souls << "Available Souls: " << soul_count;
    m_text_manager->render_text(text_shader, souls.str(), w / 2 - 100, h / 2 + 50, 0.7f, glm::vec4(1.f));

    std::stringstream dmg;
    dmg << "[1] Increase Damage (Cost: 5) Current: " << (int)player.get_damage();
    m_text_manager->render_text(text_shader, dmg.str(), w / 2 - 200, h / 2, 0.6f, glm::vec4(1.f, 0.2f, 0.2f, 1.f));

    std::stringstream spd;
    spd << "[2] Increase Attack Speed (Cost: 5) Current: " << (int)player.get_speed();
    m_text_manager->render_text(text_shader, spd.str(), w / 2 - 200, h / 2 - 40, 0.6f, glm::vec4(0.2f, 0.2f, 1.f, 1.f));

    std::stringstream pot;
    pot << "[3] Buy Health Potion (Cost: 10) Owned: " << player.get_potions();
    m_text_manager->render_text(text_shader, pot.str(), w / 2 - 200, h / 2 - 80, 0.6f, glm::vec4(0.f, 1.f, 0.f, 1.f));

    m_text_manager->render_text(text_shader, "Press E to Resume", w / 2 - 120, h / 2 - 140, 0.5f, glm::vec4(0.8f));
}
