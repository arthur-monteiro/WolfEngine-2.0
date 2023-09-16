#include "vulkan/vulkan.h"

static PFN_vkCmdSetFragmentShadingRateKHR call_vkCmdSetFragmentShadingRateKHR;

void initializeShadingRateFunctions(VkDevice device)
{
    call_vkCmdSetFragmentShadingRateKHR = reinterpret_cast<PFN_vkCmdSetFragmentShadingRateKHR>(
        vkGetDeviceProcAddr(device, "vkCmdSetFragmentShadingRateKHR"));
}

VKAPI_ATTR void VKAPI_CALL
vkCmdSetFragmentShadingRateKHR(VkCommandBuffer  commandBuffer,
    const VkExtent2D*                           pFragmentSize,
    const VkFragmentShadingRateCombinerOpKHR    combinerOps[2])
{
    return call_vkCmdSetFragmentShadingRateKHR(commandBuffer, pFragmentSize, combinerOps);
}