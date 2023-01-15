#pragma once

#include <iostream>

#include "Buffer.h"
#include "CommandBuffer.h"

namespace Wolf
{
	class Mesh
	{
	public:
		template <typename T>
		Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices);
		Mesh(const Mesh&) = delete;

		void draw(VkCommandBuffer commandBuffer);

	private:
		std::unique_ptr<Wolf::Buffer> m_vertexBuffer;
		std::unique_ptr<Wolf::Buffer> m_indexBuffer;

		uint32_t m_indexCount;
	};

	template<typename T>
	inline Mesh::Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices)
	{
		m_vertexBuffer.reset(new Buffer(sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
		m_vertexBuffer->transferCPUMemoryWithStagingBuffer((void*)vertices.data(), sizeof(T) * vertices.size());

		m_indexBuffer.reset(new Buffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
		m_indexBuffer->transferCPUMemoryWithStagingBuffer((void*)indices.data(), sizeof(uint32_t) * indices.size());

		m_indexCount = indices.size();
	}
}

