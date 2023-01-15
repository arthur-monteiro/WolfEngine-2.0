#include "Vulkan.h"

#include <set>

#include "SwapChainSupportDetails.h"
#include "VulkanHelper.h"

Wolf::Vulkan* Wolf::g_vulkanInstance = nullptr;

Wolf::Vulkan::Vulkan(GLFWwindow* glfwWindowPtr, bool useOVR)
{
	if (g_vulkanInstance)
	{
		Debug::sendCriticalError("Can't instanciate Vulkan twice");
	}

	if(useOVR)
	{
		ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
		ovrResult result = ovr_Initialize(&initParams);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendCriticalError("Failed to initialize OVR");
			return;
		}

		result = ovr_Create(&m_session, &m_luid);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendCriticalError("Failed to create OVR");
			return;
		}
	}

//#ifndef NDEBUG
	m_validationLayers = { "VK_LAYER_KHRONOS_validation" };
//#endif
	createInstance();
//#ifndef NDEBUG
	setupDebugMessenger();
//#endif
	if (glfwCreateWindowSurface(m_instance, glfwWindowPtr, nullptr, &m_surface) != VK_SUCCESS)
	{
		Debug::sendCriticalError("Error : window surface creation");
	}

	m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, "VK_KHR_external_memory_win32", VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
		"VK_KHR_external_semaphore_win32", VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME, "VK_KHR_external_fence", "VK_KHR_external_fence_win32" };
	m_raytracingDeviceExtensions = { VK_NV_RAY_TRACING_EXTENSION_NAME };
	m_meshShaderDeviceExtensions = { VK_NV_MESH_SHADER_EXTENSION_NAME };

	pickPhysicalDevice();
	createDevice();

	createCommandPools();

	m_descriptorPool.reset(new DescriptorPool(m_device));
}

Wolf::Vulkan::~Vulkan()
{
}

std::vector<const char*> getRequiredExtensions()
{
	std::vector<const char*> extensions;

	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
		extensions.push_back(glfwExtensions[i]);

//#ifndef NDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//#endif

	extensions.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);

	return extensions;
}

void Wolf::Vulkan::createInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "App Name";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Wolf Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char*> extensions = getRequiredExtensions();
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();

	populateDebugMessengerCreateInfo(debugCreateInfo);
	createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
	createInfo.enabledLayerCount = 0;
	createInfo.pNext = nullptr;
#endif

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		Debug::sendCriticalError("Error: instance creation");
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
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

	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

VkPhysicalDeviceRayTracingPropertiesNV getPhysicalDeviceRayTracingProperties(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceRayTracingPropertiesNV raytracingProperties{};

	// Query the values of shaderHeaderSize and maxRecursionDepth in current implementation
	raytracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV;
	raytracingProperties.pNext = nullptr;
	raytracingProperties.maxRecursionDepth = 0;
	raytracingProperties.shaderGroupHandleSize = 0;
	VkPhysicalDeviceProperties2 props;
	props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	props.pNext = &raytracingProperties;
	props.properties = {};
	vkGetPhysicalDeviceProperties2(physicalDevice, &props);

	return raytracingProperties;
}

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

void Wolf::Vulkan::pickPhysicalDevice()
{
	if (m_session)
	{
		ovrResult result = ovr_GetSessionPhysicalDeviceVk(m_session, m_luid, m_instance, &m_physicalDevice);
		if (!OVR_SUCCESS(result))
		{
			Debug::sendCriticalError("Failed to get physical device from OVR");
			return;
		}
		return;
	}

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

	if (deviceCount == 0)
		Debug::sendCriticalError("Error : No GPU with Vulkan support found !");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (auto& device : devices)
	{
		if (isDeviceSuitable(device, m_deviceExtensions, m_hardwareCapabilities))
		{
			m_hardwareCapabilities.rayTracingAvailable = isDeviceSuitable(device, m_raytracingDeviceExtensions, m_hardwareCapabilities);
			m_hardwareCapabilities.meshShaderAvailable = isDeviceSuitable(device, m_meshShaderDeviceExtensions, m_hardwareCapabilities);

			if (m_hardwareCapabilities.rayTracingAvailable)
				for (int i(0); i < m_raytracingDeviceExtensions.size(); ++i)
					m_deviceExtensions.push_back(m_raytracingDeviceExtensions[i]);

			m_physicalDevice = device;
			m_maxMsaaSamples = getMaxUsableSampleCount(m_physicalDevice);

			PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR =
				reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(m_instance, "vkGetPhysicalDeviceProperties2KHR"));
			VkPhysicalDeviceProperties2KHR deviceProps2{};
			m_conservativeRasterProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
			deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
			deviceProps2.pNext = &m_conservativeRasterProps;
			vkGetPhysicalDeviceProperties2KHR(m_physicalDevice, &deviceProps2);

			if (m_hardwareCapabilities.rayTracingAvailable)
				m_raytracingProperties = getPhysicalDeviceRayTracingProperties(m_physicalDevice);
			if (m_hardwareCapabilities.meshShaderAvailable)
				m_meshShaderProperties = getPhysicalDeviceMeshShaderProperties(m_physicalDevice);
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
	std::set<int> uniqueQueueFamilies = { m_queueFamilyIndices.graphicsFamily, m_queueFamilyIndices.presentFamily };

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

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descIndexFeatures = {};
	descIndexFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

	VkPhysicalDeviceFeatures2 supportedFeatures = {};
	supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	supportedFeatures.pNext = &descIndexFeatures;
	supportedFeatures.features.shaderStorageImageMultisample = VK_TRUE;
	vkGetPhysicalDeviceFeatures2(m_physicalDevice, &supportedFeatures);

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &(supportedFeatures.features);
	createInfo.pNext = &descIndexFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

#ifndef NDEBUG
	createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
	createInfo.ppEnabledLayerNames = m_validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

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

bool Wolf::Vulkan::isDeviceSuitable(VkPhysicalDevice physicalDevice, const std::vector<const char*>& deviceExtensions, HardwareCapabilities& outHardwareCapabilities)
{
	QueueFamilyIndices queueFamilyIndices;
	findQueueFamilies(queueFamilyIndices, physicalDevice, m_surface);

	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice, deviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport;
		querySwapChainSupport(swapChainSupport, physicalDevice, m_surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	//for (uint32_t i(0); i < memoryProperties.memoryHeapCount; ++i)
	//{
	//	if (memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
	//	{
	//		outHardwareCapabilities.VRAMSize = memoryProperties.memoryHeaps[i].size;
	//		if (memoryProperties.memoryHeaps[i].size < 1073741824)
	//		{
	//			std::cout << "Not enough memory : " << memoryProperties.memoryHeaps[i].size << std::endl;
	//			return false;
	//		}
	//	}
	//}

	return queueFamilyIndices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}
