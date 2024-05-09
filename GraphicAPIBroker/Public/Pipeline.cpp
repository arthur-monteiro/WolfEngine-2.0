#include "Pipeline.h"

#ifdef WOLF_USE_VULKAN
#include "../Private/Vulkan/PipelineVulkan.h"
#endif

Wolf::Pipeline* Wolf::Pipeline::createRenderingPipeline(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo)
{
#ifdef WOLF_USE_VULKAN
	return new PipelineVulkan(renderingPipelineCreateInfo);
#else
	return nullptr;
#endif
}

Wolf::Pipeline* Wolf::Pipeline::createComputePipeline(const ShaderCreateInfo& computeShaderInfo,
	std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts)
{
#ifdef WOLF_USE_VULKAN
	return new PipelineVulkan(computeShaderInfo, descriptorSetLayouts);
#else
	return nullptr;
#endif
}

Wolf::Pipeline* Wolf::Pipeline::createRayTracingPipeline(
	const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo,
	std::span<const DescriptorSetLayout*> descriptorSetLayouts)
{
#ifdef WOLF_USE_VULKAN
	return new PipelineVulkan(rayTracingPipelineCreateInfo, descriptorSetLayouts);
#else
	return nullptr;
#endif
}