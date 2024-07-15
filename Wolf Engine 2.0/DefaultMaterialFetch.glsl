R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = £BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = £BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

struct InputMaterialInfo
{
    uint albedoIdx;
	uint normalIdx;
	uint roughnessMetalnessAOIdx;

    uint shadingMode;
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
    float anisoStrength;

    uint shadingMode;
};

MaterialInfo fetchMaterial(in const vec2 texCoords, in const uint materialId, in const mat3 matrixTBN, in const vec3 worldPos)
{
    MaterialInfo materialInfo;

    materialInfo.albedo = texture(sampler2D(textures[materialsInfo[materialId].albedoIdx], textureSampler), texCoords).rgb;
    materialInfo.normal = (texture(sampler2D(textures[materialsInfo[materialId].normalIdx], textureSampler), texCoords).rgb * 2.0 - vec3(1.0)) * matrixTBN;
    vec4 combinedRoughnessMetalnessAOAniso = texture(sampler2D(textures[materialsInfo[materialId].roughnessMetalnessAOIdx], textureSampler), texCoords).rgba;
	materialInfo.roughness = combinedRoughnessMetalnessAOAniso.r;
	materialInfo.metalness = combinedRoughnessMetalnessAOAniso.g;
    materialInfo.matAO = combinedRoughnessMetalnessAOAniso.b;
    materialInfo.anisoStrength = combinedRoughnessMetalnessAOAniso.a;
    materialInfo.shadingMode = materialsInfo[materialId].shadingMode;

    return materialInfo;
}
)"