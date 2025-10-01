#pragma once

#include <glm/glm.hpp>

#ifndef __ANDROID__
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

#include <Extents.h>

#include "ResourceNonOwner.h"

namespace Wolf
{
	class AABB;
	class DescriptorSet;
	class InputHandler;

	struct CameraUpdateContext
	{
#ifndef __ANDROID__
		const ResourceNonOwner<const InputHandler> inputHandler;
#endif
		uint32_t frameIdx = 0;
		Extent3D swapChainExtent = {};

#ifndef  __ANDROID__
		CameraUpdateContext(const ResourceNonOwner<const InputHandler>& inputHandler) : inputHandler(inputHandler) {}
#endif	
	};

	class CameraInterface
	{
	public:
		virtual void update(const CameraUpdateContext& context) = 0;
		virtual ~CameraInterface() = default;

		[[nodiscard]] virtual const glm::mat4& getViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getPreviousViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getProjectionMatrix() const = 0;

		[[nodiscard]] virtual float getNear() const = 0;
		[[nodiscard]] virtual float getFar() const = 0;
		[[nodiscard]] virtual float getFOV() const = 0;
		[[nodiscard]] virtual glm::vec3 getPosition() const = 0;
		[[nodiscard]] virtual glm::vec3 getOrientation() const = 0;

		[[nodiscard]] virtual const DescriptorSet* getDescriptorSet() const = 0;

		[[nodiscard]] virtual bool isAABBVisible(const AABB& aabb) const = 0;

		void setEnableJittering(bool value) { m_enableJittering = value; }

	protected:
		CameraInterface() = default;

		const glm::vec2 JITTER_OFFSET[16] = {
			glm::vec2(0.500000f, 0.333333f),
			glm::vec2(0.250000f, 0.666667),
			glm::vec2(0.750000f, 0.111111f),
			glm::vec2(0.125000f, 0.444444f),
			glm::vec2(0.625000f, 0.777778f),
			glm::vec2(0.375000f, 0.222222f),
			glm::vec2(0.875000f, 0.555556f),
			glm::vec2(0.062500f, 0.888889f),
			glm::vec2(0.562500f, 0.037037f),
			glm::vec2(0.312500f, 0.370370f),
			glm::vec2(0.812500f, 0.703704f),
			glm::vec2(0.187500f, 0.148148f),
			glm::vec2(0.687500f, 0.481481f),
			glm::vec2(0.437500f, 0.814815f),
			glm::vec2(0.937500f, 0.259259f),
			glm::vec2(0.031250f, 0.592593)
		};
		bool m_enableJittering = false;
	};
}
