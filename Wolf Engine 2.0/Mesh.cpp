#include "Mesh.h"

#include "CameraInterface.h"
#include "RenderMeshList.h"

void Wolf::Mesh::addSubMesh(uint32_t indicesOffset, uint32_t indexCount, AABB aabb)
{
	if (!m_subMeshes.empty() && m_subMeshes.back()->getIndicesOffset() + m_subMeshes.back()->getIndexCount() != indicesOffset)
		Debug::sendError("Adding submesh which is not just after the previous one. This is required to have an efficient merge");

	m_subMeshes.push_back(std::make_unique<SubMesh>(indicesOffset, indexCount, aabb));
}

void Wolf::Mesh::cullForCamera(uint32_t cameraIdx, const CameraInterface* camera, const glm::mat4& transform, bool isInstanced)
{
	if (m_subMeshes.empty())
		return;

	std::vector<std::unique_ptr<SubMeshToDrawInfo>>& subMeshes = m_subMeshesToDrawByCamera[cameraIdx];
	subMeshes.clear();

	// TODO: find a way to cull by instance
	if (!isInstanced && !camera->isAABBVisible(m_AABB * transform))
		return;

	// TODO : add frustum culling by sub mesh
	subMeshes.push_back(std::make_unique<SubMeshToDrawInfo>(SubMeshToDrawInfo{ m_subMeshes[0]->getIndicesOffset(), m_subMeshes[0]->getIndexCount() }));
	SubMeshToDrawInfo* currentSubMesh = subMeshes.back().get();
	for (uint32_t subMeshIdx = 1; subMeshIdx < m_subMeshes.size(); ++subMeshIdx)
	{
		const SubMesh* subMesh = m_subMeshes[subMeshIdx].get();
		if (subMesh->getIndicesOffset() == currentSubMesh->indicesOffset + currentSubMesh->indexCount)
		{
			currentSubMesh->indexCount += subMesh->getIndexCount();
		}
		else
		{
			subMeshes.push_back(std::make_unique<SubMeshToDrawInfo>(SubMeshToDrawInfo{ subMesh->getIndicesOffset(), subMesh->getIndexCount() }));
			currentSubMesh = subMeshes.back().get();
		}
	}
}

void Wolf::Mesh::draw(const CommandBuffer& commandBuffer, uint32_t cameraIdx, uint32_t instanceCount) const
{
	commandBuffer.bindVertexBuffer(*m_vertexBuffer);
	commandBuffer.bindIndexBuffer(*m_indexBuffer, IndexType::U32);

	if(m_subMeshes.empty() || cameraIdx == RenderMeshList::NO_CAMERA_IDX)
	{
		if (m_subMeshes.empty())
			Debug::sendMessageOnce("Drawing a mesh without any submesh. The entire mesh will be drawn", Debug::Severity::WARNING, this);
		commandBuffer.drawIndexed(m_indexCount, instanceCount, 0, 0, 0);
	}
	else
	{
		for (const std::unique_ptr<SubMeshToDrawInfo>& subMesh : m_subMeshesToDrawByCamera.at(cameraIdx))
		{
			commandBuffer.drawIndexed(subMesh->indexCount, instanceCount, subMesh->indicesOffset, 0, 0);
		}
	}
}
