#include "SamplerVulkan.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::SamplerVulkan::SamplerVulkan(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy, float minLod, float mipLodBias)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;

	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;

	samplerInfo.anisotropyEnable = maxAnisotropy > 0.0f;
	samplerInfo.maxAnisotropy = maxAnisotropy;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	if (mipLevels > 1.0f)
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = mipLodBias;
		samplerInfo.minLod = minLod;
		samplerInfo.maxLod = mipLevels;
	}
	else
	{
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.mipLodBias = 0;
		samplerInfo.minLod = 0;
		samplerInfo.maxLod = 1;
	}

	samplerInfo.pNext = VK_NULL_HANDLE;

	if (vkCreateSampler(g_vulkanInstance->getDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
		Debug::sendError("Sampler creation");
}

Wolf::SamplerVulkan::~SamplerVulkan()
{
	vkDestroySampler(g_vulkanInstance->getDevice(), m_sampler, nullptr);
}
