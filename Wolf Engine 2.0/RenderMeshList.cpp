#include "RenderMeshList.h"

#include "CameraList.h"
#include "CommandRecordBase.h"
#include "Debug.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "PipelineSet.h"

void Wolf::RenderMeshList::addMeshToRender(const MeshToRenderInfo& meshToRenderInfo)
{
	m_nextFrameMeshesToRender.push_back(std::make_unique<RenderMesh>(meshToRenderInfo.mesh, meshToRenderInfo.pipelineSet, meshToRenderInfo.descriptorSets, 
		meshToRenderInfo.instanceInfos));

	const std::vector<uint64_t> pipelinesHash = meshToRenderInfo.pipelineSet->retrieveAllPipelinesHash();
	for (uint64_t pipelineHash : pipelinesHash)
	{
		if (std::ranges::find(m_uniquePipelinesHash, pipelineHash) == m_uniquePipelinesHash.end())
		{
			m_uniquePipelinesHash.push_back(pipelineHash);
		}
	}

	if (pipelinesHash.empty())
		Debug::sendError("Mesh added without any pipeline, it won't be rendered");

	m_pipelineIdxCount = std::max(m_pipelineIdxCount, static_cast<uint32_t>(pipelinesHash.size()));
}

void Wolf::RenderMeshList::draw(const RecordContext& context, VkCommandBuffer commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<std::pair<uint32_t, const DescriptorSet*>>& descriptorSetsToBind) const
{
	if (m_meshesToRenderByPipelineIdx.empty()) // no mesh added yet
		return;

	const std::vector<RenderMesh*>& meshesToRender = m_meshesToRenderByPipelineIdx[pipelineIdx];

	const Pipeline* currentPipeline = nullptr;
	for (const RenderMesh* mesh : meshesToRender)
	{
		if (const Pipeline* meshPipeline = mesh->getPipelineSet()->getOrCreatePipeline(pipelineIdx, renderPass); meshPipeline != currentPipeline)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->getPipeline());
			currentPipeline = meshPipeline;

			for (const std::pair<uint32_t, const DescriptorSet*>& descriptorSetToBind : descriptorSetsToBind)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->getPipelineLayout(), descriptorSetToBind.first, 1, descriptorSetToBind.second->getDescriptorSet(),
					0, nullptr);
			}

			if (cameraIdx != NO_CAMERA_IDX)
			{
				const uint32_t cameraDescriptorSlot = mesh->getPipelineSet()->getCameraDescriptorSlot(pipelineIdx);
				if (cameraDescriptorSlot == static_cast<uint32_t>(-1))
					Debug::sendError("Trying to bind camera descriptor set but slot hasn't been defined");

				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->getPipelineLayout(), cameraDescriptorSlot, 1,
					context.cameraList->getCamera(cameraIdx)->getDescriptorSet()->getDescriptorSet(),
					0, nullptr);
			}

			if (const uint32_t bindlessDescriptorSlot = mesh->getPipelineSet()->getBindlessDescriptorSlot(pipelineIdx); bindlessDescriptorSlot != static_cast<uint32_t>(-1))
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->getPipelineLayout(), bindlessDescriptorSlot, 1,
					context.bindlessDescriptorSet->getDescriptorSet(), 0, nullptr);
			}
		}

		mesh->draw(commandBuffer, currentPipeline);
	}
}

void Wolf::RenderMeshList::moveToNextFrame()
{
	m_meshesToRenderByPipelineIdx.clear();
	m_meshesToRenderByPipelineIdx.resize(m_pipelineIdxCount);

	m_currentFrameMeshesToRender.clear();
	m_currentFrameMeshesToRender.swap(m_nextFrameMeshesToRender);

	// Append mesh to render in the lists ordered by pipeline
	for (uint32_t pipelineIdx = 0; pipelineIdx < m_pipelineIdxCount; ++pipelineIdx)
	{
		for (const uint64_t pipelineHash : m_uniquePipelinesHash)
		{
			for (std::unique_ptr<RenderMesh>& meshToRender : m_currentFrameMeshesToRender)
			{
				if (meshToRender->getPipelineSet()->getPipelineHash(pipelineIdx) == pipelineHash)
					m_meshesToRenderByPipelineIdx[pipelineIdx].push_back(meshToRender.get());
			}
		}
	}
}

void Wolf::RenderMeshList::RenderMesh::draw(VkCommandBuffer commandBuffer, const Pipeline* pipeline) const
{
	for (const MeshToRenderInfo::DescriptorSetBindInfo& descriptorSetBindInfo : m_descriptorSets)
	{
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayout(), descriptorSetBindInfo.descriptorSetBindingSlot, 1, 
			descriptorSetBindInfo.descriptorSet->getDescriptorSet(), 0, nullptr);
	}

	if (m_instanceInfos.instanceBuffer)
	{
		constexpr VkDeviceSize offsets[1] = { 0 };
		const VkBuffer instanceBuffer = m_instanceInfos.instanceBuffer->getBuffer();
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &instanceBuffer, offsets);
	}

	m_mesh->draw(commandBuffer, m_instanceInfos.instanceCount);
}
