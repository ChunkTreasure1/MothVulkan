#pragma once

#include "Lamp/Core/Base.h"

#include <vulkan/vulkan.h>

namespace Lamp
{
	class RenderGraph;
	class ImageBarrier;
	class DependencyGraph
	{
	public:
		DependencyGraph(Ref<RenderGraph> renderGraph);
		void InsertBarriersForPass(VkCommandBuffer commandBuffer, size_t passHash);

		static Ref<DependencyGraph> Create(Ref<RenderGraph> renderGraph);

	private:
		void Invalidate();

		Ref<RenderGraph> m_renderGraph;
		std::unordered_map<size_t, std::vector<Ref<ImageBarrier>>> m_toDependencyImageBarriers; // Pass Hash -> Image barriers to run before pass
	};
}