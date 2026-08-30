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
#include <sstream>
#include <iomanip>
#include "btBulletDynamicsCommon.h"
#include <stdlib.h>

example_layer::example_layer()
    :m_2d_camera(-1.6f, 1.6f, -0.9f, 0.9f),
    m_3d_camera((float)engine::application::window().width(), (float)engine::application::window().height())
{
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
    m_audio_manager->load_sound("assets/audio/bounce.wav", engine::sound_type::spatialised, "bounce");
    m_audio_manager->load_sound("assets/audio/fall.wav", engine::sound_type::spatialised, "dash");
    //Enough sound effects for the mark scheme
    m_audio_manager->play("music");
    m_audio_manager->volume("music", m_music_volume);

    auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
    auto text_shader = engine::renderer::shaders_library()->get("text_2D");

    //ALL OF THE LIGHTS
    //Directional low level light to mimic a sunset and the themic yellow sky using low intensity and ambience
    m_directionalLight.Color = glm::vec3(1.0f, 1.0f, 1.0f);
    m_directionalLight.AmbientIntensity = 0.05f;
    m_directionalLight.DiffuseIntensity = 0.4f;
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

    //Yellow lgihts spawned arround the trees 
    for (int i = 0; i < 4; i++) {
        engine::PointLight p;
        p.Color = glm::vec3(1.0f, 0.8f, 0.2f);
        p.AmbientIntensity = 0.0f;
        p.DiffuseIntensity = 25.0f;

        int idx = rand() % 67; //randomised 
        glm::vec3 tPos = glm::vec3(0.f, 0.0f, 0.f);
        p.Position = tPos + glm::vec3(0.f, 4.0f, 0.f); //higher up so its near the leaves
        p.Attenuation.Constant = 1.0f;
        p.Attenuation.Linear = 0.01f; //faster fall off for hte lights instead of a gradient
        p.Attenuation.Exp = 0.01f;

        m_tree_lights.push_back(p);
    }
    // Update the total count of point lights for the shader
    num_point_lights = 2 + m_tree_lights.size();

    //Submits initial lights data to the shader
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->bind();
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gColorMap", 0);
    m_directionalLight.submit(mesh_shader);

    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gNumPointLights", (int)num_point_lights);
    m_pointLight.submit(mesh_shader, 0);
    m_halberdLight.submit(mesh_shader, 1);

    for (size_t i = 0; i < m_tree_lights.size(); i++) {
        m_tree_lights[i].submit(mesh_shader, 2 + i);
    }

    //More shader setup 
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gMatSpecularIntensity", 1.f);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gSpecularPower", 10.f);
    std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("transparency", 1.0f);

    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->bind();
    std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("projection",
        glm::ortho(0.f, (float)engine::application::window().width(), 0.f,
            (float)engine::application::window().height()));
    m_material = engine::material::create(1.0f, glm::vec3(1.0f, 0.1f, 0.07f),
        glm::vec3(1.0f, 0.1f, 0.07f), glm::vec3(0.5f, 0.5f, 0.5f), 1.0f);


    m_hologram_material = engine::material::create(32.0f,
        glm::vec3(0.0f, 1.0f, 1.0f),
        glm::vec3(0.0f, 1.0f, 1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        0.6f);


    m_skybox = engine::skybox::create(50.f,
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
    m_berzerker_textures.push_back(engine::texture_2d::create("assets/textures/berzerker.png", false));
    m_berzerker_mesh = engine::skinned_mesh::create("assets/models/animated/berzerker/berzerker.fbx");
    m_berzerker_mesh->switch_root_movement(false);
    m_berzerker_mesh->set_textures(m_berzerker_textures);

    //PRIEST (Ranged mage) from https://opengameart.org/content/animated-priest
    m_priest_textures.push_back(engine::texture_2d::create("assets/textures/priest.png", false));
    m_priest_mesh = engine::skinned_mesh::create("assets/models/animated/priest/priest.fbx");
    m_priest_mesh->switch_root_movement(false);
    m_priest_mesh->set_textures(m_priest_textures);

    //GRASS (Static mesh to add detail to terrain) from https://opengameart.org/content/3d-grass-patch
    m_grass_model = engine::model::create("assets/models/static/high_grass.fbx");
    m_grass_texture = engine::texture_2d::create("assets/textures/grass.png", false);

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
        SpawnEnemy();
    }
    for (int i = 0; i < m_num_priests_to_spawn; i++)
    {
        SpawnPriest();
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
    boss_props.scale = glm::vec3(0.3f);
    boss_props.textures = boss_textures;
    boss_props.type = 0;
    boss_props.mass = 1.0f;
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
    m_intro_texture = engine::texture_2d::create("assets/textures/Rock1.jpg", true);
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
        float randomisetree = ((float)rand() / (float)RAND_MAX - 2.f) * 0.1f;
        angle += randomisetree;
        float x_pos = sin(angle) * tree_circle_radius;
        float z_pos = cos(angle) * tree_circle_radius;
        float tree_rotation = angle + (float)(i % 5);
        glm::mat4 loop_tree_generation(1.0f);
        loop_tree_generation = glm::translate(loop_tree_generation, glm::vec3(x_pos, 0.0f, z_pos));
        loop_tree_generation = glm::rotate(loop_tree_generation, tree_rotation, m_tree->rotation_axis());
        loop_tree_generation = glm::scale(loop_tree_generation, m_tree->scale() * scale_multiplier);
        m_tree_transforms.push_back(loop_tree_generation);
    }

    //PHYSICS to prevent the player from walking through it
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

    m_physics_manager = engine::bullet_manager::create(m_game_objects);
    //Sets up damping for the halberd projectile after its physical body is made
    if (m_physics_manager->physical_objects.size() > 2) {
        if (m_physics_manager->physical_objects.size() > 3) {
            btRigidBody* halberd_body = m_physics_manager->physical_objects.at(3)->get_body();
            if (halberd_body) halberd_body->setDamping(0.5f, 0.8f);
        }
    }

    m_text_manager = engine::text_manager::create();
    m_player.update_camera(m_3d_camera);
}

