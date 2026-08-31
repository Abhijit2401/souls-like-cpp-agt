#include "pch.h"
#include "Hud.h"
#include "platform/opengl/gl_shader.h"
#include <sstream>
#include <iomanip>

void Hud::initialise(engine::ref<engine::text_manager> text_manager)
{
    m_text_manager = text_manager;
    std::vector<engine::mesh::vertex> quad_vertices
    {
        { {0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f} },
        { {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f} },
        { {1.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f} },
        { {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f} },
    };
    const std::vector<uint32_t> quad_indices{ 0, 1, 2, 0, 2, 3 };
    m_ui_quad = engine::mesh::create(quad_vertices, quad_indices);
    m_bar_material = engine::material::create(1.0f, glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f), 1.0f);
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
    m_text_manager->render_text(text_shader, "Mouse: Move Camera | Middle Click: Lock On", 10.f, height - 75.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "Left Click: Attack | E: Toggle Upgrade Menu", 10.f, height - 100.f, 0.5f, glm::vec4(1.f));
    m_text_manager->render_text(text_shader, "P: Spawn Enemy", 10.f, height - 125.f, 0.5f, glm::vec4(1.f));
}

void Hud::render_game_hud(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count)
{
    std::stringstream hp_ss;
    hp_ss << (int)player.get_health() << "/" << (int)player.get_max_health();
    m_text_manager->render_text(text_shader, hp_ss.str(), 200.f, 120.f, 0.4f, glm::vec4(1.f, 0.4f, 0.4f, 1.f));

    std::stringstream stm_ss;
    stm_ss << (int)player.get_stamina() << "/" << (int)player.get_max_stamina();
    m_text_manager->render_text(text_shader, stm_ss.str(), 200.f, 98.f, 0.4f, glm::vec4(0.4f, 1.f, 0.4f, 1.f));

    // Souls white text
    std::string soul_text = "Soul Fragments: " + std::to_string(soul_count);
    m_text_manager->render_text(text_shader, soul_text, 10.f, 75.f, 0.5f, glm::vec4(1.f, 1.f, 1.f, 1.f));

    // Potions blue text
    std::stringstream pot_ss;
    pot_ss << "Potions: " << player.get_potions();
    m_text_manager->render_text(text_shader, pot_ss.str(), 10.f, 50.f, 0.5f, glm::vec4(0.f, 1.f, 1.f, 1.f));
}

void Hud::render_bar(const engine::ref<engine::shader>& mesh_shader,
    float x, float y, float width, float height,
    float fill_percent, const glm::vec3& fill_colour)
{
    float screen_w = (float)engine::application::window().width();
    float screen_h = (float)engine::application::window().height();
    auto to_ndc_x = [&](float px) { return (px / screen_w) * 3.2f - 1.6f; };
    auto to_ndc_y = [&](float py) { return (py / screen_h) * 1.8f - 0.9f; };

    float ndc_x = to_ndc_x(x);
    float ndc_y = to_ndc_y(y);
    float ndc_w = (width / screen_w) * 3.2f;
    float ndc_h = (height / screen_h) * 1.8f;
    glm::mat4 bg_transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(ndc_x, ndc_y, 0.f)), glm::vec3(ndc_w, ndc_h, 1.f));
    m_bar_material->set_ambient(glm::vec3(0.08f, 0.08f, 0.08f));
    m_bar_material->set_transparency(0.7f);
    m_bar_material->submit(mesh_shader);
    engine::renderer::submit(mesh_shader, m_ui_quad, bg_transform);
    float clamped_fill = glm::clamp(fill_percent, 0.0f, 1.0f);
    if (clamped_fill > 0.0f)
    {
        glm::mat4 fill_transform = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(ndc_x, ndc_y, 0.f)), glm::vec3(ndc_w * clamped_fill, ndc_h, 1.f));
        m_bar_material->set_ambient(fill_colour);
        m_bar_material->set_transparency(0.95f);
        m_bar_material->submit(mesh_shader);
        engine::renderer::submit(mesh_shader, m_ui_quad, fill_transform);
    }
}
void Hud::render_player_bars(const engine::ref<engine::shader>& mesh_shader, const player& player)
{
    float hp_pct = player.get_max_health() > 0.f ? player.get_health() / player.get_max_health() : 0.f;
    float stamina_pct = player.get_max_stamina() > 0.f ? player.get_stamina() / player.get_max_stamina() : 0.f;

    render_bar(mesh_shader, 10.f, 118.f, 180.f, 16.f, hp_pct, glm::vec3(0.85f, 0.15f, 0.15f));
    render_bar(mesh_shader, 10.f, 96.f, 180.f, 16.f, stamina_pct, glm::vec3(0.15f, 0.85f, 0.15f));
}
void Hud::render_floating_bar(const engine::ref<engine::shader>& mesh_shader,
    const engine::perspective_camera& camera,
    const glm::vec3& world_position,
    float health_percent,
    const glm::vec3& fill_colour)
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

    render_bar(mesh_shader, x - 30.f, y, 60.f, 8.f, health_percent, fill_colour);
}

