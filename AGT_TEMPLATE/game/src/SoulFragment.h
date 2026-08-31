#pragma once
#include <vector>
#include <engine.h>

namespace engine
{
	class mesh;

	class SoulFragment
	{
	public:

		// Constructor takes a vector of vertices (defining the octahedron shape)
		SoulFragment(std::vector<glm::vec3> vertices);
		~SoulFragment();

		// Getter for the generated mesh
		ref<engine::mesh> mesh() const { return m_mesh; }

		// Static factory method for creation
		static ref<SoulFragment> create(std::vector<glm::vec3> vertices);

	private:
		std::vector<glm::vec3> m_vertices;
		ref<engine::mesh> m_mesh;
	};
}
