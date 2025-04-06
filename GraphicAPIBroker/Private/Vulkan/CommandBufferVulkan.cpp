#ifdef WOLF_VULKAN
#include "CommandBufferVulkan.h"

#include <Configuration.h>
#include <Debug.h>
#include <RuntimeContext.h>

#include "../../Public/ShaderBindingTable.h"
#include "BufferVulkan.h"
#include "DescriptorSetVulkan.h"
#include "ImageVulkan.h"
#include "FenceVulkan.h"
#include "FrameBufferVulkan.h"
#include "PipelineVulkan.h"
#include "RenderPassVulkan.h"
#include "SemaphoreVulkan.h"
#include "Vulkan.h"

Wolf::CommandBufferVulkan::CommandBufferVulkan(QueueType queueType, bool isTransient, bool preRecord)
{
	m_commandBuffers.resize((isTransient || preRecord) ? 1 : g_configuration->getMaxCachedFrames());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

	VkCommandPool commandPool = VK_NULL_HANDLE;
	if (queueType == QueueType::GRAPHIC || queueType == QueueType::TRANSFER || queueType == QueueType::COMPUTE || queueType == QueueType::RAY_TRACING)
	{
		if (isTransient)
			commandPool = g_vulkanInstance->getGraphicsTransientCommandPool()->getCommandPool();
		else
			commandPool = g_vulkanInstance->getGraphicsCommandPool()->getCommandPool();
	}
	else if (queueType == QueueType::ASYNC_COMPUTE)
	{
		if (isTransient)
			commandPool = g_vulkanInstance->getComputeTransientCommandPool()->getCommandPool();
		else
			commandPool = g_vulkanInstance->getComputeCommandPool()->getCommandPool();
	}

	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

	if (vkAllocateCommandBuffers(g_vulkanInstance->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		Debug::sendError("Error : command buffer allocation");

	m_usedCommandPool = commandPool;
	m_queueType = queueType;
	m_isTransient = isTransient;
	m_isPreRecorded = preRecord;
}

Wolf::CommandBufferVulkan::~CommandBufferVulkan()
{
	vkFreeCommandBuffers(g_vulkanInstance->getDevice(), m_usedCommandPool, static_cast<uint32_t>(m_commandBuffers.size()), m_commandBuffers.data());
}

void Wolf::CommandBufferVulkan::beginCommandBuffer() const
{
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = m_isPreRecorded ? VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(getCommandBuffer(), &beginInfo);
}

void Wolf::CommandBufferVulkan::endCommandBuffer() const
{
	if (vkEndCommandBuffer(getCommandBuffer()) != VK_SUCCESS)
		Debug::sendError("Error : end command buffer");
}

void Wolf::CommandBufferVulkan::submit(const std::vector<const Semaphore*>& waitSemaphores, const std::vector<const Semaphore*>& signalSemaphores, const ResourceReference<const Fence>& fence) const
{
	VkQueue queue;
	if (m_queueType == QueueType::GRAPHIC || m_queueType == QueueType::TRANSFER || m_queueType == QueueType::COMPUTE || m_queueType == QueueType::RAY_TRACING)
		queue = g_vulkanInstance->getGraphicsQueue();
	else
		queue = g_vulkanInstance->getComputeQueue();

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	std::vector<VkSemaphore> vkSignalSemaphores(signalSemaphores.size());
	for (uint32_t i = 0, end = static_cast<uint32_t>(signalSemaphores.size()); i < end; ++i)
		vkSignalSemaphores[i] = static_cast<const SemaphoreVulkan*>(signalSemaphores[i])->getSemaphore();
	submitInfo.pSignalSemaphores = vkSignalSemaphores.data();

	const VkCommandBuffer commandBuffer = getCommandBuffer();
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.commandBufferCount = 1;

	std::vector<VkSemaphore> semaphores;
	std::vector<VkPipelineStageFlags> stages;
	for (const Semaphore* waitSemaphore : waitSemaphores)
	{
		if (const SemaphoreVulkan* semaphoreVulkan = static_cast<const SemaphoreVulkan*>(waitSemaphore))
		{
			semaphores.push_back(semaphoreVulkan->getSemaphore());
			stages.push_back(semaphoreVulkan->getPipelineStage());
		}
	}
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>(semaphores.size());
	submitInfo.pWaitSemaphores = semaphores.data();
	submitInfo.pWaitDstStageMask = stages.data();

	if (const VkResult result = vkQueueSubmit(queue, 1, &submitInfo, !fence.isNull() ? fence.operator-><const FenceVulkan>()->getFence() : VK_NULL_HANDLE); result != VK_SUCCESS)
	{
		Debug::sendCriticalError("Error : submit to graphics queue");
	}
}

void Wolf::CommandBufferVulkan::beginRenderPass(const RenderPass& renderPass, const FrameBuffer& frameBuffer, const std::vector<ClearValue>& clearValues) const
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = static_cast<const RenderPassVulkan*>(&renderPass)->getRenderPass();
	renderPassInfo.framebuffer = static_cast<const FrameBufferVulkan*>(&frameBuffer)->getFrameBuffer();
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { renderPass.getExtent().width, renderPass.getExtent().height };

	std::vector<VkClearValue> vkClearValues(clearValues.size());
	for (uint32_t i = 0; i < vkClearValues.size(); ++i)
	{
		static_assert(sizeof(VkClearValue) == sizeof(ClearValue));
		memcpy(&vkClearValues[i], &clearValues[i], sizeof(VkClearValue));
	}

	renderPassInfo.clearValueCount = static_cast<uint32_t>(vkClearValues.size());
	renderPassInfo.pClearValues = vkClearValues.data();

	vkCmdBeginRenderPass(getCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void Wolf::CommandBufferVulkan::endRenderPass() const
{
	vkCmdEndRenderPass(getCommandBuffer());
}

void Wolf::CommandBufferVulkan::bindVertexBuffer(const Buffer& buffer, uint32_t bindingIdx) const
{
	const VkBuffer vkBuffer = static_cast<const BufferVulkan&>(buffer).getBuffer();
	constexpr VkDeviceSize offsets[1] = { 0 };

	vkCmdBindVertexBuffers(getCommandBuffer(), bindingIdx, 1, &vkBuffer, offsets);
}

void Wolf::CommandBufferVulkan::bindIndexBuffer(const Buffer& buffer, IndexType indexType) const
{
	VkIndexType vkIndexType = VK_INDEX_TYPE_MAX_ENUM;
	switch (indexType) 
	{
	case IndexType::U16: 
		vkIndexType = VK_INDEX_TYPE_UINT16;
		break;
	case IndexType::U32: 
		vkIndexType = VK_INDEX_TYPE_UINT32;
		break;
	}

	vkCmdBindIndexBuffer(getCommandBuffer(), static_cast<const BufferVulkan&>(buffer).getBuffer(), 0, vkIndexType);
}

void Wolf::CommandBufferVulkan::bindPipeline(const ResourceReference<const Pipeline>& pipeline) const
{
	vkCmdBindPipeline(getCommandBuffer(), pipeline.operator-><const PipelineVulkan>()->getPipelineBindPoint(), pipeline.operator-><const PipelineVulkan>()->getPipeline());
}

void Wolf::CommandBufferVulkan::bindDescriptorSet(const ResourceReference<const DescriptorSet>& descriptorSet, uint32_t slot, const Pipeline& pipeline) const
{
	vkCmdBindDescriptorSets(getCommandBuffer(), static_cast<const PipelineVulkan*>(&pipeline)->getPipelineBindPoint(), static_cast<const PipelineVulkan*>(&pipeline)->getPipelineLayout(), slot, 1, 
		&descriptorSet.operator-><const DescriptorSetVulkan>()->getDescriptorSet(), 0, nullptr);
}

VkFragmentShadingRateCombinerOpKHR fragmentShadingRateCombinerOpToVkType(Wolf::FragmentShadingRateCombinerOp in)
{
	switch (in) 
	{
		case Wolf::FragmentShadingRateCombinerOp::KEEP: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
		case Wolf::FragmentShadingRateCombinerOp::REPLACE: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
		case Wolf::FragmentShadingRateCombinerOp::MIN: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR;
		case Wolf::FragmentShadingRateCombinerOp::MAX: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
		case Wolf::FragmentShadingRateCombinerOp::MUL: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR;
	}

	return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_ENUM_KHR;
}

void Wolf::CommandBufferVulkan::setFragmentShadingRate(FragmentShadingRateCombinerOp fragmentShadingRateCombinerOps[2], const Extent2D& fragmentExtent) const
{
	VkFragmentShadingRateCombinerOpKHR combiners[2];
	combiners[0] = fragmentShadingRateCombinerOpToVkType(fragmentShadingRateCombinerOps[0]);
	combiners[1] = fragmentShadingRateCombinerOpToVkType(fragmentShadingRateCombinerOps[1]);
	VkExtent2D vkExtent{ fragmentExtent.width, fragmentExtent.height };
	vkCmdSetFragmentShadingRateKHR(getCommandBuffer(), &vkExtent, combiners);
}

void Wolf::CommandBufferVulkan::setViewport(const Viewport& viewport) const
{
	VkViewport vkViewport;
	vkViewport.x = viewport.x;
	vkViewport.y = viewport.y;
	vkViewport.width = viewport.width;
	vkViewport.height = viewport.height;
	vkViewport.minDepth = viewport.minDepth;
	vkViewport.maxDepth = viewport.maxDepth;

	vkCmdSetViewport(getCommandBuffer(), 0, 1, &vkViewport);
}

void Wolf::CommandBufferVulkan::clearColorImage(const Image& image, VkImageLayout imageLayout, ColorFloat clearColor, const VkImageSubresourceRange& range) const
{
	const VkClearColorValue color = { clearColor.components[0], clearColor.components[1], clearColor.components[2], clearColor.components[3] };
	vkCmdClearColorImage(getCommandBuffer(), static_cast<const ImageVulkan*>(&image)->getImage(), imageLayout, &color, 1, &range);
}

void Wolf::CommandBufferVulkan::fillBuffer(const Buffer& buffer, uint64_t dstOffset, uint64_t size, uint32_t data) const
{
	vkCmdFillBuffer(getCommandBuffer(), static_cast<const BufferVulkan*>(&buffer)->getBuffer(), dstOffset, size, data);
}

void Wolf::CommandBufferVulkan::imageCopy(const Image& imageSrc, VkImageLayout srcImageLayout, const Image& imageDst,
	VkImageLayout dstImageLayout, const ImageCopyInfo& imageCopyInfo) const
{
	VkImageCopy copyRegion{};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = imageCopyInfo.srcBaseArrayLayer;
	copyRegion.srcSubresource.mipLevel = imageCopyInfo.srcMipLevel;
	copyRegion.srcSubresource.layerCount = imageCopyInfo.srcLayerCount;
	copyRegion.srcOffset = { imageCopyInfo.srcOffset.x, imageCopyInfo.srcOffset.y, imageCopyInfo.srcOffset.z };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = imageCopyInfo.dstBaseArrayLayer;
	copyRegion.dstSubresource.mipLevel = imageCopyInfo.dstMipLevel;
	copyRegion.dstSubresource.layerCount = imageCopyInfo.dstLayerCount;
	copyRegion.dstOffset = { imageCopyInfo.dstOffset.x, imageCopyInfo.dstOffset.y, imageCopyInfo.dstOffset.z };;

	copyRegion.extent = { imageCopyInfo.extent.x, imageCopyInfo.extent.y, imageCopyInfo.extent.z };

	vkCmdCopyImage(getCommandBuffer(), static_cast<const ImageVulkan*>(&imageSrc)->getImage(), srcImageLayout, static_cast<const ImageVulkan*>(&imageDst)->getImage(), dstImageLayout, 1, &copyRegion);
}

void Wolf::CommandBufferVulkan::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const
{
	vkCmdDraw(getCommandBuffer(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void Wolf::CommandBufferVulkan::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                            int32_t vertexOffset, uint32_t firstInstance) const
{
	vkCmdDrawIndexed(getCommandBuffer(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void Wolf::CommandBufferVulkan::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const
{
	vkCmdDispatch(getCommandBuffer(), groupCountX, groupCountY, groupCountZ);
}

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
void Wolf::CommandBufferVulkan::traceRays(const ResourceReference<const ShaderBindingTable>& shaderBindingTable, const Extent3D& extent) const
{
	VkDeviceAddress sbtBufferAddress = static_cast<const BufferVulkan&>(shaderBindingTable->getBuffer()).getBufferDeviceAddress();

	VkStridedDeviceAddressRegionKHR rgenRegion;
	rgenRegion.deviceAddress = sbtBufferAddress;
	rgenRegion.stride = shaderBindingTable->getBaseAlignment();
	rgenRegion.size = shaderBindingTable->getBaseAlignment();

	VkStridedDeviceAddressRegionKHR rmissRegion;
	rmissRegion.deviceAddress = sbtBufferAddress + rgenRegion.size;
	rmissRegion.stride = shaderBindingTable->getBaseAlignment();
	rmissRegion.size = shaderBindingTable->getBaseAlignment();

	VkStridedDeviceAddressRegionKHR rhitRegion;
	rhitRegion.deviceAddress = sbtBufferAddress + rgenRegion.size + rmissRegion.size;
	rhitRegion.stride = shaderBindingTable->getBaseAlignment();
	rhitRegion.size = shaderBindingTable->getBaseAlignment();

	const VkStridedDeviceAddressRegionKHR callRegion{};

	vkCmdTraceRaysKHR(getCommandBuffer(), &rgenRegion,
		&rmissRegion,
		&rhitRegion,
		&callRegion, extent.width, extent.height, extent.depth);
}
#endif

void Wolf::CommandBufferVulkan::debugMarkerInsert(const DebugMarkerInfo& debugMarkerInfo) const
{
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
	VkDebugMarkerMarkerInfoEXT markerInfo = {};
	markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
	memcpy(markerInfo.color, &debugMarkerInfo.color, sizeof(float) * 4);
	markerInfo.pMarkerName = debugMarkerInfo.name.c_str();
	vkCmdDebugMarkerInsertEXT(getCommandBuffer(), &markerInfo);
#endif
}

void Wolf::CommandBufferVulkan::debugMarkerBegin(const DebugMarkerInfo& debugMarkerInfo) const
{
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
	VkDebugMarkerMarkerInfoEXT markerInfo = {};
	markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
	memcpy(markerInfo.color, &debugMarkerInfo.color, sizeof(float) * 4);
	markerInfo.pMarkerName = debugMarkerInfo.name.c_str();
	vkCmdDebugMarkerBeginEXT(getCommandBuffer(), &markerInfo);
#endif
}

void Wolf::CommandBufferVulkan::debugMarkerEnd() const
{
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
	vkCmdDebugMarkerEndEXT(getCommandBuffer());
#endif
}

VkCommandBuffer Wolf::CommandBufferVulkan::getCommandBuffer() const
{
	uint32_t bufferIdx = (m_isTransient || m_isPreRecorded) ? 0 : (g_runtimeContext->getCurrentCPUFrameNumber() % g_configuration->getMaxCachedFrames());

	return m_commandBuffers[bufferIdx];
}

#endif
