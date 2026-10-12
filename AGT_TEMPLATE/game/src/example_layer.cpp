#include "pch.h"
#include "example_layer.h"
#include "platform/opengl/gl_shader.h"
#include "SoulFragment.h"
#include "tetrahedron.h"
#include "engine/entities/shapes/cuboid.h" 
#include "Player.h" 
#include <glm/gtc/matrix_transform.hpp> 
#include <glm/gtc/type_ptr.hpp>
#include "engine/events/key_event.h"
#include "engine/utils/track.h"
#include <string> 
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "btBulletDynamicsCommon.h"
#include <stdlib.h>

example_layer::example_layer()
    :m_2d_camera(-1.6f, 1.6f, -0.9f, 0.9f),
    m_3d_camera((float)engine::application::window().width(), (float)engine::application::window().height(), 45.f, 0.1f, 400.f)
{
    m_shadow_renderer.initialise();

    //Shows mouse on main menu options screen before the main game start
    engine::application::window().show_mouse_cursor();
    m_game_state = GameState::MainMenu;

    //Audio initialisation
    m_audio_manager = engine::audio_manager::instance();
    m_audio_manager->init();

    //AUDIOS (for background music, souls pickup, attackin and dash sound effect) (Didnt use bounce since it wasn't in theme)
    m_audio_manager->load_sound("assets/audio/Soliloquy.mp3", engine::sound_type::track, "music");
    m_audio_manager->load_sound("assets/audio/key_pickup.wav", engine::sound_type::spatialised, "pickup");
    m_audio_manager->load_sound("assets/audio/sword-arm-2a.wav", engine::sound_type::spatialised, "attack");
    m_audio_manager->load_sound("assets/audio/fall.wav", engine::sound_type::spatialised, "dash");
    //Enough sound effects for the mark scheme
    m_audio_manager->play("music");
    m_audio_manager->volume("music", m_music_volume);

    auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
    auto text_shader = engine::renderer::shaders_library()->get("text_2D");

    m_directionalLight.Color = glm::vec3(1.0f, 1.0f, 1.0f);
    m_directionalLight.AmbientIntensity = 0.12f;
    m_directionalLight.DiffuseIntensity = 0.55f;
    m_directionalLight.Direction = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));

    //Reddish light in front/near spawn
    m_pointLight.Color = glm::vec3(1.0f, 0.2f, 0.2f);
    m_pointLight.AmbientIntensity = 0.3f;
    m_pointLight.DiffuseIntensity = 0.8f;
    m_pointLight.Position = glm::vec3(0.f, 2.f, -5.f);
    m_pointLight.Attenuation.Constant = 1.0f;
    m_pointLight.Attenuation.Linear = 0.1f;
    m_pointLight.Attenuation.Exp = 0.01f;


    //Red point light glow for the silver halberd statue for like a reddish aura
    m_halberdLight.Color = glm::vec3(1.0f, 0.0f, 0.0f);
    m_halberdLight.AmbientIntensity = 0.1f;
    m_halberdLight.DiffuseIntensity = 4.0f;
    m_halberdLight.Position = glm::vec3(0.f, 12.f, -20.f); //Placed near the statue of the halberd in the mound (tetrahedron)
    m_halberdLight.Attenuation.Constant = 1.0f;
    m_halberdLight.Attenuation.Linear = 0.1f;
    m_halberdLight.Attenuation.Exp = 0.01f;

    //Submits initial lights data to the shader (the shader only supports 2 point lights so its the spawn light and the halberd light)
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->bind();
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gColorMap", 0);
    m_directionalLight.submit(mesh_shader);

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gNumPointLights", (int)num_point_lights);
    m_pointLight.submit(mesh_shader, 0);
    m_halberdLight.submit(mesh_shader, 1);

    //More shader setup 
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gMatSpecularIntensity", 1.f);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gSpecularPower", 10.f);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("transparency", 1.0f);

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("fog_on", true);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("fog_factor_type", 0);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("fog_colour", glm::vec3(0.72f, 0.68f, 0.60f));

    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->bind();
    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("projection",
        glm::ortho(0.f, (float)engine::application::window().width(), 0.f,
            (float)engine::application::window().height()));
    m_material = engine::material::create(1.0f, glm::vec3(1.0f, 0.1f, 0.07f),
        glm::vec3(1.0f, 0.1f, 0.07f), glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);


    m_combat_vfx.initialise();
    m_lock_on.initialise();

    m_skybox = engine::skybox::create(200.f,
        { engine::texture_2d::create("assets/textures/skybox/stormydays_ft.tga", true),
          engine::texture_2d::create("assets/textures/skybox/stormydays_lf.tga", true),
          engine::texture_2d::create("assets/textures/skybox/stormydays_bk.tga", true),
          engine::texture_2d::create("assets/textures/skybox/stormydays_rt.tga", true),
          engine::texture_2d::create("assets/textures/skybox/stormydays_up.tga", true),
          engine::texture_2d::create("assets/textures/skybox/stormydays_dn.tga", true)
        });

    // MESH AND TEXTURE SETUP
    std::vector<engine::ref<engine::texture_2d>> knight_textures;
    knight_textures.push_back(engine::texture_2d::create("assets/textures/Knight.png", false));

    //BEZERKER (Melee horde enemy 1) from https://opengameart.org/content/animated-bezerker
    std::vector<engine::ref<engine::texture_2d>> berzerker_textures;
    berzerker_textures.push_back(engine::texture_2d::create("assets/textures/berzerker.png", false));
    engine::ref<engine::skinned_mesh> berzerker_mesh = engine::skinned_mesh::create("assets/models/animated/berzerker/berzerker.fbx");
    berzerker_mesh->switch_root_movement(false);
    berzerker_mesh->set_textures(berzerker_textures);

    //PRIEST (Ranged mage) from https://opengameart.org/content/animated-priest
    std::vector<engine::ref<engine::texture_2d>> priest_textures;
    priest_textures.push_back(engine::texture_2d::create("assets/textures/priest.png", false));
    engine::ref<engine::skinned_mesh> priest_mesh = engine::skinned_mesh::create("assets/models/animated/priest/priest.fbx");
    priest_mesh->switch_root_movement(false);
    priest_mesh->set_textures(priest_textures);

    m_spawn_manager.initialise(berzerker_mesh, berzerker_textures, priest_mesh, priest_textures, m_spawn_radius, m_safe_radius);

    //GRASS (Static mesh to add detail to terrain) from https://opengameart.org/content/3d-grass-patch
    m_grass_model = engine::model::create("assets/models/static/high_grass.fbx");
    m_grass_texture = engine::texture_2d::create("assets/textures/grass.png", false);
    m_grass_material = engine::material::create(1.0f, glm::vec3(0.1f, 0.5f, 0.1f),
        glm::vec3(0.1f, 0.5f, 0.1f), glm::vec3(0.2f, 0.2f, 0.2f), 1.0f);

    //Grass setup showing how many grass patches to add and size
    float map_limit = 40.0f;
    int num_grass_patches = 1500;
    float grass_scale = 0.04f; 

    for (int i = 0; i < num_grass_patches; ++i) {
        float x_pos = ((float)rand() / (float)RAND_MAX * map_limit * 2) - map_limit;
        float z_pos = ((float)rand() / (float)RAND_MAX * map_limit * 2) - map_limit;

        glm::mat4 transform(1.0f);
        //Make it slightly above ground by making Y = -0.3
        transform = glm::translate(transform, glm::vec3(x_pos, 0.3f, z_pos));
        //Rando rotate the grass to add variation
        transform = glm::rotate(transform, ((float)rand() / (float)RAND_MAX * 3.14159f), glm::vec3(0, 1, 0));
        //Scale of it
        transform = glm::scale(transform, glm::vec3(grass_scale));
        m_grass_transforms.push_back(transform);
    }

    //PLAYER SETUP
    engine::ref<engine::skinned_mesh> m_player_mesh = engine::skinned_mesh::create("assets/models/animated/player/knight.fbx");
    m_player_mesh->switch_root_movement(false);
    m_player_mesh->set_textures(knight_textures);

    //Player object properties 
    engine::game_object_properties player_props;
    player_props.animated_mesh = m_player_mesh;
    player_props.scale = glm::vec3(0.11f);
    player_props.position = glm::vec3(0.f, 2.0f, 10.f);
    player_props.textures = knight_textures;
    player_props.bounding_shape = glm::vec3(0.5f, 0.9f, 0.5f);
    player_props.type = 0;
    player_props.mass = 80.0f;
    player_props.friction = 0.0f;
    player_props.restitution = 0.0f;
    m_knight = engine::game_object::create(player_props);
    m_knight->set_angular_factor_lock(true);
    m_knight->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));
    m_player.initialise(m_knight);
    m_game_objects.push_back(m_knight);

    //Enemy spawn (Enemy and Priest)
    for (int i = 0; i < m_num_enemies_to_spawn; i++)
    {
        m_spawn_manager.spawn_enemy(m_player, m_game_objects);
    }
    for (int i = 0; i < m_num_priests_to_spawn; i++)
    {
        m_spawn_manager.spawn_priest(m_player, m_game_objects);
    }

    //BOSS SETUP
    std::vector<engine::ref<engine::texture_2d>> boss_textures;
    boss_textures.push_back(engine::texture_2d::create("assets/textures/diablous_texture.png", false));
    //Boss model from https://opengameart.org/content/animated-diablous
    engine::ref<engine::skinned_mesh> boss_mesh = engine::skinned_mesh::create("assets/models/animated/diablous/diablous_v002.fbx");
    boss_mesh->switch_root_movement(false);
    boss_mesh->set_textures(boss_textures);

    //Boss properties
    engine::game_object_properties boss_props;
    boss_props.animated_mesh = boss_mesh;
    boss_props.scale = glm::vec3(0.2f);
    boss_props.textures = boss_textures;
    boss_props.type = 0;
    boss_props.mass = 80.0f;
    boss_props.friction = 1.0f;
    boss_props.restitution = 0.0f;
    boss_props.bounding_shape = glm::vec3(1.0f, 1.8f, 1.0f);
    boss_props.position = glm::vec3(0.0f, 5.0f, -15.0f);

    m_boss_object = engine::game_object::create(boss_props);
    m_boss_object->set_angular_factor_lock(true);
    m_boss_object->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    m_game_objects.push_back(m_boss_object);
    m_boss_logic.initialise(m_boss_object, boss_props.position, m_player.object());


    //TERRAIN
    std::vector<engine::ref<engine::texture_2d>> terrain_textures = { engine::texture_2d::create("assets/textures/rocky_terrain.jpg", false) };
    engine::ref<engine::terrain> terrain_shape = engine::terrain::create(100.f, 0.5f, 100.f);
    engine::game_object_properties terrain_props;
    terrain_props.meshes = { terrain_shape->mesh() };
    terrain_props.textures = terrain_textures;
    terrain_props.is_static = true;
    terrain_props.type = 0;
    terrain_props.bounding_shape = glm::vec3(100.f, 0.5f, 100.f);
    terrain_props.position = glm::vec3(0.f, -0.25f, 0.f);
    terrain_props.friction = 1.0f;
    terrain_props.restitution = 0.0f;

    m_terrain = engine::game_object::create(terrain_props);
    m_terrain->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    //PRIMATIVES
    //TETRAHEDRON setup (learnt from one of the tutorials/labs)
    std::vector<glm::vec3> tetrahedron_vertices;
    tetrahedron_vertices.push_back(glm::vec3(0.f, 10.f, 0.f));
    tetrahedron_vertices.push_back(glm::vec3(0.f, 0.f, 10.f));
    tetrahedron_vertices.push_back(glm::vec3(-10.f, 0.f, -10.f));
    tetrahedron_vertices.push_back(glm::vec3(10.f, 0.f, -10.f));
    engine::ref<engine::tetrahedron> tetrahedron_shape = engine::tetrahedron::create(tetrahedron_vertices);
    engine::game_object_properties tetrahedron_props;
    tetrahedron_props.position = { 0.f, 0.09f, -20.f };
    tetrahedron_props.meshes = { tetrahedron_shape->mesh() };
    tetrahedron_props.textures = terrain_textures;
    tetrahedron_props.scale = glm::vec3(1.3f);
    tetrahedron_props.is_static = true;
    tetrahedron_props.type = 0;
    tetrahedron_props.bounding_shape = glm::vec3(10.f, 10.f, 10.f);
    m_tetrahedron = engine::game_object::create(tetrahedron_props);
    m_tetrahedron_material = engine::material::create(32.0f, glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.5f), 1.0f);

    //Stone ground block as a CUBOID
    engine::ref<engine::cuboid> block_shape = engine::cuboid::create(glm::vec3(1.5f, 2.5f, 1.5f), false);
    engine::game_object_properties block_props;
    block_props.position = { -15.f, 2.5f, -10.f };
    block_props.meshes = { block_shape->mesh() };
    block_props.textures = terrain_textures;
    block_props.scale = glm::vec3(1.0f);
    block_props.is_static = true;
    block_props.type = 0;
    block_props.bounding_shape = glm::vec3(1.5f, 2.5f, 1.5f);
    m_stone_block = engine::game_object::create(block_props);

    //Castle CUBOIDS
    std::vector<engine::ref<engine::texture_2d>> castle_textures = {
        engine::texture_2d::create("assets/textures/rck_3.png", false) //Using the rock texture for hte castle
    };

    //Main Castle CUBE in the centre of the 2 tall cuboids
    engine::ref<engine::cuboid> castle_main_shape = engine::cuboid::create(glm::vec3(8.f, 8.f, 8.f), false);
    engine::game_object_properties castle_main_props;
    castle_main_props.position = { 0.f, 8.0f, 65.f };
    castle_main_props.meshes = { castle_main_shape->mesh() };
    castle_main_props.textures = castle_textures;
    castle_main_props.scale = glm::vec3(1.0f);
    castle_main_props.is_static = true;
    castle_main_props.bounding_shape = glm::vec3(8.f, 8.f, 8.f);
    m_castle_main = engine::game_object::create(castle_main_props);

    //Towers for the tall cuboids
    engine::ref<engine::cuboid> tower_shape = engine::cuboid::create(glm::vec3(3.f, 12.f, 3.f), false);
    engine::game_object_properties tower_props;
    tower_props.position = { -11.f, 12.f, 65.f };
    tower_props.meshes = { tower_shape->mesh() };
    tower_props.textures = castle_textures;
    tower_props.scale = glm::vec3(1.0f);
    tower_props.is_static = true;
    tower_props.bounding_shape = glm::vec3(3.f, 12.f, 3.f);
    m_castle_tower_left = engine::game_object::create(tower_props);

    tower_props.position = { 11.f, 12.f, 65.f }; 
    m_castle_tower_right = engine::game_object::create(tower_props);


    //Halberd statue setup
    m_projectile_model = engine::model::create("assets/models/static/Halberd.fbx");
    std::vector<engine::ref<engine::texture_2d>> halberd_textures;
    halberd_textures.push_back(engine::texture_2d::create("assets/textures/Halberd.png", false));

    engine::game_object_properties halberd_props;
    halberd_props.meshes = m_projectile_model->meshes();
    halberd_props.textures = halberd_textures;
    //Its set up to be stuck into the tetrahedron to mimic a massive sword(halberd) being stuck in stone(the tetrahedron mound)
    halberd_props.position = { 0.f, 14.0f, -20.0f };
    halberd_props.scale = glm::vec3(4.0f);
    halberd_props.bounding_shape = m_projectile_model->size() / 2.f;
    halberd_props.type = 2;
    halberd_props.is_static = true;
    halberd_props.mass = 0.0f;
    //Rotates the halberd appropriately to stab into the tetrahedron stone mound
    halberd_props.rotation_axis = glm::vec3(1.0f, 0.0f, 0.0f);
    halberd_props.rotation_amount = 1.57f;
    m_halberd = engine::game_object::create(halberd_props);
    m_halberd_material = engine::material::create(32.0f, glm::vec3(0.5f), glm::vec3(0.5f), glm::vec3(0.5f), 1.0f);

    //SOULS (Collectable PRIMATIVE learnt from a combination of different labs and experimented)
    std::vector<glm::vec3> soul_vertices;
    soul_vertices.push_back(glm::vec3(0.f, 1.f, 0.f));
    soul_vertices.push_back(glm::vec3(0.f, 0.f, 1.f));
    soul_vertices.push_back(glm::vec3(-1.f, 0.f, 0.f));
    soul_vertices.push_back(glm::vec3(0.f, 0.f, -1.f));
    soul_vertices.push_back(glm::vec3(1.f, 0.f, 0.f));
    soul_vertices.push_back(glm::vec3(0.f, -1.f, 0.f));

    engine::ref<engine::SoulFragment> soul_shape = engine::SoulFragment::create(soul_vertices);
    m_soul_mesh = soul_shape->mesh();
    m_soul_texture = engine::texture_2d::create("assets/textures/soul_fabric.jpg", false);
    m_soul_material = engine::material::create(32.0f, glm::vec3(0.f, 0.7f, 1.f), glm::vec3(0.f, 0.7f, 1.f), glm::vec3(1.f), 0.7f);

    m_soul_pickups.push_back({ glm::vec3(0.f, 0.7f, 5.f) });
    m_soul_pickups.push_back({ glm::vec3(-3.f, 0.7f, 2.f) });
    m_soul_pickups.push_back({ glm::vec3(2.f, 1.0f, 3.f) });

    //INTRO SETUP
    m_intro_texture = engine::texture_2d::create("assets/textures/skybox/stormydays_ft.tga", true);
    std::vector<engine::mesh::vertex> quad_vertices
    {
        { {-1.6f, -0.9f, -0.1f}, {0.f, 0.f, 1.f}, {0.f, 0.f} },
        { { 1.6f, -0.9f, -0.1f}, {0.f, 0.f, 1.f}, {1.f, 0.f} },
        { { 1.6f,  0.9f, -0.1f}, {0.f, 0.f, 1.f}, {1.f, 1.f} },
        { {-1.6f,  0.9f, -0.1f}, {0.f, 0.f, 1.f}, {0.f, 1.f} }
    };
    std::vector<uint32_t> quad_indices = { 0, 1, 2, 2, 3, 0 };
    m_quad_mesh = engine::mesh::create(quad_vertices, quad_indices);

    //TREES GENERATION SETUP
    engine::ref <engine::model> tree_model = engine::model::create("assets/models/static/elm.3ds");
    engine::game_object_properties tree_props;
    tree_props.meshes = tree_model->meshes();
    tree_props.textures = tree_model->textures();
    float tree_scale = 6.f / glm::max(tree_model->size().x, glm::max(tree_model->size().y, tree_model->size().z));
    tree_props.position = { 4.f, 0.0f, -5.f };
    tree_props.bounding_shape = tree_model->size() / 2.f * tree_scale;
    tree_props.scale = glm::vec3(tree_scale);
    m_tree = engine::game_object::create(tree_props);
    //Spawns 67 trees around a 35 radius at a scale of 3
    int num_trees = 67;
    float tree_circle_radius = 35.f;
    float scale_multiplier = 3.f;
    //Loop for the tree generation
    for (int i = 0; i < num_trees; ++i)
    {
        float angle = (float)i / (float)num_trees * 2.f * 3.14f;
        float angle_jitter = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.3f;
        angle += angle_jitter;
        float radius_jitter = ((float)rand() / (float)RAND_MAX - 0.5f) * 16.0f;
        float radius = tree_circle_radius + radius_jitter;
        float x_pos = sin(angle) * radius;
        float z_pos = cos(angle) * radius;
        float tree_rotation = angle + (float)(i % 5);
        float tree_scale_variation = 0.8f + ((float)rand() / (float)RAND_MAX) * 0.6f;
        glm::mat4 loop_tree_generation(1.0f);
        loop_tree_generation = glm::translate(loop_tree_generation, glm::vec3(x_pos, 0.0f, z_pos));
        loop_tree_generation = glm::rotate(loop_tree_generation, tree_rotation, m_tree->rotation_axis());
        loop_tree_generation = glm::scale(loop_tree_generation, m_tree->scale() * scale_multiplier * tree_scale_variation);
        m_tree_transforms.push_back(loop_tree_generation);
    }

    const int num_rocks = 14;
    for (int i = 0; i < num_rocks; ++i)
    {
        float angle = ((float)rand() / (float)RAND_MAX) * 6.28318f;
        float radius = 12.0f + ((float)rand() / (float)RAND_MAX) * 22.0f;
        glm::vec3 rock_pos(sin(angle) * radius, 0.0f, cos(angle) * radius);

        glm::vec3 half_extents(
            0.5f + ((float)rand() / (float)RAND_MAX) * 1.1f,
            0.35f + ((float)rand() / (float)RAND_MAX) * 0.9f,
            0.5f + ((float)rand() / (float)RAND_MAX) * 1.1f);

        engine::ref<engine::cuboid> rock_shape = engine::cuboid::create(half_extents, false);
        engine::game_object_properties rock_props;
        rock_props.position = { rock_pos.x, half_extents.y, rock_pos.z };
        rock_props.meshes = { rock_shape->mesh() };
        rock_props.textures = terrain_textures;
        rock_props.scale = glm::vec3(1.0f);
        rock_props.is_static = true;
        rock_props.type = 0;
        rock_props.bounding_shape = half_extents;
        rock_props.rotation_axis = glm::vec3(0.f, 1.f, 0.f);
        rock_props.rotation_amount = ((float)rand() / (float)RAND_MAX) * 6.28318f;

        m_rocks.push_back(engine::game_object::create(rock_props));
    }

    m_game_objects.push_back(m_terrain);
    //Halberd
    if (m_halberd) m_game_objects.push_back(m_halberd);
    //Tetrahedron rock mound
    if (m_tetrahedron) m_game_objects.push_back(m_tetrahedron);
    //Stone Block cuboid
    if (m_stone_block) m_game_objects.push_back(m_stone_block);
    //Castle
    if (m_castle_main) m_game_objects.push_back(m_castle_main);
    if (m_castle_tower_left) m_game_objects.push_back(m_castle_tower_left);
    if (m_castle_tower_right) m_game_objects.push_back(m_castle_tower_right);
    for (const auto& rock : m_rocks) m_game_objects.push_back(rock);

    m_physics_manager = engine::bullet_manager::create(m_game_objects);

    m_text_manager = engine::text_manager::create();
    m_hud.initialise(m_text_manager);
    m_player.update_camera(m_3d_camera, engine::timestep(0.f));
    m_dim_overlay_material = engine::material::create(1.0f, glm::vec3(0.04f), glm::vec3(0.f), glm::vec3(0.f), 0.55f);
    m_hurt_overlay_material = engine::material::create(1.0f, glm::vec3(0.85f, 0.05f, 0.05f), glm::vec3(0.f), glm::vec3(0.f), 0.0f);
}

