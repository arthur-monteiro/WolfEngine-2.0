R"(
#extension GL_EXT_nonuniform_qualifier : enable

layout (binding = 0, set = @BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] textures;
layout (binding = 1, set = @BINDLESS_DESCRIPTOR_SLOT) uniform sampler textureSampler;

const uint PAGE_SIZE = 256;

const uint INVALID_INDIRECTION_OFFSET = -1;
struct TextureInfo
{
    uint width;
	uint height;

    uint virtualTextureIndirectionOffset;
};
layout(std430, binding = 4, set = @BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer TexturesInfoBuffer
{
    TextureInfo texturesInfo[];
};

const uint DITHER_PIXEL_COUNT_PER_SIDE = 32;
layout(std430, binding = 5, set = @BINDLESS_DESCRIPTOR_SLOT) buffer VirtualTextureFeedbackBuffer
{
	uvec3 feedbacks[];
};

layout (binding = 6, set = @BINDLESS_DESCRIPTOR_SLOT) uniform texture2D[] atlases;
layout (binding = 7, set = @BINDLESS_DESCRIPTOR_SLOT) uniform sampler atlasesSampler;

const uint MAX_INDIRECTION_COUNT = 16384;
const uint INVALID_INDIRECTION = -1;
layout(std430, binding = 8, set = @BINDLESS_DESCRIPTOR_SLOT) readonly restrict buffer VirtualTextureIndirectionBuffer
{
    uint virtualTextureIndirection[MAX_INDIRECTION_COUNT];
};

float rand2(vec2 co)
{
    return max(fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453), 0.25);
}

vec3 randomColor(uint seed)
{
    vec2 co = vec2(float(seed), float(seed / 4952000));
    return vec3(rand2(co), rand2(co + vec2(47)), rand2(co + vec2(29214)));
}

void requestSlice(in uint textureIdx, in const uint sliceX, in const uint sliceY, in const uint mipLevel, in const uint atlasIdx);
vec4 fetchForIndirectionInfo(in uint indirectionInfo, in uint textureWidth, in uint textureHeight, in uint sliceX, in uint sliceY, in vec2 texCoords, in uint atlasIdx);
void computeInfoForMipLevel(in uint mipLevel, in vec2 textureSize, in vec2 texCoords, in uint textureIdx, out uint indirectionInfo, out uint textureWidth, out uint textureHeight, out uint sliceX, out uint sliceY);

const uint PAGE_COUNT_PER_SIDE = 16u;
vec4 sampleTexture(in uint textureIdx, in const vec2 texCoords, in const uint atlasIdx)
{
    // No textures placeholder
    if (textureIdx < 3)
    {
        return texture(sampler2D(textures[textureIdx], textureSampler), texCoords).rgba;
    }
    else if (atlasIdx == -1)
    {
        return vec4(0, 0, 0, 1);
    }

    vec2 textureSize = vec2(float(texturesInfo[textureIdx].width), float(texturesInfo[textureIdx].height));

    vec2 texCoordsTexelUnits = texCoords * textureSize;
    vec2 dxTexCoords = dFdx(texCoordsTexelUnits);
    vec2 dyTexCoords = dFdy(texCoordsTexelUnits);
    float deltaMinSqr = min(dot(dxTexCoords, dxTexCoords), dot(dyTexCoords, dyTexCoords));
    float mipLevel = 0.5 * log2(deltaMinSqr); // log2(x ^ y) = y * log2(x)

    uint maxMipCount = computeMipCount(texturesInfo[textureIdx].width, texturesInfo[textureIdx].height);
    mipLevel = min(mipLevel, maxMipCount - 1);

    float bestMipLevelFactor = 1.0 - (mipLevel - uint(mipLevel));

    uint indirectionInfo;
    uint textureWidth;
    uint textureHeight;
    uint sliceX;
    uint sliceY;
    computeInfoForMipLevel(uint(mipLevel), textureSize, texCoords, textureIdx, indirectionInfo, textureWidth, textureHeight, sliceX, sliceY);
    
    requestSlice(textureIdx, sliceX, sliceY, uint(mipLevel), atlasIdx);

    if (indirectionInfo != INVALID_INDIRECTION)
    {
        vec4 currentMipValue = fetchForIndirectionInfo(indirectionInfo, textureWidth, textureHeight, sliceX, sliceY, texCoords, atlasIdx);

        if (bestMipLevelFactor < 0.99)
        {
            computeInfoForMipLevel(uint(mipLevel) + 1, textureSize, texCoords, textureIdx, indirectionInfo, textureWidth, textureHeight, sliceX, sliceY);
            currentMipValue = currentMipValue * bestMipLevelFactor + fetchForIndirectionInfo(indirectionInfo, textureWidth, textureHeight, sliceX, sliceY, texCoords, atlasIdx) * (1.0 - bestMipLevelFactor);
        }

        return currentMipValue;
    }
    else
        return vec4(1, 0, 0, 1);
}

void requestSlice(in uint textureIdx, in const uint sliceX, in const uint sliceY, in const uint mipLevel, in const uint atlasIdx)
{ 
    uint pixelIndex = uint(gl_FragCoord.x) % DITHER_PIXEL_COUNT_PER_SIDE + (uint(gl_FragCoord.y) % DITHER_PIXEL_COUNT_PER_SIDE) * DITHER_PIXEL_COUNT_PER_SIDE;
    if (getCameraFrameIndex() % (DITHER_PIXEL_COUNT_PER_SIDE * DITHER_PIXEL_COUNT_PER_SIDE) == pixelIndex)
    {
        uint feedbackValue = (textureIdx & 0x7FF) << 21 | (mipLevel & 0x1F) << 16 | (sliceX & 0xFF) << 8 | (sliceY & 0xFF);
        feedbacks[uint(gl_FragCoord.x) / DITHER_PIXEL_COUNT_PER_SIDE + (uint(gl_FragCoord.y) / DITHER_PIXEL_COUNT_PER_SIDE) * (getScreenWidth() / DITHER_PIXEL_COUNT_PER_SIDE)][atlasIdx] = feedbackValue;
    }
}

void computeInfoForMipLevel(in uint mipLevel, in vec2 textureSize, in vec2 texCoords, in uint textureIdx, out uint indirectionInfo, out uint textureWidth, out uint textureHeight, out uint sliceX, out uint sliceY)
{
    textureWidth = uint(textureSize.x) >> mipLevel;
    textureHeight = uint(textureSize.y) >> mipLevel;

    sliceX = uint((fract(texCoords.x) * float(textureWidth)) / float(PAGE_SIZE));
    sliceY = uint((fract(texCoords.y) * float(textureHeight)) / float(PAGE_SIZE));

    uint indirectionId = computeVirtualTextureIndirectionId(sliceX, sliceY, uint(textureSize.x / float(PAGE_SIZE)), uint(textureSize.y / float(PAGE_SIZE)), mipLevel);
    indirectionInfo = INVALID_INDIRECTION;
    if (texturesInfo[textureIdx].virtualTextureIndirectionOffset != INVALID_INDIRECTION_OFFSET)
    {
        indirectionInfo = virtualTextureIndirection[texturesInfo[textureIdx].virtualTextureIndirectionOffset + indirectionId];
    }
}

vec4 fetchForIndirectionInfo(in uint indirectionInfo, in uint textureWidth, in uint textureHeight, in uint sliceX, in uint sliceY, in vec2 texCoords, in uint atlasIdx)
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

    //return vec4(randomColor(indirectionInfo), 1.0);
    return texture(sampler2D(atlases[atlasIdx], atlasesSampler), finalUV).rgba;
}
)"