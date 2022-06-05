#pragma once

#include "Lamp/Core/Base.h"

#include "Lamp/Rendering/Buffer/ShaderStorageBuffer/ShaderStorageBuffer.h"

#include <vector>

namespace Lamp
{
	class ShaderStorageBuffer;
	class ShaderStorageBufferSet
	{
	public:
		ShaderStorageBufferSet(uint64_t size, uint32_t count, bool indirectBuffer);
		~ShaderStorageBufferSet();

		inline const Ref<ShaderStorageBuffer> Get(uint32_t index) const { return m_storageBuffers[index]; }

		static Ref<ShaderStorageBufferSet> Create(uint64_t size, uint32_t count, bool indirectBuffer = false);

	private:
		std::vector<Ref<ShaderStorageBuffer>> m_storageBuffers;
	};
}