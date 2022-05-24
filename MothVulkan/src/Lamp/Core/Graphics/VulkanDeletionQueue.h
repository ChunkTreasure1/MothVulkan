#pragma once

#include <stack>
#include <functional>

namespace Lamp
{
	class VulkanDeletionQueue
	{
	public:
		static void Push(std::function<void()>&& func);
		static void Flush();

	private:
		VulkanDeletionQueue() = delete;
		
		inline static std::stack<std::function<void()>> s_deletionFunctions;
	};
}