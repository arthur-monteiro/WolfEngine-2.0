#include "Image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "Buffer.h"
#include "CommandBuffer.h"
#include "Fence.h"
#include "VulkanHelper.h"

Wolf::Image::Image(const CreateImageInfo& createImageInfo)
{
	m_imageFormat = createImageInfo.format;
	m_extent = createImageInfo.extent;
	m_sampleCount = createImageInfo.sampleCount;
	if (createImageInfo.mipLevelCount == UINT32_MAX)
		m_mipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(m_extent.width, m_extent.height)))) + 1;
	else
		m_mipLevelCount = createImageInfo.mipLevelCount;
	m_arrayLayerCount = createImageInfo.arrayLayerCount;
	m_aspectFlags = createImageInfo.aspect;
	m_imageLayouts.resize(m_mipLevelCount);
	for (VkImageLayout& imageLayout : m_imageLayouts)
		imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	if (m_extent.depth == 1) imageInfo.imageType = VK_IMAGE_TYPE_2D;
	else imageInfo.imageType = VK_IMAGE_TYPE_3D;
	imageInfo.extent.width = m_extent.width;
	imageInfo.extent.height = m_extent.height;
	imageInfo.extent.depth = m_extent.depth;
	imageInfo.mipLevels = m_mipLevelCount;
	imageInfo.arrayLayers = m_arrayLayerCount;
	imageInfo.format = m_imageFormat;
	imageInfo.tiling = createImageInfo.imageTiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = createImageInfo.usage;
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
	allocInfo.memoryTypeIndex = findMemoryType(g_vulkanInstance->getPhysicalDevice(), memRequirements.memoryTypeBits, createImageInfo.memoryProperties);

	if (allocInfo.memoryTypeIndex < 0)
		Debug::sendError("Error : no memory type found");

	if (vkAllocateMemory(g_vulkanInstance->getDevice(), &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
		Debug::sendError("Failed to allocate image memory");

	vkBindImageMemory(g_vulkanInstance->getDevice(), m_image, m_imageMemory, 0);

	setBBP();
}

Wolf::Image::Image(VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent2D extent)
{
	m_image = image;
	m_imageMemory = VK_NULL_HANDLE;
	m_imageFormat = format;
	m_extent = { extent.width, extent.height, 1 };
	m_mipLevelCount = 1;
	m_aspectFlags = aspect;
	m_arrayLayerCount = 1;
	m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

	m_imageLayouts.resize(m_mipLevelCount);
	for (VkImageLayout& imageLayout : m_imageLayouts)
		imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	setBBP();
}

Wolf::Image::~Image()
{
	for (const std::pair<uint32_t, VkImageView> imageView : m_imageViews)
		vkDestroyImageView(g_vulkanInstance->getDevice(), imageView.second, nullptr);

	if (m_imageMemory == VK_NULL_HANDLE)
		return;
	vkDestroyImage(g_vulkanInstance->getDevice(), m_image, nullptr);
	vkFreeMemory(g_vulkanInstance->getDevice(), m_imageMemory, nullptr);
}

void Wolf::Image::copyCPUBuffer(const unsigned char* pixels, uint32_t mipLevel)
{
	const VkDeviceSize imageSize = (m_extent.width * m_extent.height * m_extent.depth * m_bbp) >> mipLevel;

	const Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UpdateRate::NEVER);
	
	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory(), 0, imageSize, 0, &mappedData);
	std::memcpy(mappedData, pixels, imageSize);
	vkUnmapMemory(g_vulkanInstance->getDevice(), stagingBuffer.getBufferMemory());

	VkBufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyRegion.imageSubresource.mipLevel = mipLevel;
	copyRegion.imageSubresource.baseArrayLayer = 0;
	copyRegion.imageSubresource.layerCount = 1;

	copyRegion.imageOffset = { 0, 0, 0 };
	const VkExtent3D extent = { m_extent.width >> mipLevel, m_extent.height >> mipLevel, m_extent.depth };
	copyRegion.imageExtent = extent;

	copyGPUBuffer(stagingBuffer, copyRegion);
}

