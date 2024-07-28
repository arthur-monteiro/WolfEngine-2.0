#include "PipelineSet.h"

#include <xxh64.hpp>

#include "MaterialsGPUManager.h"
#include "Debug.h"
#include "GraphicCameraInterface.h"
#include "RenderPass.h"
#include "ShaderList.h"

uint32_t Wolf::PipelineSet::addPipeline(const PipelineInfo& pipelineInfo, int32_t forceIdx)
{
	if (forceIdx >= 0)
	{
		if (static_cast<uint32_t>(forceIdx) > m_infoForPipelines.size())
			Debug::sendWarning("A pipeline is inserted far from the previous one. It may cause issues if the gap is not filled before draw");
		else if (static_cast<uint32_t>(forceIdx) < m_infoForPipelines.size() && m_infoForPipelines[forceIdx])
			Debug::sendError("Trying to add a pipeline at a location where another pipeline resides. This will remove the previous pipeline");
	}

	const uint32_t insertionIdx = forceIdx >= 0 ? forceIdx : static_cast<uint32_t>(m_infoForPipelines.size());
	if (insertionIdx >= m_infoForPipelines.size())
		m_infoForPipelines.resize(insertionIdx + 1);

	m_infoForPipelines[insertionIdx].reset(new InfoForPipeline(pipelineInfo));

	return insertionIdx;
}

uint32_t Wolf::PipelineSet::addEmptyPipeline(int32_t forceIdx)
{
	if (forceIdx >= 0)
	{
		if (static_cast<uint32_t>(forceIdx) < m_infoForPipelines.size() && m_infoForPipelines[forceIdx])
			Debug::sendError("Trying to add a pipeline at a location where another pipeline resides. This will remove the previous pipeline");
	}

	const uint32_t insertionIdx = forceIdx >= 0 ? forceIdx : static_cast<uint32_t>(m_infoForPipelines.size());
	if (m_infoForPipelines.size() < insertionIdx + 1)
		m_infoForPipelines.resize(insertionIdx + 1);

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
	{
		if (infoForPipeline)
			pipelinesHash.push_back(infoForPipeline->getHash());
	}

	return pipelinesHash;
}

std::vector<const Wolf::PipelineSet::PipelineInfo*> Wolf::PipelineSet::retrieveAllPipelinesInfo() const
{
	std::vector<const PipelineInfo*> r;
	for (const std::unique_ptr<InfoForPipeline>& infoForPipeline : m_infoForPipelines)
	{
		r.emplace_back(&infoForPipeline->getPipelineInfo());
	}
	return r;
}

const Wolf::Pipeline* Wolf::PipelineSet::getOrCreatePipeline(uint32_t idx, RenderPass* renderPass, ShaderList& shaderList) const
{
	if (!m_infoForPipelines[idx]->getPipelines().contains(renderPass))
	{
		renderPass->registerNewExtentChangedCallback([this](const RenderPass* paramRenderPass)
			{
				renderPassExtentChanged(paramRenderPass);
			});

		RenderingPipelineCreateInfo renderingPipelineCreateInfo{};

		const PipelineInfo& pipelineInfo = m_infoForPipelines[idx]->getPipelineInfo();

		renderingPipelineCreateInfo.renderPass = renderPass;

		// Programming stages
		renderingPipelineCreateInfo.shaderCreateInfos.resize(pipelineInfo.shaderInfos.size());
		for(uint32_t i = 0; i < pipelineInfo.shaderInfos.size(); ++i)
		{
			ShaderList::AddShaderInfo addShaderInfo(pipelineInfo.shaderInfos[i].shaderFilename, pipelineInfo.shaderInfos[i].conditionBlocksToInclude);
			addShaderInfo.callbackWhenModified.emplace_back([this](const ShaderParser* shaderParser)
			{
				shaderCodeChanged(shaderParser);
			});
			addShaderInfo.cameraDescriptorSlot = pipelineInfo.cameraDescriptorSlot;
			addShaderInfo.bindlessDescriptorSlot = pipelineInfo.shaderInfos[i].stage == VK_SHADER_STAGE_FRAGMENT_BIT ? pipelineInfo.bindlessDescriptorSlot : -1;
			addShaderInfo.materialFetchProcedure = pipelineInfo.shaderInfos[i].materialFetchProcedure;
			if (!addShaderInfo.materialFetchProcedure.codeString.empty())
			{
				if (pipelineInfo.shaderInfos[i].stage != VK_SHADER_STAGE_FRAGMENT_BIT)
					Debug::sendError("Material fetch procedure is only supported for fragment shaders");
			}

			const ShaderParser* shaderParser = shaderList.addShader(addShaderInfo);
			shaderParser->readCompiledShader(renderingPipelineCreateInfo.shaderCreateInfos[i].shaderCode);
			renderingPipelineCreateInfo.shaderCreateInfos[i].stage = pipelineInfo.shaderInfos[i].stage;
		}

		// IA
		renderingPipelineCreateInfo.vertexInputBindingDescriptions = pipelineInfo.vertexInputBindingDescriptions;
		renderingPipelineCreateInfo.vertexInputAttributeDescriptions = pipelineInfo.vertexInputAttributeDescriptions;
		renderingPipelineCreateInfo.topology = pipelineInfo.topology;
		renderingPipelineCreateInfo.primitiveRestartEnable = pipelineInfo.primitiveRestartEnable;

		// Resources layouts
		uint32_t maxSlot = 0;
		for (const std::pair<ResourceReference<const DescriptorSetLayout>, uint32_t>& descriptorSetLayout : pipelineInfo.descriptorSetLayouts)
		{
			if (descriptorSetLayout.second > maxSlot)
				maxSlot = descriptorSetLayout.second;
		}
		if (pipelineInfo.cameraDescriptorSlot != static_cast<uint32_t>(-1))
		{
			if (pipelineInfo.cameraDescriptorSlot > maxSlot)
				maxSlot = pipelineInfo.cameraDescriptorSlot;
		}
		if (pipelineInfo.bindlessDescriptorSlot != static_cast<uint32_t>(-1))
		{
			if (pipelineInfo.bindlessDescriptorSlot > maxSlot)
				maxSlot = pipelineInfo.bindlessDescriptorSlot;
		}

		for (uint32_t slot = 0; slot <= maxSlot; ++slot)
		{
			bool slotFound = false;
			for (const std::pair<ResourceReference<const DescriptorSetLayout>, uint32_t>& descriptorSetLayout : pipelineInfo.descriptorSetLayouts)
			{
				if (descriptorSetLayout.second == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(descriptorSetLayout.first);
					slotFound = true;
				}
			}

			if (!slotFound)
			{
				if (pipelineInfo.cameraDescriptorSlot == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(GraphicCameraInterface::getDescriptorSetLayout());
					slotFound = true;
				}
				else if (pipelineInfo.bindlessDescriptorSlot == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(MaterialsGPUManager::getDescriptorSetLayout());
					slotFound = true;
				}
			}

			if (!slotFound)
			{
				Debug::sendCriticalError("An empty slot has been found");
			}
		}

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

		m_infoForPipelines[idx]->getPipelines()[renderPass].reset(Pipeline::createRenderingPipeline(renderingPipelineCreateInfo));
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
