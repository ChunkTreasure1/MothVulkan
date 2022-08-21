#pragma once

#include "MeshTypeImporter.h"

namespace Lamp
{
	class Mesh;
	class LPGFImporter : public MeshTypeImporter
	{
	public:
		LPGFImporter() = default;

	protected:
		Ref<Mesh> ImportMeshImpl(const std::filesystem::path& path);
	};
}