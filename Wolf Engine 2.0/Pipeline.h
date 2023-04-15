#pragma once

#include <array>
#include <span>

#include "Vulkan.h"

namespace Wolf
{
	static const std::string defaultEntryPointName = "main";

	struct ShaderCreateInfo
	{
		std::span<char> shaderCode;
		const std::string& entryPointName = defaultEntryPointName;
		VkShaderStageFlagBits stage;
	};

	struct RenderingPipelineCreateInfo
	{
		VkRenderPass renderPass;

		// Programming stages
		std::span<ShaderCreateInfo> shaderCreateInfos;

		// IA
		std::span<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
		std::span<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool primitiveRestartEnable = false;

		// Resources
		std::span<VkDescriptorSetLayout> descriptorSetLayouts;

		// Viewport
		VkExtent2D extent = { 0, 0 };
		std::array<float, 2> viewportScale = { 1.0f, 1.0f };
		std::array<float, 2> viewportOffset = { 0.0f, 0.0f };

		// Rasterization
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		bool enableConservativeRasterization = false;
		float maxExtraPrimitiveOverestimationSize = 0.75f;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor = 0.0f;

		// Multi-sampling
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		// Color Blend
		enum class BLEND_MODE { OPAQUE, TRANS_ADD, TRANS_ALPHA };
		std::span<BLEND_MODE> blendModes;

		// Depth testing
		VkBool32 enableDepthTesting = VK_TRUE;
		VkBool32 enableDepthWrite = VK_TRUE;
		VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Tessellation
		uint32_t patchControlPoint = 0;
	};

	struct RayTracingPipelineCreateInfo
	{
		std::span<ShaderCreateInfo> shaderCreateInfos;
		std::span<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupsCreateInfos;
	};

	class Pipeline
	{
	public:
		// Rasterization
		Pipeline(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo);
		// Compute
		Pipeline(const ShaderCreateInfo& computeShaderInfo, std::span<VkDescriptorSetLayout> descriptorSetLayouts);
		// Ray tracing
		Pipeline(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<VkDescriptorSetLayout> descriptorSetLayouts);
		~Pipeline();

		[[nodiscard]] VkPipeline getPipeline() const { return m_pipeline; }
		[[nodiscard]] VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

	private:
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

	public:
		static std::vector<char> readFile(const std::string& filename);
		static VkShaderModule createShaderModule(const std::span<char>& code);
	};


}