#include "lppch.h"
#include "FbxImporter.h"

#include "Lamp/Log/Log.h"

#include "Lamp/Asset/Mesh/Mesh.h"
#include "Lamp/Asset/Mesh/Material.h"

#include "Lamp/Rendering/RenderPipeline/RenderPipelineRegistry.h"

namespace Lamp
{
	Ref<Mesh> FbxImporter::ImportMeshImpl(const std::filesystem::path& path)
	{
		FbxManager* sdkManager = FbxManager::Create();
		FbxIOSettings* ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);

		ioSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
		ioSettings->SetBoolProp(IMP_FBX_TEXTURE, false);
		ioSettings->SetBoolProp(IMP_FBX_LINK, false);
		ioSettings->SetBoolProp(IMP_FBX_SHAPE, false);
		ioSettings->SetBoolProp(IMP_FBX_GOBO, false);
		ioSettings->SetBoolProp(IMP_FBX_ANIMATION, false);
		ioSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

		sdkManager->SetIOSettings(ioSettings);

		fbxsdk::FbxImporter* importer = fbxsdk::FbxImporter::Create(sdkManager, "");

		bool importStatus = importer->Initialize(path.string().c_str(), -1, sdkManager->GetIOSettings());
		if (!importStatus)
		{
			LP_CORE_ERROR("Unable to import file {0}!", path.string().c_str());
			return nullptr;
		}

		FbxScene* fbxScene = FbxScene::Create(sdkManager, "Scene");

		fbxsdk::FbxAxisSystem axisSystem(fbxsdk::FbxAxisSystem::eOpenGL);

		importer->Import(fbxScene);
		axisSystem.DeepConvertScene(fbxScene);

		FbxNode* rootNode = fbxScene->GetRootNode();

		std::vector<FbxNode*> geomNodes;
		FetchGeometryNodes(rootNode, geomNodes);

		Ref<Mesh> mesh = CreateRef<Mesh>();
		mesh->m_material = CreateRef<MultiMaterial>();

		for (auto node : geomNodes)
		{
			FbxGeometryConverter geomConverter(sdkManager);
			if (!geomConverter.Triangulate(node->GetNodeAttribute(), true, true))
			{
				geomConverter.Triangulate(node->GetNodeAttribute(), true, false);
			}

			ProcessMesh(node->GetMesh(), fbxScene, mesh);
		}

		mesh->Construct();

		importer->Destroy();

