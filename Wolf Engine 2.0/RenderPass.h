#pragma once

#include <vector>

#include "Attachment.h"

namespace Wolf
{
	class FrameBuffer;

	class RenderPass
	{
	public:
		RenderPass(const std::vector<Attachment>& attachments);
		~RenderPass();

		void beginRenderPass(VkFramebuffer framebuffer, const std::vector<VkClearValue>& clearValues, VkCommandBuffer commandBuffer);
		void endRenderPass(VkCommandBuffer commandBuffer);

		// Getters
		VkRenderPass getRenderPass() const { return m_renderPass; }

		// Setters
		void setExtent(VkExtent2D extent) { m_extent = extent; }

	private:
		VkRenderPass m_renderPass;

		VkExtent2D m_extent;
	};
}
