#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Vertex2DTextured
{
	glm::vec2 pos;
	glm::vec2 texCoord;

	static void getBindingDescription(Wolf::VertexInputBindingDescription& bindingDescription, uint32_t binding)
	{
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(Vertex2DTextured);
		bindingDescription.inputRate = Wolf::VertexInputRate::VERTEX;
	}

	static void getAttributeDescriptions(std::vector<Wolf::VertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
	{
		attributeDescriptions.resize(2);

		attributeDescriptions[0].binding = binding;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = Wolf::Format::R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex2DTextured, pos);

		attributeDescriptions[1].binding = binding;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = Wolf::Format::R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex2DTextured, texCoord);
	}

	bool operator==(const Vertex2DTextured& other) const
	{
		return pos == other.pos && texCoord == other.texCoord;
	}
};