		return mesh;
	}

	void FbxImporter::ProcessMesh(FbxMesh* fbxMesh, FbxScene* aScene, Ref<Mesh> mesh)
	{
		if (!fbxMesh)
		{
			return;
		}

		//if (fbxMesh->GetElementBinormalCount() == 0 || fbxMesh->GetElementTangentCount() == 0)
		//{
		//	bool result = fbxMesh->GenerateTangentsData(0, true, false);
		//	LP_CORE_ASSERT(result, "Unable to generate tangent data!");
		//}

		Ref<Material> material;
		int32_t matIndex = 0;

		for (int32_t meshMatIndex = 0; meshMatIndex < fbxMesh->GetNode()->GetMaterialCount() || meshMatIndex == 0; meshMatIndex++)
		{
			for (int32_t sceneMatIndex = 0; sceneMatIndex < aScene->GetMaterialCount(); sceneMatIndex++)
			{
				FbxSurfaceMaterial* sceneMaterial = aScene->GetMaterial(sceneMatIndex);
				FbxSurfaceMaterial* meshMaterial = fbxMesh->GetNode()->GetMaterial(meshMatIndex);

				if (sceneMaterial == meshMaterial)
				{
					if (mesh->m_material->m_materials.find(sceneMatIndex) == mesh->m_material->m_materials.end())
					{
						material = Material::Create(sceneMaterial->GetName(), sceneMatIndex, RenderPipelineRegistry::Get("trimesh"));
						mesh->m_material->m_materials[sceneMatIndex] = material;
					}
					matIndex = sceneMatIndex;
				}
			}
		}

		const FbxVector4* ctrlPoints = fbxMesh->GetControlPoints();
		const uint32_t triangleCount = fbxMesh->GetPolygonCount();
		uint32_t vertexCount = 0;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(triangleCount * 3);
		indices.reserve(triangleCount);

		for (uint32_t i = 0; i < triangleCount; i++)
		{
			const int32_t polySize = fbxMesh->GetPolygonSize(i);
			LP_CORE_ASSERT(polySize == 3, "Mesh must be fully triangulated!");

			for (int32_t j = 0; j < polySize; j++)
			{
				Vertex vertex;
				const int32_t ctrlPointIndex = fbxMesh->GetPolygonVertex(i, j);

				vertex.position.x = (float)ctrlPoints[ctrlPointIndex][0];
				vertex.position.y = (float)ctrlPoints[ctrlPointIndex][1];
				vertex.position.z = (float)ctrlPoints[ctrlPointIndex][2];

				const int32_t numUVs = fbxMesh->GetElementUVCount();
				const int32_t texUvIndex = fbxMesh->GetTextureUVIndex(i, j);

				// TODO: Add ability to have multiple UV sets
				for (int32_t uv = 0; uv < numUVs && uv < 4; uv++)
				{
					FbxGeometryElementUV* uvElement = fbxMesh->GetElementUV(uv);
					const auto coord = uvElement->GetDirectArray().GetAt(texUvIndex);

					vertex.textureCoords.x = (float)coord.mData[0];
					vertex.textureCoords.y = (float)coord.mData[1];

					break;
				}

				ReadNormal(fbxMesh, ctrlPointIndex, vertexCount, vertex.normal);
				ReadTangent(fbxMesh, ctrlPointIndex, vertexCount, vertex.tangent);
				ReadBitangent(fbxMesh, ctrlPointIndex, vertexCount, vertex.bitangent);

				size_t size = vertices.size();
				size_t i = 0;

				for (i = 0; i < size; i++)
				{
					if (vertex == vertices[i])
					{
						break;
					}
				}

				if (i == size)
				{
					vertices.emplace_back(vertex);
				}

				indices.emplace_back((uint32_t)i);
				vertexCount++;
			}
		}

		// Copy new vertices into vector
		{
			size_t preVertexCount = mesh->m_vertices.size();
			size_t preIndexCount = mesh->m_indices.size();

			mesh->m_vertices.resize(preVertexCount + vertices.size());
			mesh->m_indices.resize(mesh->m_indices.size() + indices.size());

			memcpy_s(&mesh->m_vertices[preVertexCount], sizeof(Vertex) * vertices.size(), vertices.data(), sizeof(Vertex) * vertices.size());
			memcpy_s(&mesh->m_indices[preIndexCount], sizeof(uint32_t) * indices.size(), indices.data(), sizeof(uint32_t) * indices.size());
			
			auto& submesh = mesh->m_subMeshes.emplace_back();
			submesh.indexCount = (uint32_t)indices.size();
			submesh.indexStartOffset = (uint32_t)preIndexCount;
			submesh.vertexStartOffset = (uint32_t)preVertexCount;
			submesh.materialIndex = matIndex;
			submesh.GenerateHash();
		}
	}

	void FbxImporter::FetchGeometryNodes(FbxNode* node, std::vector<FbxNode*>& outNodes)
	{
		if (node->GetNodeAttribute())
		{
			switch (node->GetNodeAttribute()->GetAttributeType())
			{
				case FbxNodeAttribute::eMesh:
				{
					if (node->GetMesh())
					{
						outNodes.emplace_back(node);
					}
				}
			}
		}

		for (uint32_t i = 0; i < (uint32_t)node->GetChildCount(); i++)
		{
			FetchGeometryNodes(node->GetChild(i), outNodes);
		}
	}

	void FbxImporter::ReadNormal(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& normal)
	{
		if (mesh->GetElementNormalCount() < 1)
		{
			return;
		}

		FbxGeometryElementNormal* fbxNormal = mesh->GetElementNormal(0);

		switch (fbxNormal->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (fbxNormal->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						normal.x = (float)fbxNormal->GetDirectArray().GetAt(ctrlPointIndex)[0];
						normal.y = (float)fbxNormal->GetDirectArray().GetAt(ctrlPointIndex)[1];
						normal.z = (float)fbxNormal->GetDirectArray().GetAt(ctrlPointIndex)[2];

						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxNormal->GetIndexArray().GetAt(ctrlPointIndex);
						normal.x = (float)fbxNormal->GetDirectArray().GetAt(id)[0];
						normal.y = (float)fbxNormal->GetDirectArray().GetAt(id)[1];
						normal.z = (float)fbxNormal->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}

			case FbxGeometryElement::eByPolygonVertex:
			{
				switch (fbxNormal->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						normal.x = (float)fbxNormal->GetDirectArray().GetAt(vertCount)[0];
						normal.y = (float)fbxNormal->GetDirectArray().GetAt(vertCount)[1];
						normal.z = (float)fbxNormal->GetDirectArray().GetAt(vertCount)[2];
						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxNormal->GetIndexArray().GetAt(vertCount);
						normal.x = (float)fbxNormal->GetDirectArray().GetAt(id)[0];
						normal.y = (float)fbxNormal->GetDirectArray().GetAt(id)[1];
						normal.z = (float)fbxNormal->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}

			break;
		}
	}

	void FbxImporter::ReadTangent(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& tangent)
	{
		if (mesh->GetElementTangentCount() < 1)
		{
			return;
		}

		FbxGeometryElementTangent* fbxTangent = mesh->GetElementTangent(0);

		switch (fbxTangent->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (fbxTangent->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						tangent.x = (float)fbxTangent->GetDirectArray().GetAt(ctrlPointIndex)[0];
						tangent.y = (float)fbxTangent->GetDirectArray().GetAt(ctrlPointIndex)[1];
						tangent.z = (float)fbxTangent->GetDirectArray().GetAt(ctrlPointIndex)[2];

						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxTangent->GetIndexArray().GetAt(ctrlPointIndex);
						tangent.x = (float)fbxTangent->GetDirectArray().GetAt(id)[0];
						tangent.y = (float)fbxTangent->GetDirectArray().GetAt(id)[1];
						tangent.z = (float)fbxTangent->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}

			case FbxGeometryElement::eByPolygonVertex:
			{
				switch (fbxTangent->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						tangent.x = (float)fbxTangent->GetDirectArray().GetAt(vertCount)[0];
						tangent.y = (float)fbxTangent->GetDirectArray().GetAt(vertCount)[1];
						tangent.z = (float)fbxTangent->GetDirectArray().GetAt(vertCount)[2];
						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxTangent->GetIndexArray().GetAt(vertCount);
						tangent.x = (float)fbxTangent->GetDirectArray().GetAt(id)[0];
						tangent.y = (float)fbxTangent->GetDirectArray().GetAt(id)[1];
						tangent.z = (float)fbxTangent->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}

			break;
		}
	}

	void FbxImporter::ReadBitangent(FbxMesh* mesh, int32_t ctrlPointIndex, int32_t vertCount, glm::vec3& bitangent)
	{
		if (mesh->GetElementBinormalCount() < 1)
		{
			return;
		}

		FbxGeometryElementBinormal* fbxBitangent = mesh->GetElementBinormal(0);

		switch (fbxBitangent->GetMappingMode())
		{
			case FbxGeometryElement::eByControlPoint:
			{
				switch (fbxBitangent->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						bitangent.x = (float)fbxBitangent->GetDirectArray().GetAt(ctrlPointIndex)[0];
						bitangent.y = (float)fbxBitangent->GetDirectArray().GetAt(ctrlPointIndex)[1];
						bitangent.z = (float)fbxBitangent->GetDirectArray().GetAt(ctrlPointIndex)[2];

						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxBitangent->GetIndexArray().GetAt(ctrlPointIndex);
						bitangent.x = (float)fbxBitangent->GetDirectArray().GetAt(id)[0];
						bitangent.y = (float)fbxBitangent->GetDirectArray().GetAt(id)[1];
						bitangent.z = (float)fbxBitangent->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}

			case FbxGeometryElement::eByPolygonVertex:
			{
				switch (fbxBitangent->GetReferenceMode())
				{
					case FbxGeometryElement::eDirect:
					{
						bitangent.x = (float)fbxBitangent->GetDirectArray().GetAt(vertCount)[0];
						bitangent.y = (float)fbxBitangent->GetDirectArray().GetAt(vertCount)[1];
						bitangent.z = (float)fbxBitangent->GetDirectArray().GetAt(vertCount)[2];
						break;
					}

					case FbxGeometryElement::eIndexToDirect:
					{
						int32_t id = fbxBitangent->GetIndexArray().GetAt(vertCount);
						bitangent.x = (float)fbxBitangent->GetDirectArray().GetAt(id)[0];
						bitangent.y = (float)fbxBitangent->GetDirectArray().GetAt(id)[1];
						bitangent.z = (float)fbxBitangent->GetDirectArray().GetAt(id)[2];

						break;
					}

					default:
						break;
				}

				break;
			}
			break;
		}
	}
}