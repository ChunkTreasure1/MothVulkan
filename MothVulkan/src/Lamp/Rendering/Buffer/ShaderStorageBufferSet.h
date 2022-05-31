#pragma once

#include "Lamp/Core/Base.h"

#include <vector>

namespace Lamp
{
	class ShaderStorageBuffer;
	class ShaderStorageBufferSet
	{
	public:
		ShaderStorageBufferSet(uint64_t size, uint32_t count);
		~ShaderStorageBufferSet();

		inline const Ref<ShaderStorageBuffer> Get(uint32_t index) const { return m_storageBuffers[index]; }

		static Ref<ShaderStorageBufferSet> Create(uint64_t size, uint32_t count);

	private:
		std::vector<Ref<ShaderStorageBuffer>> m_storageBuffers;
	};
}