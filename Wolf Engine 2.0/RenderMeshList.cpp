#include "RenderMeshList.h"

#include "CameraList.h"
#include "CommandRecordBase.h"
#include "Debug.h"
#include "LightManager.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "PipelineSet.h"
#include "ProfilerCommon.h"

void Wolf::RenderMeshList::moveToNextFrame()
{
	m_transientMeshesCurrentFrame.clear();
	m_transientMeshesCurrentFrame.swap(m_transientMeshesNextFrame);

	m_transientInstancedMeshesCurrentFrame.clear();
	m_transientInstancedMeshesCurrentFrame.swap(m_transientInstancedMeshesNextFrame);
}

void Wolf::RenderMeshList::clear()
{
	m_transientMeshesCurrentFrame.clear();
	m_transientMeshesNextFrame.clear();

	m_transientInstancedMeshesCurrentFrame.clear();
	m_transientInstancedMeshesNextFrame.clear();

	m_meshes.clear();
	m_instancedMeshes.clear();
}

uint32_t Wolf::RenderMeshList::registerMesh(const MeshToRender& mesh)
{
	m_meshes.emplace_back(mesh);
	return static_cast<uint32_t>(m_meshes.size()) - 1;
}

void Wolf::RenderMeshList::addTransientMesh(const MeshToRender& mesh)
{
	m_transientMeshesNextFrame.emplace_back(mesh);
}

uint32_t Wolf::RenderMeshList::registerInstancedMesh(const InstancedMesh& mesh, uint32_t maxInstanceCount, uint32_t instanceIdx)
{
	m_instancedMeshes.emplace_back(mesh);

	InternalInstancedMesh& instancedMesh = m_instancedMeshes.back();
	instancedMesh.activatedInstances.resize(maxInstanceCount);
	for (auto&& activatedInstance : instancedMesh.activatedInstances)
		activatedInstance = false;
	instancedMesh.activatedInstances[instanceIdx] = true;
	instancedMesh.buildOffsetAndCounts();

	return static_cast<uint32_t>(m_instancedMeshes.size()) - 1;
}

void Wolf::RenderMeshList::addInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx)
{
	m_instancedMeshes[instancedMeshIdx].activatedInstances[instanceIdx] = true;
	m_instancedMeshes[instancedMeshIdx].buildOffsetAndCounts();
}

void Wolf::RenderMeshList::removeInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx)
{
	m_instancedMeshes[instancedMeshIdx].activatedInstances[instanceIdx] = false;
	m_instancedMeshes[instancedMeshIdx].buildOffsetAndCounts();
}

void Wolf::RenderMeshList::addTransientInstancedMesh(const InstancedMesh& mesh, uint32_t instanceCount)
{
	m_transientInstancedMeshesNextFrame.emplace_back(mesh);
	m_transientInstancedMeshesNextFrame.back().offsetAndCounts.push_back({ 0, instanceCount });
}

