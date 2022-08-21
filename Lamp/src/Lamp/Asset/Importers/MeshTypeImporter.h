#pragma once

#include "Lamp/Asset/Asset.h"

#include <unordered_map>

namespace Lamp
{
	class Mesh;
	class MeshTypeImporter
	{
	public:
		virtual ~MeshTypeImporter() = default;

		static void Initialize();
		static void Shutdown();
		
		static Ref<Mesh> ImportMesh(const std::filesystem::path& path);

	protected:
		virtual Ref<Mesh> ImportMeshImpl(const std::filesystem::path& path) = 0;

	private:
		enum class MeshFormat
		{
			Fbx,
			GLTF,
			LPGF,
			Other
		};

		static MeshFormat FormatFromExtension(const std::filesystem::path& path);
		
		inline static std::unordered_map<MeshFormat, Scope<MeshTypeImporter>> s_importers;
	};
}