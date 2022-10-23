#pragma once

#include "Lamp/Asset/Asset.h"
#include "Lamp/Asset/Mesh/SubMesh.h"
#include "Lamp/Asset/Mesh/MultiMaterial.h"

#include "Lamp/Rendering/Vertex.h"
#include "Lamp/Rendering/BoundingStructures.h"

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
		~Mesh() override {}

		void Construct();

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_subMeshes; }
		inline const Ref<MultiMaterial>& GetMaterial() const { return m_material; }

		inline const size_t GetVertexCount() const { return m_vertices.size(); }
		inline const size_t GetIndexCount() const { return m_indices.size(); }
		inline const BoundingSphere& GetBoundingSphere() const { return m_boundingSphere; }
		
		inline const Ref<VertexBuffer>& GetVertexBuffer() const { return m_vertexBuffer; }
		inline const Ref<IndexBuffer>& GetIndexBuffer() const { return m_indexBuffer; }

		static AssetType GetStaticType() { return AssetType::Mesh; }
		AssetType GetType() override { return GetStaticType(); }

	private:
		friend class FbxImporter;
		friend class GLTFImporter;
		friend class LPGFImporter;
		friend class MeshCompiler;

		std::vector<SubMesh> m_subMeshes;

		Ref<MultiMaterial> m_material;

		std::vector<Vertex> m_vertices;
		std::vector<uint32_t> m_indices;

		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;

		BoundingSphere m_boundingSphere;
	};
}