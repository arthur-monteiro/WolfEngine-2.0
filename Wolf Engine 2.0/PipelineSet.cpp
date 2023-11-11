#include "PipelineSet.h"

#include <xxh64.hpp>

#include "Debug.h"
#include "RenderPass.h"
#include "ShaderList.h"

uint32_t Wolf::PipelineSet::addPipeline(const PipelineInfo& pipelineInfo, int32_t forceIdx)
{
	if (forceIdx >= 0)
	{
		if (static_cast<uint32_t>(forceIdx) > m_infoForPipelines.size())
			Debug::sendWarning("A pipeline is inserted far from the previous one. It may cause issues if the gap is not filled before draw");
		else if (static_cast<uint32_t>(forceIdx) < m_infoForPipelines.size() && m_infoForPipelines[forceIdx])
			Debug::sendError("Trying to add a pipeline at a location where another pipeline resides. This is wrong and will remove the previous pipeline");
	}

	const uint32_t insertionIdx = forceIdx >= 0 ? forceIdx : static_cast<uint32_t>(m_infoForPipelines.size());
	if (insertionIdx >= m_infoForPipelines.size())
		m_infoForPipelines.resize(insertionIdx + 1);

	m_infoForPipelines[insertionIdx].reset(new InfoForPipeline(pipelineInfo));

	return insertionIdx;
}

void Wolf::PipelineSet::updatePipeline(const PipelineInfo& pipelineInfo, uint32_t idx)
{
	if (idx >= m_infoForPipelines.size())
	{
		Debug::sendError("Invalid index " + std::to_string(idx) + " for pipeline update");
		return;
	}

	if (const uint64_t hash = xxh64::hash(reinterpret_cast<const char*>(&pipelineInfo), sizeof(pipelineInfo), 0); hash != m_infoForPipelines[idx]->getHash())
		m_infoForPipelines[idx].reset(new InfoForPipeline(pipelineInfo, hash));
}

std::vector<uint64_t> Wolf::PipelineSet::retrieveAllPipelinesHash() const
{
	std::vector<uint64_t> pipelinesHash;
	pipelinesHash.reserve(m_infoForPipelines.size());

	for (const std::unique_ptr<InfoForPipeline>& infoForPipeline : m_infoForPipelines)
		pipelinesHash.push_back(infoForPipeline->getHash());

	return pipelinesHash;
}

const Wolf::Pipeline* Wolf::PipelineSet::getOrCreatePipeline(uint32_t idx, RenderPass* renderPass) const
{
	if (!m_infoForPipelines[idx]->getPipelines().contains(renderPass))
	{
		renderPass->registerNewExtentChangedCallback([this](const RenderPass* paramRenderPass)
			{
				renderPassExtentChanged(paramRenderPass);
			});

		RenderingPipelineCreateInfo renderingPipelineCreateInfo{};

		const PipelineInfo& pipelineInfo = m_infoForPipelines[idx]->getPipelineInfo();

		renderingPipelineCreateInfo.renderPass = renderPass->getRenderPass();

		// Programming stages
		renderingPipelineCreateInfo.shaderCreateInfos.resize(pipelineInfo.shaderInfos.size());
		for(uint32_t i = 0; i < pipelineInfo.shaderInfos.size(); ++i)
		{
			ShaderList::AddShaderInfo addShaderInfo(pipelineInfo.shaderInfos[i].shaderFilename, pipelineInfo.shaderInfos[i].conditionBlocksToInclude);
			addShaderInfo.callbackWhenModified.emplace_back([this](const ShaderParser* shaderParser)
			{
				shaderCodeChanged(shaderParser);
			});

			const ShaderParser* shaderParser = g_shaderList->addShader(addShaderInfo);
			shaderParser->readCompiledShader(renderingPipelineCreateInfo.shaderCreateInfos[i].shaderCode);
			renderingPipelineCreateInfo.shaderCreateInfos[i].stage = pipelineInfo.shaderInfos[i].stage;
		}

		// IA
		renderingPipelineCreateInfo.vertexInputBindingDescriptions = pipelineInfo.vertexInputBindingDescriptions;
		renderingPipelineCreateInfo.vertexInputAttributeDescriptions = pipelineInfo.vertexInputAttributeDescriptions;
		renderingPipelineCreateInfo.topology = pipelineInfo.topology;
		renderingPipelineCreateInfo.primitiveRestartEnable = pipelineInfo.primitiveRestartEnable;

		// Resources layouts
		renderingPipelineCreateInfo.descriptorSetLayouts = pipelineInfo.descriptorSetLayouts;

		// Viewport
		renderingPipelineCreateInfo.extent = renderPass->getExtent();
		renderingPipelineCreateInfo.viewportScale = pipelineInfo.viewportScale;
		renderingPipelineCreateInfo.viewportOffset = pipelineInfo.viewportOffset;

		// Rasterization
		renderingPipelineCreateInfo.polygonMode = pipelineInfo.polygonMode;
		renderingPipelineCreateInfo.cullMode = pipelineInfo.cullMode;
		renderingPipelineCreateInfo.enableConservativeRasterization = pipelineInfo.enableConservativeRasterization;
		renderingPipelineCreateInfo.maxExtraPrimitiveOverestimationSize = pipelineInfo.maxExtraPrimitiveOverestimationSize;
		renderingPipelineCreateInfo.depthBiasConstantFactor = pipelineInfo.depthBiasConstantFactor;
		renderingPipelineCreateInfo.depthBiasSlopeFactor = pipelineInfo.depthBiasSlopeFactor;

		// Multi-sampling
		renderingPipelineCreateInfo.msaaSamples = pipelineInfo.msaaSamples;

		// Color Blend
		renderingPipelineCreateInfo.blendModes = pipelineInfo.blendModes;

		// Depth testing
		renderingPipelineCreateInfo.enableDepthTesting = pipelineInfo.enableDepthTesting;
		renderingPipelineCreateInfo.enableDepthWrite = pipelineInfo.enableDepthWrite;
		renderingPipelineCreateInfo.depthCompareOp = pipelineInfo.depthCompareOp;

		// Tessellation
		renderingPipelineCreateInfo.patchControlPoint = pipelineInfo.patchControlPoint;

		// Dynamic states
		renderingPipelineCreateInfo.dynamicStates = pipelineInfo.dynamicStates;

		m_infoForPipelines[idx]->getPipelines()[renderPass].reset(new Pipeline(renderingPipelineCreateInfo));
	}

	return m_infoForPipelines[idx]->getPipelines()[renderPass].get();
}

void Wolf::PipelineSet::shaderCodeChanged(const ShaderParser* shaderParser) const
{
	for(const std::unique_ptr<InfoForPipeline>& infoForPipeline : m_infoForPipelines)
	{
		infoForPipeline->getPipelines().clear();
	}
}

void Wolf::PipelineSet::renderPassExtentChanged(const RenderPass* renderPass) const
{
	for (const std::unique_ptr<InfoForPipeline>& infoForPipeline : m_infoForPipelines)
	{
		infoForPipeline->getPipelines().erase(renderPass);
	}
}

Wolf::PipelineSet::InfoForPipeline::InfoForPipeline(PipelineInfo pipelineCreateInfo) : m_pipelineInfo(std::move(pipelineCreateInfo))
{
	m_hash = xxh64::hash(reinterpret_cast<const char*>(&m_pipelineInfo), sizeof(pipelineCreateInfo), 0);
}
