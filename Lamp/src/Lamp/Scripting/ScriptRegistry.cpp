#include "lppch.h"
#include "ScriptRegistry.h"

namespace Lamp
{
	bool ScriptRegistry::Register(const WireGUID& guid, const ScriptMetadata& data)
	{
		if (const auto& it = s_registry.find(guid); it == s_registry.end())
		{
			s_registry[guid] = data;
			return true;
		}	

		return false;
	}

	Ref<ScriptBase> ScriptRegistry::Create(const WireGUID& guid, Entity ownerEntity)
	{
		if (const auto& it = s_registry.find(guid); it != s_registry.end())
		{
			return it->second.createMethod(ownerEntity);
		}

		return nullptr;
	}
}