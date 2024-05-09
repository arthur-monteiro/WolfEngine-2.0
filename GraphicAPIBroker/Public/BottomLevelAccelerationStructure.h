#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include <span>
// TEMP
#include <vulkan/vulkan_core.h>

namespace Wolf
{
	class Buffer;

	struct GeometryInfo
	{
		struct MeshInfo
		{
			const Buffer* vertexBuffer;
			uint32_t vertexCount;
			uint32_t vertexSize;
			VkFormat vertexFormat;

			const Buffer* indexBuffer;
			uint32_t indexCount;
		};
		MeshInfo mesh;

		const Buffer* transformBuffer = nullptr;
		uint64_t transformOffsetInBytes = 0;
	};

	struct BottomLevelAccelerationStructureCreateInfo
	{
		std::span<GeometryInfo> geometryInfos;
		uint32_t buildFlags = 0;
	};

	class BottomLevelAccelerationStructure
	{
	public:
		static BottomLevelAccelerationStructure* createBottomLevelAccelerationStructure(const BottomLevelAccelerationStructureCreateInfo& createInfo);

		virtual ~BottomLevelAccelerationStructure() = default;
	};
}

#endif