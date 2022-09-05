#pragma once

#include "Lamp/Core/Base.h"

#include <Wire/Entity.h>
#include <Wire/WireGUID.h>

#include <unordered_map>

namespace Lamp
{
	class ScriptBase;
	class ScriptEngine
	{
	public:
		static bool RegisterToEntity(Ref<ScriptBase> script, Wire::EntityId entity);
		static Ref<ScriptBase> GetScript(Wire::EntityId entity, const WireGUID& guid);

	private:
		ScriptEngine() = delete;
	
		inline static std::unordered_map<Wire::EntityId, std::unordered_map<WireGUID, Ref<ScriptBase>>> s_scripts;
	};
}