void Wolf::Image::copyGPUBuffer(const Buffer& bufferSrc, const VkBufferImageCopy& copyRegion)
{
	const CommandBuffer commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer(0);

	transitionImageLayout(commandBuffer.getCommandBuffer(0), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, copyRegion.imageSubresource.mipLevel, 1);
	vkCmdCopyBufferToImage(commandBuffer.getCommandBuffer(0), bufferSrc.getBuffer(), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,	1, &copyRegion);
	transitionImageLayout(commandBuffer.getCommandBuffer(0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, copyRegion.imageSubresource.mipLevel, 1);

	commandBuffer.endCommandBuffer(0);

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<VkSemaphore> signalSemaphores;
	Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();
}

void Wolf::Image::copyGPUImage(const Image& imageSrc, const VkImageCopy& imageCopy)
{
	const CommandBuffer commandBuffer(QueueType::TRANSFER, true);
	commandBuffer.beginCommandBuffer(0);
	
	transitionImageLayout(commandBuffer.getCommandBuffer(0), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, imageCopy.dstSubresource.mipLevel, 1);
	vkCmdCopyImage(commandBuffer.getCommandBuffer(0), imageSrc.getImage(), imageSrc.getImageLayout(imageCopy.srcSubresource.mipLevel), m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
	transitionImageLayout(commandBuffer.getCommandBuffer(0), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, imageCopy.dstSubresource.mipLevel, 1);

	commandBuffer.endCommandBuffer(0);

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<VkSemaphore> signalSemaphores;
	const Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();
}

void* Wolf::Image::map() const
{
	const VkDeviceSize imageSize = m_extent.width * m_extent.height * m_extent.depth * m_bbp;
	void* mappedData;
	vkMapMemory(g_vulkanInstance->getDevice(), m_imageMemory, 0, imageSize, 0, &mappedData);
	return mappedData;
}

void Wolf::Image::unmap() const
{
	vkUnmapMemory(g_vulkanInstance->getDevice(), m_imageMemory);
}

void Wolf::Image::getResourceLayout(VkSubresourceLayout& output) const
{
	VkImageSubresource subresource;
	subresource.aspectMask = m_aspectFlags;
	subresource.mipLevel = 0;
	subresource.arrayLayer = 0;
	vkGetImageSubresourceLayout(g_vulkanInstance->getDevice(), m_image, &subresource, &output);
}

void Wolf::Image::exportToFile(const std::string& filename) const
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
	stagingCreateImageInfo.extent = m_extent;
	stagingCreateImageInfo.format = m_imageFormat;
	stagingCreateImageInfo.arrayLayerCount = m_arrayLayerCount;
	stagingCreateImageInfo.aspect = m_aspectFlags;
	stagingCreateImageInfo.sampleCount = m_sampleCount;
	stagingCreateImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	stagingCreateImageInfo.memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	stagingCreateImageInfo.imageTiling = VK_IMAGE_TILING_LINEAR;
	stagingCreateImageInfo.mipLevelCount = 1;
	Image stagingBuffer(stagingCreateImageInfo);

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

	stagingBuffer.copyGPUImage(*this, copyRegion);

	if(m_imageFormat != VK_FORMAT_R32_SFLOAT)
	{
		Debug::sendError("Only VK_FORMAT_R32_SFLOAT can be exported for now, code needs to be updated");
		return;
	}

	struct ImageOutputPixel
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	const float* floatPixels = static_cast<float*>(stagingBuffer.map());
	std::vector<ImageOutputPixel> outputPixels(m_extent.width * m_extent.height);
	for(uint32_t i = 0, end = m_extent.width * m_extent.height; i < end; ++i)
	{
		outputPixels[i].r = static_cast<uint8_t>(floatPixels[i] * 255);
		outputPixels[i].g = 0;
		outputPixels[i].b = 0;
	}

	stbi_write_jpg(filename.c_str(), m_extent.width, m_extent.height, 3, outputPixels.data(), 100);

	stagingBuffer.unmap();
}

void Wolf::Image::setImageLayout(VkImageLayout dstLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstPipelineStageFlags)
{
	const CommandBuffer commandBuffer(QueueType::GRAPHIC, true);
	commandBuffer.beginCommandBuffer(0);

	transitionImageLayout(commandBuffer.getCommandBuffer(0), dstLayout, dstAccessMask, dstPipelineStageFlags);

	commandBuffer.endCommandBuffer(0);

	const std::vector<const Semaphore*> waitSemaphores;
	const std::vector<VkSemaphore> signalSemaphores;
	Fence fence(0);
	commandBuffer.submit(0, waitSemaphores, signalSemaphores, fence.getFence());
	fence.waitForFence();
}

uint32_t computeImageViewHash(VkFormat format)
{
	return (uint32_t)format;
}

void Wolf::Image::createImageView(VkFormat format)
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

	const uint32_t hash = computeImageViewHash(m_imageFormat);
	m_imageViews[hash] = imageView;
}

