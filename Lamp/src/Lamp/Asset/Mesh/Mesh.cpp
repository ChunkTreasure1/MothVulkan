#include "lppch.h"
#include "Mesh.h"

#include "Lamp/Rendering/Renderer.h"

#include "Lamp/Rendering/Buffer/CombinedIndexBuffer.h"
#include "Lamp/Rendering/Buffer/CombinedVertexBuffer.h"

namespace Lamp
{
	void Mesh::Construct()
	{
		auto& bindlessData = Renderer::GetBindlessData();

		const uint64_t vertexLocation = bindlessData.combinedVertexBuffer->AppendToBuffer(m_vertices.data(), m_vertices.size());
		const uint64_t indexLocation = bindlessData.combinedIndexBuffer->AppendToBuffer(m_indices.data(), m_indices.size());

		for (auto& s : m_subMeshes)
		{
			s.indexStartOffset += indexLocation;
			s.vertexStartOffset += vertexLocation;
		}

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