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
	}
}