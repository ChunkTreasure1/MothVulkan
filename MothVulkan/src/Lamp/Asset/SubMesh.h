#pragma once

#include <cstdint>

namespace Lamp
{
	class SubMesh
	{
	public:
		SubMesh(uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset, uint32_t materialIndex);

		inline const uint32_t GetIndexOffset() const { return m_indexStartOffset; }
		inline const uint32_t GetIndexCount() const { return m_indexCount; }
		inline const uint32_t GetMaterialIndex() const { return m_materialIndex; }

	private:
		uint32_t m_materialIndex;
		uint32_t m_indexCount;
		
		uint32_t m_vertexStartOffset;
		uint32_t m_indexStartOffset;
	};
}