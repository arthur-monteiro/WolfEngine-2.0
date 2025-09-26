#include "ImageVulkan.h"

#include <cstring>

#include <Debug.h>
#include <GPUMemoryDebug.h>

#include "CommandBufferVulkan.h"
#include "BufferVulkan.h"
#include "FenceVulkan.h"
#include "FormatsVulkan.h"
#include "Vulkan.h"
#include "VulkanHelper.h"

Wolf::ImageVulkan::ImageVulkan(const CreateImageInfo& createImageInfo)
{
	m_imageFormat = createImageInfo.format;
	m_vkImageFormat = wolfFormatToVkFormat(createImageInfo.format);
	m_extent = { createImageInfo.extent.width, createImageInfo.extent.height, createImageInfo.extent.depth };
	m_sampleCount = createImageInfo.sampleCount;
	if (createImageInfo.mipLevelCount == UINT32_MAX)
	{
		int32_t mipCount = static_cast<int32_t>(std::floor(std::log2(std::max(m_extent.width, m_extent.height)))) - 1; // remove 2 mip levels as min size must be 4x4
		if (mipCount <= 0)
		{
			Debug::sendWarning("Image is too small to have mips");
			mipCount = 1;
		}
		m_mipLevelCount = mipCount;
	}
	else
		m_mipLevelCount = createImageInfo.mipLevelCount;
	m_arrayLayerCount = createImageInfo.arrayLayerCount;
	m_aspectFlags = createImageInfo.aspect;
	resetAllLayouts();

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	if (m_extent.depth == 1) imageInfo.imageType = VK_IMAGE_TYPE_2D;
	else imageInfo.imageType = VK_IMAGE_TYPE_3D;
	imageInfo.extent.width = m_extent.width;
	imageInfo.extent.height = m_extent.height;
	imageInfo.extent.depth = m_extent.depth;
	imageInfo.mipLevels = m_mipLevelCount;
	imageInfo.arrayLayers = m_arrayLayerCount;
	imageInfo.format = m_vkImageFormat;
	imageInfo.tiling = createImageInfo.imageTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = wolfImageUsageFlagsToVkImageUsageFlags(createImageInfo.usage);
	imageInfo.samples = m_sampleCount;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = m_arrayLayerCount == 6 ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

	if (vkCreateImage(g_vulkanInstance->getDevice(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
		Debug::sendError("Error : create image");

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(g_vulkanInstance->getDevice(), m_image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	m_memoryProperties = createImageInfo.memoryProperties;
	m_allocationSize = allocInfo.allocationSize;
	allocInfo.memoryTypeIndex = findMemoryType(g_vulkanInstance->getPhysicalDevice(), memRequirements.memoryTypeBits, createImageInfo.memoryProperties);

	if (allocInfo.memoryTypeIndex == static_cast<uint32_t>(-1))
		Debug::sendError("Error : no memory type found");

	if (vkAllocateMemory(g_vulkanInstance->getDevice(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
		Debug::sendError("Failed to allocate image memory");

	vkBindImageMemory(g_vulkanInstance->getDevice(), m_image, m_imageMemory, 0);

	setBPP();

	uint64_t totalRequestedSize = static_cast<uint64_t>(static_cast<float>(m_extent.width) * static_cast<float>(m_extent.height) * static_cast<float>(m_extent.depth) * m_bpp);
	uint32_t currentWidth = m_extent.width;
	uint32_t currentHeight = m_extent.height;
	for (uint32_t mipLevel = 1; mipLevel < m_mipLevelCount; ++mipLevel)
	{
		currentWidth /= 2;
		currentHeight /= 2;

		totalRequestedSize += static_cast<uint64_t>(static_cast<float>(currentWidth) * static_cast<float>(currentHeight) * static_cast<float>(m_extent.depth) * m_bpp);
	}
	GPUMemoryDebug::registerNewResource(GPUMemoryDebug::TYPE::TEXTURE, 0, totalRequestedSize, memRequirements.size);
}

Wolf::ImageVulkan::ImageVulkan(VkImage image, Format format, VkImageAspectFlags aspect, VkExtent2D extent)
{
	m_image = image;
	m_imageMemory = VK_NULL_HANDLE;
	m_imageFormat = format;
	m_vkImageFormat = wolfFormatToVkFormat(format);
	m_extent = { extent.width, extent.height, 1 };
	m_mipLevelCount = 1;
	m_aspectFlags = aspect;
	m_arrayLayerCount = 1;
	m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

	resetAllLayouts();

	setBPP();
}

Wolf::ImageVulkan::~ImageVulkan()
{
	for (const std::pair<uint32_t, VkImageView> imageView : m_imageViews)
		vkDestroyImageView(g_vulkanInstance->getDevice(), imageView.second, nullptr);

	if (m_imageMemory == VK_NULL_HANDLE)
		return;
	vkDestroyImage(g_vulkanInstance->getDevice(), m_image, nullptr);
	vkFreeMemory(g_vulkanInstance->getDevice(), m_imageMemory, nullptr);

	GPUMemoryDebug::unregisterResource(GPUMemoryDebug::TYPE::TEXTURE, 0, static_cast<uint64_t>(static_cast<float>(m_extent.width) * static_cast<float>(m_extent.height) * static_cast<float>(m_extent.depth) * m_bpp), m_allocationSize);
}

void Wolf::ImageVulkan::copyCPUBuffer(const unsigned char* pixels, const TransitionLayoutInfo& finalLayout, uint32_t mipLevel, uint32_t baseArrayLayer)
{
	const VkExtent3D extent = { m_extent.width >> mipLevel, m_extent.height >> mipLevel, m_extent.depth };
	const VkDeviceSize imageSize = static_cast<VkDeviceSize>(static_cast<float>(extent.width) * static_cast<float>(extent.height) * static_cast<float>(extent.depth) * m_bpp);

	const BufferVulkan stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory(), 0, imageSize, 0, &mappedData);
	std::memcpy(mappedData, pixels, imageSize);
	vkUnmapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory());

	BufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = extent.width;
	copyRegion.bufferImageHeight = extent.height;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = mipLevel;
	copyRegion.imageSubresource.baseArrayLayer = baseArrayLayer;
	copyRegion.imageSubresource.layerCount = 1;

	copyRegion.imageOffset = { 0, 0, 0 };
	copyRegion.imageExtent = { extent.width, extent.height, extent.depth };

	copyGPUBuffer(stagingBuffer, copyRegion, finalLayout);
}

void Wolf::ImageVulkan::copyGPUBuffer(const Buffer& bufferSrc, const BufferImageCopy& copyRegion, const TransitionLayoutInfo& finalLayout)
{
	const CommandBufferVulkan commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer();

	transitionImageLayout(commandBuffer, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, copyRegion.imageSubresource.mipLevel,
		1, copyRegion.imageSubresource.baseArrayLayer, copyRegion.imageSubresource.layerCount });
	VkBufferImageCopy vkBufferImageCopy = wolfBufferImageCopyToVkBufferImageCopy(copyRegion);
	vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(), static_cast<const BufferVulkan*>(&bufferSrc)->getBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	1, &vkBufferImageCopy);
	transitionImageLayout(commandBuffer, finalLayout);

	commandBuffer.endCommandBuffer();

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<const Semaphore*> signalSemaphores;
	const FenceVulkan fence(0);
	commandBuffer.submit(waitSemaphores, signalSemaphores, &fence);
	fence.waitForFence();
}

void Wolf::ImageVulkan::recordCopyGPUBuffer(const CommandBuffer& commandBuffer, const Buffer& bufferSrc, const BufferImageCopy& copyRegion, const TransitionLayoutInfo& finalLayout)
{
	const CommandBufferVulkan* commandBufferVulkan = static_cast<const CommandBufferVulkan*>(&commandBuffer);
	const BufferVulkan* srcAsBufferVulkan = static_cast<const BufferVulkan*>(&bufferSrc);

	transitionImageLayout(commandBuffer, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, copyRegion.imageSubresource.mipLevel,
		1 });

	VkBufferImageCopy vkBufferImageCopy = wolfBufferImageCopyToVkBufferImageCopy(copyRegion);
	vkCmdCopyBufferToImage(commandBufferVulkan->getCommandBuffer(), srcAsBufferVulkan->getBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &vkBufferImageCopy);

	transitionImageLayout(commandBuffer, finalLayout);
}

void Wolf::ImageVulkan::copyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy)
{
	const CommandBufferVulkan commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer();

	transitionImageLayout(commandBuffer, { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, imageCopy.dstSubresource.mipLevel, 1 });
	recordCopyGPUImage(imageSrc, imageCopy, commandBuffer);
	transitionImageLayout(commandBuffer, { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, imageCopy.dstSubresource.mipLevel,
		1 });

	commandBuffer.endCommandBuffer();

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<const Semaphore*> signalSemaphores;
	const FenceVulkan fence(0);
	commandBuffer.submit(waitSemaphores, signalSemaphores, &fence);
	fence.waitForFence();
}

void Wolf::ImageVulkan::recordCopyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy, const CommandBuffer& commandBuffer)
{
	const ImageVulkan* imageVulkanSrc = static_cast<const ImageVulkan*>(&imageSrc);
	const CommandBufferVulkan* commandBufferVulkan = static_cast<const CommandBufferVulkan*>(&commandBuffer);

	vkCmdCopyImage(commandBufferVulkan->getCommandBuffer(), imageVulkanSrc->getImage(), imageVulkanSrc->getImageLayout(imageCopy.srcSubresource.mipLevel), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
}

void* Wolf::ImageVulkan::map() const
{
	const VkDeviceSize imageSize = static_cast<VkDeviceSize>(static_cast<float>(m_extent.width) * static_cast<float>(m_extent.height) * static_cast<float>(m_extent.depth) * m_bpp);
	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), m_imageMemory, 0, imageSize, 0, &mappedData);
	return mappedData;
}

