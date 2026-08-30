#include "pch.h"
#include "tetrahedron.h"
#include <engine.h>

namespace engine
{
	tetrahedron::tetrahedron(std::vector<glm::vec3> vertices) : m_vertices(vertices)
	{
		std::vector<glm::vec3> normals;

		// --- NORMALS CALCULATION ---
		// Calculate the face normals using the cross product (outward facing)
		// front normal
		normals.push_back(glm::cross(vertices.at(0) - vertices.at(2), vertices.at(0) - vertices.at(1)));
		// left normal
		normals.push_back(glm::cross(vertices.at(0) - vertices.at(3), vertices.at(0) - vertices.at(2)));
		// right normal
		normals.push_back(glm::cross(vertices.at(0) - vertices.at(1), vertices.at(0) - vertices.at(3)));
		// bottom normal
		normals.push_back(glm::cross(vertices.at(1) - vertices.at(2), vertices.at(1) - vertices.at(3)));

		// --- VERTEX DATA DEFINITION (12 vertices total: 4 faces * 3 vertices/face) ---
		std::vector<mesh::vertex> tetrahedron_vertices
		{
			//   position             normal            tex coord
			//front face
			{ vertices.at(0),      normals.at(0),      { 0.f,  0.f } },
			{ vertices.at(2),      normals.at(0),      { 1.f,  0.f } },
			{ vertices.at(1),      normals.at(0),      { 0.5f, 1.f } },

			//left face
			{ vertices.at(0),      normals.at(1),      { 0.f,  0.f } },
			{ vertices.at(3),      normals.at(1),      { 1.f,  0.f } },
			{ vertices.at(2),      normals.at(1),      { 0.5f, 1.f } },

			//right face
			{ vertices.at(0),      normals.at(2),      { 0.f,  0.f } },
			{ vertices.at(1),      normals.at(2),      { 1.f,  0.f } },
			{ vertices.at(3),      normals.at(2),      { 0.5f, 1.f } },

			//bottom face
			{ vertices.at(1),      normals.at(3),      { 0.f,  0.f } },
			{ vertices.at(2),      normals.at(3),      { 1.f,  0.f } },
			{ vertices.at(3),      normals.at(3),      { 0.5f, 1.f } },
		};

		// --- INDEX BUFFER DEFINITION ---
		const std::vector<uint32_t> tetrahedron_indices
		{
			0,  1,  2,	//front
			3,  4,  5,	//left
			6,  7,  8,  //right
			9, 10, 11   //bottom
		};

		m_mesh = engine::mesh::create(tetrahedron_vertices, tetrahedron_indices);
	}

	tetrahedron::~tetrahedron() {}

	ref<tetrahedron> tetrahedron::create(std::vector<glm::vec3> vertices)
	{
		return std::make_shared<tetrahedron>(vertices);
	}
}