void Wolf::Image::setBBP()
{
	switch (m_imageFormat)
	{
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		m_bbp = 16;
		break;
	case VK_FORMAT_R32G32_SFLOAT:
		m_bbp = 8;
		break;
	case VK_FORMAT_R8G8B8A8_UNORM:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_SRGB:
	case VK_FORMAT_D32_SFLOAT:
	case VK_FORMAT_R32_SFLOAT:
		m_bbp = 4;
		break;
	case VK_FORMAT_BC3_UNORM_BLOCK:
		m_bbp = 1;
		break;
	default:
		m_bbp = 1;
		Debug::sendWarning("Unsupported image format");
		break;
	}
}

VkImageView Wolf::Image::getImageView(VkFormat format)
{
	const uint32_t hash = computeImageViewHash(format);
	if (m_imageViews.find(hash) == m_imageViews.end())
	{
		createImageView(format);
	}

	return m_imageViews[hash];
}

VkImageView Wolf::Image::getDefaultImageView()
{
	return getImageView(m_imageFormat);
}

void Wolf::Image::transitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout dstLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstPipelineStageFlags, uint32_t baseMipLevel, uint32_t levelCount)
{
	if (levelCount == MAX_MIP_COUNT)
		levelCount = m_mipLevelCount;

	// Sanity check
	{
		const VkImageLayout imageLayout = m_imageLayouts[baseMipLevel];
		for (uint32_t mipLevel = baseMipLevel; mipLevel < levelCount; mipLevel++)
		{
			if (m_imageLayouts[mipLevel] != imageLayout)
			{
				Debug::sendError("Image layout must be the same for all mip level asked for transition");
			}
		}
	}

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_imageLayouts[baseMipLevel];
	barrier.newLayout = dstLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = m_image;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = levelCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (bool depth = hasDepthComponent(m_imageFormat), stencil = hasStencilComponent(m_imageFormat); depth || stencil)
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
	barrier.dstAccessMask = dstAccessMask;

	vkCmdPipelineBarrier(commandBuffer, m_pipelineStageFlags, dstPipelineStageFlags, 0,	0, nullptr,	0, nullptr,	1, &barrier);

	m_accessFlags = dstAccessMask;
	for(uint32_t mipLevel = baseMipLevel; mipLevel < levelCount; mipLevel++)
		m_imageLayouts[mipLevel] = dstLayout;
	m_pipelineStageFlags = dstPipelineStageFlags;
}

void Wolf::Image::setImageLayoutWithoutOperation(VkImageLayout newImageLayout, uint32_t baseMipLevel, uint32_t levelCount)
{
	if (levelCount == MAX_MIP_COUNT)
		levelCount = m_mipLevelCount;

	for (uint32_t mipLevel = baseMipLevel; mipLevel < levelCount; mipLevel++)
		m_imageLayouts[mipLevel] = newImageLayout;
}
