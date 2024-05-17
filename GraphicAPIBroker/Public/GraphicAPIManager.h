#pragma once

#ifdef WOLF_VULKAN
// START TEMP
#include <vulkan/vulkan.h>
// END TEMP

#define WOLF_VULKAN_1_1
#ifndef WOLF_VULKAN_FORCE_1_1
#define WOLF_VULKAN_1_2
#ifndef WOLF_VULKAN_FORCE_1_2
#define WOLF_VULKAN_1_3
#endif
#endif

#endif

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