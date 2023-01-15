#include "DescriptorSetLayoutGenerator.h"

void Wolf::DescriptorSetLayoutGenerator::addUniformBuffer(VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addCombinedImageSampler(VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageBuffer(VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageImage(VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	m_descriptorLayouts.push_back(descriptorLayout);
}
