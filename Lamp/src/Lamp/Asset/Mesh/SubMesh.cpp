#include "lppch.h"
#include "SubMesh.h"

#include "Lamp/Rendering/Shader/ShaderUtility.h"

namespace Lamp
{
	SubMesh::SubMesh(uint32_t aMaterialIndex, uint32_t aIndexCount, uint32_t aVertexStartOffset, uint32_t aIndexStartOffset)
		: materialIndex(aMaterialIndex), indexCount(aIndexCount), vertexStartOffset(aVertexStartOffset), indexStartOffset(aIndexStartOffset)
	{
		GenerateHash();
	}

	bool SubMesh::operator==(const SubMesh& rhs)
	{
		return materialIndex == rhs.materialIndex &&
			indexCount == rhs.indexCount &&
			vertexStartOffset == rhs.vertexStartOffset &&
			indexStartOffset == rhs.indexStartOffset;
	}

	void SubMesh::GenerateHash()
	{
		m_hash = Utility::HashCombine(m_hash, std::hash<uint32_t>()(materialIndex));
		m_hash = Utility::HashCombine(m_hash, std::hash<uint32_t>()(indexCount));
		m_hash = Utility::HashCombine(m_hash, std::hash<uint32_t>()(vertexStartOffset));
		m_hash = Utility::HashCombine(m_hash, std::hash<uint32_t>()(indexStartOffset));
	}
	
	bool operator>(const SubMesh& lhs, const SubMesh& rhs)
	{
		return lhs.m_hash > rhs.m_hash;
	}

	bool operator<(const SubMesh& lhs, const SubMesh& rhs)
	{
		return lhs.m_hash < rhs.m_hash;
	}

}