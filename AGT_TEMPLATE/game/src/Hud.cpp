#include "pch.h"
#include "Hud.h"
#include "platform/opengl/gl_shader.h"
#include <sstream>
#include <iomanip>

namespace
{
    const glm::vec4 k_title_colour(1.f, 0.82f, 0.3f, 1.f);
    const glm::vec4 k_selected_colour(1.f, 1.f, 0.f, 1.f);
    const glm::vec4 k_normal_colour(1.f, 1.f, 1.f, 1.f);
    const glm::vec4 k_disabled_colour(0.5f, 0.5f, 0.5f, 1.f);
    const glm::vec4 k_hint_colour(0.8f, 0.8f, 0.8f, 1.f);
}

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

std::string Hud::format_value(float value)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value;
    return ss.str();
}

std::string Hud::format_percent(float value)
{
    return std::to_string((int)std::round(value * 100.0f)) + "%";
}

void Hud::render_text_centred(const engine::ref<engine::shader>& text_shader, const std::string& text, float y, float scale, const glm::vec4& colour)
{
    float w = (float)engine::application::window().width();
    m_text_manager->render_text(text_shader, text, (w - m_text_manager->text_width(text, scale)) / 2.f, y, scale, colour);
}

void Hud::render_menu_line(const engine::ref<engine::shader>& text_shader, const std::string& text, float x, float y, bool selected, bool enabled)
{
    std::string prefix = selected ? "> " : "  ";
    glm::vec4 colour = enabled ? (selected ? k_selected_colour : k_normal_colour)
                               : (selected ? glm::vec4(0.7f, 0.7f, 0.3f, 1.f) : k_disabled_colour);
    m_text_manager->render_text(text_shader, prefix + text, x, y, 0.6f, colour);
}

void Hud::render_menu_background(const engine::ref<engine::shader>& mesh_shader,
    const engine::ref<engine::texture_2d>& background_texture,
    const engine::ref<engine::mesh>& quad_mesh,
    const engine::ref<engine::material>& dim_material)
{
    //Uses the mesh shader because the text shader can't draw images
    auto gl_mesh_shader = std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader);
    engine::render_command::disable_depth_test();
    gl_mesh_shader->set_uniform("lighting_on", false);
    gl_mesh_shader->set_uniform("fog_on", false);
    gl_mesh_shader->set_uniform("transparency", 1.0f);
    gl_mesh_shader->set_uniform("has_texture", true);
    background_texture->bind();
    engine::renderer::submit(mesh_shader, quad_mesh, glm::mat4(1.0f));
    gl_mesh_shader->set_uniform("has_texture", false);

    //Darkens it so the text is easier to read
    dim_material->submit(mesh_shader);
    engine::renderer::submit(mesh_shader, quad_mesh, glm::mat4(1.0f));

    gl_mesh_shader->set_uniform("fog_on", true);
    gl_mesh_shader->set_uniform("lighting_on", true);
    engine::render_command::enable_depth_test();
}

void Hud::render_main_menu(const engine::ref<engine::shader>& text_shader,
    int menu_selection,
    float mouse_sensitivity,
    float music_volume,
    bool run_in_progress)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    render_text_centred(text_shader, "MEDIEVAL SOULS", h / 2.f + 150.f, 1.4f, k_title_colour);

    //Menu options in a column in the middle of the screen
    float x = w / 2.f - 160.f;
    float y = h / 2.f + 20.f;
    const float spacing = 45.f;
    render_menu_line(text_shader, run_in_progress ? "CONTINUE" : "START", x, y, menu_selection == MainStart);
    render_menu_line(text_shader, "SENSITIVITY   < " + format_value(mouse_sensitivity) + " >", x, y - spacing, menu_selection == MainSensitivity);
    render_menu_line(text_shader, "MUSIC VOLUME  < " + format_percent(music_volume) + " >", x, y - spacing * 2.f, menu_selection == MainVolume);
    render_menu_line(text_shader, "QUIT", x, y - spacing * 3.f, menu_selection == MainQuit);

    render_text_centred(text_shader, "W/S: Navigate  |  A/D: Adjust  |  Space/Enter: Select", 90.f, 0.45f, k_hint_colour);

    //Controls list
    m_text_manager->render_text(text_shader, "Controls:", 10.f, h - 25.f, 0.5f, k_title_colour);
    m_text_manager->render_text(text_shader, "WASD: Move | L-Shift: Roll | R: Health Potion", 10.f, h - 50.f, 0.5f, k_normal_colour);
    m_text_manager->render_text(text_shader, "Mouse: Move Camera | Middle Click: Lock On", 10.f, h - 75.f, 0.5f, k_normal_colour);
    m_text_manager->render_text(text_shader, "Left Click: Attack | Esc/E: Pause & Upgrades", 10.f, h - 100.f, 0.5f, k_normal_colour);
    m_text_manager->render_text(text_shader, "P: Spawn Enemy | F1: Wireframe | F2: Invincible", 10.f, h - 125.f, 0.5f, k_normal_colour);
}