example_layer::~example_layer() {}

void example_layer::on_update(const engine::timestep& time_step)
{
    if (m_game_state == GameState::PauseMenu || m_game_state == GameState::Defeat) return;

    if (m_game_state == GameState::InGame)
    {
        //Hit stop, freezes the game for a split second when the sword hits so it feels heavier
        if (m_hit_stop_timer > 0.0f)
        {
            m_hit_stop_timer -= (float)time_step;
            return;
        }
        if (m_hurt_flash_timer > 0.0f) m_hurt_flash_timer = glm::max(0.0f, m_hurt_flash_timer - (float)time_step);
        m_combat_vfx.on_update((float)time_step);

        m_physics_manager->dynamics_world_update(m_game_objects, double(time_step));

        bool middle_mouse_down = engine::input::mouse_button_pressed(engine::mouse_button_codes::MOUSE_BUTTON_MIDDLE);
        m_lock_on.on_update(time_step, middle_mouse_down, m_3d_camera, m_spawn_manager, m_boss_logic, m_boss_object);

        m_player.on_update(time_step, m_lock_on.is_locked(), m_lock_on.target_position());
        m_player.update_camera(m_3d_camera, time_step);

        //AUDIO SETUP FOR ROLLING/DASHING AND ATTACKING
        //Attacking audio
        bool is_attacking = m_player.is_attacking();
        if (is_attacking && !m_was_attacking) {
            m_audio_manager->play_spatialised_sound("attack", m_3d_camera.position(), m_player.object()->position());
        }
        m_was_attacking = is_attacking;
        //Rolling/Dashing audio
        bool is_rolling = m_player.is_rolling();
        if (is_rolling && !m_was_rolling) {
            m_audio_manager->play_spatialised_sound("dash", m_3d_camera.position(), m_player.object()->position());
        }
        m_was_rolling = is_rolling;

        //UPDATES BOSS FSM LOGIC
        m_boss_logic.on_update(time_step);
        if (m_boss_logic.check_hit_player()) m_player.take_damage(25.0f);

        //PLAYER SWORD HIT (one hit per swing to everything inside the attack area when the sword connects)
        if (m_player.consume_attack_hit())
        {
            const AttackArc swing = m_player.get_attack_arc();
            std::vector<glm::vec3> hit_positions = m_spawn_manager.apply_player_hit(swing, m_player.get_damage());
            const float boss_half_height = 1.2f;
            if (!m_boss_logic.is_dead() && swing.contains(m_boss_object->position(), 1.0f, boss_half_height))
            {
                m_boss_logic.take_damage(m_player.get_damage());
                hit_positions.push_back(m_boss_object->position() + glm::vec3(0.f, boss_half_height, 0.f));
            }

            //Gold sparks and a flash on everything that got hit plus camera shake and hit stop
            for (const auto& hit_position : hit_positions)
            {
                //Moves the effect a bit towards the player so it shows on the side that got hit
                glm::vec3 towards_player = glm::normalize(m_player.object()->position() + glm::vec3(0.f, 0.8f, 0.f) - hit_position);
                m_combat_vfx.spawn_hit_burst(hit_position + towards_player * 0.35f, glm::vec3(1.0f, 0.75f, 0.25f), 22, 6.0f, 0.9f);
            }
            if (!hit_positions.empty())
            {
                m_player.trigger_camera_shake(0.15f, 0.12f);
                m_hit_stop_timer = 0.06f;
            }
        }

        //UPDATES ENEMY HORDE AND PRIEST AI, HIT DETECTION, RESPAWN, AND SOUL DROPS
        SpawnManager::UpdateEvents spawn_events = m_spawn_manager.update(time_step, m_player);

        for (const auto& drop_position : spawn_events.soul_drop_positions)
        {
            SoulFragmentPickup new_soul;
            new_soul.position = drop_position;
            new_soul.is_active = true;
            m_soul_pickups.push_back(new_soul);
        }


        for (const auto& cast_position : spawn_events.firebolt_spawn_positions)
        {
            glm::vec3 aim_point = m_player.object()->position() + glm::vec3(0.f, 1.0f, 0.f);
            glm::vec3 direction = aim_point - cast_position;
            m_firebolts.push_back(Firebolt(cast_position, direction, 9.0f, 12.0f));
        }

        for (auto& bolt : m_firebolts) {
            if (bolt.is_active()) {
                bolt.on_update(time_step);
                float dmg = bolt.check_collision(m_player.object()->position());
                if (dmg > 0.0f) m_player.take_damage(dmg);
            }
        }
        //Removes expired firebolts so the list doesn't grow forever
        m_firebolts.erase(std::remove_if(m_firebolts.begin(), m_firebolts.end(),
            [](const Firebolt& bolt) { return !bolt.is_active(); }), m_firebolts.end());

        //Soul pickups 
        glm::vec3 current_player_position = m_player.object()->position();
        for (auto& soul : m_soul_pickups)
        {
            if (soul.is_active)
            {
                float distance = glm::distance(current_player_position, soul.position);
                if (distance < 1.5f)
                {
                    soul.is_active = false;
                    m_soul_count++;
                    m_audio_manager->play_spatialised_sound("pickup", m_3d_camera.position(), soul.position);
                }
            }
        }
        //Removes collected souls so the list doesn't grow forever
        m_soul_pickups.erase(std::remove_if(m_soul_pickups.begin(), m_soul_pickups.end(),
            [](const SoulFragmentPickup& soul) { return !soul.is_active; }), m_soul_pickups.end());

        m_audio_manager->update_with_camera(m_3d_camera);

        //If the player lost health this frame then flash the screen red and spawn red sparks
        if (m_player.get_health() < m_last_player_health - 0.01f)
        {
            m_hurt_flash_timer = m_hurt_flash_duration;
            m_combat_vfx.spawn_hit_burst(m_player.object()->position() + glm::vec3(0.f, 0.7f, 0.f), glm::vec3(0.9f, 0.08f, 0.05f), 16, 4.5f, 0.6f);
        }
        m_last_player_health = m_player.get_health();

        //Waits for the death animation to finish before showing the defeat screen
        if (m_player.is_death_sequence_finished())
        {
            m_hurt_flash_timer = 0.0f;
            m_game_state = GameState::Defeat;
            m_defeat_selection = 0;
            engine::application::window().show_mouse_cursor();
        }
    }
}

