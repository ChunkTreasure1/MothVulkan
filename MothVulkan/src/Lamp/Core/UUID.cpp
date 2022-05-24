#include "lppch.h"
#include "UUID.h"

#include <random>

namespace Lamp
{
	static std::random_device s_randomDevice;
	static std::mt19937_64 s_randomEngine(s_randomDevice());
	static std::uniform_int_distribution<uint64_t> s_uniformDistribution;

	UUID::UUID()
		: m_uuid(s_uniformDistribution(s_randomEngine))
	{}

	UUID::UUID(uint64_t id)
		: m_uuid(id)
	{}

	UUID::UUID(const UUID& id)
		: m_uuid(id.m_uuid)
	{}
}