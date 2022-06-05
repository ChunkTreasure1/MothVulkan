#pragma once

#include "Lamp/Asset/Asset.h"
#include "Lamp/Asset/Mesh/SubMesh.h"

#include "Lamp/Rendering/Vertex.h"

#include <vector>
#include <unordered_map>

namespace Lamp
{
	class VertexBuffer;
	class IndexBuffer;
	class Material;

	class Mesh : public Asset
	{
	public:
		Mesh() = default;

		void Construct();

		inline const std::string& GetName() const { return m_name; }
		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		inline const std::unordered_map<uint32_t, Ref<Material>> GetMaterials() const { return m_materials; }

		inline const size_t GetVertexCount() const { return m_vertices.size(); }
		inline const size_t GetIndexCount() const { return m_indices.size(); }
		
		inline const Ref<VertexBuffer>& GetVertexBuffer() const { return m_vertexBuffer; }
		inline const Ref<IndexBuffer>& GetIndexBuffer() const { return m_indexBuffer; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		AssetType GetType() override { return GetStaticType(); }

	private:
		friend class FbxImporter;

		std::vector<SubMesh> m_subMeshes;
		std::unordered_map<uint32_t, Ref<Material>>	m_materials;

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;

		std::string m_name;
	};
}