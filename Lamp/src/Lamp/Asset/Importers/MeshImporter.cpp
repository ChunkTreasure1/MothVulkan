#include "lppch.h"
#include "MeshImporter.h"

#include "FbxImporter.h"
#include "GLTFImporter.h"

namespace Lamp
{
	void MeshImporter::Initialize()
	{
		s_importers[MeshFormat::Fbx] = CreateScope<FbxImporter>();
		s_importers[MeshFormat::GLTF] = CreateScope<GLTFImporter>();
	}

	void MeshImporter::Shutdown()
	{
		s_importers.clear();
	}

	Ref<Mesh> MeshImporter::ImportMesh(const std::filesystem::path& path)
	{
		return s_importers[FormatFromExtension(path)]->ImportMeshImpl(path);
	}

	MeshImporter::MeshFormat MeshImporter::FormatFromExtension(const std::filesystem::path& path)
	{
		auto ext = path.extension().string();
		
		if (ext == ".fbx" || ext == ".FBX")
		{
			return MeshFormat::Fbx;
		}
		else if (ext == ".gltf" || ext == ".glb")
		{
			return MeshFormat::GLTF;
		}
		else if (ext == ".lgf")
		{
			return MeshFormat::LGF;
		}

		return MeshFormat::Other;
	}

}