#include "GraphicAPIManager.h"

#ifdef WOLF_VULKAN
#include "../Private/Vulkan/Vulkan.h"
#endif

Wolf::GraphicAPIManager* Wolf::GraphicAPIManager::instanciateGraphicAPIManager
#ifndef __ANDROID__
	(GLFWwindow* glfwWindowPtr, bool useOVR)
#else
	(ANativeWindow* window)
#endif
{
#ifdef WOLF_VULKAN
#ifndef __ANDROID__
	return new Vulkan(glfwWindowPtr, useOVR);
#else
	return new Vulkan(window);
#endif
#else
	return nullptr;
#endif
}
