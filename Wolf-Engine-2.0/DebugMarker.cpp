#include "DebugMarker.h"

#include "Configuration.h"

void Wolf::DebugMarker::insert(const ResourceReference<const CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerInsert({ name, color });
	}
}

void Wolf::DebugMarker::beginRegion(const ResourceReference<const CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerBegin({ name, color });
	}
}

void Wolf::DebugMarker::endRegion(const ResourceReference<const CommandBuffer>& commandBuffer)
{
	if (g_configuration->getEnableGPUDebugMarkers())
	{
		commandBuffer->debugMarkerEnd();
	}
}
