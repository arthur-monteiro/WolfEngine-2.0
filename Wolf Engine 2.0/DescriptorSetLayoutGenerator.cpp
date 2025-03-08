#include "DescriptorSetLayoutGenerator.h"

void Wolf::DescriptorSetLayoutGenerator::addUniformBuffer(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addCombinedImageSampler(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageBuffer(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageImage(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addSampler(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addImages(VkDescriptorType descriptorType, ShaderStageFlags accessibility, uint32_t binding, uint32_t count, VkDescriptorBindingFlags bindingFlags)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = descriptorType;
	descriptorLayout.count = count;
	descriptorLayout.bindingFlags = bindingFlags;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addAccelerationStructure(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	m_descriptorLayouts.push_back(descriptorLayout);
}
