#include "DescriptorSetLayout.h"

#include <vector>

#include "Debug.h"
#include "Vulkan.h"

Wolf::DescriptorSetLayout::DescriptorSetLayout(const std::span<const DescriptorLayout> descriptorLayouts, VkDescriptorSetLayoutCreateFlags flags)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorBindingFlags> bindingFlags;

	for (const auto descriptorLayout : descriptorLayouts)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.binding;
		descriptorSetLayoutBinding.descriptorType = descriptorLayout.descriptorType;
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.count;
		descriptorSetLayoutBinding.stageFlags = descriptorLayout.accessibility;
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

Wolf::DescriptorSetLayout::~DescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(g_vulkanInstance->getDevice(), m_descriptorSetLayout, nullptr);
}
