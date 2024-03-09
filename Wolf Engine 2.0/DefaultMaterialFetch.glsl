R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = £BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = £BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

struct InputMaterialInfo
{
    uint albedoIdx;
	uint normalIdx;
	uint roughnessIdx;
	uint metalnessIdx;
	uint aoIdx;
};

layout(std430, binding = 2, set = £BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer MaterialBufferLayout
{
    InputMaterialInfo materialsInfo[];
};

struct MaterialInfo
{
    vec3 albedo;
    vec3 normal;
    float roughness;
    float metalness;
    float matAO;
};

MaterialInfo fetchMaterial(in const vec2 texCoords, in const uint materialId, in const mat3 matrixTBN)
{
    MaterialInfo materialInfo;

    materialInfo.albedo = texture(sampler2D(textures[materialsInfo[materialId].albedoIdx], textureSampler), texCoords).rgb;
    materialInfo.normal = (texture(sampler2D(textures[materialId * 5    + 1], textureSampler), texCoords).rgb * 2.0 - vec3(1.0)) * matrixTBN;
	materialInfo.roughness = texture(sampler2D(textures[materialId * 5  + 2], textureSampler), texCoords).r;
	materialInfo.metalness = texture(sampler2D(textures[materialId * 5  + 3], textureSampler), texCoords).r;
    materialInfo.matAO = texture(sampler2D(textures[materialId * 5      + 4], textureSampler), texCoords).r;

    return materialInfo;
}
)"