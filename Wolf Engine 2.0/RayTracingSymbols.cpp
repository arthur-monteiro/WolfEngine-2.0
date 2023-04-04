#include "vulkan/vulkan.h"

static VkDevice s_global_device = VK_NULL_HANDLE;

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateAccelerationStructureKHR(VkDevice       device,
    const VkAccelerationStructureCreateInfoKHR* pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkAccelerationStructureKHR*                 pAccelerationStructure)
{
    s_global_device = device;
    static const auto call = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
        vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
    return call(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyAccelerationStructureNV(VkDevice                     device,
    VkAccelerationStructureNV    accelerationStructure,
    const VkAllocationCallbacks* pAllocator)
{
    static const auto call = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
        vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV"));
    return call(device, accelerationStructure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR(VkDevice device,
    VkAccelerationStructureBuildTypeKHR                                     buildType,
    const VkAccelerationStructureBuildGeometryInfoKHR*                      pBuildInfo,
    const uint32_t*                                                         pMaxPrimitiveCounts,
    VkAccelerationStructureBuildSizesInfoKHR*                               pSizeInfo)
{
    static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
    return call(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkBindAccelerationStructureMemoryNV(VkDevice                                       device,
    uint32_t                                       bindInfoCount,
    const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
    static const auto call = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
        vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
    return call(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL
vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer        commandBuffer,
    uint32_t                                               infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR*     pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
{
    static const auto call = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
        vkGetDeviceProcAddr(s_global_device, "vkCmdBuildAccelerationStructuresKHR"));
    return call(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(VkCommandBuffer           cmdBuf,
    VkAccelerationStructureNV dst,
    VkAccelerationStructureNV src,
    VkCopyAccelerationStructureModeNV mode)
{
    static const auto call = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(
        vkGetDeviceProcAddr(s_global_device, "vkCmdCopyAccelerationStructureNV"));
    return call(cmdBuf, dst, src, mode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer,
    const VkStridedDeviceAddressRegionKHR*                   pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*                   pMissShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*                   pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR*                   pCallableShaderBindingTable,
    uint32_t                                                 width,
    uint32_t                                                 height,
    uint32_t                                                 depth)
{
    static const auto call = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
        vkGetDeviceProcAddr(s_global_device, "vkCmdTraceRaysKHR"));
    return call(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable,
        pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateRayTracingPipelinesKHR(VkDevice     device,
    VkDeferredOperationKHR                  deferredOperation,
    VkPipelineCache                         pipelineCache,
    uint32_t                                createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos,
    const VkAllocationCallbacks* pAllocator,
    VkPipeline* pPipelines)
{
    s_global_device = device;
    static const auto call = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
        vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));
    return call(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR(VkDevice device,
    VkPipeline                                                               pipeline,
    uint32_t                                                                 firstGroup,
    uint32_t                                                                 groupCount,
    size_t                                                                   dataSize,
    void*                                                                    pData)
{
    static const auto call = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
        vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
    return call(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL
vkGetAccelerationStructureHandleNV(VkDevice                  device,
    VkAccelerationStructureNV accelerationStructure,
    size_t                    dataSize,
    void* pData)
{
    static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
        vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
    return call(device, accelerationStructure, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
    VkCommandBuffer                  commandBuffer,
    uint32_t                         accelerationStructureCount,
    const VkAccelerationStructureNV* pAccelerationStructures,
    VkQueryType                      queryType,
    VkQueryPool                      queryPool,
    uint32_t                         firstQuery)
{
    static const auto call = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesNV>(
        vkGetDeviceProcAddr(s_global_device, "vkCmdWriteAccelerationStructuresPropertiesNV"));
    return call(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType,
        queryPool, firstQuery);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(VkDevice   device,
    VkPipeline pipeline,
    uint32_t   shader)
{
    static const auto call = reinterpret_cast<PFN_vkCompileDeferredNV>(
        vkGetDeviceProcAddr(s_global_device, "vkCompileDeferredNV"));
    return call(device, pipeline, shader);
}
