#pragma once

#include "../../Public/FrameBuffer.h"

namespace Wolf
{
	class FrameBufferVulkan : public FrameBuffer
	{
	public:
		FrameBufferVulkan(const RenderPass& renderPass, const std::vector<Attachment>& attachments);
		~FrameBufferVulkan() override;

		[[nodiscard]] VkFramebuffer getFrameBuffer() const { return m_frameBuffer; }

	private:
		VkFramebuffer m_frameBuffer;
	};
}