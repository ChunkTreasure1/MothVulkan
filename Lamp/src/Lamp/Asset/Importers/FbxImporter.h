#pragma once

#include "MeshImporter.h"

#include "Lamp/Rendering/Vertex.h"

#include <fbxsdk.h>

namespace Lamp
{
	class Mesh;
	class FbxImporter : public MeshImporter
	{
	public:
		FbxImporter() = default;

	protected:
		Ref<Mesh> ImportMeshImpl(const std::filesystem::path& path);

	private:
		void ProcessMesh(FbxMesh* fbxMesh, FbxScene* aScene, Ref<Mesh> mesh);
		void FetchGeometryNodes(FbxNode* node, std::vector<FbxNode*>& outNodes);

		void ReadNormal(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& normal);
		void ReadTangent(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& tangent);
		void ReadBitangent(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& bitangent);
	};
}