#pragma once
#include <engine.h>
#include "Player.h"

class Hud
{
public:
    void initialise(engine::ref<engine::text_manager> text_manager);

    // Main menu screen (background quad, title, START/SENSITIVITY/VOLUME list, controls list).
    void render_main_menu(const engine::ref<engine::shader>& text_shader,
        const engine::ref<engine::texture_2d>& intro_texture,
        const engine::ref<engine::mesh>& quad_mesh,
        int menu_selection,
        float mouse_sensitivity,
        float music_volume);
    void render_game_hud(const engine::ref<engine::shader>& text_shader,
        const player& player,
        int soul_count);

    void render_player_bars(const engine::ref<engine::shader>& mesh_shader, const player& player);
    void render_bar(const engine::ref<engine::shader>& mesh_shader,
        float x, float y, float width, float height,
        float fill_percent, const glm::vec3& fill_colour);
    void render_floating_bar(const engine::ref<engine::shader>& mesh_shader,
        const engine::perspective_camera& camera,
        const glm::vec3& world_position,
        float health_percent,
        const glm::vec3& fill_colour);
    void render_boss_bar_graphic(const engine::ref<engine::shader>& mesh_shader, float health_percent);
    void render_boss_name_text(const engine::ref<engine::shader>& text_shader);
    void render_dim_overlay(const engine::ref<engine::shader>& mesh_shader,
        const engine::ref<engine::mesh>& quad_mesh,
        const engine::ref<engine::material>& dim_material);

    void render_pause_menu(const engine::ref<engine::shader>& text_shader,
        const player& player,
        int soul_count,
        int pause_selection,
        float music_volume);
    void render_defeat_screen(const engine::ref<engine::shader>& text_shader, int defeat_selection);

private:
    void render_menu_line(const engine::ref<engine::shader>& text_shader, const std::string& text, float x, float y, bool selected);

    engine::ref<engine::text_manager> m_text_manager;
    engine::ref<engine::mesh> m_ui_quad;
    engine::ref<engine::material> m_bar_material;
};
