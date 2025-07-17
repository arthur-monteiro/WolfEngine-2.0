R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = £BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = £BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

const uint PAGE_SIZE = 256;

const uint INVALID_INDIRECTION_OFFSET = -1;
struct TextureInfo
{
    uint width;
	uint height;

    uint virtualTextureIndirectionOffset;
};
layout(std430, binding = 4, set = £BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer TexturesInfoBuffer
{
    TextureInfo texturesInfo[];
};

layout(std430, binding = 5, set = £BINDLESS_DESCRIPTOR_SLOT) buffer VirtualTextureFeedbackBuffer
{
    uint currentFeedackCount;
	uint feedbacks[255];
};

layout (binding = 6, set = £BINDLESS_DESCRIPTOR_SLOT) uniform texture2D albedoAtlas;

const uint MAX_INDIRECTION_COUNT = 16384;
const uint INVALID_INDIRECTION = -1;
layout(std430, binding = 7, set = £BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer VirtualTextureIndirectionBuffer
{
    uint virtualTextureIndirection[MAX_INDIRECTION_COUNT];
};

void requestSlice(in uint textureIdx, in const uint sliceX, in const uint sliceY, in const uint mipLevel);

const uint PAGE_COUNT_PER_SIDE = 16u;
vec4 sampleTexture(in uint textureIdx, in const vec2 texCoords, bool requestVT /* temp to disable normal and other non supported textures */)
{
    // No textures placeholder
    if (textureIdx < 3)
    {
        return texture(sampler2D(textures[textureIdx], textureSampler), texCoords).rgba;
    }
    else if (!requestVT)
    {
        return vec4(0, 0, 0, 1);
    }

    vec2 textureSize = vec2(float(texturesInfo[textureIdx].width), float(texturesInfo[textureIdx].height));

    vec2 texCoordsTexelUnits = texCoords * textureSize;
    vec2 dxTexCoords = dFdx(texCoordsTexelUnits);
    vec2 dyTexCoords = dFdy(texCoordsTexelUnits);
    float deltaMaxSqr = max(dot(dxTexCoords, dxTexCoords), dot(dyTexCoords, dyTexCoords));
    uint mipLevel = uint(0.5 * log2(deltaMaxSqr)); // log2(x ^ y) = y * log2(x)

    uint maxMipCount = computeMipCount(texturesInfo[textureIdx].width, texturesInfo[textureIdx].height);
    mipLevel = min(mipLevel, maxMipCount - 1);

    uint textureWidth = uint(textureSize.x) >> mipLevel;
    uint textureHeight = uint(textureSize.y) >> mipLevel;

    uint sliceX = uint((fract(texCoords.x) * float(textureWidth)) / float(PAGE_SIZE));
    uint sliceY = uint((fract(texCoords.y) * float(textureHeight)) / float(PAGE_SIZE));

    uint indirectionId = computeVirtualTextureIndirectionId(sliceX, sliceY, uint(textureSize.x / float(PAGE_SIZE)), uint(textureSize.y / float(PAGE_SIZE)), mipLevel);
    uint indirectionInfo = INVALID_INDIRECTION;
    if (texturesInfo[textureIdx].virtualTextureIndirectionOffset != INVALID_INDIRECTION_OFFSET)
    {
        indirectionInfo = virtualTextureIndirection[texturesInfo[textureIdx].virtualTextureIndirectionOffset + indirectionId];
    }

    if (indirectionInfo == INVALID_INDIRECTION)
    {
        requestSlice(textureIdx, sliceX, sliceY, mipLevel);
    }

    if (indirectionInfo != INVALID_INDIRECTION)
    {
        vec2 pageCountForMip = vec2(float(textureWidth), float(textureHeight)) / float(PAGE_SIZE);
        vec2 uvOffset = vec2(sliceX, sliceY) / pageCountForMip; 
        vec2 uvInSlice = (fract(texCoords) - uvOffset) * pageCountForMip;

        uint indirectionX = indirectionInfo % PAGE_COUNT_PER_SIDE;
        uint indirectionY = indirectionInfo / PAGE_COUNT_PER_SIDE;
        vec2 indirection = vec2(float(indirectionX), float(indirectionY));
        vec2 sliceUVOffset = indirection / float(PAGE_COUNT_PER_SIDE);

        float borderOffset = 4.0f / (float(PAGE_COUNT_PER_SIDE) * 264.0f);
        vec2 sliceSize = vec2(float(PAGE_SIZE), float(PAGE_SIZE));
        vec2 scale = (sliceSize / ((sliceSize + 2.0f * vec2(4.0f, 4.0f)) * float(PAGE_COUNT_PER_SIDE)));
        vec2 finalUV = sliceUVOffset + uvInSlice * scale + vec2(borderOffset, borderOffset);

        return texture(sampler2D(albedoAtlas, textureSampler), finalUV).rgba;
    }
    else
        return vec4(1, 0, 0, 1);
}

void requestSlice(in uint textureIdx, in const uint sliceX, in const uint sliceY, in const uint mipLevel)
{
    uint feedbackIndex = atomicAdd(currentFeedackCount, 1);
    if (feedbackIndex < 255)
    {
        uint feedbackValue = (textureIdx & 0x7FF) << 21 | (mipLevel & 0x1F) << 16 | (sliceX & 0xFF) << 8 | (sliceY & 0xFF);
        feedbacks[feedbackIndex] = feedbackValue;
    }
}
)"