example_layer::~example_layer() {}

void example_layer::SpawnEnemy()
{
    //Checks if mesh is loaded
    if (!m_berzerker_mesh) return;

    engine::ref<engine::skinned_mesh> unique_mesh = engine::skinned_mesh::create("assets/models/animated/berzerker/berzerker.fbx");
    unique_mesh->switch_root_movement(false);
    unique_mesh->set_textures(m_berzerker_textures);

    engine::game_object_properties enemy_props;
    enemy_props.animated_mesh = unique_mesh;
    enemy_props.scale = glm::vec3(0.15f);
    enemy_props.textures = m_berzerker_textures;
    enemy_props.type = 0;
    enemy_props.mass = 1.0f;
    enemy_props.friction = 1.0f;
    enemy_props.restitution = 0.0f;
    enemy_props.bounding_shape = glm::vec3(0.5f, 0.9f, 0.5f);

    glm::vec3 spawn_pos;
    bool valid_position = false;
    int attempts = 0;

    // Loop to ensure enemy doesn't spawn right on top of player or within a certain radius
    while (!valid_position && attempts < 10)
    {
        float rX = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
        float rZ = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
        spawn_pos = glm::vec3(rX, 5.0f, rZ);

        if (m_player.object()) {
            float dist_to_player = glm::distance(spawn_pos, m_player.object()->position());
            if (dist_to_player > m_safe_radius) {
                valid_position = true;
            }
        }
        else {
            valid_position = true;
        }
        attempts++;
    }

    enemy_props.position = spawn_pos;

    engine::ref<engine::game_object> warrior = engine::game_object::create(enemy_props);
    warrior->set_angular_factor_lock(true);
    warrior->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    m_warriors.push_back(warrior);

    if (m_physics_manager) {
        m_game_objects.push_back(warrior);
    }
    else {
        m_game_objects.push_back(warrior);
    }
    //FSM setup for AI behaviour 
    Enemy enemy_logic;
    if (m_player.object()) {
        enemy_logic.initialise(warrior, enemy_props.position, m_player.object());
    }
    m_enemies.push_back(enemy_logic);
}

