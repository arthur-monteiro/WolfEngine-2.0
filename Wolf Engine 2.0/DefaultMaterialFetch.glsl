R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = �BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = �BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

struct InputTextureSetInfo
{
    uint albedoIdx;
	uint normalIdx;
	uint roughnessMetalnessAOIdx;
    uint samplingMode;

    vec3 scale;
    float pad;
};
layout(std430, binding = 2, set = �BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer TextureSetBufferLayout
{
    InputTextureSetInfo textureSetsInfo[];
};

const uint TEXTURE_SET_COUNT_PER_MATERIAL = 4;
const int SAMPLING_MODE_TEXTURE_COORDS = 0;
const int SAMPLING_MODE_TRIPLANAR = 1;
struct InputMaterialInfo
{
    uint textureSetIndices[TEXTURE_SET_COUNT_PER_MATERIAL];
    float textureSetStrengths[TEXTURE_SET_COUNT_PER_MATERIAL];
    uint shadingMode;
};
layout(std430, binding = 3, set = �BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer MaterialBufferLayout
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

void clearMaterialInfo(inout MaterialInfo materialInfo)
{
    materialInfo.albedo = vec4(0.0f);
    materialInfo.normal = vec3(0.0f);
    materialInfo.roughness = 0.0f;
    materialInfo.metalness = 0.0f;
    materialInfo.matAO = 0.0f;
    materialInfo.anisoStrength = 0.0f;

    materialInfo.sixWaysLightmap1 = vec4(0.0f);
}

void addToMaterialInfoFromTexCoords(in const vec2 texCoords, in const InputTextureSetInfo textureSetInfo, in float strength, in const mat3 matrixTBN, inout MaterialInfo materialInfo)
{
    materialInfo.albedo += texture(sampler2D(textures[textureSetInfo.albedoIdx], textureSampler), texCoords).rgba * strength;
    vec4 normal = texture(sampler2D(textures[textureSetInfo.normalIdx], textureSampler), texCoords).rgba;
    normal.xy = normal.rg * 2.0 - vec2(1.0);
    normal.z = sqrt(1.0f - normal.x * normal.x - normal.y * normal.y);
    materialInfo.normal += normal.xyz * matrixTBN * strength; // TODO: normal blending
    vec4 combinedRoughnessMetalnessAOAniso = texture(sampler2D(textures[textureSetInfo.roughnessMetalnessAOIdx], textureSampler), texCoords).rgba;
	materialInfo.roughness += combinedRoughnessMetalnessAOAniso.r * strength;
	materialInfo.metalness += combinedRoughnessMetalnessAOAniso.g * strength;
    materialInfo.matAO += combinedRoughnessMetalnessAOAniso.b * strength;
    materialInfo.anisoStrength += combinedRoughnessMetalnessAOAniso.a * strength;

    materialInfo.sixWaysLightmap1 += texture(sampler2D(textures[textureSetInfo.normalIdx], textureSampler), texCoords).rgba * strength;
}

void addToMaterialInfoTriplanr(in const InputTextureSetInfo textureSetInfo, in float strength, in const mat3 matrixTBN, in const vec3 worldPos, inout MaterialInfo materialInfo)
{
    vec3 normal = abs(vec3(matrixTBN[0][2], matrixTBN[1][2], matrixTBN[2][2]));

    // normalized total value to 1.0
    float b = (normal.x + normal.y + normal.z);
    normal /= b;

    vec3 usedWorldPos = worldPos * textureSetInfo.scale;

#define MOD_TEX_COORDS(vec) vec2(mod(vec.x, 1.0f), mod(vec.y, 1.0f))
    MaterialInfo xAxis;
    clearMaterialInfo(xAxis);
    addToMaterialInfoFromTexCoords(MOD_TEX_COORDS(usedWorldPos.yz), textureSetInfo, 1.0f, matrixTBN, xAxis);
    MaterialInfo yAxis;
    clearMaterialInfo(yAxis);
    addToMaterialInfoFromTexCoords(MOD_TEX_COORDS(usedWorldPos.xz), textureSetInfo, 1.0f, matrixTBN, yAxis);
    MaterialInfo zAxis;
    clearMaterialInfo(zAxis);
    addToMaterialInfoFromTexCoords(MOD_TEX_COORDS(usedWorldPos.xy), textureSetInfo, 1.0f, matrixTBN, zAxis);

#define LERP_INFO(name) (xAxis.name * normal.x + yAxis.name * normal.y + zAxis.name * normal.z)
    materialInfo.albedo += LERP_INFO(albedo) * strength;
    materialInfo.normal += LERP_INFO(normal) * strength;
    materialInfo.roughness += LERP_INFO(roughness) * strength;
    materialInfo.metalness += LERP_INFO(metalness) * strength;
    materialInfo.matAO += LERP_INFO(matAO) * strength;
    materialInfo.anisoStrength += LERP_INFO(anisoStrength) * strength;
    materialInfo.sixWaysLightmap1 += LERP_INFO(sixWaysLightmap1) * strength;
}

void addToMaterialInfo(in const vec2 texCoords, in const InputTextureSetInfo textureSetInfo, in float strength, in const mat3 matrixTBN, in const vec3 worldPos, inout MaterialInfo materialInfo)
{
    if (textureSetInfo.samplingMode == SAMPLING_MODE_TEXTURE_COORDS)
        addToMaterialInfoFromTexCoords(texCoords * textureSetInfo.scale.xy, textureSetInfo, strength, matrixTBN, materialInfo);
    else if (textureSetInfo.samplingMode == SAMPLING_MODE_TRIPLANAR)
        addToMaterialInfoTriplanr(textureSetInfo, strength, matrixTBN, worldPos, materialInfo);
}

#ifdef DEFAULT_MATERIAL_FETCH
MaterialInfo fetchMaterial(in const vec2 texCoords, in const uint materialIdx, in const mat3 matrixTBN, in const vec3 worldPos)
#else
MaterialInfo defaultFetchMaterial(in const vec2 texCoords, in const uint materialIdx, in const mat3 matrixTBN, in const vec3 worldPos)
#endif
{
    MaterialInfo materialInfo;
    clearMaterialInfo(materialInfo);
    materialInfo.shadingMode = materialsInfo[materialIdx].shadingMode;

    float totalStrengths = 0.0f;
    for (uint i = 0; i < TEXTURE_SET_COUNT_PER_MATERIAL; ++i)
    {
        uint textureSetIdx = materialsInfo[materialIdx].textureSetIndices[i];
        if (textureSetIdx == 0 && i > 0)
            break;

        float strength = materialsInfo[materialIdx].textureSetStrengths[i];
        totalStrengths += strength;

        InputTextureSetInfo textureSetInfo = textureSetsInfo[materialsInfo[materialIdx].textureSetIndices[i]];
        addToMaterialInfo(texCoords, textureSetInfo, strength, matrixTBN, worldPos, materialInfo);
    }

    materialInfo.albedo /= totalStrengths;
    materialInfo.normal /= totalStrengths;
    materialInfo.roughness /= totalStrengths;
    materialInfo.metalness /= totalStrengths;
    materialInfo.matAO /= totalStrengths;
    materialInfo.anisoStrength /= totalStrengths;

    materialInfo.sixWaysLightmap0 = materialInfo.albedo;
    materialInfo.sixWaysLightmap1 /= totalStrengths;

    return materialInfo;    
}
)"