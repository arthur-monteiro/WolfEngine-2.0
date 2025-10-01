#include "DebugMarker.h"

#include "Configuration.h"

void Wolf::DebugMarker::insert(const ResourceReference<CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerInsert({ name, color });
	}
}

void Wolf::DebugMarker::beginRegion(const ResourceReference<CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerBegin({ name, color });
	}
}

void Wolf::DebugMarker::endRegion(const ResourceReference<CommandBuffer>& commandBuffer)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerEnd();
	}
}
