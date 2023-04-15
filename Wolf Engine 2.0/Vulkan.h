#pragma once

#define GLFW_INCLUDE_VULKAN
#ifndef __ANDROID__
#include <GLFW/glfw3.h>
#include <OVR_CAPI_Vk.h>
#endif
#include <mutex>
#include <vector>

#include "CommandPool.h"
#include "Debug.h"
#include "DescriptorPool.h"
#include "HardwareCapabilities.h"
#include "QueueFamilyIndices.h"

namespace Wolf
{
	class Vulkan
	{
	public:
#ifndef __ANDROID__
		Vulkan(GLFWwindow* glfwWindowPtr, bool useOVR);
#else
		Vulkan(struct ANativeWindow* window);
#endif

		// Getters
		[[nodiscard]] VkDevice getDevice() const { return m_device; }
		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
		[[nodiscard]] VkSurfaceKHR getSurface() const { return m_surface; }
		[[nodiscard]] const QueueFamilyIndices& getQueueFamilyIndices() const { return m_queueFamilyIndices; }
		[[nodiscard]] VkQueue getPresentQueue() const { return m_presentQueue; }
		[[nodiscard]] const CommandPool* getGraphicsCommandPool() const { return m_graphicsCommandPool.get(); }
		[[nodiscard]] const CommandPool* getGraphicsTransientCommandPool() const { return m_graphicsTransientCommandPool.get(); }
		[[nodiscard]] const CommandPool* getComputeCommandPool() const { return m_computeCommandPool.get(); }
		[[nodiscard]] const CommandPool* getComputeTransientCommandPool() const { return m_computeTransientCommandPool.get(); }
		[[nodiscard]] VkQueue getGraphicsQueue() const { return m_graphicsQueue; }
		[[nodiscard]] VkQueue getComputeQueue() const { return m_computeQueue; }
		[[nodiscard]] VkDescriptorPool getDescriptorPool() const { return m_descriptorPool->getDescriptorPool(); }
		[[nodiscard]] const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& getRayTracingProperties() const { return m_raytracingProperties; }
		[[nodiscard]] const HardwareCapabilities& getHardwareCapabilities() const { return m_hardwareCapabilities; }

	private:
		/* Main Loading Functions */
		void createInstance();
		void setupDebugMessenger();
		void pickPhysicalDevice();
		void createDevice();
		void createCommandPools();

		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		bool isDeviceSuitable(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions, HardwareCapabilities& outHardwareCapabilities) const;
		void getPhysicalDeviceRayTracingProperties(VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracingProperties) const;

	private:
		VkInstance m_instance;
		VkSurfaceKHR m_surface;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device;

		/* Queues */
		QueueFamilyIndices m_queueFamilyIndices;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;
		VkQueue m_computeQueue;
		std::mutex* m_mutexQueues = nullptr;

		/* Extensions / Layers */
		std::vector<const char*> m_validationLayers;
		std::vector<const char*> m_deviceExtensions;
		VkDebugUtilsMessengerEXT m_debugMessenger;

		/* Ray Tracing */
		std::vector<const char*> m_raytracingDeviceExtensions;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_raytracingProperties = {};

		/* Mesh Shader */
		std::vector<const char*> m_meshShaderDeviceExtensions;
		VkPhysicalDeviceMeshShaderPropertiesNV m_meshShaderProperties = {};

		/* Properties */
		VkSampleCountFlagBits m_maxMsaaSamples = VK_SAMPLE_COUNT_1_BIT;
		HardwareCapabilities m_hardwareCapabilities;
		VkPhysicalDeviceConservativeRasterizationPropertiesEXT m_conservativeRasterProps{};

		/* VR */
#ifndef __ANDROID__
		ovrSession                  m_session = nullptr;
		ovrGraphicsLuid             m_luid;
#endif

		/* Command Pools */
		std::unique_ptr<CommandPool> m_graphicsCommandPool;
		std::unique_ptr<CommandPool> m_graphicsTransientCommandPool;
		std::unique_ptr<CommandPool> m_computeCommandPool;
		std::unique_ptr<CommandPool> m_computeTransientCommandPool;

		/* Descritor Pools */
		std::unique_ptr<DescriptorPool> m_descriptorPool;
	};

	extern Vulkan* g_vulkanInstance;
}