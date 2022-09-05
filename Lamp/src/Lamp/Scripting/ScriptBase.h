#pragma once

#include "Lamp/Scene/Entity.h"

#include <Wire/WireGUID.h>

namespace Lamp
{
	class Entity;
	class ScriptBase
	{
	public:
		ScriptBase(Entity entity);
		~ScriptBase() = default;

		virtual void OnStart() {}
		virtual void OnAwake() {}
		virtual void OnUpdate(float aDeltaTime) {}
		virtual void OnStop() {}

		virtual WireGUID GetGUID() = 0;

	protected:
		Entity m_entity;
	};
}