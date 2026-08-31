#pragma once
#include <engine.h>
#include "Player.h"
#include "SoulFragment.h"
#include "Boss.h"
#include "Firebolt.h"
#include "SpawnManager.h"
#include "Hud.h"
#include "CombatVfx.h"
#include "LockOnSystem.h"
#include "ShadowRenderer.h"
#include <vector>
#include <glm/glm.hpp>

class example_layer : public engine::layer
{
public:
	example_layer();
	~example_layer();

	// Main update, render, and event loop functions
	void on_update(const engine::timestep& time_step) override;
	void on_render() override;
	void on_event(engine::event& event) override;

private:
	enum class GameState
	{
		MainMenu,
		InGame,
		PauseMenu,
		Defeat
	};
	GameState m_game_state;

	ShadowRenderer m_shadow_renderer;
	void render_shadow_casters(const engine::ref<engine::shader>& shader);
	void render_static_world(const engine::ref<engine::shader>& mesh_shader, const glm::vec3& camera_pos);
	engine::ref<engine::texture_2d> m_intro_texture;
	engine::ref<engine::mesh> m_quad_mesh;
	int m_menu_selection = 0;
	int m_pause_selection = 0;
	int m_defeat_selection = 0;
	float m_music_volume = 0.1f;
	engine::ref<engine::material> m_dim_overlay_material{};

	// Structure for a collectable soul fragment
	struct SoulFragmentPickup
	{
		glm::vec3 position;
		bool is_active = true;
	};

	// Helper to check for the old bounce sound effect (not used anymore since its not realistic for the theme)
	void check_bounce();

	// World Objects
	engine::ref<engine::skybox>			m_skybox{};
	engine::ref<engine::game_object>	m_terrain{};

	//Trees
	engine::ref<engine::game_object>    m_tree{};
	engine::ref<engine::model>          m_tree_model{};
	std::vector<glm::mat4>              m_tree_transforms;

	// Halberd Statue
	engine::ref<engine::game_object>	m_halberd{};
	engine::ref<engine::material>		m_halberd_material{};

	//Enemies, priests: spawning, AI update, and rendering owned by SpawnManager
	SpawnManager m_spawn_manager;

	// Settings for enemy spawning
	int m_num_enemies_to_spawn = 5;
	int m_num_priests_to_spawn = 2;
	float m_spawn_radius = 30.0f; // Max distance from center
	float m_safe_radius = 10.0f;  // Min distance from player that they can spawn

	std::vector<Firebolt> m_firebolts;

	CombatVfx m_combat_vfx;

	// Elden-Ring-style lock-on targeting - see LockOnSystem.h.
	LockOnSystem m_lock_on;

	//Grass
	engine::ref<engine::model> m_grass_model{};
	engine::ref<engine::texture_2d> m_grass_texture{};
	std::vector<glm::mat4> m_grass_transforms; // Transformations for each grass patch

	//Halberd Mesh for projectiles (reused to stick to the theme)
	engine::ref<engine::model> m_projectile_model{};

	//Devil Boss
	engine::ref<engine::game_object>    m_boss_object{};
	Boss                                m_boss_logic; // Boss AI logic

	engine::ref<engine::game_object>	m_knight{};  // Player game object

	//PRIMITIVES
	// Tetrahedron rock mound
	engine::ref<engine::game_object>	m_tetrahedron{};
	engine::ref<engine::material>		m_tetrahedron_material{};
	//Cuboid stone block
	engine::ref<engine::game_object>	m_stone_block{};
	//Scattered rock props (visual variety - see constructor)
	std::vector<engine::ref<engine::game_object>> m_rocks{};

	//Castle Cuboids
	engine::ref<engine::game_object>    m_castle_main{};
	engine::ref<engine::game_object>    m_castle_tower_left{};
	engine::ref<engine::game_object>    m_castle_tower_right{};

	engine::ref<engine::material>		m_material{}; //Generic material

	player m_player{}; // Player logic component

	//Soul Fragments (collectables)
	engine::ref<engine::texture_2d>		m_soul_texture{};
	engine::ref<engine::mesh>			m_soul_mesh{};
	engine::ref<engine::material>		m_soul_material{};
	std::vector<SoulFragmentPickup>		m_soul_pickups;
	int									m_soul_count = 0; //The player's current soul fragment currency

	//LIGHST
	engine::DirectionalLight            m_directionalLight;
	engine::PointLight                  m_pointLight;
	engine::PointLight                  m_halberdLight; // Red Light aura for the halberd statue
	std::vector<engine::PointLight>     m_tree_lights; // Yellow lights around trees
	uint32_t                            num_point_lights = 2;

	std::vector<engine::ref<engine::game_object>>    m_game_objects{}; //All physics objects
	engine::ref<engine::bullet_manager> m_physics_manager{}; //Manages Bullet Physics simulation
	engine::ref<engine::audio_manager>  m_audio_manager{};

	float								m_prev_halberd_y_vel = 0.f;
	engine::ref<engine::text_manager>	m_text_manager{};
	Hud									m_hud;

	//Cameras
	engine::orthographic_camera         m_2d_camera;
	engine::perspective_camera          m_3d_camera;
};
