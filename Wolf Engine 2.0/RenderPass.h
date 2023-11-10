#pragma once

#include <functional>
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

		void beginRenderPass(VkFramebuffer framebuffer, const std::vector<VkClearValue>& clearValues, VkCommandBuffer commandBuffer) const;
		void endRenderPass(VkCommandBuffer commandBuffer);

		// Getters
		[[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }
		[[nodiscard]] const VkExtent2D& getExtent() const { return m_extent; }

		// Setters
		void setExtent(const VkExtent2D& extent);
		void registerNewExtentChangedCallback(const std::function<void(const RenderPass*)>& callback) { m_callbackWhenExtentChanged.push_back(callback); }

	private:
		VkRenderPass m_renderPass;
		VkExtent2D m_extent;

		std::vector<std::function<void(const RenderPass*)>> m_callbackWhenExtentChanged;
	};
}
