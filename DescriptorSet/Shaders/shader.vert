layout(binding = 0) uniform UniformBuffer
{
    float offset;
} uniformBuffer;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoords;

layout(location = 0) out vec2 outTexCoords;
 
out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
    gl_Position = vec4(inPosition.x + uniformBuffer.offset, inPosition.y, 0.0, 1.0);

    outTexCoords = inTexCoords;
} 
