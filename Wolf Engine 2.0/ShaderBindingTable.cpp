#include "ShaderBindingTable.h"

#include "Vulkan.h"

Wolf::ShaderBindingTable::ShaderBindingTable(uint32_t shaderCount, VkPipeline pipeline)
{
	uint32_t groupHandleSize = g_vulkanInstance->getRayTracingProperties().shaderGroupHandleSize;
	m_baseAlignement = g_vulkanInstance->getRayTracingProperties().shaderGroupBaseAlignment;

	uint32_t sbtSize = m_baseAlignement * shaderCount;
	m_buffer.reset(new Buffer(sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, UpdateRate::NEVER));

	// Generation
	uint32_t groupCount = shaderCount;
	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	if (vkGetRayTracingShaderGroupHandlesKHR(g_vulkanInstance->getDevice(), pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS)
	{
		Debug::sendError("Error in vkGetRayTracingShaderGroupHandlesKHR");
		return;
	}

	void* pData;
	m_buffer->map(&pData);

	uint8_t* data = static_cast<uint8_t*>(pData);
	int index = 0;
	for (uint32_t shaderIndex = 0; shaderIndex < shaderCount; ++shaderIndex)
	{
		memcpy(data, shaderHandleStorage.data() + index * groupHandleSize, groupHandleSize);
		data += m_baseAlignement;
		index++;
	}

	m_buffer->unmap();
}
