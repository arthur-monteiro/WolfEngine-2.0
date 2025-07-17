#include "DescriptorSetLayoutVulkan.h"

#include <vector>

#include <Debug.h>

#include "ShaderStagesVulkan.h"
#include "Vulkan.h"

VkDescriptorType Wolf::wolfDescriptorTypeToVkDescriptorType(DescriptorType descriptorLayout)
{
	switch (descriptorLayout)
	{
		case DescriptorType::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case DescriptorType::UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case DescriptorType::STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case DescriptorType::STORAGE_IMAGE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case DescriptorType::SAMPLED_IMAGE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case DescriptorType::COMBINED_IMAGE_SAMPLER:
			return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case DescriptorType::ACCELERATION_STRUCTURE:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		default:
			Debug::sendCriticalError("Unhandled descriptor layout");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

Wolf::DescriptorSetLayoutVulkan::DescriptorSetLayoutVulkan(const std::span<const DescriptorLayout> descriptorLayouts, VkDescriptorSetLayoutCreateFlags flags)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorBindingFlags> bindingFlags;

	for (const auto descriptorLayout : descriptorLayouts)
	{
		if (descriptorLayout.descriptorType == DescriptorType::UNIFORM_BUFFER)
		{
			m_containsUniformBuffer = true;
		}

		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.binding;
		descriptorSetLayoutBinding.descriptorType = wolfDescriptorTypeToVkDescriptorType(descriptorLayout.descriptorType);
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.count;
		descriptorSetLayoutBinding.stageFlags = wolfStageFlagsToVkStageFlags(descriptorLayout.accessibility);
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(descriptorSetLayoutBinding);

		bindingFlags.push_back(descriptorLayout.bindingFlags);
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{};
	bindingFlagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	bindingFlagsCreateInfo.pNext = nullptr;
	bindingFlagsCreateInfo.pBindingFlags = bindingFlags.data();
	bindingFlagsCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();
	layoutInfo.flags = flags;
	layoutInfo.pNext = &bindingFlagsCreateInfo;

	if (vkCreateDescriptorSetLayout(g_vulkanInstance->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		Debug::sendError("Error : create descriptor set layout");
}

Wolf::DescriptorSetLayoutVulkan::~DescriptorSetLayoutVulkan()
{
	vkDestroyDescriptorSetLayout(g_vulkanInstance->getDevice(), m_descriptorSetLayout, nullptr);
}
