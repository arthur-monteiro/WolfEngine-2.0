#pragma once

// START TEMP
#ifdef WOLF_USE_VULKAN
#include <vulkan/vulkan.h>
#endif
// END TEMP

struct GLFWwindow;

namespace Wolf
{
	class GraphicAPIManager
	{
	public:
		static GraphicAPIManager* instanciateGraphicAPIManager
#ifndef __ANDROID__
			(GLFWwindow* glfwWindowPtr, bool useOVR)
#else
			(struct ANativeWindow* window)
#endif
		;
		virtual ~GraphicAPIManager() = default;

		virtual void waitIdle() const = 0;

		virtual bool isRayTracingAvailable() const = 0;
		virtual VkFormat getDepthFormat() const = 0;
	};
}