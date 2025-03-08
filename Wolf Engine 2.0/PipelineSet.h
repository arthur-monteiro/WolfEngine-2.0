/*
 * Main idea: Regroup multiple pipelines used for same input layout
 * Example: Draw a mesh in a depth pre-pass then on screen: we need 2 pipelines (1 with vertex shader only and 1 with pixel drawing)
 */

#pragma once

#include <map>
#include <vector>

#include "DescriptorSet.h"
#include "DescriptorSetBindInfo.h"
#include "Pipeline.h"
#include "ResourceReference.h"
#include "ShaderParser.h"

namespace Wolf
{
	class RenderPass;
	class ShaderList;

	class PipelineSet
	{
	public:
		static constexpr uint32_t MAX_PIPELINE_COUNT = 16;

		struct PipelineInfo
		{
			struct ShaderInfo
			{
				std::string shaderFilename;
				std::vector<std::string> conditionBlocksToInclude;
				std::string entryPointName = defaultEntryPointName;
				ShaderStageFlagBits stage;

				ShaderParser::MaterialFetchProcedure materialFetchProcedure;
			};

			// Programming stages
			std::vector<ShaderInfo> shaderInfos;

			// IA
			std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			bool primitiveRestartEnable = false;

			// Resources layouts
			uint32_t cameraDescriptorSlot = -1;
			uint32_t bindlessDescriptorSlot = -1;
			uint32_t lightDescriptorSlot = -1;
			uint32_t customMask = 0;

			// Viewport
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
		uint32_t addEmptyPipeline(int32_t forceIdx = -1);
		void updatePipeline(const PipelineInfo& pipelineInfo, uint32_t idx);

		std::vector<uint64_t> retrieveAllPipelinesHash() const;
		uint32_t getPipelineCount() const;
		std::vector<const PipelineInfo*> retrieveAllPipelinesInfo() const;
		const Pipeline* getOrCreatePipeline(uint32_t idx, RenderPass* renderPass, const std::vector<DescriptorSetBindInfo>& meshDescriptorSetsBindInfo, const std::vector<DescriptorSetBindInfo>& additionalDescriptorSetsBindInfo, ShaderList& shaderList) const;
		uint64_t getPipelineHash(uint32_t idx) const { return (idx < m_infoForPipelines.size() && m_infoForPipelines[idx]) ? m_infoForPipelines[idx]->getHash() : 0; }
		uint32_t getCameraDescriptorSlot(uint32_t idx) const { return m_infoForPipelines[idx]->getPipelineInfo().cameraDescriptorSlot; }
		uint32_t getBindlessDescriptorSlot(uint32_t idx) const { return m_infoForPipelines[idx]->getPipelineInfo().bindlessDescriptorSlot; }
		uint32_t getLightDescriptorSlot(uint32_t idx) const { return m_infoForPipelines[idx]->getPipelineInfo().lightDescriptorSlot; }
		uint32_t getCustomMask(uint32_t idx) const { return m_infoForPipelines[idx]->getPipelineInfo().customMask; }

	private:
		void shaderCodeChanged(const ShaderParser* shaderParser) const;
		void renderPassExtentChanged(const RenderPass* renderPass) const;

		class InfoForPipeline
		{
		public:
			InfoForPipeline(PipelineInfo pipelineCreateInfo);
			InfoForPipeline(PipelineInfo pipelineCreateInfo, uint64_t hash) : m_pipelineInfo(std::move(pipelineCreateInfo)), m_hash(hash) {}

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
