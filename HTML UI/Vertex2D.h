#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex2D
{
	glm::vec2 pos;

	static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription, uint32_t binding)
	{
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(Vertex2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	}

	static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
	{
		attributeDescriptions.resize(1);

		attributeDescriptions[0].binding = binding;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2D, pos);
	}

	bool operator==(const Vertex2D& other) const
	{
		return pos == other.pos;
	}
};