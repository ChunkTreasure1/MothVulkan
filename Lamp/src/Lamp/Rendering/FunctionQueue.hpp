#pragma once

#include "Lamp/Log/Log.h"
#include "Lamp/Core/Profiling.h"

#include <deque>
#include <functional>

namespace Lamp
{
	class FunctionQueue
	{
	public:
		void Push(std::function<void()> function)
		{
#ifdef LP_DEBUG
			LP_CORE_INFO("Pushed to function queue function!");
#endif

			m_queue.emplace_back(function);
		}

		void Flush()
		{
			LP_PROFILE_FUNCTION();

			for (auto it = m_queue.rbegin(); it != m_queue.rend(); it++)
			{
				(*it)();
			}

			m_queue.clear();
		}

	private:
		std::deque<std::function<void()>> m_queue;
	};
}