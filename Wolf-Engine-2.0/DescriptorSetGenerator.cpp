#include "DescriptorSetGenerator.h"

#include <Configuration.h>
#include <Debug.h>

#include "UniformBuffer.h"

Wolf::DescriptorSetGenerator::DescriptorSetGenerator(const std::span<const DescriptorLayout> descriptorLayouts)
{
	for (const DescriptorLayout& descriptorLayout : descriptorLayouts)
	{
		if (descriptorLayout.descriptorType == DescriptorType::UNIFORM_BUFFER || descriptorLayout.descriptorType == DescriptorType::STORAGE_BUFFER)
		{
			DescriptorSetUpdateInfo::DescriptorBuffer descriptorBuffer;
			descriptorBuffer.descriptorLayout = descriptorLayout;

			m_descriptorSetCreateInfo.descriptorBuffers.push_back(descriptorBuffer);
			m_mapBindingCreateInfo[descriptorLayout.binding] = { descriptorLayout.descriptorType, static_cast<uint32_t>(m_descriptorSetCreateInfo.descriptorBuffers.size()) - 1 };
		}
		else if (descriptorLayout.descriptorType == DescriptorType::COMBINED_IMAGE_SAMPLER || descriptorLayout.descriptorType == DescriptorType::STORAGE_IMAGE ||
				 descriptorLayout.descriptorType == DescriptorType::SAMPLER || descriptorLayout.descriptorType == DescriptorType::SAMPLED_IMAGE)
		{
			DescriptorSetUpdateInfo::DescriptorImage descriptorImage;
			descriptorImage.descriptorLayout = descriptorLayout;

			m_descriptorSetCreateInfo.descriptorImages.push_back(descriptorImage);
			m_mapBindingCreateInfo[descriptorLayout.binding] = { descriptorLayout.descriptorType, static_cast<uint32_t>(m_descriptorSetCreateInfo.descriptorImages.size()) - 1 };
		}
		else if (descriptorLayout.descriptorType == DescriptorType::ACCELERATION_STRUCTURE)
		{
			DescriptorSetUpdateInfo::DescriptorAccelerationStructure descriptorAccelerationStructure;
			descriptorAccelerationStructure.descriptorLayout = descriptorLayout;

			m_descriptorSetCreateInfo.descriptorAccelerationStructures.push_back(descriptorAccelerationStructure);
			m_mapBindingCreateInfo[descriptorLayout.binding] = { descriptorLayout.descriptorType, static_cast<uint32_t>(m_descriptorSetCreateInfo.descriptorAccelerationStructures.size()) - 1 };
		}
		else
		{
			Debug::sendCriticalError("Unhandled case for descriptor layouts");
		}
	}
}

void Wolf::DescriptorSetGenerator::setBuffer(uint32_t binding, const Buffer& buffer)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::STORAGE_BUFFER)
		Debug::sendError("Binding provided is not a buffer");

	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers.resize(1);
	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers[0] = &buffer;
}

void Wolf::DescriptorSetGenerator::setBuffers(uint32_t binding, const std::vector<ResourceNonOwner<Buffer>>& buffers)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::STORAGE_BUFFER)
		Debug::sendError("Binding provided is not a buffer");

	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers.reserve(buffers.size());
	for (const ResourceNonOwner<Buffer>& buffer : buffers)
	{
		m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers.push_back(&(*buffer));
	}
}

void Wolf::DescriptorSetGenerator::setUniformBuffer(uint32_t binding, const UniformBuffer& buffer)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::UNIFORM_BUFFER)
		Debug::sendError("Binding provided is not a uniform buffer");

	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].isForMultipleSets = true;
	m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers.resize(g_configuration->getMaxCachedFrames());
	for (uint32_t i = 0; i < g_configuration->getMaxCachedFrames(); ++i)
	{
		m_descriptorSetCreateInfo.descriptorBuffers[descriptor.second].buffers[i] = &buffer.getBuffer(i);
	}
}

void Wolf::DescriptorSetGenerator::setCombinedImageSampler(uint32_t binding, VkImageLayout imageLayout, ImageView imageView, const Sampler& sampler)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::COMBINED_IMAGE_SAMPLER)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetUpdateInfo::ImageData imageData;
	imageData.imageLayout = imageLayout;
	imageData.imageView = imageView;
	imageData.sampler = &sampler;

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = imageData;
}

void Wolf::DescriptorSetGenerator::setImage(uint32_t binding, const ImageDescription& imageDescription)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::STORAGE_IMAGE && descriptor.first != DescriptorType::SAMPLED_IMAGE)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetUpdateInfo::ImageData imageData;
	imageData.imageLayout = imageDescription.imageLayout;
	imageData.imageView = imageDescription.imageView;
	imageData.sampler = VK_NULL_HANDLE;

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = imageData;
}

void Wolf::DescriptorSetGenerator::setImages(uint32_t binding, const std::vector<ImageDescription>& imageDescriptions)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::STORAGE_IMAGE && descriptor.first != DescriptorType::SAMPLED_IMAGE)
		Debug::sendError("Binding provided is not an image");

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.reserve(imageDescriptions.size());
	for (const ImageDescription& imageDescription : imageDescriptions)
	{
		DescriptorSetUpdateInfo::ImageData imageData;
		imageData.imageLayout = imageDescription.imageLayout;
		imageData.imageView = imageDescription.imageView;
		imageData.sampler = VK_NULL_HANDLE;

		m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.push_back(imageData);
	}
}

void Wolf::DescriptorSetGenerator::setSampler(uint32_t binding, const Sampler& sampler)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::SAMPLER)
		Debug::sendError("Binding provided is not an image");

	DescriptorSetUpdateInfo::ImageData imageData;
	imageData.sampler = &sampler;

	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images.resize(1);
	m_descriptorSetCreateInfo.descriptorImages[descriptor.second].images[0] = imageData;
}

void Wolf::DescriptorSetGenerator::setAccelerationStructure(uint32_t binding, const TopLevelAccelerationStructure& accelerationStructure)
{
	const std::pair<DescriptorType, uint32_t /* descriptor index */>& descriptor = m_mapBindingCreateInfo[binding];

	if (descriptor.first != DescriptorType::ACCELERATION_STRUCTURE)
		Debug::sendError("Binding provided is not an acceleration structure");

	m_descriptorSetCreateInfo.descriptorAccelerationStructures[descriptor.second].accelerationStructure = &accelerationStructure;
}