void example_layer::SpawnPriest()
{
    if (!m_priest_mesh) return;
    //Creates another unique mesh for the priest mage
    engine::ref<engine::skinned_mesh> unique_mesh = engine::skinned_mesh::create("assets/models/animated/priest/priest.fbx");
    unique_mesh->switch_root_movement(false);
    unique_mesh->set_textures(m_priest_textures);

    engine::game_object_properties priest_props;
    priest_props.animated_mesh = unique_mesh;
    priest_props.scale = glm::vec3(0.15f);
    priest_props.textures = m_priest_textures;
    priest_props.type = 0;
    priest_props.mass = 1.0f;
    priest_props.friction = 1.0f;
    priest_props.restitution = 0.0f;
    priest_props.bounding_shape = glm::vec3(0.5f, 0.9f, 0.5f);

    float rX = ((float)rand() / (float)RAND_MAX * 50.0f) - 25.0f;
    float rZ = ((float)rand() / (float)RAND_MAX * 50.0f) - 25.0f;
    priest_props.position = glm::vec3(rX, 5.0f, rZ);

    engine::ref<engine::game_object> priest_obj = engine::game_object::create(priest_props);
    priest_obj->set_angular_factor_lock(true);
    priest_obj->set_offset(glm::vec3(0.0f, 0.0f, 0.0f));

    m_priest_objects.push_back(priest_obj);

    if (m_physics_manager) {
        m_game_objects.push_back(priest_obj);
    }
    else {
        m_game_objects.push_back(priest_obj);
    }

    Priest priest_logic;
    if (m_player.object()) {
        priest_logic.initialise(priest_obj, priest_props.position, m_player.object());
    }
    m_priests.push_back(priest_logic);
}

