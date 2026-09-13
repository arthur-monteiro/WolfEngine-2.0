#include "vulkan/vulkan.h"

#if !defined(__ANDROID__) or __ANDROID_MIN_SDK_VERSION__ > 31

static VkDevice sGlobalDevice = VK_NULL_HANDLE;

void registerGlobalDeviceForMeshShaders(VkDevice device)
{
    sGlobalDevice = device;
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectEXT(
    VkCommandBuffer                             commandBuffer,
    VkBuffer                                    buffer,
    VkDeviceSize                                offset,
    uint32_t                                    drawCount,
    uint32_t                                    stride)
{
    static const auto call = reinterpret_cast<PFN_vkCmdDrawMeshTasksIndirectEXT>(
        vkGetDeviceProcAddr(sGlobalDevice, "vkCmdDrawMeshTasksIndirectEXT"));
    return call(commandBuffer, buffer, offset, drawCount, stride);
}

#endif
