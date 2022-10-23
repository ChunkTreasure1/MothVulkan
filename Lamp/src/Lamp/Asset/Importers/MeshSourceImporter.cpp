#include "lppch.h"
#include "MeshSourceImporter.h"

#include "MeshTypeImporter.h"
#include "Lamp/Log/Log.h"

#include "Lamp/Asset/Mesh/Mesh.h"

namespace Lamp
{
	bool MeshSourceImporter::Load(const std::filesystem::path& path, Ref<Asset>& asset) const
	{
		if (!std::filesystem::exists(path)) [[unlikely]]
		{
			LP_CORE_ERROR("File {0} not found!", path.string().c_str());
			asset->SetFlag(AssetFlag::Missing, true);
			return false;
		}
		auto mesh = MeshTypeImporter::ImportMesh(path);

		if (!mesh) [[unlikely]]
		{
			asset->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		asset = mesh;
		asset->path = path;
		return true;
	}

	void MeshSourceImporter::Save(const Ref<Asset>& asset) const
	{
		
	}
}