void example_layer::render_shadow_casters(const engine::ref<engine::shader>& shader)
{
    engine::renderer::submit(shader, m_terrain);

    for (const auto& tree_transform : m_tree_transforms) engine::renderer::submit(shader, tree_transform, m_tree);

    if (m_tetrahedron) engine::renderer::submit(shader, m_tetrahedron);
    if (m_stone_block) engine::renderer::submit(shader, m_stone_block);
    if (m_castle_main) engine::renderer::submit(shader, m_castle_main);
    if (m_castle_tower_left) engine::renderer::submit(shader, m_castle_tower_left);
    if (m_castle_tower_right) engine::renderer::submit(shader, m_castle_tower_right);
    for (const auto& rock : m_rocks) engine::renderer::submit(shader, rock);

    if (m_halberd)
    {
        glm::mat4 halberd_transform(1.0f);
        m_halberd->transform(halberd_transform);
        for (const auto& mesh : m_halberd->meshes()) engine::renderer::submit(shader, mesh, halberd_transform);
    }

    m_spawn_manager.render(shader);

    if (!m_boss_logic.has_vanished())
    {
        glm::mat4 boss_transform(1.0f);
        m_boss_object->transform(boss_transform);
        engine::renderer::submit(shader, boss_transform, m_boss_object);
    }

    engine::renderer::submit(shader, m_player.render_transform(), m_player.object());
}

