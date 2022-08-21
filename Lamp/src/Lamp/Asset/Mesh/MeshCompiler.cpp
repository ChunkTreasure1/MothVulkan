#include "lppch.h"
#include "MeshCompiler.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Asset/AssetManager.h"
#include "Lamp/Rendering/Renderer.h"

namespace Lamp
{
	bool MeshCompiler::TryCompile(Ref<Mesh> mesh, const std::filesystem::path& destination, AssetHandle materialHandle)
	{
		if (!mesh || !mesh->IsValid())
		{
			LP_CORE_ERROR("Mesh {0} is invalid!", mesh->path.string());
			return false;
		}

		if (materialHandle == Asset::Null())
		{
			CreateMaterial(mesh, destination);
		}

		/*
		* Encoding:
		* uint32_t: Sub mesh count
		* AssetHandle: Material handle
		* 
		* uint32_t: Vertex count
		* vertices
		* 
		* uint32_t: Index count
		* indices
		* 
		* glm::vec3: Bounding sphere center
		* float: Bounding sphere radius
		* 
		* Per sub mesh:
		* uint32_t: Material index
		* uint32_t: Index count
		* uint32_t: Vertex start offset
		* uint32_t: Index start offset
		*/

		std::vector<uint8_t> bytes;
		bytes.resize(CalculateMeshSize(mesh));

		size_t offset = 0;

		// Main
		{
			const uint32_t subMeshCount = (uint32_t)mesh->m_subMeshes.size();
			memcpy_s(&bytes[offset], sizeof(uint32_t), &subMeshCount, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			AssetHandle matHandle = materialHandle;
			if (matHandle == Asset::Null())
			{
				matHandle = mesh->m_material->handle;
			}

			memcpy_s(&bytes[offset], sizeof(AssetHandle), &matHandle, sizeof(AssetHandle));
			offset += sizeof(AssetHandle);

			const uint32_t vertexCount = (uint32_t)mesh->m_vertices.size();
			memcpy_s(&bytes[offset], sizeof(uint32_t), &vertexCount, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy_s(&bytes[offset], sizeof(Vertex) * vertexCount, mesh->m_vertices.data(), sizeof(Vertex) * vertexCount);
			offset += sizeof(Vertex) * vertexCount;

			const uint32_t indexCount = (uint32_t)mesh->m_indices.size();
			memcpy_s(&bytes[offset], sizeof(uint32_t), &indexCount, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy_s(&bytes[offset], sizeof(uint32_t) * indexCount, mesh->m_indices.data(), sizeof(uint32_t) * indexCount);
			offset += sizeof(uint32_t) * indexCount;

			const glm::vec3 boundingCenter = mesh->GetBoundingSphere().center;
			memcpy_s(&bytes[offset], sizeof(glm::vec3), &boundingCenter, sizeof(glm::vec3));
			offset += sizeof(glm::vec3);

			const float boundingRadius = mesh->GetBoundingSphere().radius;
			memcpy_s(&bytes[offset], sizeof(float), &boundingRadius, sizeof(float));
			offset += sizeof(float);
		}

		// Sub meshes
		{
			for (const auto& subMesh : mesh->m_subMeshes)
			{
				memcpy_s(&bytes[offset], sizeof(uint32_t), &subMesh.materialIndex, sizeof(uint32_t));
				offset += sizeof(uint32_t);

				memcpy_s(&bytes[offset], sizeof(uint32_t), &subMesh.indexCount, sizeof(uint32_t));
				offset += sizeof(uint32_t);

				memcpy_s(&bytes[offset], sizeof(uint32_t), &subMesh.vertexStartOffset, sizeof(uint32_t));
				offset += sizeof(uint32_t);

				memcpy_s(&bytes[offset], sizeof(uint32_t), &subMesh.indexStartOffset, sizeof(uint32_t));
				offset += sizeof(uint32_t);
			}
		}

		std::ofstream output(destination, std::ios::binary);
		output.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
		output.close();

		return true;
	}

	size_t MeshCompiler::CalculateMeshSize(Ref<Mesh> mesh)
	{
		size_t size = 0;

		size += sizeof(uint32_t); // Sub mesh count
		size += sizeof(AssetHandle); // Material handle

		size += sizeof(uint32_t); // Vertex count
		size += sizeof(Vertex) * mesh->m_vertices.size(); // Vertices
		size += sizeof(uint32_t); // Index count
		size += sizeof(uint32_t) * mesh->m_indices.size(); // Indices

		size += sizeof(glm::vec3); // Bounding sphere center
		size += sizeof(float); // Bounding sphere radius

		for (const auto& subMesh : mesh->m_subMeshes)
		{
			size += sizeof(uint32_t); // Material index
			size += sizeof(uint32_t); // Index count
			size += sizeof(uint32_t); // Vertex start offset
			size += sizeof(uint32_t); // Index start offset
		}

		return size;
	}

	void MeshCompiler::CreateMaterial(Ref<Mesh> mesh, const std::filesystem::path& destination)
	{
		mesh->m_material->path = destination.parent_path().string() + "\\" + mesh->path.stem().string() + ".lpmat";
		AssetManager::Get().SaveAsset(mesh->m_material);
	}
}