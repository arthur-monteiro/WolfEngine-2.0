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
    vec4 albedo;
    vec3 normal;
    float roughness;
    float metalness;
    float matAO;
    float anisoStrength;

    vec4 sixWaysLightmap0;
    vec4 sixWaysLightmap1;

    uint shadingMode;
};

MaterialInfo fetchMaterial(in const vec2 texCoords, in const uint materialIdx, in const mat3 matrixTBN, in const vec3 worldPos)
{
    MaterialInfo materialInfo;

    materialInfo.albedo = texture(sampler2D(textures[materialsInfo[materialIdx].albedoIdx], textureSampler), texCoords).rgba;
    vec4 normal = texture(sampler2D(textures[materialsInfo[materialIdx].normalIdx], textureSampler), texCoords).rgba;
    materialInfo.normal = (normal.rgb * 2.0 - vec3(1.0)) * matrixTBN;
    vec4 combinedRoughnessMetalnessAOAniso = texture(sampler2D(textures[materialsInfo[materialIdx].roughnessMetalnessAOIdx], textureSampler), texCoords).rgba;
	materialInfo.roughness = combinedRoughnessMetalnessAOAniso.r;
	materialInfo.metalness = combinedRoughnessMetalnessAOAniso.g;
    materialInfo.matAO = combinedRoughnessMetalnessAOAniso.b;
    materialInfo.anisoStrength = combinedRoughnessMetalnessAOAniso.a;
    materialInfo.shadingMode = materialsInfo[materialIdx].shadingMode;

    materialInfo.sixWaysLightmap0  = materialInfo.albedo;
    materialInfo.sixWaysLightmap1  = normal;

    return materialInfo;
}
)"