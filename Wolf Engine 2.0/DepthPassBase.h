#pragma once

#include "CommandRecordBase.h"
#include "Image.h"
#include "FrameBuffer.h"
#include "RenderPass.h"

namespace Wolf
{
	struct Attachment;

	class DepthPassBase
	{
	public:
		virtual void initializeResources(const InitializationContext& context);
		virtual void resize(const InitializationContext& context);
		virtual void record(const RecordContext& context);

		[[nodiscard]] virtual Image* getOutput() const { return m_depthImage.get(); }

	private:
		void getAttachments(const InitializationContext& context, std::vector<Attachment>& attachments);
		void createDepthImage(const InitializationContext& context);

	protected:
		virtual uint32_t getWidth() = 0;
		virtual uint32_t getHeight() = 0;

		virtual void recordDraws(const RecordContext& context) = 0;
		virtual VkCommandBuffer getCommandBuffer(const RecordContext& context) = 0;
		virtual VkImageUsageFlags getAdditionalUsages() { return 0; }
		virtual VkImageLayout getFinalLayout() { return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; }

		/* Parameters */
		std::unique_ptr<RenderPass> m_renderPass;
		std::unique_ptr<Framebuffer> m_frameBuffer;
		std::unique_ptr<Image> m_depthImage;
	};
}