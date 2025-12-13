#pragma once

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 30

#include <glm/glm.hpp>

#include "BottomLevelAccelerationStructure.h"

namespace Wolf
{
	class CommandBuffer;

	struct BLASInstance
	{
		const BottomLevelAccelerationStructure* bottomLevelAS;
		glm::mat4 transform;
		uint32_t instanceID; // Instance ID visible in the shader
		uint32_t hitGroupIndex; // Hit group index used to fetch the shaders from the SBT
	};

	class TopLevelAccelerationStructure
	{
	public:
		static TopLevelAccelerationStructure* createTopLevelAccelerationStructure(uint32_t maxInstanceCount);

		virtual void build(const CommandBuffer* commandBuffer, std::span<BLASInstance> blasInstances) = 0;
		virtual void recordBuildBarriers(const CommandBuffer* commandBuffer) = 0;
		virtual uint32_t getInstanceCount() const = 0;

		virtual ~TopLevelAccelerationStructure() = default;
	};
}

#endif