void example_layer::on_update(const engine::timestep& time_step)
{
    if (m_game_state == GameState::PauseMenu) return;

    if (m_game_state == GameState::InGame)
    {
        m_physics_manager->dynamics_world_update(m_game_objects, double(time_step));

        m_player.on_update(time_step);
        m_player.update_camera(m_3d_camera);
        //AUDIO SETUP FOR ROLLING/DASHING AND ATTACKING
        //Attacking audio
        static bool was_attacking = false;
        bool is_attacking = m_player.is_attacking();
        if (is_attacking && !was_attacking) {

            m_audio_manager->play_spatialised_sound("attack", m_3d_camera.position(), m_player.object()->position());
        }
        was_attacking = is_attacking;
        //Rolling/Dashing audio
        static bool was_rolling = false;
        bool is_rolling = m_player.is_rolling();
        if (is_rolling && !was_rolling) {
            m_audio_manager->play_spatialised_sound("dash", m_3d_camera.position(), m_player.object()->position());
        }
        was_rolling = is_rolling;

        //UPDATES BOSS FSM LOGIC
        m_boss_logic.on_update(time_step);
        if (m_player.is_attacking()) {
            float dist = glm::distance(m_player.object()->position(), m_boss_object->position());
            if (dist < 5.0f) {
                glm::vec3 to_boss = glm::normalize(m_boss_object->position() - m_player.object()->position());
                if (glm::dot(m_player.object()->forward(), to_boss) > 0.5f) {
                    m_boss_logic.take_damage(m_player.get_damage() * (float)time_step);
                }
            }
        }
        if (m_boss_logic.check_hit_player()) m_player.take_damage(25.0f);

        //UPDATES ENEMY HORDE SIZE AND LOGIC
        for (size_t i = 0; i < m_enemies.size(); i++)
        {
            m_enemies[i].on_update(time_step);

            //Checks if player attack connects to enemy
            if (m_player.is_attacking()) {
                float dist = glm::distance(m_player.object()->position(), m_warriors[i]->position());
                if (dist < 3.0f) {
                    glm::vec3 to_enemy = glm::normalize(m_warriors[i]->position() - m_player.object()->position());
                    if (glm::dot(m_player.object()->forward(), to_enemy) > 0.5f) {
                        if (!m_enemies[i].is_dead()) {
                            m_enemies[i].take_damage(m_player.get_damage() * (float)time_step);
                        }
                    }
                }
            }

            if (m_enemies[i].check_hit_player()) {
                m_player.take_damage(10.0f);
            }

            //RESPAWN MECHANIC where if the enemy is killed and vanishes, then they will be respawned outside the safe zone)
            if (m_enemies[i].has_vanished())
            {
                glm::vec3 spawn_pos;
                bool valid = false;
                int attempts = 0;
                while (!valid && attempts < 10) {
                    float rX = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
                    float rZ = ((float)rand() / (float)RAND_MAX * (m_spawn_radius * 2.0f)) - m_spawn_radius;
                    spawn_pos = glm::vec3(rX, 5.0f, rZ);
                    if (glm::distance(spawn_pos, m_player.object()->position()) > m_safe_radius) valid = true;
                    attempts++;
                }
                m_enemies[i].initialise(m_warriors[i], spawn_pos, m_player.object());
            }

            //Drops a random set number of souls
            if (m_enemies[i].is_dead() && !m_enemies[i].souls_dropped())
            {
                int num_souls = (rand() % 5) + 1;
                glm::vec3 drop_center = m_warriors[i]->position();
                for (int s = 0; s < num_souls; s++) {
                    float rX = ((float)rand() / (float)RAND_MAX * 3.0f) - 1.5f;
                    float rZ = ((float)rand() / (float)RAND_MAX * 3.0f) - 1.5f;
                    SoulFragmentPickup new_soul;
                    new_soul.position = glm::vec3(drop_center.x + rX, 0.5f, drop_center.z + rZ);
                    new_soul.is_active = true;
                    m_soul_pickups.push_back(new_soul);
                }
                m_enemies[i].set_souls_dropped(true);
            }
        }

        //UPDATES PRIEST RUNNING AI
        for (size_t i = 0; i < m_priests.size(); i++)
        {
            m_priests[i].on_update(time_step);

            if (m_priests[i].should_spawn_projectile()) {
                HolyProjectile proj;
                engine::game_object_properties proj_props;
                proj_props.meshes = m_projectile_model->meshes();
                proj_props.scale = glm::vec3(0.15f);
                proj_props.position = m_player.object()->position() + glm::vec3(0.0f, 10.0f, 0.0f);
                proj_props.bounding_shape = m_projectile_model->size() / 2.f;
                engine::ref<engine::game_object> proj_obj = engine::game_object::create(proj_props);

                proj.initialise(proj_obj);
                m_projectiles.push_back(proj);
            }

            //Checks if players attack connects with them
            if (m_player.is_attacking()) {
                float dist = glm::distance(m_player.object()->position(), m_priest_objects[i]->position());
                if (dist < 3.0f) {
                    glm::vec3 to_enemy = glm::normalize(m_priest_objects[i]->position() - m_player.object()->position());
                    if (glm::dot(m_player.object()->forward(), to_enemy) > 0.5f) {
                        m_priests[i].take_damage(m_player.get_damage() * (float)time_step);
                    }
                }
            }

            //The Priest mage soul drop logic
            if (m_priests[i].is_dead() && !m_priests[i].souls_dropped())
            {
                int num_souls = (rand() % 6) + 3;
                glm::vec3 drop_center = m_priest_objects[i]->position();

                for (int s = 0; s < num_souls; s++) {
                    float rX = ((float)rand() / (float)RAND_MAX * 2.0f) - 1.0f;
                    float rZ = ((float)rand() / (float)RAND_MAX * 2.0f) - 1.0f;
                    SoulFragmentPickup new_soul;
                    new_soul.position = glm::vec3(drop_center.x + rX, 0.5f, drop_center.z + rZ);
                    new_soul.is_active = true;
                    m_soul_pickups.push_back(new_soul);
                }
                m_priests[i].set_souls_dropped(true);
            }
        }

        //Updates the halberd projectiles
        for (auto& proj : m_projectiles) {
            if (proj.is_active()) {
                proj.on_update(time_step);
                float dmg = proj.check_collision(m_player.object());
                if (dmg > 0.0f) m_player.take_damage(dmg);
            }
        }

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

        m_audio_manager->update_with_camera(m_3d_camera);
        check_bounce();
    }
}

