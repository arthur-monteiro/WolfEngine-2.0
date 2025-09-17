#include "Vulkan.h"

#include <set>

#include "Configuration.h"
#include "Debug.h"
#include "ShadingRateSymbols.h"
#include "SwapChainSupportDetails.h"
#include "VulkanHelper.h"

const Wolf::Vulkan* Wolf::g_vulkanInstance = nullptr;

#ifndef __ANDROID__
void registerGlobalDeviceForDebugMarker(VkDevice device);
#endif

#ifdef __ANDROID__
Wolf::Vulkan::Vulkan(struct ANativeWindow* window)
#else
Wolf::Vulkan::Vulkan(GLFWwindow* glfwWindowPtr, bool useOVR)
#endif
{
	if (g_vulkanInstance)
	{
		Debug::sendCriticalError("Can't instanciate Vulkan twice");
	}
	g_vulkanInstance = this;

#ifndef __ANDROID__
	if(useOVR)
	{
		// constexpr ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
		// ovrResult result = ovr_Initialize(&initParams);
		// if (!OVR_SUCCESS(result))
		// {
		// 	Debug::sendCriticalError("Failed to initialize OVR");
		// 	return;
		// }
		//
		// result = ovr_Create(&m_session, &m_luid);
		// if (!OVR_SUCCESS(result))
		// {
		// 	Debug::sendCriticalError("Failed to create OVR");
		// 	return;
		// }
	}
#endif

	const bool useVIL = g_configuration->getUseVIL();
	const bool useDebugMarkers = g_configuration->getEnableGPUDebugMarkers();

//#ifndef NDEBUG
	if(useVIL)
		m_validationLayers = { "VK_LAYER_live_introspection" };

	m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");

//#endif
	createInstance();

	setupDebugMessenger();
#ifndef __ANDROID__

	if (glfwCreateWindowSurface(m_instance, glfwWindowPtr, nullptr, &m_surface) != VK_SUCCESS)
	{
		Debug::sendCriticalError("Error : window surface creation");
	}
#else
	const VkAndroidSurfaceCreateInfoKHR create_info
	{
			.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.window = window
	};

	vkCreateAndroidSurfaceKHR(m_instance, &create_info, nullptr /* pAllocator */, &m_surface);
#endif

#ifndef __ANDROID__
	m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, "VK_KHR_external_memory_win32", VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		"VK_KHR_external_semaphore_win32", VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, "VK_KHR_external_fence", "VK_KHR_external_fence_win32", "VK_KHR_buffer_device_address" };
	if (!useVIL)
	{
		m_raytracingDeviceExtensions = { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME,  };
	}
	m_shadingRateDeviceExtensions = { VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME };
	if(useDebugMarkers)
	{
		m_deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	}
	//m_meshShaderDeviceExtensions = { VK_NV_MESH_SHADER_EXTENSION_NAME };
#else
    m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, "VK_KHR_buffer_device_address" };
#endif

	pickPhysicalDevice();
	createDevice();

	initializeShadingRateFunctions(m_device);

#ifndef __ANDROID__
	if(useDebugMarkers)
	{
		registerGlobalDeviceForDebugMarker(m_device);
	}
#endif

	createCommandPools();

	m_descriptorPool.reset(new DescriptorPool(m_device));
	m_depthFormat = findDepthFormat(m_physicalDevice);
}

void Wolf::Vulkan::waitIdle() const
{
	vkDeviceWaitIdle(m_device);
}

std::vector<const char*> getRequiredExtensions()
{
	std::vector<const char*> extensions;

#ifndef __ANDROID__
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
		extensions.push_back(glfwExtensions[i]);
#else
	extensions.push_back("VK_KHR_surface");
	extensions.push_back("VK_KHR_android_surface");
	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

//#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif

	return extensions;
}

Wolf::Format Wolf::Vulkan::getDepthFormat() const
{
	return m_depthFormat;
}

