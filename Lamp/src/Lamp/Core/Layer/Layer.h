#pragma once

#include "Lamp/Event/Event.h"

namespace Lamp
{
	class Layer
	{
	public:
		virtual ~Layer() = default;

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnEvent(Event& e) {}
	};
}