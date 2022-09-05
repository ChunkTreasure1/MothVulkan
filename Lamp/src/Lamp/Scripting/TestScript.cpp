#include "lppch.h"
#include "TestScript.h"

#include "Lamp/Scripting/ScriptRegistry.h"

namespace Lamp
{
	TestScript::TestScript(Entity entity)
		: ScriptBase(entity)
	{
	}

	LP_REGISTER_SCRIPT(TestScript);
}