void Wolf::Vulkan::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "App Name";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Wolf Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const std::vector<const char*> extensions = getRequiredExtensions();
//	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
//	extensions.push_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();

	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = &debugCreateInfo;

	//createInfo.enabledLayerCount = 0;
	//createInfo.pNext = nullptr;

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (result != VK_SUCCESS)
		Debug::sendCriticalError("Error: instance creation");
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	const auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Wolf::Vulkan::setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
	{
		Debug::sendCriticalError("Failed to set up debug messenger!");
	}
}

void findQueueFamilies(Wolf::QueueFamilyIndices& indices, VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.graphicsFamily = i;

		if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) // check compute queue dedicated
			indices.computeFamily = i;

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 && presentSupport)
			indices.presentFamily = i;

		if (indices.isComplete())
			break;

		i++;
	}

	if (indices.computeFamily < 0) // compute family not dedicated => probably the same than graphics
	{
		i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
				indices.computeFamily = i;

			if (indices.isComplete())
				break;

			i++;
		}
	}
}


bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& deviceExtensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	const VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

#ifndef __ANDROID__
VkPhysicalDeviceMeshShaderPropertiesNV getPhysicalDeviceMeshShaderProperties(VkPhysicalDevice physicalDevice)
{

	VkPhysicalDeviceMeshShaderPropertiesNV meshShaderProperties{};

	meshShaderProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_NV;
	meshShaderProperties.pNext = nullptr;
	VkPhysicalDeviceProperties2 props;
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &meshShaderProperties;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(physicalDevice, &props);

	return meshShaderProperties;
}
#endif

void Wolf::Vulkan::pickPhysicalDevice()
{
#ifndef __ANDROID__
	// if (m_session)
	// {
	// 	const ovrResult result = ovr_GetSessionPhysicalDeviceVk(m_session, m_luid, m_instance, &m_physicalDevice);
	// 	if (!OVR_SUCCESS(result))
	// 	{
	// 		Debug::sendCriticalError("Failed to get physical device from OVR");
	// 	}
	// 	return;
	// }
#endif

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		Debug::sendCriticalError("Error : No GPU with Vulkan support found !");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			if (m_availableFeatures.rayTracing)
			{
				for (auto rayTracingDeviceExtension : m_raytracingDeviceExtensions)
					m_deviceExtensions.push_back(rayTracingDeviceExtension);
			}
			if (m_availableFeatures.variableShadingRate)
			{
				for (auto shadingRateExtension : m_shadingRateDeviceExtensions)
					m_deviceExtensions.push_back(shadingRateExtension);
			}

			m_physicalDevice = device;
			m_maxMsaaSamples = getMaxUsableSampleCount(m_physicalDevice);

			if (m_availableFeatures.rayTracing)
				retrievePhysicalDeviceRayTracingProperties();
			//if (m_availableFeatures.meshShader)
			//	m_meshShaderProperties = getPhysicalDeviceMeshShaderProperties(m_physicalDevice);
			if (m_availableFeatures.variableShadingRate)
				retrievePhysicalDeviceShadingRateProperties();
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
		Debug::sendCriticalError("Error : No suitable GPU found");
}

