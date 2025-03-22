#pragma once

#include <cstdint>
#include <vector>

// TEMP VkImageLayout + VkImageSubresourceRange
#include <vulkan/vulkan_core.h>

#include "Buffer.h"
#include "ClearValue.h"
#include "Enums.h"
#include "Extents.h"
#include "Fence.h"
#include "ResourceReference.h"
#include "Structs.h"

namespace Wolf
{
	class Image;
	class ShaderBindingTable;
	class Pipeline;
	class DescriptorSet;
	class RenderPass;
	class FrameBuffer;
	class Semaphore;

	enum class QueueType { GRAPHIC, COMPUTE, TRANSFER, RAY_TRACING, ASYNC_COMPUTE };

	class CommandBuffer
	{
	public:
		static CommandBuffer* createCommandBuffer(QueueType queueType, bool isTransient);

		virtual ~CommandBuffer() = default;

		// Begin / end
		virtual void beginCommandBuffer() const = 0;
		virtual void endCommandBuffer() const = 0;
		virtual void submit(const std::vector<const Semaphore*>& waitSemaphores, const std::vector<const Semaphore*>& signalSemaphores, const ResourceReference<const Fence>& fence) const = 0;

		virtual void beginRenderPass(const RenderPass& renderPass, const FrameBuffer& frameBuffer, const std::vector<ClearValue>& clearValues) const = 0;
		virtual void endRenderPass() const = 0;

		// Binds
		virtual void bindVertexBuffer(const Buffer& buffer, uint32_t bindingIdx = 0) const = 0;
		virtual void bindIndexBuffer(const Buffer& buffer, IndexType indexType) const = 0;

		virtual void bindPipeline(const ResourceReference<const Pipeline>& pipeline) const = 0;

		virtual void bindDescriptorSet(const ResourceReference<const DescriptorSet>& descriptorSet, uint32_t slot, const Pipeline& pipeline) const = 0;

		// Params
		virtual void setFragmentShadingRate(FragmentShadingRateCombinerOp fragmentShadingRateCombinerOps[2], const Extent2D& fragmentExtent) const = 0;
		virtual void setViewport(const Viewport& viewport) const = 0;

		// Commands
		virtual void clearColorImage(const Image& image, VkImageLayout imageLayout, ColorFloat clearColor, const VkImageSubresourceRange& range) const = 0;
		virtual void fillBuffer(const Buffer& buffer, uint64_t  dstOffset, uint64_t size, uint32_t data) const = 0;
		virtual void imageCopy(const Image& imageSrc, VkImageLayout srcImageLayout, const Image& imageDst, VkImageLayout dstImageLayout, const ImageCopyInfo& imageCopyInfo) const = 0;

		virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const = 0;
		virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const = 0;
		virtual void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const = 0;
#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30
		virtual void traceRays(const ResourceReference<const ShaderBindingTable>& shaderBindingTable, const Extent3D& extent) const = 0;
#endif

		virtual void debugMarkerInsert(const DebugMarkerInfo& debugMarkerInfo) const = 0;
		virtual void debugMarkerBegin(const DebugMarkerInfo& debugMarkerInfo) const = 0;
		virtual void debugMarkerEnd() const = 0;
	};
}
