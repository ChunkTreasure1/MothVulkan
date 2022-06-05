#include "lppch.h"
#include "SubMesh.h"

namespace Lamp
{
	bool SubMesh::operator==(const SubMesh& rhs)
	{
		return materialIndex == rhs.materialIndex &&
			indexCount == rhs.indexCount &&
			vertexStartOffset == rhs.vertexStartOffset &&
			indexStartOffset == rhs.indexStartOffset;
	}
}