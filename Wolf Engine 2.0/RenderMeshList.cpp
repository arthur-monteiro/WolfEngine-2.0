#include "RenderMeshList.h"

#include "CameraList.h"
#include "CommandRecordBase.h"
#include "Debug.h"
#include "LightManager.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "PipelineSet.h"

void Wolf::RenderMeshList::addMeshToRender(const MeshToRenderInfo& meshToRenderInfo)
{
	m_nextFrameMeshesToRender.push_back(std::make_unique<RenderMesh>(meshToRenderInfo.mesh, meshToRenderInfo.transform, meshToRenderInfo.pipelineSet, meshToRenderInfo.perPipelineDescriptorSets, 
		meshToRenderInfo.instanceInfos));

	const std::vector<uint64_t> pipelinesHash = meshToRenderInfo.pipelineSet->retrieveAllPipelinesHash();
	for (uint64_t pipelineHash : pipelinesHash)
	{
		if (std::find(m_uniquePipelinesHash.begin(), m_uniquePipelinesHash.end(), pipelineHash) == m_uniquePipelinesHash.end())
		{
			m_uniquePipelinesHash.push_back(pipelineHash);
		}
	}

	if (pipelinesHash.empty())
		Debug::sendError("Mesh added without any pipeline, it won't be rendered");

	m_pipelineIdxCount = std::max(m_pipelineIdxCount, meshToRenderInfo.pipelineSet->getPipelineCount());
}

void Wolf::RenderMeshList::draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& descriptorSetsToBind) const
{
	if (m_meshesToRenderByPipelineIdx.size() <= pipelineIdx) // no mesh added yet
		return;

	const std::vector<RenderMesh*>& meshesToRender = m_meshesToRenderByPipelineIdx[pipelineIdx];

	const Pipeline* currentPipeline = nullptr;
	for (const RenderMesh* mesh : meshesToRender)
	{
		std::vector<DescriptorSetBindInfo> descriptorSetsBindInfo;
		descriptorSetsBindInfo.reserve(descriptorSetsToBind.size());

		for (const AdditionalDescriptorSet& additionalDescriptorSet : descriptorSetsToBind)
		{
			if (additionalDescriptorSet.mask == 0 || additionalDescriptorSet.mask & mesh->getPipelineSet()->getCustomMask(pipelineIdx))
			{
				descriptorSetsBindInfo.push_back(additionalDescriptorSet.descriptorSetBindInfo);
			}
		}

		if (const Pipeline* meshPipeline = mesh->getPipelineSet()->getOrCreatePipeline(pipelineIdx, renderPass, mesh->getDescriptorSets(pipelineIdx), descriptorSetsBindInfo, m_shaderList); meshPipeline != currentPipeline)
		{
			commandBuffer.bindPipeline(meshPipeline);
			currentPipeline = meshPipeline;

			for (const DescriptorSetBindInfo& descriptorSetToBind : descriptorSetsBindInfo)
			{
				commandBuffer.bindDescriptorSet(descriptorSetToBind.getDescriptorSet(), descriptorSetToBind.getBindingSlot(), *meshPipeline);
			}

			if (cameraIdx != NO_CAMERA_IDX)
			{
				const uint32_t cameraDescriptorSlot = mesh->getPipelineSet()->getCameraDescriptorSlot(pipelineIdx);
				if (cameraDescriptorSlot == static_cast<uint32_t>(-1))
					Debug::sendError("Trying to bind camera descriptor set but slot hasn't been defined");

				commandBuffer.bindDescriptorSet(context.cameraList->getCamera(cameraIdx)->getDescriptorSet(), cameraDescriptorSlot, *meshPipeline);
			}

			if (const uint32_t bindlessDescriptorSlot = mesh->getPipelineSet()->getBindlessDescriptorSlot(pipelineIdx); bindlessDescriptorSlot != static_cast<uint32_t>(-1))
			{
				commandBuffer.bindDescriptorSet(context.bindlessDescriptorSet, bindlessDescriptorSlot, *meshPipeline);
			}

			if (const uint32_t lightDescriptorSlot = mesh->getPipelineSet()->getLightDescriptorSlot(pipelineIdx); lightDescriptorSlot != static_cast<uint32_t>(-1))
			{
				commandBuffer.bindDescriptorSet(context.lightManager->getDescriptorSet().createConstNonOwnerResource(), lightDescriptorSlot, *meshPipeline);
			}
		}

		mesh->draw(commandBuffer, currentPipeline, cameraIdx, pipelineIdx);
	}
}

void Wolf::RenderMeshList::clear()
{
	m_meshesToRenderByPipelineIdx.clear();
	m_currentFrameMeshesToRender.clear();
	m_nextFrameMeshesToRender.clear();
}

void Wolf::RenderMeshList::moveToNextFrame(const CameraList& cameraList)
{
	m_meshesToRenderByPipelineIdx.clear();
	m_meshesToRenderByPipelineIdx.resize(m_pipelineIdxCount);

	m_currentFrameMeshesToRender.clear();
	m_currentFrameMeshesToRender.swap(m_nextFrameMeshesToRender);

	std::vector<RenderMesh*> uniqueRenderMeshes;

	// Append mesh to render in the lists ordered by pipeline
	for (uint32_t pipelineIdx = 0; pipelineIdx < m_pipelineIdxCount; ++pipelineIdx)
	{
		for (const uint64_t pipelineHash : m_uniquePipelinesHash)
		{
			for (std::unique_ptr<RenderMesh>& meshToRender : m_currentFrameMeshesToRender)
			{
				if (meshToRender->getPipelineSet()->getPipelineHash(pipelineIdx) == pipelineHash)
				{
					RenderMesh* renderMeshPtr = meshToRender.get();
					m_meshesToRenderByPipelineIdx[pipelineIdx].push_back(renderMeshPtr);

					if (std::find(uniqueRenderMeshes.begin(), uniqueRenderMeshes.end(), renderMeshPtr) == uniqueRenderMeshes.end())
						uniqueRenderMeshes.push_back(renderMeshPtr);
				}
			}
		}
	}

	const std::vector<CameraInterface*>& activeCameras = cameraList.getCurrentCameras();
	for (const RenderMesh* renderMesh : uniqueRenderMeshes)
	{
		for (uint32_t cameraIdx = 0; cameraIdx < activeCameras.size(); ++cameraIdx)
		{
			if (const CameraInterface* camera = activeCameras[cameraIdx])
			{
				renderMesh->getMesh()->cullForCamera(cameraIdx, camera, renderMesh->getTransform(), renderMesh->isInstanced());
			}
		}
	}
}

void Wolf::RenderMeshList::RenderMesh::draw(const CommandBuffer& commandBuffer, const Pipeline* pipeline, uint32_t cameraIdx, uint32_t pipelineIdx) const
{
	for (const DescriptorSetBindInfo& descriptorSetBindInfo : m_perPipelineDescriptorSets[pipelineIdx])
	{
		commandBuffer.bindDescriptorSet(descriptorSetBindInfo.getDescriptorSet(), descriptorSetBindInfo.getBindingSlot(), *pipeline);
	}

	if (m_instanceInfos.instanceBuffer)
	{
		commandBuffer.bindVertexBuffer(*m_instanceInfos.instanceBuffer, 1);
	}

	m_mesh->draw(commandBuffer, cameraIdx, m_instanceInfos.instanceCount);
}
