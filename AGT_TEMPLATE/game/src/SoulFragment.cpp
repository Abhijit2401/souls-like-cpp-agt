#include "pch.h"
#include "SoulFragment.h"
#include <engine.h>

namespace engine
{
	// NOTE: The geometry created here is an octahedron (two pyramids base-to-base), which has 8 faces.
	SoulFragment::SoulFragment(std::vector<glm::vec3> vertices) : m_vertices(vertices)
	{
		// An octahedron has 8 triangle faces
		// We need 8 normals, one for each face
		std::vector<glm::vec3> normals;

		// top tetrahedron faces 
		normals.push_back(glm::normalize(glm::cross(vertices.at(1) - vertices.at(0), vertices.at(2) - vertices.at(0)))); // Front-Right
		normals.push_back(glm::normalize(glm::cross(vertices.at(2) - vertices.at(0), vertices.at(3) - vertices.at(0)))); // Front-Left
		normals.push_back(glm::normalize(glm::cross(vertices.at(3) - vertices.at(0), vertices.at(4) - vertices.at(0)))); // Back-Left
		normals.push_back(glm::normalize(glm::cross(vertices.at(4) - vertices.at(0), vertices.at(1) - vertices.at(0)))); // Back-Right

		// bottom ones 
		normals.push_back(glm::normalize(glm::cross(vertices.at(2) - vertices.at(5), vertices.at(1) - vertices.at(5)))); // Bottom Front-Right
		normals.push_back(glm::normalize(glm::cross(vertices.at(3) - vertices.at(5), vertices.at(2) - vertices.at(5)))); // Bottom Front-Left
		normals.push_back(glm::normalize(glm::cross(vertices.at(4) - vertices.at(5), vertices.at(3) - vertices.at(5)))); // Bottom Back-Left
		normals.push_back(glm::normalize(glm::cross(vertices.at(1) - vertices.at(5), vertices.at(4) - vertices.at(5)))); // Bottom Back-Right

		// An octahedron has 24 vertices in total (8 faces * 3 vertices/face)
		// VERTEX DATA DEFINITION
		std::vector<mesh::vertex> soul_vertices
		{
			//   position             normal            tex coord

			// Top faces (sharing the top vertex at index 0)
			{ vertices.at(0),      normals.at(0),      { 0.5f, 1.f } },
			{ vertices.at(1),      normals.at(0),      { 0.f,  0.f } },
			{ vertices.at(2),      normals.at(0),      { 1.f,  0.f } },

			{ vertices.at(0),      normals.at(1),      { 0.5f, 1.f } },
			{ vertices.at(2),      normals.at(1),      { 0.f,  0.f } },
			{ vertices.at(3),      normals.at(1),      { 1.f,  0.f } },

			{ vertices.at(0),      normals.at(2),      { 0.5f, 1.f } },
			{ vertices.at(3),      normals.at(2),      { 0.f,  0.f } },
			{ vertices.at(4),      normals.at(2),      { 1.f,  0.f } },

			{ vertices.at(0),      normals.at(3),      { 0.5f, 1.f } },
			{ vertices.at(4),      normals.at(3),      { 0.f,  0.f } },
			{ vertices.at(1),      normals.at(3),      { 1.f,  0.f } },

			// Bottom faces (sharing the bottom vertex at index 5)
			{ vertices.at(5),      normals.at(4),      { 0.5f, 1.f } },
			{ vertices.at(2),      normals.at(4),      { 0.f,  0.f } },
			{ vertices.at(1),      normals.at(4),      { 1.f,  0.f } },

			{ vertices.at(5),      normals.at(5),      { 0.5f, 1.f } },
			{ vertices.at(3),      normals.at(5),      { 0.f,  0.f } },
			{ vertices.at(2),      normals.at(5),      { 1.f,  0.f } },

			{ vertices.at(5),      normals.at(6),      { 0.5f, 1.f } },
			{ vertices.at(4),      normals.at(6),      { 0.f,  0.f } },
			{ vertices.at(3),      normals.at(6),      { 1.f,  0.f } },

			{ vertices.at(5),      normals.at(7),      { 0.5f, 1.f } },
			{ vertices.at(1),      normals.at(7),      { 0.f,  0.f } },
			{ vertices.at(4),      normals.at(7),      { 1.f,  0.f } },
		};

		// --- INDEX BUFFER DEFINITION (Standard triangle list) ---
		const std::vector<uint32_t> soul_indices
		{
			0,  1,  2,	// Top Front-Right
			3,  4,  5,	// Top Front-Left
			6,  7,  8,  // Top Back-Left
			9, 10, 11,  // Top Back-Right
			12, 13, 14, // Bottom Front-Right
			15, 16, 17, // Bottom Front-Left
			18, 19, 20, // Bottom Back-Left
			21, 22, 23  // Bottom Back-Right
		};

		m_mesh = engine::mesh::create(soul_vertices, soul_indices);
	}

	SoulFragment::~SoulFragment() {}

	ref<SoulFragment> SoulFragment::create(std::vector<glm::vec3> vertices)
	{
		return std::make_shared<SoulFragment>(vertices);
	}
}
