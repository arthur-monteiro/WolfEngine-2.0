#pragma once

#include <string>

#include "CommandBuffer.h"

namespace Wolf
{
	class DebugMarker
	{
	public:
		inline static float renderPassDebugColor[4] = { 0.1f, 0.4f, 0.7f, 1.0f };
		inline static float computePassDebugColor[4] = { 0.65f, 0.65f, 0.7f, 1.0f };
		inline static float rayTracePassDebugColor[4] = { 0.75f, 0.6f, 0.6f, 1.0f };

		static void insert(VkCommandBuffer commandBuffer, const float* color, const std::string& name);
		static void beginRegion(VkCommandBuffer commandBuffer, const float* color, const std::string& name);
		static void endRegion(VkCommandBuffer commandBuffer);
	};

}
