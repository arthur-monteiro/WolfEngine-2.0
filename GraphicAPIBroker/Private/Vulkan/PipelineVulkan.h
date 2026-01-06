#pragma once

#include <span>
#include <vulkan/vulkan.h>

#include "../../Public/Pipeline.h"

namespace Wolf
{
	class PipelineVulkan : public Pipeline
	{
	public:
		// Rasterization
		PipelineVulkan(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo);
		// Compute
		PipelineVulkan(const ShaderCreateInfo& computeShaderInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts);
		// Ray tracing
		PipelineVulkan(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts);
		~PipelineVulkan() override;

		[[nodiscard]] VkPipeline getPipeline() const { return m_pipeline; }
		[[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

		[[nodiscard]] VkPipelineBindPoint getPipelineBindPoint() const;

	private:
		static VkShaderModule createShaderModule(const std::vector<char>& code);
		static VkPolygonMode wolfPolygonModeToVkPolygonMode(PolygonMode polygonMode);
		static VkPrimitiveTopology wolfPrimitiveTopologyToVkPrimitiveTopology(PrimitiveTopology primitiveTopology);
		static VkCullModeFlagBits wolfCullModeFlagBitsToVkCullModeFlagBits(CullModeFlagBits cullModeFlagBits);
		static VkCullModeFlags wolfCullModeFlagsToVkCullModeFlags(CullModeFlags cullModeFlags);
		static VkDynamicState wolfDynamicStateToVkDynamicState(DynamicState dynamicState);
		static VkVertexInputRate wolfVertexInputRateToVkVertexInputRate(VertexInputRate vertexInputRate);
		static VkRayTracingShaderGroupTypeKHR wolfRayTracingShaderGroupTypeToVkRayTracingShaderGroupType(RayTracingShaderGroupType rayTracingShaderGroup);

		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		enum class Type { RENDERING, COMPUTE, RAY_TRACING  } m_type;
	};
}
