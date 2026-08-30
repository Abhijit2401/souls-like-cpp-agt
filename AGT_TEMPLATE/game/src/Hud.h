#pragma once
#include <engine.h>
#include "Player.h"

// Owns all 2D text/HUD rendering: main menu, in-game HUD, floating world-space
// health bars, boss health bar, and the pause/upgrade menu overlay.
// Extracted out of example_layer so rendering-only code isn't mixed in with
// scene setup and gameplay update logic.
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

    // Top-left in-game stat readout (HP/Stamina/Soul Fragments/Potions).
    void render_game_hud(const engine::ref<engine::shader>& text_shader,
        const player& player,
        int soul_count);

    // Floating "[||||     ]" bar above a world-space position (used for enemies and priests).
    // Draws nothing if the position falls outside the camera's view frustum.
    void render_floating_health_bar(const engine::ref<engine::shader>& text_shader,
        const engine::perspective_camera& camera,
        const glm::vec3& world_position,
        float health_percent,
        const glm::vec4& colour);

    // Always-on-screen boss health bar at the bottom-centre of the screen.
    void render_boss_health_bar(const engine::ref<engine::shader>& text_shader, float health_percent);

    // Upgrade menu shown while the game is paused.
    void render_pause_menu(const engine::ref<engine::shader>& text_shader,
        const player& player,
        int soul_count);

private:
    engine::ref<engine::text_manager> m_text_manager;
};
