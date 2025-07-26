#pragma once

#include <array>
#include <span>

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

		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

		enum class Type { RENDERING, COMPUTE, RAY_TRACING  } m_type;
	};
}