void example_layer::on_render()
{
    engine::render_command::clear_color({ 0.0f, 0.0f, 0.0f, 1.0f });
    engine::render_command::clear();

    if (m_game_state == GameState::MainMenu)
    {
        const auto text_shader = engine::renderer::shaders_library()->get("text_2D");
        engine::renderer::begin_scene(m_2d_camera, text_shader);
        std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("has_texture", true);
        m_intro_texture->bind();
        glm::mat4 transform(1.0f);
        engine::renderer::submit(text_shader, m_quad_mesh, transform);
        std::dynamic_pointer_cast<engine::gl_shader>(text_shader)->set_uniform("has_texture", false);

        float width = (float)engine::application::window().width();
        float height = (float)engine::application::window().height();

        m_text_manager->render_text(text_shader, "medievalsoulsgameforadvancedgamesproject", 100.f, height / 2.f + 100.f, 1.0f, glm::vec4(1.f, 1.f, 0.f, 1.f));

        //Main menu selection text setup
        if (m_menu_selection == 0)
        {
            m_text_manager->render_text(text_shader, "> START", 100.f, height / 2.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
            m_text_manager->render_text(text_shader, "  SENSITIVITY", 100.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
            m_text_manager->render_text(text_shader, "  VOLUME", 100.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        }
        else if (m_menu_selection == 1)
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
        //Mouse sens setup
        std::stringstream sensitivity_calc;
        sensitivity_calc << std::fixed << std::setprecision(2) << m_player.get_mouse_sensitivity();
        std::string sens_text = "< " + sensitivity_calc.str() + " >";
        m_text_manager->render_text(text_shader, sens_text, 350.f, height / 2.f - 50.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
        //Music volume setup
        std::stringstream vol_calc;
        vol_calc << std::fixed << std::setprecision(2) << m_music_volume;
        std::string vol_text = "< " + vol_calc.str() + " >";
        m_text_manager->render_text(text_shader, vol_text, 350.f, height / 2.f - 100.f, 0.7f, glm::vec4(1.f, 1.f, 0.f, 1.f));
        //Controls setup
        m_text_manager->render_text(text_shader, "Controls:", 10.f, height - 25.f, 0.5f, glm::vec4(1.f));
        m_text_manager->render_text(text_shader, "WASD: Move | L-Shift: Dash | R: Health Potion", 10.f, height - 50.f, 0.5f, glm::vec4(1.f));
        m_text_manager->render_text(text_shader, "Mouse: Move Camera", 10.f, height - 75.f, 0.5f, glm::vec4(1.f));
        m_text_manager->render_text(text_shader, "Space: Attack | E: Toggle Upgrade Menu", 10.f, height - 100.f, 0.5f, glm::vec4(1.f));
        m_text_manager->render_text(text_shader, "P: Spawn Enemy", 10.f, height - 125.f, 0.5f, glm::vec4(1.f));

        engine::renderer::end_scene();
    }
    else if (m_game_state == GameState::InGame || m_game_state == GameState::PauseMenu)
    {
        const auto mesh_shader = engine::renderer::shaders_library()->get("mesh");
        engine::renderer::begin_scene(m_3d_camera, mesh_shader);

        glm::vec3 camera_pos = m_3d_camera.position();
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gEyeWorldPos", camera_pos);
        glm::mat4 skybox_transform(1.0f);
        skybox_transform = glm::translate(skybox_transform, m_3d_camera.position());
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", false);
        for (const auto& texture : m_skybox->textures()) texture->bind();
        engine::renderer::submit(mesh_shader, m_skybox, skybox_transform);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("lighting_on", true);
        engine::renderer::submit(mesh_shader, m_terrain);

        if (m_grass_model && m_grass_texture) {

            std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", true);

            m_grass_texture->bind();

            engine::ref<engine::material> green_material = engine::material::create(
                1.0f, glm::vec3(0.1f, 0.5f, 0.1f),
                glm::vec3(0.1f, 0.5f, 0.1f),
                glm::vec3(0.2f, 0.2f, 0.2f), 1.0f
            );
            green_material->submit(mesh_shader);

            for (const auto& transform : m_grass_transforms) {
                for (const auto& mesh : m_grass_model->meshes()) {
                    engine::renderer::submit(mesh_shader, mesh, transform);
                }
            }

            std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);
        }
        //Holographic halberd projectiles setup
        m_hologram_material->submit(mesh_shader);
        m_halberd->textures().at(0)->bind();
        for (const auto& proj : m_projectiles) {
            if (proj.is_active()) {
                glm::mat4 proj_transform(1.0f);
                proj_transform = glm::translate(proj_transform, proj.object()->position());
                //Adds a rotation to seem like a falling spinning halberd magic spell
                proj_transform = glm::rotate(proj_transform, proj.object()->rotation_amount(), proj.object()->rotation_axis());

                proj_transform = glm::scale(proj_transform, glm::vec3(0.15f));
                for (const auto& mesh : m_projectile_model->meshes()) {
                    engine::renderer::submit(mesh_shader, mesh, proj_transform);
                }
            }
        }
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);

        //RENDERS ALL ENEMIES
        for (size_t i = 0; i < m_enemies.size(); i++)
        {
            if (!m_enemies[i].has_vanished())
            {
                glm::mat4 warrior_transform(1.0f);
                m_warriors[i]->transform(warrior_transform);
                engine::renderer::submit(mesh_shader, warrior_transform, m_warriors[i]);
            }
        }
        //RENDERS ALL PRIESTS
        for (size_t i = 0; i < m_priests.size(); i++)
        {
            if (!m_priests[i].has_vanished())
            {
                glm::mat4 priest_transform(1.0f);
                m_priest_objects[i]->transform(priest_transform);
                engine::renderer::submit(mesh_shader, priest_transform, m_priest_objects[i]);
            }
        }
        //RENDERSE BOSS
        if (!m_boss_logic.has_vanished()) {
            glm::mat4 boss_transform(1.0f);
            m_boss_object->transform(boss_transform);
            engine::renderer::submit(mesh_shader, boss_transform, m_boss_object);
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

        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);

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
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("gNumPointLights", 1);
        m_pointLight.submit(mesh_shader, 0);
        std::dynamic_pointer_cast<engine::gl_shader>(mesh_shader)->set_uniform("has_texture", false);

        engine::renderer::submit(mesh_shader, m_player.object());
        engine::renderer::end_scene();
        //2D HUD
        const auto text_shader = engine::renderer::shaders_library()->get("text_2D");
        //HP RED TEXT
        std::stringstream hp_ss;
        hp_ss << "HP: " << (int)m_player.get_health() << "/" << (int)m_player.get_max_health();
        m_text_manager->render_text(text_shader, hp_ss.str(), 10.f, 25.f, 0.5f, glm::vec4(1.f, 0.2f, 0.2f, 1.f));
        //STAMINA GREEN TEXTE
        std::stringstream stm_ss;
        stm_ss << "Stamina: " << (int)m_player.get_stamina() << "/" << (int)m_player.get_max_stamina();
        m_text_manager->render_text(text_shader, stm_ss.str(), 10.f, 50.f, 0.5f, glm::vec4(0.f, 1.f, 0.f, 1.f));
        //SOULS WHITE TEXT
        std::string soul_text = "Soul Fragments: " + std::to_string(m_soul_count);
        m_text_manager->render_text(text_shader, soul_text, 10.f, 75.f, 0.5f, glm::vec4(1.f, 1.f, 1.f, 1.f));
        //POTION BLUE TEXT
        std::stringstream pot_ss;
        pot_ss << "Potions: " << m_player.get_potions();
        m_text_manager->render_text(text_shader, pot_ss.str(), 10.f, 100.f, 0.5f, glm::vec4(0.f, 1.f, 1.f, 1.f));

        //FLOATING RED ENEMY HEALTH BARS taht are attached to the enemys
        for (size_t i = 0; i < m_enemies.size(); i++)
        {
            if (!m_enemies[i].is_dead())
            {
                glm::vec3 pos = m_warriors[i]->position();
                pos.y += 2.5f;
                glm::vec4 clipSpace = m_3d_camera.projection_matrix() * m_3d_camera.view_matrix() * glm::vec4(pos, 1.0f);
                if (clipSpace.w > 0.0f) {
                    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
                    if (ndc.z >= 0.0f && ndc.z <= 1.0f) {
                        float screenW = (float)engine::application::window().width();
                        float screenH = (float)engine::application::window().height();
                        float x = (ndc.x + 1.0f) / 2.0f * screenW;
                        float y = (ndc.y + 1.0f) / 2.0f * screenH;

                        float pct = m_enemies[i].get_health_percent();
                        std::string bar = "[";
                        int bars = 10;
                        int fill = (int)(pct * bars);
                        for (int b = 0; b < bars; b++) bar += (b < fill) ? "|" : " ";
                        bar += "]";

                        m_text_manager->render_text(text_shader, bar, x - 30, y, 0.3f, glm::vec4(1.f, 0.f, 0.f, 1.f));
                    }
                }
            }
        }

        //Same floating health bars but for hte priests but theyre blue to allow the player to see the difference
        for (size_t i = 0; i < m_priests.size(); i++)
        {
            if (!m_priests[i].is_dead())
            {
                glm::vec3 pos = m_priest_objects[i]->position();
                pos.y += 2.5f;
                glm::vec4 clipSpace = m_3d_camera.projection_matrix() * m_3d_camera.view_matrix() * glm::vec4(pos, 1.0f);
                if (clipSpace.w > 0.0f) {
                    glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;
                    if (ndc.z >= 0.0f && ndc.z <= 1.0f) {
                        float screenW = (float)engine::application::window().width();
                        float screenH = (float)engine::application::window().height();
                        float x = (ndc.x + 1.0f) / 2.0f * screenW;
                        float y = (ndc.y + 1.0f) / 2.0f * screenH;

                        float pct = m_priests[i].get_health_percent();
                        std::string bar = "[";
                        int bars = 10;
                        int fill = (int)(pct * bars);
                        for (int b = 0; b < bars; b++) bar += (b < fill) ? "|" : " ";
                        bar += "]";

                        m_text_manager->render_text(text_shader, bar, x - 30, y, 0.3f, glm::vec4(0.f, 0.8f, 1.f, 1.f));
                    }
                }
            }
        }

        //Boss HUD Appearance which always is on the screen just like in other souls games
        if (!m_boss_logic.is_dead()) {
            float pct = m_boss_logic.get_health_percent();
            std::string bar = "[";
            int bars = 20;
            int fill = (int)(pct * bars);
            for (int b = 0; b < bars; b++) bar += (b < fill) ? "|" : " ";
            bar += "]";

            float screenW = (float)engine::application::window().width();
            float screenH = (float)engine::application::window().height();
            m_text_manager->render_text(text_shader, "BOSS: DEVIL", screenW / 2 - 100, screenH - 40, 0.5f, glm::vec4(1.f, 0.5f, 0.f, 1.f));
            m_text_manager->render_text(text_shader, bar, screenW / 2 - 150, screenH - 70, 0.5f, glm::vec4(1.f, 0.f, 0.f, 1.f));
        }

        //Upgrade pause menu that pauses the game and alllows the player to spend souls
        if (m_game_state == GameState::PauseMenu)
        {
            float w = (float)engine::application::window().width();
            float h = (float)engine::application::window().height();

            m_text_manager->render_text(text_shader, "Pause Upgrade Menu", w / 2 - 150, h / 2 + 100, 1.0f, glm::vec4(1.f, 1.f, 0.f, 1.f));

            std::stringstream souls;
            souls << "Available Souls: " << m_soul_count;
            m_text_manager->render_text(text_shader, souls.str(), w / 2 - 100, h / 2 + 50, 0.7f, glm::vec4(1.f));

            std::stringstream dmg;
            dmg << "[1] Increase Damage (Cost: 5) Current: " << (int)m_player.get_damage();
            m_text_manager->render_text(text_shader, dmg.str(), w / 2 - 200, h / 2, 0.6f, glm::vec4(1.f, 0.2f, 0.2f, 1.f));

            std::stringstream spd;
            spd << "[2] Increase Attack Speed (Cost: 5) Current: " << (int)m_player.get_speed();
            m_text_manager->render_text(text_shader, spd.str(), w / 2 - 200, h / 2 - 40, 0.6f, glm::vec4(0.2f, 0.2f, 1.f, 1.f));

            std::stringstream pot;
            pot << "[3] Buy Health Potion (Cost: 10) Owned: " << m_player.get_potions();
            m_text_manager->render_text(text_shader, pot.str(), w / 2 - 200, h / 2 - 80, 0.6f, glm::vec4(0.f, 1.f, 0.f, 1.f));

            m_text_manager->render_text(text_shader, "Press E to Resume", w / 2 - 120, h / 2 - 140, 0.5f, glm::vec4(0.8f));
        }
    }
}
//Keyboard set up for the HUD and upgrades and others when its paused ingame, ingame, upgrades in pause menu and main menu
void example_layer::on_event(engine::event& event)
{
    if (event.event_type() == engine::event_type_e::key_pressed)
    {
        auto& e = dynamic_cast<engine::key_pressed_event&>(event);

        if (e.key_code() == engine::key_codes::KEY_E)
        {
            if (m_game_state == GameState::InGame) {
                m_game_state = GameState::PauseMenu;
                engine::application::window().show_mouse_cursor();
            }
            else if (m_game_state == GameState::PauseMenu) {
                m_game_state = GameState::InGame;
                engine::application::window().hide_mouse_cursor();
            }
        }

        if (m_game_state == GameState::InGame)
        {
            if (e.key_code() == engine::key_codes::KEY_P)
            {
                SpawnEnemy();
            }
            if (e.key_code() == engine::key_codes::KEY_R)
            {
                m_player.use_potion();
            }
        }

        if (m_game_state == GameState::PauseMenu)
        {
            if (e.key_code() == engine::key_codes::KEY_1)
            {
                if (m_soul_count >= 5) {
                    m_soul_count -= 5;
                    m_player.increase_damage(10.0f);
                }
            }
            if (e.key_code() == engine::key_codes::KEY_2)
            {
                if (m_soul_count >= 5) {
                    m_soul_count -= 5;
                    m_player.increase_speed(2.0f);
                }
            }
            if (e.key_code() == engine::key_codes::KEY_3)
            {
                if (m_soul_count >= 10) {
                    m_soul_count -= 10;
                    m_player.add_potion(1);
                }
            }
        }

        if (m_game_state == GameState::MainMenu)
        {
            if (e.key_code() == engine::key_codes::KEY_SPACE)
            {
                if (m_menu_selection == 0)
                {
                    m_game_state = GameState::InGame;
                    engine::application::window().hide_mouse_cursor();
                }
            }
            if (e.key_code() == engine::key_codes::KEY_W) { if (m_menu_selection > 0) m_menu_selection--; }
            if (e.key_code() == engine::key_codes::KEY_S) { if (m_menu_selection < 2) m_menu_selection++; }

            if (m_menu_selection == 1)
            {
                if (e.key_code() == engine::key_codes::KEY_A) { float s = m_player.get_mouse_sensitivity(); m_player.set_mouse_sensitivity(glm::max(s - 0.05f, 0.05f)); }
                if (e.key_code() == engine::key_codes::KEY_D) { float s = m_player.get_mouse_sensitivity(); m_player.set_mouse_sensitivity(s + 0.05f); }
            }

            else if (m_menu_selection == 2)
            {
                if (e.key_code() == engine::key_codes::KEY_A) { m_music_volume = glm::max(m_music_volume - 0.05f, 0.0f); m_audio_manager->volume("music", m_music_volume); }
                if (e.key_code() == engine::key_codes::KEY_D) { m_music_volume = glm::min(m_music_volume + 0.05f, 1.0f); m_audio_manager->volume("music", m_music_volume); }
            }
        }
        if (e.key_code() == engine::key_codes::KEY_F1)
        {
            engine::render_command::toggle_wireframe();
        }
    }
}
//from tutorial i dont use the check_bounce bounce sound effect anymore
void example_layer::check_bounce()
{
    if (!m_halberd) return;
    float current_vel = m_halberd->velocity().y;
    if (m_prev_halberd_y_vel < -1.0f && current_vel >= 0.0f)
    {
        m_audio_manager->play_spatialised_sound("bounce", m_3d_camera.position(), m_halberd->position());
    }
    m_prev_halberd_y_vel = current_vel;
}
