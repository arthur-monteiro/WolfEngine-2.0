#pragma once

#include "Buffer.h"
#include "CommandBuffer.h"
#include "Debug.h"

namespace Wolf
{
	class Mesh
	{
	public:
		template <typename T>
		Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, VkBufferUsageFlags additionalVertexBufferUsages = 0, VkBufferUsageFlags additionalIndexBufferUsages = 0, VkFormat vertexFormat = VK_FORMAT_UNDEFINED);
		Mesh(const Mesh&) = delete;

		[[nodiscard]] uint32_t getVertexCount() const { return m_vertexCount; }
		[[nodiscard]] uint32_t getVertexSize() const { return m_vertexSize; }

		[[nodiscard]] VkFormat getVertexFormat() const
		{ 
			if (m_vertexFormat == VK_FORMAT_UNDEFINED)
			{
				Debug::sendError("Format is undefined");
			}
			return m_vertexFormat; 
		}

		[[nodiscard]] uint32_t getIndexCount() const { return m_indexCount; }
		[[nodiscard]] const Buffer& getVertexBuffer() const { return *m_vertexBuffer; }
		[[nodiscard]] const Buffer& getIndexBuffer() const { return *m_indexBuffer; }

		void draw(VkCommandBuffer commandBuffer) const;

	private:
		std::unique_ptr<Buffer> m_vertexBuffer;
		std::unique_ptr<Buffer> m_indexBuffer;

		uint32_t m_vertexCount;
		uint32_t m_vertexSize;
		VkFormat m_vertexFormat;
		uint32_t m_indexCount;
	};

	template<typename T>
	Mesh::Mesh(const std::vector<T>& vertices, const std::vector<uint32_t>& indices, VkBufferUsageFlags additionalVertexBufferUsages, VkBufferUsageFlags additionalIndexBufferUsages, VkFormat vertexFormat)
	{
		m_vertexBuffer.reset(new Buffer(sizeof(T) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | additionalVertexBufferUsages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
		m_vertexBuffer->transferCPUMemoryWithStagingBuffer((void*)vertices.data(), sizeof(T) * vertices.size());

		m_indexBuffer.reset(new Buffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | additionalIndexBufferUsages, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));
		m_indexBuffer->transferCPUMemoryWithStagingBuffer((void*)indices.data(), sizeof(uint32_t) * indices.size());

		m_vertexCount = static_cast<uint32_t>(vertices.size());
		m_vertexSize = sizeof(T);
		m_vertexFormat = vertexFormat;
		m_indexCount = static_cast<uint32_t>(indices.size());
	}
}

