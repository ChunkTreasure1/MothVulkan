#pragma once

#include "Lamp/Asset/Asset.h"
#include "Lamp/Asset/SubMesh.h"

#include "Lamp/Rendering/Vertex.h"

#include <vector>
#include <unordered_map>

namespace Lamp
{
	class VertexBuffer;
	class IndexBuffer;

	class Mesh : public Asset
	{
	public:
		Mesh() = default;

		inline const std::string& GetName() const { return m_name; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		AssetType GetType() override { return GetStaticType(); }

	private:
		std::vector<SubMesh> m_subMeshes;

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;

		std::string m_name;
	};
}