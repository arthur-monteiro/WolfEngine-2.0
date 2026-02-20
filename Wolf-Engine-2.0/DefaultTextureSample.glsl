R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = @BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = @BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

vec4 sampleTexture(in uint textureIdx, in const vec2 texCoords, in const uint unused)
{
	return texture(sampler2D(textures[nonuniformEXT(textureIdx)], textureSampler), texCoords).rgba;
}
)"