void example_layer::render_static_world(const engine::ref<engine::shader>& mesh_shader, const glm::vec3& camera_pos)
{
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gEyeWorldPos", camera_pos);
    glm::mat4 skybox_transform(1.0f);
    skybox_transform = glm::translate(skybox_transform, camera_pos);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
    for (const auto& texture : m_skybox->textures()) texture->bind();
    engine::renderer::submit(mesh_shader, m_skybox, skybox_transform);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
    engine::renderer::submit(mesh_shader, m_terrain);

    if (m_grass_model && m_grass_texture) {

        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);

        m_grass_texture->bind();
        m_grass_material->submit(mesh_shader);

        for (const auto& transform : m_grass_transforms) {
            for (const auto& mesh : m_grass_model->meshes()) {
                engine::renderer::submit(mesh_shader, mesh, transform);
            }
        }

        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
    }

    for (const auto& tree_transform : m_tree_transforms) engine::renderer::submit(mesh_shader, tree_transform, m_tree);

    //RENDERS PRIMATIVES
    m_tetrahedron_material->submit(mesh_shader);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);

    if (m_tetrahedron) {
        m_tetrahedron->textures().at(0)->bind();
        engine::renderer::submit(mesh_shader, m_tetrahedron);
    }

    if (m_stone_block) {
        m_stone_block->textures().at(0)->bind();
        engine::renderer::submit(mesh_shader, m_stone_block);
    }
    //Castle uses primatives to make a castle appearance
    if (m_castle_main) {
        m_castle_main->textures().at(0)->bind();

        engine::renderer::submit(mesh_shader, m_castle_main);
        if (m_castle_tower_left) engine::renderer::submit(mesh_shader, m_castle_tower_left);
        if (m_castle_tower_right) engine::renderer::submit(mesh_shader, m_castle_tower_right);
    }

    //Scattered rocks
    for (const auto& rock : m_rocks)
    {
        rock->textures().at(0)->bind();
        engine::renderer::submit(mesh_shader, rock);
    }

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
}

