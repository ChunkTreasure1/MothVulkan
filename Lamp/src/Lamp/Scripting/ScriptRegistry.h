#pragma once

#include "Lamp/Core/Base.h"
#include "Lamp/Scene/Entity.h"

#include <Wire/WireGUID.h>

#include <unordered_map>

#define LP_REGISTER_SCRIPT(x) static bool x ## _entry = Lamp::ScriptRegistry::Register(x::GetStaticGUID(), Lamp::ScriptMetadata{ #x, x::Create});

namespace Lamp
{
	class ScriptBase;

	struct ScriptMetadata
	{
		using CreateMethod = Ref<ScriptBase>(*)(Entity entity);

		std::string name;
		CreateMethod createMethod = nullptr;
	};

	class ScriptRegistry
	{
	public:

		ScriptRegistry() = delete;

		static bool Register(const WireGUID& guid, const ScriptMetadata& data);
		static Ref<ScriptBase> Create(const WireGUID& guid, Entity ownerEntity);

		inline static const std::unordered_map<WireGUID, ScriptMetadata>& GetRegistry() { return s_registry; }

	private:
		inline static std::unordered_map<WireGUID, ScriptMetadata> s_registry;
	};
}