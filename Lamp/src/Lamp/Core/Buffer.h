#pragma once

#include <cstdint>

namespace Lamp
{
	class Buffer
	{
	public:
		Buffer() = default;
		Buffer(uint64_t size);
		~Buffer();

		bool IsValid() const;

		void Allocate(uint64_t size);
		void Free();

		void Copy(const void* src, uint64_t size);

		inline const void* GetRawPointer() const { return m_data; }
		inline const uint64_t GetSize() const { return m_size; }

		template<typename T>
		T* As() const;

	private:
		void* m_data = nullptr;
		uint64_t m_size = 0;
	};

	template<typename T>
	inline T* Buffer::As() const
	{
		return reinterpret_cast<T*>(m_data);
	}
}