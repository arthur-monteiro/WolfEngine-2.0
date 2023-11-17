layout (binding = 0) uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoords;

layout(location = 0) out vec4 outColor;

void main() 
{
	outColor = vec4(texture(tex, inTexCoords).rgba);
}
