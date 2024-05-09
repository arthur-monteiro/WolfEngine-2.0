#pragma once

#include <glm/vec3.hpp>
#include <string>

namespace Wolf
{
	struct ColorFloat
	{
		float components[4];
	};

	struct DebugMarkerInfo
	{
		std::string name;
		ColorFloat  color;
	};

	struct Viewport
	{
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	struct ImageCopyInfo
	{
		uint32_t   srcMipLevel;
		uint32_t   srcBaseArrayLayer;
		uint32_t   srcLayerCount;
		glm::ivec3 srcOffset;

		uint32_t   dstMipLevel;
		uint32_t   dstBaseArrayLayer;
		uint32_t   dstLayerCount;
		glm::ivec3 dstOffset;

		glm::uvec3 extent;
	};
}
