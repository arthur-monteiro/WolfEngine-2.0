#include "DescriptorSetGenerator.h"

#include "Vulkan.h"

Wolf::DescriptorSetGenerator::DescriptorSetGenerator(const std::span<const DescriptorLayout> descriptorLayouts)
{
	for (const DescriptorLayout& descriptorLayout : descriptorLayouts)
	{
		if (descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			DescriptorSetCreateInfo::DescriptorBuffer descriptorBuffer;
			descriptorBuffer.descriptorLayout = descriptorLayout;

			m_descriptorSetCreateInfo.descriptorBuffers.push_back(descriptorBuffer);
			m_mapBindingCreateInfo[descriptorLayout.binding] = { DescriptorType::BUFFER, m_descriptorSetCreateInfo.descriptorBuffers.size() - 1 };
		}
		else if (descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
			descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER || descriptorLayout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			DescriptorSetCreateInfo::DescriptorImage descriptorImage;
			descriptorImage.descriptorLayout = descriptorLayout;

			m_descriptorSetCreateInfo.descriptorImages.push_back(descriptorImage);
			m_mapBindingCreateInfo[descriptorLayout.binding] = { DescriptorType::IMAGE, m_descriptorSetCreateInfo.descriptorImages.size() - 1 };
		}
	}
}

void Wolf::DescriptorSetGenerator::setBuffer(uint32_t binding, const Buffer& buffer)
{
	std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::BUFFER)
		Debug::sendError("Binding provided is not a buffer");

	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers.resize(1);
	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers[0] = &buffer;
}

void Wolf::DescriptorSetGenerator::setCombinedImageSampler(uint32_t binding, VkImageLayout imageLayout, VkImageView imageView, const Sampler& sampler)
{
	std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::IMAGE)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetCreateInfo::ImageData imageData;
	imageData.imageLayout = imageLayout;
	imageData.imageView = imageView;
	imageData.sampler = sampler.getSampler();

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = std::move(imageData);
}

void Wolf::DescriptorSetGenerator::setImage(uint32_t binding, ImageDescription& imageDescription)
{
	std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::IMAGE)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetCreateInfo::ImageData imageData;
	imageData.imageLayout = imageDescription.imageLayout;
	imageData.imageView = imageDescription.imageView;
	imageData.sampler = VK_NULL_HANDLE;

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = std::move(imageData);
}

void Wolf::DescriptorSetGenerator::setImages(uint32_t binding, std::vector<ImageDescription>& imageDescriptions)
{
	std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::IMAGE)
		Debug::sendError("Binding provided is not an image");

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.reserve(imageDescriptions.size());
	for (ImageDescription& imageDescription : imageDescriptions)
	{
		DescriptorSetCreateInfo::ImageData imageData;
		imageData.imageLayout = imageDescription.imageLayout;
		imageData.imageView = imageDescription.imageView;
		imageData.sampler = VK_NULL_HANDLE;

		m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.push_back(std::move(imageData));
	}
}

void Wolf::DescriptorSetGenerator::setSampler(uint32_t binding, const Sampler& sampler)
{
	std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::IMAGE)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetCreateInfo::ImageData imageData;
	imageData.sampler = sampler.getSampler();

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = std::move(imageData);
}
