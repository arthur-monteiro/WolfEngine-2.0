#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "BottomLevelAccelerationStructure.h"

namespace Wolf
{
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
		static TopLevelAccelerationStructure* createTopLevelAccelerationStructure(std::span<BLASInstance> blasInstances);

		virtual ~TopLevelAccelerationStructure() = default;
	};
}

