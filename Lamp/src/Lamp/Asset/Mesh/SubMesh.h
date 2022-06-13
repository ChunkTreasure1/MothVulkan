#pragma once

#include <cstdint>

namespace Lamp
{
	struct SubMesh
	{
		SubMesh(uint32_t aMaterialIndex, uint32_t aIndexCount, uint32_t aVertexStartOffset, uint32_t aIndexStartOffset);
		SubMesh() = default;

		void GenerateHash();

		bool operator==(const SubMesh& rhs);
		friend bool operator>(const SubMesh& lhs, const SubMesh& rhs);
		friend bool operator<(const SubMesh& lhs, const SubMesh& rhs);

		uint32_t materialIndex;
		uint32_t indexCount;
		uint32_t vertexStartOffset;
		uint32_t indexStartOffset;

	private:
		size_t m_hash = 0;
	};
}