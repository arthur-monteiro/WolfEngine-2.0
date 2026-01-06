#pragma once

#include <array>
#include <span>
#include <string>
#include <vector>

#include "Enums.h"
#include "Extents.h"
#include "Formats.h"
#include "RayTracingShaderGroupCreateInfo.h"
#include "ResourceReference.h"
#include "ShaderStages.h"
#include "VertexInputs.h"

namespace Wolf
{
	class DescriptorSetLayout;
	class RenderPass;
	static const std::string defaultEntryPointName = "main";

	enum class PolygonMode { FILL, LINE, POINT };
	enum class PrimitiveTopology { TRIANGLE_LIST, PATCH_LIST, LINE_LIST };
	enum CullModeFlagBits : uint32_t
	{
		NONE = 1 << 0,
		BACK = 1 << 1,
		FRONT = 1 << 2,
		CULL_MODE_FLAG_BITS_MAX = 1 << 3
	};
	using CullModeFlags = uint32_t;
	enum class DynamicState { VIEWPORT };

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
		std::vector<VertexInputBindingDescription> vertexInputBindingDescriptions;
		std::vector<VertexInputAttributeDescription> vertexInputAttributeDescriptions;
		PrimitiveTopology topology = PrimitiveTopology::TRIANGLE_LIST;
		bool primitiveRestartEnable = false;

		// Resources
		std::vector<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts;

		// Viewport
		Extent2D extent = { 0, 0 };
		std::array<float, 2> viewportScale = { 1.0f, 1.0f };
		std::array<float, 2> viewportOffset = { 0.0f, 0.0f };

		// Rasterization
		PolygonMode polygonMode = PolygonMode::FILL;
		CullModeFlags cullModeFlags = CullModeFlagBits::BACK;
		bool enableConservativeRasterization = false;
		float maxExtraPrimitiveOverestimationSize = 0.75f;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor = 0.0f;

		// Multi-sampling
		SampleCountFlagBits msaaSamplesFlagBit = SAMPLE_COUNT_1;

		// Color Blend
		enum class BLEND_MODE { OPAQUE, TRANS_ADD, TRANS_ALPHA };
		std::vector<BLEND_MODE> blendModes;

		// Depth testing
		bool enableDepthTesting = true;
		bool enableDepthWrite = true;
		CompareOp depthCompareOp = CompareOp::LESS_OR_EQUAL;

		// Tessellation
		uint32_t patchControlPoint = 0;

		// Dynamic state
		std::vector<DynamicState> dynamicStates;
	};

	struct RayTracingPipelineCreateInfo
	{
		std::span<ShaderCreateInfo> shaderCreateInfos;
		std::span<RayTracingShaderGroupCreateInfo> shaderGroupsCreateInfos;
	};

	class Pipeline
	{
	public:
		static Pipeline* createRenderingPipeline(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo);
		static Pipeline* createComputePipeline(const ShaderCreateInfo& computeShaderInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts);
		static Pipeline* createRayTracingPipeline(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts);
		virtual ~Pipeline() = default;
	};


}
