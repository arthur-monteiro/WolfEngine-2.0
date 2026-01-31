#pragma once

#ifdef WOLF_VULKAN

#define WOLF_VULKAN_1_1
#ifndef WOLF_VULKAN_FORCE_1_1
#define WOLF_VULKAN_1_2
#ifndef WOLF_VULKAN_FORCE_1_2
#define WOLF_VULKAN_1_3
#endif
#endif
#endif

#ifdef __ANDROID__
#include <android/native_window.h>
#endif

#include "Formats.h"

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
			(ANativeWindow* window)
#endif
		;
		virtual ~GraphicAPIManager() = default;

		virtual void waitIdle() const = 0;

		virtual bool isRayTracingAvailable() const = 0;
		virtual Format getDepthFormat() const = 0;
	};
}