void Hud::render_boss_bar_graphic(const engine::ref<engine::shader>& mesh_shader, float health_percent)
{
    float screen_w = (float)engine::application::window().width();
    float screen_h = (float)engine::application::window().height();
    float bar_width = 400.f;
    render_bar(mesh_shader, screen_w / 2.f - bar_width / 2.f, screen_h - 70.f, bar_width, 20.f, health_percent, glm::vec3(0.8f, 0.1f, 0.1f));
}

void Hud::render_boss_name_text(const engine::ref<engine::shader>& text_shader)
{
    float screen_w = (float)engine::application::window().width();
    float screen_h = (float)engine::application::window().height();
    m_text_manager->render_text(text_shader, "BOSS: DEVIL", screen_w / 2 - 100, screen_h - 40, 0.5f, glm::vec4(1.f, 0.5f, 0.f, 1.f));
}

void Hud::render_dim_overlay(const engine::ref<engine::shader>& mesh_shader,
    const engine::ref<engine::mesh>& quad_mesh,
    const engine::ref<engine::material>& dim_material)
{
    engine::render_command::disable_depth_test();

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
    dim_material->submit(mesh_shader);
    glm::mat4 transform(1.0f);
    engine::renderer::submit(mesh_shader, quad_mesh, transform);

    engine::render_command::enable_depth_test();
}

void Hud::render_menu_line(const engine::ref<engine::shader>& text_shader, const std::string& text, float x, float y, bool selected)
{
    std::string prefix = selected ? "> " : "  ";
    glm::vec4 colour = selected ? glm::vec4(1.f, 1.f, 0.f, 1.f) : glm::vec4(1.f);
    m_text_manager->render_text(text_shader, prefix + text, x, y, 0.6f, colour);
}

void Hud::render_pause_menu(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count, int pause_selection, float music_volume)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    m_text_manager->render_text(text_shader, "Paused", w / 2 - 70, h / 2 + 160, 1.0f, glm::vec4(1.f, 1.f, 0.f, 1.f));

    std::stringstream souls;
    souls << "Available Souls: " << soul_count;
    m_text_manager->render_text(text_shader, souls.str(), w / 2 - 220, h / 2 + 115, 0.6f, glm::vec4(1.f));

    std::stringstream dmg;
    dmg << "Increase Damage (Cost: 5) Current: " << (int)player.get_damage();
    render_menu_line(text_shader, dmg.str(), w / 2 - 220, h / 2 + 70, pause_selection == 0);

    std::stringstream spd;
    spd << "Increase Attack Speed (Cost: 5) Current: " << (int)player.get_speed();
    render_menu_line(text_shader, spd.str(), w / 2 - 220, h / 2 + 35, pause_selection == 1);

    std::stringstream pot;
    pot << "Buy Health Potion (Cost: 10) Owned: " << player.get_potions();
    render_menu_line(text_shader, pot.str(), w / 2 - 220, h / 2, pause_selection == 2);

    std::stringstream sens;
    sens << std::fixed << std::setprecision(2) << "Sensitivity < " << player.get_mouse_sensitivity() << " >";
    render_menu_line(text_shader, sens.str(), w / 2 - 220, h / 2 - 35, pause_selection == 3);

    std::stringstream vol;
    vol << std::fixed << std::setprecision(2) << "Volume < " << music_volume << " >";
    render_menu_line(text_shader, vol.str(), w / 2 - 220, h / 2 - 70, pause_selection == 4);

    render_menu_line(text_shader, "Return to Main Menu", w / 2 - 220, h / 2 - 105, pause_selection == 5);
    render_menu_line(text_shader, "Quit Game", w / 2 - 220, h / 2 - 140, pause_selection == 6);

    m_text_manager->render_text(text_shader, "Controls:", w / 2 - 220, h / 2 - 190, 0.5f, glm::vec4(0.8f));
    m_text_manager->render_text(text_shader, "W/S: Navigate | Space: Select | A/D: Adjust | E: Resume", w / 2 - 220, h / 2 - 215, 0.5f, glm::vec4(0.8f));
}

void Hud::render_defeat_screen(const engine::ref<engine::shader>& text_shader, int defeat_selection)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    m_text_manager->render_text(text_shader, "YOU DIED", w / 2 - 135, h / 2 + 90, 1.5f, glm::vec4(0.75f, 0.05f, 0.05f, 1.f));

    render_menu_line(text_shader, "Respawn", w / 2 - 90, h / 2, defeat_selection == 0);
    render_menu_line(text_shader, "Return to Main Menu", w / 2 - 90, h / 2 - 35, defeat_selection == 1);
    render_menu_line(text_shader, "Quit Game", w / 2 - 90, h / 2 - 70, defeat_selection == 2);

    m_text_manager->render_text(text_shader, "W/S: Navigate | Space: Select", w / 2 - 150, h / 2 - 120, 0.5f, glm::vec4(0.8f));
}
