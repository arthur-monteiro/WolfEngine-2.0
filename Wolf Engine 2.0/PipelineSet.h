/*
 * Main idea: Regroup multiple pipelines used for same input layout
 * Example: Draw a mesh in a depth pre-pass then on screen: we need 2 pipelines (1 with vertex shader only and 1 with pixel drawing)
 */

#pragma once

#include <map>
#include <vector>

#include "DescriptorSet.h"
#include "Pipeline.h"

namespace Wolf
{
	class RenderPass;
	class ShaderParser;

	class PipelineSet
	{
	public:
		struct PipelineInfo
		{
			struct ShaderInfo
			{
				std::string shaderFilename;
				std::string entryPointName = defaultEntryPointName;
				VkShaderStageFlagBits stage;
			};

			// Programming stages
			std::vector<ShaderInfo> shaderInfos;

			// IA
			std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			bool primitiveRestartEnable = false;

			// Resources layouts
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

			// Viewport
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
			std::vector<RenderingPipelineCreateInfo::BLEND_MODE> blendModes;

			// Depth testing
			VkBool32 enableDepthTesting = VK_TRUE;
			VkBool32 enableDepthWrite = VK_TRUE;
			VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

			// Tessellation
			uint32_t patchControlPoint = 0;

			// Dynamic state
			std::vector<VkDynamicState> dynamicStates;
		};
		uint32_t addPipeline(const PipelineInfo& pipelineInfo, int32_t forceIdx = -1);

		std::vector<uint64_t> retrieveAllPipelinesHash() const;
		const Pipeline* getOrCreatePipeline(uint32_t idx, RenderPass* renderPass) const;
		uint64_t getPipelineHash(uint32_t idx) const { return m_infoForPipelines[idx]->getHash(); }

	private:
		void shaderCodeChanged(const ShaderParser* shaderParser) const;
		void renderPassExtentChanged(const RenderPass* renderPass) const;

		class InfoForPipeline
		{
		public:
			InfoForPipeline(PipelineInfo pipelineCreateInfo);

			std::map<const RenderPass*, std::unique_ptr<Pipeline>>& getPipelines() { return m_pipelines; }
			const PipelineInfo& getPipelineInfo() const { return m_pipelineInfo; }
			uint64_t getHash() const { return m_hash; }

		private:
			const PipelineInfo m_pipelineInfo;
			std::map<const RenderPass*, std::unique_ptr<Pipeline>> m_pipelines;
			uint64_t m_hash;
		};

		std::vector<std::unique_ptr<InfoForPipeline>> m_infoForPipelines;
	};
}