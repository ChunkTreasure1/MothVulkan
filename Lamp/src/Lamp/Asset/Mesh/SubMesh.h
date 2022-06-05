#pragma once

#include <cstdint>

namespace Lamp
{
	struct SubMesh
	{
		bool operator==(const SubMesh& rhs);

		uint32_t materialIndex;
		uint32_t indexCount;
		uint32_t vertexStartOffset;
		uint32_t indexStartOffset;
	};
}