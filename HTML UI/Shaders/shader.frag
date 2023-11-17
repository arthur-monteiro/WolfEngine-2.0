layout(binding = 0) uniform UniformBuffer
{
    vec3 color;
} uniformBuffer;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(uniformBuffer.color, 1.0);
}
