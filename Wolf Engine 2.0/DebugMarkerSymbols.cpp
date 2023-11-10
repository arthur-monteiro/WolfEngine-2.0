#include "vulkan/vulkan.h"

static VkDevice sGlobalDevice = VK_NULL_HANDLE;

void registerGlobalDeviceForDebugMarker(VkDevice device)
{
    sGlobalDevice = device;
}

VKAPI_ATTR void VKAPI_CALL
vkCmdDebugMarkerInsertEXT(VkCommandBuffer                   commandBuffer,
                          const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
    static const auto call = reinterpret_cast<PFN_vkCmdDebugMarkerInsertEXT>(
        vkGetDeviceProcAddr(sGlobalDevice, "vkCmdDebugMarkerInsertEXT"));
    return call(commandBuffer, pMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdDebugMarkerBeginEXT(VkCommandBuffer                   commandBuffer,
                         const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
	static const auto call = reinterpret_cast<PFN_vkCmdDebugMarkerBeginEXT>(
        vkGetDeviceProcAddr(sGlobalDevice, "vkCmdDebugMarkerBeginEXT"));
    return call(commandBuffer, pMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
    static const auto call = reinterpret_cast<PFN_vkCmdDebugMarkerEndEXT>(
        vkGetDeviceProcAddr(sGlobalDevice, "vkCmdDebugMarkerEndEXT"));
    return call(commandBuffer);
}