void example_layer::on_render()
{
    engine::render_command::clear_color({ 0.0f, 0.0f, 0.0f, 1.0f });
    engine::render_command::clear();

    if (m_game_state == GameState::MainMenu)
    {
        const auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
        engine::renderer::begin_scene(m_2d_camera, mesh_shader);
        m_hud.render_menu_background(mesh_shader, m_intro_texture, m_quad_mesh, m_dim_overlay_material);
        engine::renderer::end_scene();

        const auto text_shader = engine::renderer::shaders_library()->get("text_2D");
        m_hud.render_main_menu(text_shader, m_menu_selection, m_player.get_mouse_sensitivity(), m_music_volume, m_run_started);
    }
    else if (m_game_state == GameState::InGame || m_game_state == GameState::PauseMenu || m_game_state == GameState::Defeat)
    {
        m_shadow_renderer.render(m_directionalLight.Direction,
            m_player.object() ? m_player.object()->position() : glm::vec3(0.f),
            [&](const engine::ref<engine::shader>& shadow_shader) { render_shadow_casters(shadow_shader); });

        const auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
        engine::renderer::begin_scene(m_3d_camera, mesh_shader);
        m_shadow_renderer.bind_for_sampling(mesh_shader, 1);

        glm::vec3 camera_pos = m_3d_camera.position();
        render_static_world(mesh_shader, camera_pos);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
        m_spawn_manager.render(mesh_shader);

        if (!m_boss_logic.has_vanished()) {
            glm::mat4 boss_transform(1.0f);
            m_boss_object->transform(boss_transform);
            engine::renderer::submit(mesh_shader, boss_transform, m_boss_object);
        }

        //Renders souls
        m_soul_material->submit(mesh_shader);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);
        m_soul_texture->bind();
        for (const auto& soul : m_soul_pickups)
        {
            if (soul.is_active)
            {
                glm::vec3 soul_pos = soul.position;
                glm::vec3 direction_to_camera = glm::normalize(camera_pos - soul_pos);
                float angle = atan2(direction_to_camera.x, direction_to_camera.z);
                glm::mat4 soul_transform(1.0f);
                soul_transform = glm::translate(soul_transform, soul_pos);
                soul_transform = glm::rotate(soul_transform, angle, glm::vec3(0, 1, 0));
                soul_transform = glm::scale(soul_transform, glm::vec3(0.2f));
                engine::renderer::submit(mesh_shader, m_soul_mesh, soul_transform);
            }
        }
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
        //Halberd and lights rendering
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);
        m_halberd_material->submit(mesh_shader);
        m_halberd->textures().at(0)->bind();
        glm::mat4 halberd_transform(1.0f);
        m_halberd->transform(halberd_transform);
        for (const auto& mesh : m_halberd->meshes()) engine::renderer::submit(mesh_shader, mesh, halberd_transform);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);

        engine::renderer::submit(mesh_shader, m_player.render_transform(), m_player.object());

        m_combat_vfx.render(mesh_shader, camera_pos, m_player, m_spawn_manager, m_boss_logic, m_boss_object, m_firebolts);
        m_lock_on.render_reticle(mesh_shader, camera_pos);

        engine::renderer::end_scene();

        engine::renderer::begin_scene(m_2d_camera, mesh_shader);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
        engine::render_command::disable_depth_test();

        //Red screen tint when hit that fades out quickly
        if (m_hurt_flash_timer > 0.0f)
        {
            m_hurt_overlay_material->set_transparency(0.14f * (m_hurt_flash_timer / m_hurt_flash_duration));
            m_hurt_overlay_material->submit(mesh_shader);
            engine::renderer::submit(mesh_shader, m_quad_mesh, glm::mat4(1.0f));
        }

        m_hud.render_player_bars(mesh_shader, m_player);
        if (!m_boss_logic.is_dead())
            m_hud.render_boss_bar_graphic(mesh_shader, m_boss_logic.get_health_percent());

        engine::ref<engine::game_object> bar_target = m_lock_on.is_locked() ? m_lock_on.target()
            : m_lock_on.find_closest_target_to_screen_center(m_3d_camera, m_spawn_manager, m_boss_logic, m_boss_object);
        if (bar_target && bar_target != m_boss_object)
        {
            float target_pct = -1.0f;
            glm::vec3 target_colour(1.f, 0.f, 0.f);

            const auto& enemies = m_spawn_manager.enemies();
            const auto& warriors = m_spawn_manager.warrior_objects();
            for (size_t i = 0; i < warriors.size() && target_pct < 0.0f; i++)
                if (warriors[i] == bar_target) target_pct = enemies[i].get_health_percent();

            if (target_pct < 0.0f)
            {
                const auto& priests = m_spawn_manager.priests();
                const auto& priest_objects = m_spawn_manager.priest_objects();
                for (size_t i = 0; i < priest_objects.size() && target_pct < 0.0f; i++)
                    if (priest_objects[i] == bar_target) { target_pct = priests[i].get_health_percent(); target_colour = glm::vec3(0.f, 0.8f, 1.f); }
            }

            if (target_pct >= 0.0f)
                m_hud.render_floating_bar(mesh_shader, m_3d_camera, bar_target->position(), target_pct, target_colour);
        }

        engine::render_command::enable_depth_test();
        engine::renderer::end_scene();

        const auto text_shader = engine::renderer::shaders_library()->get("text_2D");
        m_hud.render_game_hud(text_shader, m_player, m_soul_count);
        if (!m_boss_logic.is_dead())
            m_hud.render_boss_name_text(text_shader);

        if (m_game_state == GameState::PauseMenu || m_game_state == GameState::Defeat)
        {
            engine::renderer::begin_scene(m_2d_camera, mesh_shader);
            m_hud.render_dim_overlay(mesh_shader, m_quad_mesh, m_dim_overlay_material);
            engine::renderer::end_scene();

            if (m_game_state == GameState::PauseMenu)
                m_hud.render_pause_menu(text_shader, m_player, m_soul_count, m_pause_selection, m_music_volume, m_pause_message);
            else
                m_hud.render_defeat_screen(text_shader, m_defeat_selection);
        }
    }
}

