#pragma once

#include "MeshImporter.h"

namespace Lamp
{
	class FbxImporter : public MeshImporter
	{
	public:
		FbxImporter() = default;

	protected:
		Ref<Mesh> ImportMeshImpl(const std::filesystem::path& path);
	};
}