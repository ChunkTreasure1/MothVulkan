#include "lppch.h"
#include "LPGFImporter.h"

#include "Lamp/Log/Log.h"
#include "Lamp/Asset/Mesh/Mesh.h"

#include "Lamp/Asset/AssetManager.h"

namespace Lamp
{
	Ref<Mesh> LPGFImporter::ImportMeshImpl(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			LP_CORE_ERROR("File does not exist: {0}", path.string().c_str());
			return nullptr;
		}

		std::ifstream file(path, std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			LP_CORE_ERROR("Could not open mesh file!");
		}

		std::vector<uint8_t> totalData;
		totalData.resize(file.seekg(0, std::ios::end).tellg());
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(totalData.data()), totalData.size());
		file.close();

		Ref<Mesh> mesh = CreateRef<Mesh>();

		size_t offset = 0;

		const uint32_t subMeshCount = *(uint32_t*)&totalData[offset];
		offset += sizeof(uint32_t);

		const AssetHandle materialHandle = *(AssetHandle*)&totalData[offset];
		offset += sizeof(AssetHandle);

		const uint32_t vertexCount = *(uint32_t*)&totalData[offset];
		offset += sizeof(uint32_t);

		mesh->m_vertices.resize(vertexCount);
		memcpy_s(mesh->m_vertices.data(), sizeof(Vertex) * vertexCount, &totalData[offset], sizeof(Vertex) * vertexCount);
		offset += sizeof(Vertex) * vertexCount;

		const uint32_t indexCount = *(uint32_t*)&totalData[offset];
		offset += sizeof(uint32_t);

		mesh->m_indices.resize(indexCount);
		memcpy_s(mesh->m_indices.data(), sizeof(uint32_t) * indexCount, &totalData[offset], sizeof(uint32_t) * indexCount);
		offset += sizeof(uint32_t) * indexCount;
		
		mesh->m_boundingSphere.center = *(glm::vec3*)&totalData[offset];
		offset += sizeof(glm::vec3);

		mesh->m_boundingSphere.radius = *(float*)&totalData[offset];
		offset += sizeof(float);

		for (uint32_t i = 0; i < subMeshCount; i++)
		{
			auto& subMesh = mesh->m_subMeshes.emplace_back();
			
			subMesh.materialIndex = *(uint32_t*)&totalData[offset];
			offset += sizeof(uint32_t);

			subMesh.indexCount = *(uint32_t*)&totalData[offset];
			offset += sizeof(uint32_t);

			subMesh.vertexStartOffset = *(uint32_t*)&totalData[offset];
			offset += sizeof(uint32_t);

			subMesh.indexStartOffset = *(uint32_t*)&totalData[offset];
			offset += sizeof(uint32_t);

			subMesh.GenerateHash();
		}

		mesh->m_material = AssetManager::GetAsset<MultiMaterial>(materialHandle);
		for (auto& submesh : mesh->m_subMeshes)
		{
			if (submesh.materialIndex >= mesh->m_material->GetMaterials().size())
			{
				submesh.materialIndex = 0;
			}
		}

		mesh->Construct();

		return mesh;
	}
}