#include "PipelineSet.h"

#include <xxh64.hpp>

#include "MaterialsGPUManager.h"
#include "Debug.h"
#include "GraphicCameraInterface.h"
#include "LightManager.h"
#include "ProfilerCommon.h"
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

	if (insertionIdx >= MAX_PIPELINE_COUNT)
	{
		Debug::sendCriticalError("Adding more pipeline than supported");
	}

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

uint32_t Wolf::PipelineSet::getPipelineCount() const
{
	return static_cast<uint32_t>(m_infoForPipelines.size());
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

const Wolf::Pipeline* Wolf::PipelineSet::getOrCreatePipeline(uint32_t idx, RenderPass* renderPass, const std::vector<DescriptorSetBindInfo>& meshDescriptorSetsBindInfo, const std::vector<DescriptorSetBindInfo>& additionalDescriptorSetsBindInfo, ShaderList& shaderList) const
{
	PROFILE_FUNCTION

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
			addShaderInfo.bindlessDescriptorSlot = pipelineInfo.shaderInfos[i].stage == ShaderStageFlagBits::FRAGMENT ? pipelineInfo.bindlessDescriptorSlot : -1;
			addShaderInfo.lightDescriptorSlot = pipelineInfo.shaderInfos[i].stage == ShaderStageFlagBits::FRAGMENT ? pipelineInfo.lightDescriptorSlot : -1;
			addShaderInfo.materialFetchProcedure = pipelineInfo.shaderInfos[i].materialFetchProcedure;
			if (!addShaderInfo.materialFetchProcedure.codeString.empty())
			{
				if (pipelineInfo.shaderInfos[i].stage != ShaderStageFlagBits::FRAGMENT)
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
		for (const DescriptorSetBindInfo& descriptorSetLayout : additionalDescriptorSetsBindInfo)
		{
			if (descriptorSetLayout.getBindingSlot() > maxSlot)
				maxSlot = descriptorSetLayout.getBindingSlot();
		}
		for (const DescriptorSetBindInfo& descriptorSetLayout : meshDescriptorSetsBindInfo)
		{
			if (descriptorSetLayout.getBindingSlot() > maxSlot)
				maxSlot = descriptorSetLayout.getBindingSlot();
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
		if (pipelineInfo.lightDescriptorSlot != static_cast<uint32_t>(-1))
		{
			if (pipelineInfo.lightDescriptorSlot > maxSlot)
				maxSlot = pipelineInfo.lightDescriptorSlot;
		}

		for (uint32_t slot = 0; slot <= maxSlot; ++slot)
		{
			bool slotFound = false;
			for (const DescriptorSetBindInfo& descriptorSetLayout : additionalDescriptorSetsBindInfo)
			{
				if (descriptorSetLayout.getBindingSlot() == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(descriptorSetLayout.getDescriptorSetLayout());
					slotFound = true;
				}
			}

			for (const DescriptorSetBindInfo& descriptorSetLayout : meshDescriptorSetsBindInfo)
			{
				if (descriptorSetLayout.getBindingSlot() == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(descriptorSetLayout.getDescriptorSetLayout());
					slotFound = true;
				}
			}

			if (!slotFound)
			{
				if (pipelineInfo.cameraDescriptorSlot == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(GraphicCameraInterface::getDescriptorSetLayout().createConstNonOwnerResource());
					slotFound = true;
				}
				else if (pipelineInfo.bindlessDescriptorSlot == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(MaterialsGPUManager::getDescriptorSetLayout().createConstNonOwnerResource());
					slotFound = true;
				}
				else if (pipelineInfo.lightDescriptorSlot == slot)
				{
					renderingPipelineCreateInfo.descriptorSetLayouts.emplace_back(LightManager::getDescriptorSetLayout().createConstNonOwnerResource());
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
		if (infoForPipeline)
			infoForPipeline->getPipelines().erase(renderPass);
	}
}

Wolf::PipelineSet::InfoForPipeline::InfoForPipeline(PipelineInfo pipelineCreateInfo) : m_pipelineInfo(std::move(pipelineCreateInfo))
{
	m_hash = xxh64::hash(reinterpret_cast<const char*>(&m_pipelineInfo), sizeof(pipelineCreateInfo), 0);
}
