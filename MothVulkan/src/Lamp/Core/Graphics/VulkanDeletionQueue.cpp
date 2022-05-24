#include "lppch.h"
#include "VulkanDeletionQueue.h"

#include "Lamp/Core/Graphics/GraphicsContext.h"
#include "Lamp/Core/Graphics/GraphicsDevice.h"

namespace Lamp
{
	void VulkanDeletionQueue::Push(std::function<void()>&& func)
	{
		s_deletionFunctions.push(func);
	}

	void VulkanDeletionQueue::Flush()
	{
		vkDeviceWaitIdle(GraphicsContext::GetDevice()->GetHandle());

		for (size_t i = 0; i < s_deletionFunctions.size(); i++)
		{
			s_deletionFunctions.top()();
			s_deletionFunctions.pop();
		}
	}
}