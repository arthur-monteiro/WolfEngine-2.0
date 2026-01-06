#pragma once

#include <glm/glm.hpp>
#include <vector>

#include <VertexInputs.h>

struct Vertex2D
{
    glm::vec2 pos;

    static void getBindingDescription(Wolf::VertexInputBindingDescription& bindingDescription, uint32_t binding)
    {
        bindingDescription.binding = binding;
        bindingDescription.stride = sizeof(Vertex2D);
        bindingDescription.inputRate = Wolf::VertexInputRate::VERTEX;
    }

    static void getAttributeDescriptions(std::vector<Wolf::VertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
    {
        attributeDescriptions.resize(1);

        attributeDescriptions[0].binding = binding;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = Wolf::Format::R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex2D, pos);
    }

    bool operator==(const Vertex2D& other) const
    {
        return pos == other.pos;
    }
};