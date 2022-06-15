#include "lppch.h"
#include "Mesh.h"

#include "Lamp/Rendering/Buffer/VertexBuffer.h"
#include "Lamp/Rendering/Buffer/IndexBuffer.h"

namespace Lamp
{
	void Mesh::Construct()
	{
		m_vertexBuffer = VertexBuffer::Create(m_vertices, sizeof(Vertex) * (uint32_t)m_vertices.size());
		m_indexBuffer = IndexBuffer::Create(m_indices, (uint32_t)m_indices.size());
	
		glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
		
		for (const auto& vert : m_vertices)
		{
			minAABB.x = std::min(minAABB.x, vert.position.x);
			minAABB.y = std::min(minAABB.y, vert.position.y);
			minAABB.z = std::min(minAABB.z, vert.position.z);

			maxAABB.x = std::max(maxAABB.x, vert.position.x);
			maxAABB.y = std::max(maxAABB.y, vert.position.y);
			maxAABB.z = std::max(maxAABB.z, vert.position.z);
		}

		glm::vec3 center = (maxAABB + minAABB) * 0.5f;
		float radius = glm::length(minAABB - maxAABB);

		m_boundingSphere = { center, radius };
	}
}