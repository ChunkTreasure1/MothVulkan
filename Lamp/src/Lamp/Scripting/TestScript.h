#pragma once

#include "Lamp/Scripting/ScriptBase.h"

namespace Lamp
{
	class TestScript : public ScriptBase
	{
	public:
		TestScript(Entity entity);

		static Ref<ScriptBase> Create(Entity entity) { return CreateRef<TestScript>(entity); }

		WireGUID GetGUID() override { return GetStaticGUID(); }
		static WireGUID GetStaticGUID() { return "{74B5757F-AAAD-4E22-ADE5-C01F3F7407D0}"_guid; }

	private:
	};
}