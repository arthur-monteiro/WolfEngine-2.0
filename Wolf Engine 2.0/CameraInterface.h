#pragma once

#include <glm/glm.hpp>

#ifndef __ANDROID__
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif

namespace Wolf
{
	class CameraInterface
	{
	public:
#ifndef __ANDROID__
		virtual void update(GLFWwindow* window) = 0;
#else
        virtual void update() = 0;
#endif

		[[nodiscard]] virtual const glm::mat4& getViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getPreviousViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getProjectionMatrix() const = 0;

		[[nodiscard]] virtual float getNear() const = 0;
		[[nodiscard]] virtual float getFar() const = 0;
		[[nodiscard]] virtual float getFOV() const = 0;
		[[nodiscard]] virtual glm::vec3 getPosition() const = 0;
		[[nodiscard]] virtual glm::vec3 getOrientation() const = 0;
	};
}
