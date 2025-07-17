#include "DescriptorSetLayoutGenerator.h"

void Wolf::DescriptorSetLayoutGenerator::addUniformBuffer(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = DescriptorType::UNIFORM_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addCombinedImageSampler(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageBuffer(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = DescriptorType::STORAGE_BUFFER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addStorageImage(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = DescriptorType::STORAGE_IMAGE;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addSampler(ShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = DescriptorType::SAMPLER;

	m_descriptorLayouts.push_back(descriptorLayout);
}

void Wolf::DescriptorSetLayoutGenerator::addImages(DescriptorType descriptorType, ShaderStageFlags accessibility, uint32_t binding, uint32_t count, VkDescriptorBindingFlags bindingFlags)
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
	descriptorLayout.descriptorType = DescriptorType::ACCELERATION_STRUCTURE;

	m_descriptorLayouts.push_back(descriptorLayout);
}
