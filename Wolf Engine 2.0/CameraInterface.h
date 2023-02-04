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
		
		virtual glm::mat4 getViewMatrix() = 0;
		virtual glm::mat4 getProjection() = 0;
	};
}
