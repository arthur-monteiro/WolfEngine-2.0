#pragma once

#include <string>

#include "CommandBuffer.h"

namespace Wolf
{
	class DebugMarker
	{
	public:
		inline static ColorFloat renderPassDebugColor = { 0.1f, 0.4f, 0.7f, 1.0f };
		inline static ColorFloat computePassDebugColor = { 0.65f, 0.65f, 0.7f, 1.0f };
		inline static ColorFloat rayTracePassDebugColor = { 0.75f, 0.6f, 0.6f, 1.0f };

		static void insert(const ResourceReference<CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name);
		static void beginRegion(const ResourceReference<CommandBuffer>& commandBuffer, const ColorFloat& color, const std::string& name);
		static void endRegion(const ResourceReference<CommandBuffer>& commandBuffer);
	};

}
