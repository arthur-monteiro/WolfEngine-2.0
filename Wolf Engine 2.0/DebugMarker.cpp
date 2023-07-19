#include "DebugMarker.h"

#include "Configuration.h"

void Wolf::DebugMarker::insert(VkCommandBuffer commandBuffer, const float* color, const std::string& name)
{
	if (g_configuration->getUseRenderDoc())
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, color, sizeof(float) * 4);
		markerInfo.pMarkerName = name.c_str();
		vkCmdDebugMarkerInsertEXT(commandBuffer, &markerInfo);
	}
}

void Wolf::DebugMarker::beginRegion(VkCommandBuffer commandBuffer, const float* color, const std::string& name)
{
	if (g_configuration->getUseRenderDoc())
	{
		VkDebugMarkerMarkerInfoEXT markerInfo = {};
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = name.c_str();
		vkCmdDebugMarkerBeginEXT(commandBuffer, &markerInfo);
	}
}

void Wolf::DebugMarker::endRegion(VkCommandBuffer commandBuffer)
{
	if (g_configuration->getUseRenderDoc())
	{
		vkCmdDebugMarkerEndEXT(commandBuffer);
	}
}
