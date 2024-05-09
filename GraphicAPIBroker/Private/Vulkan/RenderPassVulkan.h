#pragma once

#include "../../Public/RenderPass.h"

namespace Wolf
{
	class FrameBuffer;

	class RenderPassVulkan : public RenderPass
	{
	public:
		RenderPassVulkan(const std::vector<Attachment>& attachments);
		~RenderPassVulkan() override;

		[[nodiscard]] VkRenderPass getRenderPass() const { return m_renderPass; }

	private:
		VkRenderPass m_renderPass;
	};
}
