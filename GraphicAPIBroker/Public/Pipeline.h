#pragma once

#include <array>
#include <span>
#include <string>
#include <vector>

// TEMP
#include <vulkan/vulkan.h>

#include "Extents.h"
#include "ResourceReference.h"
#include "ShaderStages.h"

namespace Wolf
{
	class DescriptorSetLayout;
	class RenderPass;
	static const std::string defaultEntryPointName = "main";

	enum class PolygonMode { FILL, LINE, POINT };

	struct ShaderCreateInfo
	{
		std::vector<char> shaderCode;
		const std::string& entryPointName = defaultEntryPointName;
		ShaderStageFlagBits stage;
	};

	struct RenderingPipelineCreateInfo
	{
		const RenderPass* renderPass;

		// Programming stages
		std::vector<ShaderCreateInfo> shaderCreateInfos;

		// IA
		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		bool primitiveRestartEnable = false;

		// Resources
		std::vector<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts;

		// Viewport
		Extent2D extent = { 0, 0 };
		std::array<float, 2> viewportScale = { 1.0f, 1.0f };
		std::array<float, 2> viewportOffset = { 0.0f, 0.0f };

		// Rasterization
		PolygonMode polygonMode = PolygonMode::FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		bool enableConservativeRasterization = false;
		float maxExtraPrimitiveOverestimationSize = 0.75f;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor = 0.0f;

		// Multi-sampling
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

		// Color Blend
		enum class BLEND_MODE { OPAQUE, TRANS_ADD, TRANS_ALPHA };
		std::vector<BLEND_MODE> blendModes;

		// Depth testing
		VkBool32 enableDepthTesting = VK_TRUE;
		VkBool32 enableDepthWrite = VK_TRUE;
		VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		// Tessellation
		uint32_t patchControlPoint = 0;

		// Dynamic state
		std::vector<VkDynamicState> dynamicStates;
	};

	struct RayTracingPipelineCreateInfo
	{
		std::span<ShaderCreateInfo> shaderCreateInfos;
		std::span<VkRayTracingShaderGroupCreateInfoKHR> shaderGroupsCreateInfos;
	};

	class Pipeline
	{
	public:
		static Pipeline* createRenderingPipeline(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo);
		static Pipeline* createComputePipeline(const ShaderCreateInfo& computeShaderInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts);
		static Pipeline* createRayTracingPipeline(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<const DescriptorSetLayout*> descriptorSetLayouts);
		virtual ~Pipeline() = default;
	};


}
