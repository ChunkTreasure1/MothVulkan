#include "lppch.h"
#include "ScriptEngine.h"

#include "Lamp/Scripting/ScriptBase.h"
#include "Lamp/Log/Log.h"

namespace Lamp
{
	bool ScriptEngine::RegisterToEntity(Ref<ScriptBase> script, Wire::EntityId entity)
	{
		auto& entScripts = s_scripts[entity];
		if (const auto& it = entScripts.find(script->GetGUID()); it != entScripts.end())
		{
			LP_CORE_ERROR("Script has already been registered to this entity!");
			return false;
		}

		entScripts[script->GetGUID()] = script;
		return true;
	}

	Ref<ScriptBase> ScriptEngine::GetScript(Wire::EntityId entity, const WireGUID& guid)
	{
		const auto& entScripts = s_scripts[entity];

		if (const auto& it = entScripts.find(guid); it != entScripts.end())
		{
			return it->second;
		}

		return nullptr;
	}
}