void Hud::render_game_hud(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count)
{
    std::stringstream hp_ss;
    hp_ss << (int)std::ceil(player.get_health()) << "/" << (int)player.get_max_health();
    m_text_manager->render_text(text_shader, hp_ss.str(), 200.f, 120.f, 0.4f, glm::vec4(1.f, 0.4f, 0.4f, 1.f));

    std::stringstream stm_ss;
    stm_ss << (int)player.get_stamina() << "/" << (int)player.get_max_stamina();
    m_text_manager->render_text(text_shader, stm_ss.str(), 200.f, 98.f, 0.4f, glm::vec4(0.4f, 1.f, 0.4f, 1.f));

    //Souls white text
    std::string soul_text = "Soul Fragments: " + std::to_string(soul_count);
    m_text_manager->render_text(text_shader, soul_text, 10.f, 70.f, 0.5f, k_normal_colour);

    //Potions blue text
    std::stringstream pot_ss;
    pot_ss << "Potions: " << player.get_potions() << "  (R to drink)";
    m_text_manager->render_text(text_shader, pot_ss.str(), 10.f, 45.f, 0.5f, glm::vec4(0.f, 1.f, 1.f, 1.f));

    if (player.is_invincible())
        m_text_manager->render_text(text_shader, "INVINCIBLE (F2)", 10.f, 20.f, 0.4f, k_selected_colour);
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
    float screen_h = (float)engine::application::window().height();
    render_text_centred(text_shader, "BOSS: DEVIL", screen_h - 40.f, 0.5f, glm::vec4(1.f, 0.5f, 0.f, 1.f));
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

void Hud::render_pause_menu(const engine::ref<engine::shader>& text_shader, const player& player, int soul_count, int pause_selection, float music_volume, const std::string& status_message)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    render_text_centred(text_shader, "PAUSED", h / 2.f + 200.f, 1.0f, k_title_colour);
    render_text_centred(text_shader, "Available Souls: " + std::to_string(soul_count), h / 2.f + 155.f, 0.6f, glm::vec4(0.f, 0.7f, 1.f, 1.f));

    float x = w / 2.f - 260.f;
    float y = h / 2.f + 105.f;
    const float spacing = 35.f;

    render_menu_line(text_shader, "Resume", x, y, pause_selection == PauseResume);

    std::stringstream dmg;
    dmg << "Increase Damage (" << k_damage_upgrade_cost << " souls)  Current: " << (int)player.get_damage();
    render_menu_line(text_shader, dmg.str(), x, y - spacing, pause_selection == PauseDamage, soul_count >= k_damage_upgrade_cost);

    std::stringstream spd;
    spd << "Increase Move Speed (" << k_speed_upgrade_cost << " souls)  Current: " << (int)player.get_speed();
    render_menu_line(text_shader, spd.str(), x, y - spacing * 2.f, pause_selection == PauseSpeed, soul_count >= k_speed_upgrade_cost);

    std::stringstream pot;
    pot << "Buy Health Potion (" << k_potion_cost << " souls)  Owned: " << player.get_potions();
    render_menu_line(text_shader, pot.str(), x, y - spacing * 3.f, pause_selection == PausePotion, soul_count >= k_potion_cost);

    render_menu_line(text_shader, "Sensitivity   < " + format_value(player.get_mouse_sensitivity()) + " >", x, y - spacing * 4.f, pause_selection == PauseSensitivity);
    render_menu_line(text_shader, "Music Volume  < " + format_percent(music_volume) + " >", x, y - spacing * 5.f, pause_selection == PauseVolume);
    render_menu_line(text_shader, "Return to Main Menu", x, y - spacing * 6.f, pause_selection == PauseMainMenu);
    render_menu_line(text_shader, "Quit Game", x, y - spacing * 7.f, pause_selection == PauseQuit);

    if (!status_message.empty())
        render_text_centred(text_shader, status_message, y - spacing * 8.f - 10.f, 0.5f, k_selected_colour);

    render_text_centred(text_shader, "W/S: Navigate  |  Space/Enter: Select  |  A/D: Adjust  |  Esc/E: Resume", y - spacing * 9.f - 15.f, 0.45f, k_hint_colour);
}

void Hud::render_defeat_screen(const engine::ref<engine::shader>& text_shader, int defeat_selection)
{
    float w = (float)engine::application::window().width();
    float h = (float)engine::application::window().height();

    render_text_centred(text_shader, "YOU DIED", h / 2.f + 90.f, 1.5f, glm::vec4(0.75f, 0.05f, 0.05f, 1.f));

    float x = w / 2.f - 120.f;
    render_menu_line(text_shader, "Respawn", x, h / 2.f, defeat_selection == DefeatRespawn);
    render_menu_line(text_shader, "Return to Main Menu", x, h / 2.f - 35.f, defeat_selection == DefeatMainMenu);
    render_menu_line(text_shader, "Quit Game", x, h / 2.f - 70.f, defeat_selection == DefeatQuit);

    render_text_centred(text_shader, "W/S: Navigate  |  Space/Enter: Select", h / 2.f - 120.f, 0.45f, k_hint_colour);
}
