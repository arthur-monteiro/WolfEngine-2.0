#include "ShaderBindingTable.h"

#include <cstring>

#include <Debug.h>

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/BufferVulkan.h"
#include "../Private/Vulkan/PipelineVulkan.h"
#include "../Private/Vulkan/Vulkan.h"
#endif

Wolf::ShaderBindingTable::ShaderBindingTable(uint32_t shaderCount, const Pipeline& pipeline)
{
#ifdef WOLF_VULKAN
	const uint32_t groupHandleSize = g_vulkanInstance->getRayTracingProperties().shaderGroupHandleSize;
	m_baseAlignement = g_vulkanInstance->getRayTracingProperties().shaderGroupBaseAlignment;

	const uint32_t sbtSize = m_baseAlignement * shaderCount;
	m_buffer.reset(new BufferVulkan(sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

	// Generation
	const uint32_t groupCount = shaderCount;
	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	if (vkGetRayTracingShaderGroupHandlesKHR(g_vulkanInstance->getDevice(), static_cast<const PipelineVulkan&>(pipeline).getPipeline(), 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS)
	{
		Debug::sendError("Error in vkGetRayTracingShaderGroupHandlesKHR");
		return;
	}

	void* pData = m_buffer->map();

	uint8_t* data = static_cast<uint8_t*>(pData);
	int index = 0;
	for (uint32_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex)
	{
		memcpy(data, shaderHandleStorage.data() + index * groupHandleSize, groupHandleSize);
		data += m_baseAlignement;
		index++;
	}

	m_buffer->unmap();
#endif
}
