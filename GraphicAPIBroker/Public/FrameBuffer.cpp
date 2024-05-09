#include "FrameBuffer.h"

#ifdef WOLF_USE_VULKAN
#include "../Private/Vulkan/FrameBufferVulkan.h"
#endif

Wolf::FrameBuffer* Wolf::FrameBuffer::createFrameBuffer(const RenderPass& renderPass,
	const std::vector<Attachment>& attachments)
{
#ifdef WOLF_USE_VULKAN
	return new FrameBufferVulkan(renderPass, attachments);
#else
	return nullptr;
#endif
}