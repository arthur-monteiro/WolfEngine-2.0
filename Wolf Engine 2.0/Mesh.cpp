#include "Mesh.h"

void Wolf::Mesh::draw(VkCommandBuffer commandBuffer, uint32_t instanceCount) const
{
	constexpr VkDeviceSize offsets[1] = { 0 };
	const VkBuffer vertexBuffer = m_vertexBuffer->getBuffer();
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(commandBuffer, m_indexCount, instanceCount, 0, 0, 0);
}