void Wolf::RenderMeshList::draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& descriptorSetsToBind) const
{
	PROFILE_FUNCTION

	const Pipeline* currentPipeline = nullptr;
	for (uint32_t i = 0; i < m_transientMeshesCurrentFrame.size() + m_meshes.size() + m_transientInstancedMeshesCurrentFrame.size() + m_instancedMeshes.size(); ++i)
	{
		const MeshToRender* meshToRender = nullptr;
		const InternalInstancedMesh* internalInstancedMesh = nullptr;

		if (i < m_transientMeshesCurrentFrame.size())
			meshToRender = &m_transientMeshesCurrentFrame[i].meshToRender;
		else if (i < m_transientMeshesCurrentFrame.size() + m_meshes.size())
			meshToRender = &m_meshes[i - m_transientMeshesCurrentFrame.size()].meshToRender;
		else if (i < m_transientMeshesCurrentFrame.size() + m_meshes.size() + m_transientInstancedMeshesCurrentFrame.size())
		{
			meshToRender = &m_transientInstancedMeshesCurrentFrame[i - m_transientMeshesCurrentFrame.size() - m_meshes.size()].instancedMesh.mesh;
			internalInstancedMesh = &m_transientInstancedMeshesCurrentFrame[i - m_transientMeshesCurrentFrame.size() - m_meshes.size()];
		}
		else
		{
			meshToRender = &m_instancedMeshes[i - m_transientMeshesCurrentFrame.size() - m_meshes.size() - m_transientInstancedMeshesCurrentFrame.size()].instancedMesh.mesh;
			internalInstancedMesh = &m_instancedMeshes[i - m_transientMeshesCurrentFrame.size() - m_meshes.size() - m_transientInstancedMeshesCurrentFrame.size()];
		}

		if (meshToRender->pipelineSet->getPipelineHash(pipelineIdx) == 0)
			continue;

		std::vector<DescriptorSetBindInfo> descriptorSetsBindInfo;
		descriptorSetsBindInfo.reserve(descriptorSetsToBind.size());

		for (const AdditionalDescriptorSet& additionalDescriptorSet : descriptorSetsToBind)
		{
			if (additionalDescriptorSet.mask == 0 || additionalDescriptorSet.mask & meshToRender->pipelineSet->getCustomMask(pipelineIdx))
			{
				descriptorSetsBindInfo.push_back(additionalDescriptorSet.descriptorSetBindInfo);
			}
		}

		if (const Pipeline* meshPipeline = meshToRender->pipelineSet->getOrCreatePipeline(pipelineIdx, renderPass, meshToRender->perPipelineDescriptorSets[pipelineIdx], descriptorSetsBindInfo, *m_shaderList); meshPipeline != currentPipeline)
		{
			commandBuffer.bindPipeline(meshPipeline);
			currentPipeline = meshPipeline;

			for (const DescriptorSetBindInfo& descriptorSetToBind : descriptorSetsBindInfo)
			{
				commandBuffer.bindDescriptorSet(descriptorSetToBind.getDescriptorSet(), descriptorSetToBind.getBindingSlot(), *meshPipeline);
			}

			if (cameraIdx != NO_CAMERA_IDX)
			{
				const uint32_t cameraDescriptorSlot = meshToRender->pipelineSet->getCameraDescriptorSlot(pipelineIdx);
				if (cameraDescriptorSlot == static_cast<uint32_t>(-1))
					Debug::sendError("Trying to bind camera descriptor set but slot hasn't been defined");

				commandBuffer.bindDescriptorSet(context.cameraList->getCamera(cameraIdx)->getDescriptorSet(), cameraDescriptorSlot, *meshPipeline);
			}

			if (const uint32_t bindlessDescriptorSlot = meshToRender->pipelineSet->getBindlessDescriptorSlot(pipelineIdx); bindlessDescriptorSlot != static_cast<uint32_t>(-1))
			{
				commandBuffer.bindDescriptorSet(context.bindlessDescriptorSet, bindlessDescriptorSlot, *meshPipeline);
			}

			if (const uint32_t lightDescriptorSlot = meshToRender->pipelineSet->getLightDescriptorSlot(pipelineIdx); lightDescriptorSlot != static_cast<uint32_t>(-1))
			{
				commandBuffer.bindDescriptorSet(context.lightManager->getDescriptorSet().createConstNonOwnerResource(), lightDescriptorSlot, *meshPipeline);
			}
		}

		for (const DescriptorSetBindInfo& descriptorSetBindInfo : meshToRender->perPipelineDescriptorSets[pipelineIdx])
		{
			commandBuffer.bindDescriptorSet(descriptorSetBindInfo.getDescriptorSet(), descriptorSetBindInfo.getBindingSlot(), *currentPipeline);
		}

		if (internalInstancedMesh)
		{
			if (internalInstancedMesh->instancedMesh.instanceBuffer)
			{
				commandBuffer.bindVertexBuffer(*internalInstancedMesh->instancedMesh.instanceBuffer, 1);
			}

			for (const InternalInstancedMesh::OffsetAndCount& offsetAndCount : internalInstancedMesh->offsetAndCounts)
			{
				meshToRender->mesh->draw(commandBuffer, cameraIdx, offsetAndCount.count, offsetAndCount.offset);
			}
		}
		else
		{
			meshToRender->mesh->draw(commandBuffer, cameraIdx, 1);
		}
	}
}

void Wolf::RenderMeshList::InternalInstancedMesh::buildOffsetAndCounts()
{
	offsetAndCounts.clear();

	uint32_t startOffset = 0;
	uint32_t currentCount = 0;
	bool previous = false;

	for (uint32_t i = 0; i < activatedInstances.size(); ++i)
	{
		if (activatedInstances[i] && !previous)
		{
			startOffset = i;
			currentCount = 1;
		}
		else if (activatedInstances[i] && previous)
		{
			currentCount++;
		}
		else if (!activatedInstances[i] && previous)
		{
			offsetAndCounts.push_back({ startOffset, currentCount });
		}

		previous = activatedInstances[i];
	}

	if (previous)
	{
		offsetAndCounts.push_back({ startOffset, currentCount });
	}
}