void Wolf::ImageVulkan::unmap() const
{
	vkUnmapMemory(g_vulkanInstance->getDevice(), m_imageMemory);
}

void Wolf::ImageVulkan::getResourceLayout(VkSubresourceLayout& output) const
{
	VkImageSubresource subresource{};
	subresource.aspectMask = m_aspectFlags;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;
	vkGetImageSubresourceLayout(g_vulkanInstance->getDevice(), m_image, &subresource, &output);
}

void Wolf::ImageVulkan::exportToBuffer(std::vector<uint8_t>& outBuffer) const
{
	if (m_sampleCount != VK_SAMPLE_COUNT_1_BIT)
	{
		Debug::sendError("Can't export multi-sampled image");
		return;
	}
	if (m_extent.depth != 1)
	{
		Debug::sendError("Can't export 3D image");
		return;
	}
	if (m_arrayLayerCount != 1)
	{
		Debug::sendError("Can't export multi-layer image");
		return;
	}

	if (m_memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		Debug::sendWarning("Memory is host visible, no need for a staging image");
	}

	CreateImageInfo stagingCreateImageInfo{};
	stagingCreateImageInfo.extent = { m_extent.width, m_extent.height, m_extent.depth };
	stagingCreateImageInfo.format = m_imageFormat;
	stagingCreateImageInfo.arrayLayerCount = m_arrayLayerCount;
	stagingCreateImageInfo.aspect = m_aspectFlags;
	stagingCreateImageInfo.sampleCount = m_sampleCount;
	stagingCreateImageInfo.usage = TRANSFER_DST;
	stagingCreateImageInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagingCreateImageInfo.imageTiling = VK_IMAGE_TILING_LINEAR;
	stagingCreateImageInfo.mipLevelCount = 1;
	ImageVulkan stagingImage(stagingCreateImageInfo);

	VkImageCopy copyRegion = {};

	copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.srcSubresource.baseArrayLayer = 0;
	copyRegion.srcSubresource.mipLevel = 0;
	copyRegion.srcSubresource.layerCount = 1;
	copyRegion.srcOffset = { 0, 0, 0 };

	copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.dstSubresource.baseArrayLayer = 0;
	copyRegion.dstSubresource.mipLevel = 0;
	copyRegion.dstSubresource.layerCount = 1;
	copyRegion.dstOffset = { 0, 0, 0 };

	copyRegion.extent.width = m_extent.width;
	copyRegion.extent.height = m_extent.height;
	copyRegion.extent.depth = m_extent.depth;

	stagingImage.copyGPUImage(*this, copyRegion);

	const uint8_t* pixels = static_cast<uint8_t*>(stagingImage.map());
	outBuffer.assign(pixels, pixels + static_cast<size_t>(m_extent.width) * m_extent.height * static_cast<size_t>(computeBPP()));

	stagingImage.unmap();
}

