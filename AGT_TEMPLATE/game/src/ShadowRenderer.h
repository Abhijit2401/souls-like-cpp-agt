#pragma once
#include <engine.h>
#include <functional>

class ShadowRenderer
{
public:

    void initialise(uint32_t resolution = 2048);

    bool is_available() const { return m_shadow_map && m_shadow_map->is_valid(); }

    void render(const glm::vec3& light_direction, const glm::vec3& focus_position,
        const std::function<void(const engine::ref<engine::shader>&)>& draw_casters);
    void bind_for_sampling(const engine::ref<engine::shader>& mesh_shader, uint32_t texture_slot);

private:
    engine::ref<engine::shadow_map> m_shadow_map;
    glm::mat4 m_light_space_matrix{ 1.0f };

    float m_frustum_half_size = 45.0f;
    float m_distance_back = 80.0f;
};