void Wolf::Vulkan::createDevice()
{
	findQueueFamilies(m_queueFamilyIndices, m_physicalDevice, m_surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set uniqueQueueFamilies = { m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.presentFamily, m_queueFamilyIndices.computeFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceVulkan11Features features11{};
	features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	VkPhysicalDeviceVulkan12Features features12{};
	features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features12.bufferDeviceAddress = VK_TRUE;
	features12.pNext = &features11;

	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
	rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rayTracingPipelineFeatures.rayTracingPipeline = true;
	rayTracingPipelineFeatures.pNext = &features12;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeature{};
	accelerationStructureFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelerationStructureFeature.accelerationStructure = true;
	accelerationStructureFeature.pNext = &rayTracingPipelineFeatures;

	VkPhysicalDeviceFragmentShadingRateFeaturesKHR variableShadingRateFeatures{};
	variableShadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
	variableShadingRateFeatures.pipelineFragmentShadingRate = true;
	variableShadingRateFeatures.attachmentFragmentShadingRate = true;
	variableShadingRateFeatures.pNext = &accelerationStructureFeature;

	VkPhysicalDeviceFeatures2 supportedFeatures = {};
	supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	supportedFeatures.pNext = &variableShadingRateFeatures;
	supportedFeatures.features.shaderStorageImageMultisample = VK_TRUE;
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures);

	if (features12.bufferDeviceAddress == VK_FALSE)
	{
		Debug::sendWarning("bufferDeviceAddress not supported");
	}

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = nullptr; // &(supportedFeatures.features);
	createInfo.pNext = &supportedFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

//#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();
//#else
//	createInfo.enabledLayerCount = 0;
//#endif

	if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
		Debug::sendCriticalError("Error : create device");

	vkGetDeviceQueue(m_device, m_queueFamilyIndices.graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.presentFamily, 0, &m_presentQueue);
	vkGetDeviceQueue(m_device, m_queueFamilyIndices.computeFamily, 0, &m_computeQueue);

	m_mutexQueues = new std::mutex();
}

void Wolf::Vulkan::createCommandPools()
{
	m_graphicsCommandPool.reset(new CommandPool(m_device, m_queueFamilyIndices.graphicsFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
	m_graphicsTransientCommandPool.reset(new CommandPool(m_device, m_queueFamilyIndices.graphicsFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
	m_computeCommandPool.reset(new CommandPool(m_device, m_queueFamilyIndices.computeFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));
	m_computeTransientCommandPool.reset(new CommandPool(m_device, m_queueFamilyIndices.computeFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT));
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	Wolf::Debug::Severity severity = Wolf::Debug::Severity::INFO;
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		severity = Wolf::Debug::Severity::VERBOSE;
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		severity = Wolf::Debug::Severity::WARNING;
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		severity = Wolf::Debug::Severity::ERROR;
	Wolf::Debug::sendVulkanMessage(pCallbackData->pMessage, severity);

	return VK_FALSE;
}

void Wolf::Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

bool Wolf::Vulkan::isDeviceSuitable(VkPhysicalDevice physicalDevice)
{
	QueueFamilyIndices queueFamilyIndices;
	findQueueFamilies(queueFamilyIndices, physicalDevice, m_surface);

	bool mandatoryExtensionsSupported = checkDeviceExtensionSupport(physicalDevice, m_deviceExtensions);
	m_availableFeatures.rayTracing = checkDeviceExtensionSupport(physicalDevice, m_raytracingDeviceExtensions);
	m_availableFeatures.meshShader = checkDeviceExtensionSupport(physicalDevice, m_meshShaderDeviceExtensions);
	m_availableFeatures.variableShadingRate = checkDeviceExtensionSupport(physicalDevice, m_shadingRateDeviceExtensions);

	bool swapChainAdequate = false;
	if (mandatoryExtensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport;
		querySwapChainSupport(swapChainSupport, physicalDevice, m_surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	return queueFamilyIndices.isComplete() && mandatoryExtensionsSupported && swapChainAdequate /*&& supportedFeatures.samplerAnisotropy*/;
}

void Wolf::Vulkan::retrievePhysicalDeviceRayTracingProperties()
{
#ifndef __ANDROID__
	// Query the values of shaderHeaderSize and maxRecursionDepth in current implementation
	m_raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
	m_raytracingProperties.pNext = nullptr;
	m_raytracingProperties.maxRayRecursionDepth = 0;
	m_raytracingProperties.shaderGroupHandleSize = 0;
	VkPhysicalDeviceProperties2 props;
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &m_raytracingProperties;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &props);
#endif
}

void Wolf::Vulkan::retrievePhysicalDeviceShadingRateProperties()
{
#ifndef __ANDROID__
	m_shadingRateProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
	m_shadingRateProperties.pNext = nullptr;
	VkPhysicalDeviceProperties2 props;
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &m_shadingRateProperties;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(m_physicalDevice, &props);
#endif
}