void Wolf::ImageVulkan::setImageLayout(const TransitionLayoutInfo& transitionLayoutInfo)
{
	const CommandBufferVulkan commandBuffer(QueueType::GRAPHIC, true);
	commandBuffer.beginCommandBuffer();

	transitionImageLayout(commandBuffer, transitionLayoutInfo);

	commandBuffer.endCommandBuffer();

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<const Semaphore*> signalSemaphores;
	const FenceVulkan fence(0);
	commandBuffer.submit(waitSemaphores, signalSemaphores, &fence);
	fence.waitForFence();
}

uint32_t computeImageViewHash(VkFormat format)
{
	return (uint32_t)format;
}

void Wolf::ImageVulkan::createImageView(VkFormat format)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_image;
	viewInfo.viewType = m_arrayLayerCount == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : (m_extent.depth != 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D);
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = m_aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_mipLevelCount;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = m_arrayLayerCount;

	VkImageView imageView;
	if (vkCreateImageView(g_vulkanInstance->getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
		Debug::sendError("Error : create image view");

	const uint32_t hash = computeImageViewHash(m_vkImageFormat);
	m_imageViews[hash] = imageView;
}

float Wolf::ImageVulkan::computeBPP() const
{
	switch (m_vkImageFormat)
	{
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16.0f;
			break;
		case VK_FORMAT_R32G32B32_SFLOAT:
			return 12.0f;
			break;
		case VK_FORMAT_R32G32_SFLOAT:
			return 8.0f;
			break;
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
		case VK_FORMAT_D32_SFLOAT:
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32_SINT:
			return 4.0f;
			break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
			return 0.5f;
			break;
		case VK_FORMAT_BC3_UNORM_BLOCK:
		case VK_FORMAT_BC3_SRGB_BLOCK:
		case VK_FORMAT_BC5_UNORM_BLOCK:
		case VK_FORMAT_R8_UINT:
			return 1.0f;
			break;
		default:
			return 1.0f;
			Debug::sendError("Unsupported image format");
			break;
	}
}

void Wolf::ImageVulkan::setBPP()
{
	m_bpp = computeBPP();
}

void Wolf::ImageVulkan::resetAllLayouts()
{
	m_imageLayouts.resize(m_arrayLayerCount);
	for (std::vector<VkImageLayout>& imageLayoutArray : m_imageLayouts)
	{
		imageLayoutArray.resize(m_mipLevelCount);

		for (VkImageLayout& imageLayout : imageLayoutArray)
		{
			imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}
}

VkImageUsageFlagBits Wolf::ImageVulkan::wolfImageUsageFlagBitsToVkImageUsageFlagBits(ImageUsageFlagBits imageUsageFlagBits)
{
	switch (imageUsageFlagBits)
	{
		case SAMPLED:
			return VK_IMAGE_USAGE_SAMPLED_BIT;
		case TRANSFER_SRC:
			return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		case TRANSFER_DST:
			return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		case DEPTH_STENCIL_ATTACHMENT:
			return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		case COLOR_ATTACHMENT:
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		case TRANSIENT_ATTACHMENT:
			return VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		case FRAGMENT_SHADING_RATE_ATTACHMENT:
			return VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		case STORAGE:
			return VK_IMAGE_USAGE_STORAGE_BIT;
		default: 
			Debug::sendError("Unhandled image usage");
			return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	}
}

VkImageUsageFlags Wolf::ImageVulkan::wolfImageUsageFlagsToVkImageUsageFlags(ImageUsageFlags imageUsageFlags)
{
	VkImageUsageFlags vkImageUsageFlags = 0;

#define ADD_FLAG_IF_PRESENT(flag) if (imageUsageFlags & (flag)) vkImageUsageFlags |= wolfImageUsageFlagBitsToVkImageUsageFlagBits(flag)

	for (uint32_t flag = 1; flag < IMAGE_USAGE_MAX; flag <<= 1)
		ADD_FLAG_IF_PRESENT(static_cast<ImageUsageFlagBits>(flag));

#undef ADD_FLAG_IF_PRESENT

	return vkImageUsageFlags;
}

VkBufferImageCopy Wolf::ImageVulkan::wolfBufferImageCopyToVkBufferImageCopy(const BufferImageCopy& bufferImageCopy)
{
	VkBufferImageCopy vkBufferImageCopy{};
	vkBufferImageCopy.bufferOffset = bufferImageCopy.bufferOffset;
	vkBufferImageCopy.bufferRowLength = bufferImageCopy.bufferRowLength;
	vkBufferImageCopy.bufferImageHeight = bufferImageCopy.bufferImageHeight;
	vkBufferImageCopy.imageSubresource = bufferImageCopy.imageSubresource;
	vkBufferImageCopy.imageOffset.x = bufferImageCopy.imageOffset.x;
	vkBufferImageCopy.imageOffset.y = bufferImageCopy.imageOffset.y;
	vkBufferImageCopy.imageOffset.z = bufferImageCopy.imageOffset.z;
	vkBufferImageCopy.imageExtent.width = bufferImageCopy.imageExtent.width;
	vkBufferImageCopy.imageExtent.height = bufferImageCopy.imageExtent.height;
	vkBufferImageCopy.imageExtent.depth = bufferImageCopy.imageExtent.depth;
	return vkBufferImageCopy;
}

Wolf::ImageView Wolf::ImageVulkan::getImageView(Format format)
{
	const uint32_t hash = computeImageViewHash(wolfFormatToVkFormat(format));
	if (m_imageViews.find(hash) == m_imageViews.end())
	{
		createImageView(wolfFormatToVkFormat(format));
	}

	return m_imageViews[hash];
}

Wolf::ImageView Wolf::ImageVulkan::getDefaultImageView()
{
	return getImageView(m_imageFormat);
}

void Wolf::ImageVulkan::transitionImageLayout(const CommandBuffer& commandBuffer, const TransitionLayoutInfo& transitionLayoutInfo)
{
	uint32_t mipLevelCount = transitionLayoutInfo.levelCount;

	if (mipLevelCount == MAX_MIP_COUNT)
		mipLevelCount = m_mipLevelCount;

	// Sanity checks
	{
		const VkImageLayout imageLayout = m_imageLayouts[transitionLayoutInfo.baseArrayLayer][transitionLayoutInfo.baseMipLevel];
		for (uint32_t layer = transitionLayoutInfo.baseArrayLayer; layer < transitionLayoutInfo.baseArrayLayer + transitionLayoutInfo.layerCount; layer++)
		{
			for (uint32_t mipLevel = transitionLayoutInfo.baseMipLevel; mipLevel < mipLevelCount; mipLevel++)
			{
				if (m_imageLayouts[layer][mipLevel] != imageLayout)
					Debug::sendError("Image layout must be the same for all mip level and all layers asked for transition");
			}
		}

		//if (std::this_thread::get_id() != g_mainThreadId)
		{
			// TODO: trigger this error for shared between multiple command records resources only
			//if (transitionLayoutInfo.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
			//	Debug::sendError("Old layout must be specified if not running from main thread (as there's no guarantee that previous transition has been recorded)");
		}
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = transitionLayoutInfo.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED ? m_imageLayouts[transitionLayoutInfo.baseArrayLayer][transitionLayoutInfo.baseMipLevel] : transitionLayoutInfo.oldLayout;
	barrier.newLayout = transitionLayoutInfo.dstLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.baseMipLevel = transitionLayoutInfo.baseMipLevel;
	barrier.subresourceRange.levelCount = mipLevelCount;
	barrier.subresourceRange.baseArrayLayer = transitionLayoutInfo.baseArrayLayer;
	barrier.subresourceRange.layerCount = transitionLayoutInfo.layerCount;

	if (bool depth = hasDepthComponent(m_vkImageFormat), stencil = hasStencilComponent(m_vkImageFormat); depth || stencil)
	{
		if(depth)
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (stencil)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.srcAccessMask = m_accessFlags;
	barrier.dstAccessMask = transitionLayoutInfo.dstAccessMask;

	vkCmdPipelineBarrier(static_cast<const CommandBufferVulkan*>(&commandBuffer)->getCommandBuffer(), m_pipelineStageFlags, transitionLayoutInfo.dstPipelineStageFlags, 0,	0, nullptr, 0, nullptr,	1, &barrier);

	m_accessFlags = transitionLayoutInfo.dstAccessMask;
	for (uint32_t layer = transitionLayoutInfo.baseArrayLayer; layer < transitionLayoutInfo.baseArrayLayer + transitionLayoutInfo.layerCount; layer++)
	{
		for (uint32_t mipLevel = transitionLayoutInfo.baseMipLevel; mipLevel < mipLevelCount; mipLevel++)
			m_imageLayouts[layer][mipLevel] = transitionLayoutInfo.dstLayout;
	}

	m_pipelineStageFlags = transitionLayoutInfo.dstPipelineStageFlags;
}

void Wolf::ImageVulkan::setImageLayoutWithoutOperation(VkImageLayout newImageLayout, uint32_t baseMipLevel, uint32_t levelCount)
{
	if (levelCount == MAX_MIP_COUNT)
		levelCount = m_mipLevelCount;

	for (uint32_t mipLevel = baseMipLevel; mipLevel < levelCount; mipLevel++)
		m_imageLayouts[0 /* layer */][mipLevel] = newImageLayout;
}
