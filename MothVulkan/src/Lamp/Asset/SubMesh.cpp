#include "lppch.h"
#include "SubMesh.h"

namespace Lamp
{
	SubMesh::SubMesh(uint32_t indexCount, uint32_t indexOffset, uint32_t materialIndex)
		: m_indexCount(indexCount), m_indexStartOffset(indexOffset), m_materialIndex(materialIndex)
	{}
}