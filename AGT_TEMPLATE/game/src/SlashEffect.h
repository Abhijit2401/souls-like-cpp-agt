#pragma once
#include <engine.h>

namespace engine
{
	class mesh;

	class SlashEffect
	{
	public:
		SlashEffect(float inner_radius, float outer_radius, float arc_degrees, int segments, float thickness);
		~SlashEffect();

		ref<engine::mesh> mesh() const { return m_mesh; }

		static ref<SlashEffect> create(float inner_radius, float outer_radius, float arc_degrees, int segments, float thickness);

	private:
		ref<engine::mesh> m_mesh;
	};
}
