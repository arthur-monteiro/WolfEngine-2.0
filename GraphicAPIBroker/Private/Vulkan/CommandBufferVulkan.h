#pragma once

#ifdef WOLF_USE_VULKAN

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../../Public/CommandBuffer.h"

namespace Wolf
{
	class CommandBufferVulkan : public CommandBuffer
	{
	public:
		CommandBufferVulkan(QueueType queueType, bool isTransient);
		~CommandBufferVulkan() override;

		void beginCommandBuffer() const override;
		void endCommandBuffer() const override;
		void submit(const std::vector<const Semaphore*>& waitSemaphores, const std::vector<const Semaphore*>& signalSemaphores, const ResourceReference<const Fence>& fence) const override;

		void beginRenderPass(const RenderPass& renderPass, const FrameBuffer& frameBuffer, const std::vector<VkClearValue>& clearValues) const override;
		void endRenderPass() const override;

		void bindVertexBuffer(const Buffer& buffer, uint32_t bindingIdx = 0) const override;
		void bindIndexBuffer(const Buffer& buffer, IndexType indexType) const override;

		void bindPipeline(const ResourceReference<const Pipeline>& pipeline) const override;

		void bindDescriptorSet(const ResourceReference<const DescriptorSet>& descriptorSet, uint32_t slot, const Pipeline& pipeline) const override;

		void setFragmentShadingRate(FragmentShadingRateCombinerOp fragmentShadingRateCombinerOps[2], const VkExtent2D& fragmentExtent) const override;
		void setViewport(const Viewport& viewport) const override;

		void clearColorImage(const Image& image, VkImageLayout imageLayout, ColorFloat clearColor, const VkImageSubresourceRange& range) const override;
		void fillBuffer(const Buffer& buffer, uint64_t  dstOffset, uint64_t size, uint32_t data) const override;
		void imageCopy(const Image& imageSrc, VkImageLayout srcImageLayout, const Image& imageDst, VkImageLayout dstImageLayout, const ImageCopyInfo& imageCopyInfo) const override;

		void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const override;
		void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const override;
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
		void traceRays(const ResourceReference<const ShaderBindingTable>& shaderBindingTable, const VkExtent3D& extent) const override;
#endif

		void debugMarkerInsert(const DebugMarkerInfo& debugMarkerInfo) const override;
		void debugMarkerBegin(const DebugMarkerInfo& debugMarkerInfo) const override;
		void debugMarkerEnd() const override;

		[[nodiscard]] VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }

	private:
		VkCommandBuffer m_commandBuffer;
		VkCommandPool m_usedCommandPool;
		QueueType m_queueType;
		bool m_oneTimeSubmit;
	};
}

#endif