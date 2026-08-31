#include "pch.h"
#include "SlashEffect.h"
#include <engine.h>

namespace engine
{
	SlashEffect::SlashEffect(float inner_radius, float outer_radius, float arc_degrees, int segments, float thickness)
	{
		std::vector<mesh::vertex> vertices;
		std::vector<uint32_t> indices;

		float half_arc = glm::radians(arc_degrees) * 0.5f;
		float half_thickness = thickness * 0.5f;
		const glm::vec3 normal(0.f, 0.f, 1.f);

		auto add_quad = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d)
		{
			uint32_t base = (uint32_t)vertices.size();
			vertices.push_back({ a, normal, { 0.f, 0.f } });
			vertices.push_back({ b, normal, { 1.f, 0.f } });
			vertices.push_back({ c, normal, { 1.f, 1.f } });
			vertices.push_back({ d, normal, { 0.f, 1.f } });
			indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
			indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
		};

		std::vector<glm::vec3> front_inner(segments + 1), front_outer(segments + 1);
		std::vector<glm::vec3> back_inner(segments + 1), back_outer(segments + 1);
		for (int i = 0; i <= segments; i++)
		{
			float t = (float)i / (float)segments;
			float angle = -half_arc + t * (half_arc * 2.f);
			glm::vec3 dir(sin(angle), cos(angle), 0.f);
			glm::vec3 z_offset(0.f, 0.f, half_thickness);

			front_inner[i] = dir * inner_radius + z_offset;
			front_outer[i] = dir * outer_radius + z_offset;
			back_inner[i] = dir * inner_radius - z_offset;
			back_outer[i] = dir * outer_radius - z_offset;
		}

		for (int i = 0; i < segments; i++)
		{
			add_quad(front_inner[i], front_outer[i], front_outer[i + 1], front_inner[i + 1]); // front cap
			add_quad(back_inner[i], back_inner[i + 1], back_outer[i + 1], back_outer[i]);      // back cap
			add_quad(front_outer[i], back_outer[i], back_outer[i + 1], front_outer[i + 1]);    // outer wall
			add_quad(front_inner[i], front_inner[i + 1], back_inner[i + 1], back_inner[i]);    // inner wall
		}

		add_quad(front_inner[0], front_outer[0], back_outer[0], back_inner[0]);
		add_quad(front_inner[segments], back_inner[segments], back_outer[segments], front_outer[segments]);

		m_mesh = engine::mesh::create(vertices, indices);
	}

	SlashEffect::~SlashEffect() {}

	ref<SlashEffect> SlashEffect::create(float inner_radius, float outer_radius, float arc_degrees, int segments, float thickness)
	{
		return std::make_shared<SlashEffect>(inner_radius, outer_radius, arc_degrees, segments, thickness);
	}
}