void example_layer::start_game()
{
    m_run_started = true;
    m_game_state = GameState::InGame;
    engine::application::window().hide_mouse_cursor();
}

void example_layer::open_pause_menu()
{
    m_game_state = GameState::PauseMenu;
    m_pause_selection = Hud::PauseResume;
    m_pause_message.clear();
    engine::application::window().show_mouse_cursor();
}

void example_layer::open_main_menu()
{
    m_game_state = GameState::MainMenu;
    m_menu_selection = Hud::MainStart;
    engine::application::window().show_mouse_cursor();
}

void example_layer::adjust_setting(bool sensitivity, int direction)
{
    if (sensitivity)
    {
        m_player.set_mouse_sensitivity(m_player.get_mouse_sensitivity() + 0.05f * direction);
    }
    else
    {
        m_music_volume = glm::clamp(m_music_volume + 0.05f * direction, 0.0f, 1.0f);
        m_audio_manager->volume("music", m_music_volume);
    }
}

bool example_layer::try_spend_souls(int cost)
{
    if (m_soul_count < cost)
    {
        m_pause_message = "Not enough souls (need " + std::to_string(cost) + ")";
        return false;
    }
    m_soul_count -= cost;
    return true;
}

void example_layer::on_event(engine::event& event)
{
    if (event.event_type() != engine::event_type_e::key_pressed) return;

    auto& e = dynamic_cast<engine::key_pressed_event&>(event);
    const int key = e.key_code();

    //Menu controls (W/S or arrows to move, A/D or arrows to change values, Space/Enter to select)
    const bool up = key == engine::key_codes::KEY_W || key == engine::key_codes::KEY_UP;
    const bool down = key == engine::key_codes::KEY_S || key == engine::key_codes::KEY_DOWN;
    const bool left = key == engine::key_codes::KEY_A || key == engine::key_codes::KEY_LEFT;
    const bool right = key == engine::key_codes::KEY_D || key == engine::key_codes::KEY_RIGHT;
    const bool select = key == engine::key_codes::KEY_SPACE || key == engine::key_codes::KEY_ENTER || key == engine::key_codes::KEY_KP_ENTER;
    const bool pause_key = key == engine::key_codes::KEY_ESCAPE || key == engine::key_codes::KEY_E;
    auto navigate = [&](int& selection, int count)
    {
        if (up) selection = (selection + count - 1) % count;
        if (down) selection = (selection + 1) % count;
    };

    switch (m_game_state)
    {
    case GameState::MainMenu:
        navigate(m_menu_selection, Hud::MainMenuItemCount);
        if (m_menu_selection == Hud::MainSensitivity && (left || right)) adjust_setting(true, right ? 1 : -1);
        if (m_menu_selection == Hud::MainVolume && (left || right)) adjust_setting(false, right ? 1 : -1);
        if (select)
        {
            if (m_menu_selection == Hud::MainStart) start_game();
            else if (m_menu_selection == Hud::MainQuit) engine::application::exit();
        }
        break;

    case GameState::InGame:
        if (pause_key) open_pause_menu();
        if (key == engine::key_codes::KEY_P) m_spawn_manager.spawn_enemy(m_player, m_game_objects);
        if (key == engine::key_codes::KEY_R) m_player.use_potion();
        break;

    case GameState::PauseMenu:
        if (pause_key) { start_game(); break; }
        if (up || down) m_pause_message.clear();
        navigate(m_pause_selection, Hud::PauseMenuItemCount);
        if (m_pause_selection == Hud::PauseSensitivity && (left || right)) adjust_setting(true, right ? 1 : -1);
        if (m_pause_selection == Hud::PauseVolume && (left || right)) adjust_setting(false, right ? 1 : -1);
        if (select)
        {
            switch (m_pause_selection)
            {
            case Hud::PauseResume: start_game(); break;
            case Hud::PauseDamage:
                if (try_spend_souls(Hud::k_damage_upgrade_cost)) { m_player.increase_damage(10.0f); m_pause_message = "Damage increased!"; }
                break;
            case Hud::PauseSpeed:
                if (try_spend_souls(Hud::k_speed_upgrade_cost)) { m_player.increase_speed(2.0f); m_pause_message = "Move speed increased!"; }
                break;
            case Hud::PausePotion:
                if (try_spend_souls(Hud::k_potion_cost)) { m_player.add_potion(1); m_pause_message = "Bought a health potion!"; }
                break;
            case Hud::PauseMainMenu: open_main_menu(); break;
            case Hud::PauseQuit: engine::application::exit(); break;
            default: break;
            }
        }
        break;

    case GameState::Defeat:
        navigate(m_defeat_selection, Hud::DefeatMenuItemCount);
        if (select)
        {
            if (m_defeat_selection == Hud::DefeatRespawn) { m_player.respawn(); start_game(); }
            else if (m_defeat_selection == Hud::DefeatMainMenu) { m_player.respawn(); open_main_menu(); }
            else if (m_defeat_selection == Hud::DefeatQuit) engine::application::exit();
        }
        break;
    }

    //Debug keys work in any state
    if (key == engine::key_codes::KEY_F1) engine::render_command::toggle_wireframe();
    if (key == engine::key_codes::KEY_F2) m_player.toggle_invincible();
}
