#include "lppch.h"
#include "MeshTypeImporter.h"

#include "FbxImporter.h"
#include "GLTFImporter.h"
#include "LPGFImporter.h"

namespace Lamp
{
	void MeshTypeImporter::Initialize()
	{
		s_importers[MeshFormat::Fbx] = CreateScope<FbxImporter>();
		s_importers[MeshFormat::GLTF] = CreateScope<GLTFImporter>();
		s_importers[MeshFormat::LPGF] = CreateScope<LPGFImporter>();
	}

	void MeshTypeImporter::Shutdown()
	{
		s_importers.clear();
	}

	Ref<Mesh> MeshTypeImporter::ImportMesh(const std::filesystem::path& path)
	{
		return s_importers[FormatFromExtension(path)]->ImportMeshImpl(path);
	}

	MeshTypeImporter::MeshFormat MeshTypeImporter::FormatFromExtension(const std::filesystem::path& path)
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
		else if (ext == ".lpgf")
		{
			return MeshFormat::LPGF;
		}

		return MeshFormat::Other;
	}

}