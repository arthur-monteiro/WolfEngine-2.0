#pragma once

#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Wolf
{
	class CameraInterface
	{
	public:
		virtual void update(GLFWwindow* window) = 0;
		
		virtual glm::mat4 getViewMatrix() const = 0;
		virtual glm::mat4 getPreviousViewMatrix() const = 0;
		virtual glm::mat4 getProjection() const = 0;

		virtual float getNear() const = 0;
		virtual float getFar() const = 0;
		virtual float getFOV() const = 0;
		virtual glm::vec3 getPosition() const = 0;
		virtual glm::vec3 getOrientation() const = 0;
	};
}
