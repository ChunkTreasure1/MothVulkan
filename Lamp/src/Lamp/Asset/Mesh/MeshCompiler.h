#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Asset/Asset.h"

namespace Lamp
{
	class Mesh;
	class MeshCompiler
	{
	public:
		static bool TryCompile(Ref<Mesh> mesh, const std::filesystem::path& destination, AssetHandle materialHandle = Asset::Null());

	private:
		static size_t CalculateMeshSize(Ref<Mesh> mesh);
		static void CreateMaterial(Ref<Mesh> mesh, const std::filesystem::path& destination);

		MeshCompiler() = delete;
	};
}