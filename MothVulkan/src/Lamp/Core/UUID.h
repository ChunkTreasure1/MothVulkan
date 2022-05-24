#pragma once

#include <cstdint>

namespace Lamp
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t id);
		UUID(const UUID& id);

		operator uint64_t() { return m_uuid; }
		operator const uint64_t() const { return m_uuid; }

	private:
		uint64_t m_uuid;
	};
}