#include "TextureSetLoader.h"

#include <filesystem>
#include <fstream>

#include "CodeFileHashes.h"
#include "ImageFileLoader.h"
#include "MipMapGenerator.h"
#include "Timer.h"

Wolf::TextureSetLoader::TextureSetLoader(const TextureSetFileInfoGGX& textureSet, const OutputLayout& outputLayout, bool useCache) : m_useCache(useCache)
{
	Debug::sendInfo("Loading material " + textureSet.name);
	Timer globalTimer("Loading material " + textureSet.name);

	// Albedo
	if (!textureSet.albedo.empty())
	{
		Debug::sendInfo("Loading albedo " + textureSet.albedo);
		Timer albedoTimer("Loading albedo " + textureSet.albedo);

		if (!createImageFileFromCache(textureSet.albedo, true, outputLayout.albedoCompression, 0))
		{
			createImageFileFromSource<ImageCompression::RGBA8>(textureSet.albedo, true, outputLayout.albedoCompression, 0);
		}
	}

	// Normal
	if (!textureSet.normal.empty())
	{
		Debug::sendInfo("Loading normal " + textureSet.normal);
		Timer albedoTimer("Loading normal " + textureSet.normal);

		if (!createImageFileFromCache(textureSet.normal, false, outputLayout.normalCompression, 1))
		{
			createImageFileFromSource<ImageCompression::RG32F>(textureSet.normal, false, outputLayout.normalCompression, 1);
		}
	}

	std::vector<ImageCompression::RGBA8> combinedRoughnessMetalnessAOAniso;
	std::vector<std::vector<ImageCompression::RGBA8>> combinedRoughnessMetalnessAOMipLevels;
	VkExtent3D combinedRoughnessMetalnessAOExtent = { 1, 1, 1};
	combinedRoughnessMetalnessAOAniso.resize(1);

	// Roughness
	if (!textureSet.roughness.empty())
	{
		Debug::sendInfo("Loading roughness " + textureSet.roughness);
		Timer albedoTimer("Loading roughness " + textureSet.roughness);

		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		loadImageFile(textureSet.roughness, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, combinedRoughnessMetalnessAOExtent);

		combinedRoughnessMetalnessAOAniso.resize(pixels.size());
		for (uint32_t i = 0; i < pixels.size(); ++i)
		{
			combinedRoughnessMetalnessAOAniso[i].r = pixels[i].r;
		}
		combinedRoughnessMetalnessAOMipLevels.resize(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			combinedRoughnessMetalnessAOMipLevels[i].resize(mipLevels[i].size());
			for (uint32_t j = 0; j < mipLevels[i].size(); ++j)
			{
				combinedRoughnessMetalnessAOMipLevels[i][j].r = mipLevels[i][j].r;
			}
		}
	}

	// Metalness
	if (!textureSet.metalness.empty())
	{
		Debug::sendInfo("Loading metalness " + textureSet.metalness);
		Timer albedoTimer("Loading metalness " + textureSet.metalness);

		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(textureSet.metalness, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		if (extent.width != combinedRoughnessMetalnessAOExtent.width || extent.height != combinedRoughnessMetalnessAOExtent.height)
		{
			Debug::sendWarning("Metalness has not same resolution than roughness, this is not supported and will be set to default value");
		}
		else
		{
			for (uint32_t i = 0; i < pixels.size(); ++i)
			{
				combinedRoughnessMetalnessAOAniso[i].g = pixels[i].r;
			}
			for (uint32_t i = 0; i < mipLevels.size(); ++i)
			{
				for (uint32_t j = 0; j < mipLevels[i].size(); ++j)
				{
					combinedRoughnessMetalnessAOMipLevels[i][j].g = mipLevels[i][j].r;
				}
			}
		}
	}

	// AO
	if (!textureSet.ao.empty())
	{
		Debug::sendInfo("Loading ao " + textureSet.ao);
		Timer albedoTimer("Loading ao " + textureSet.ao);

		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(textureSet.ao, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		if (extent.width != combinedRoughnessMetalnessAOExtent.width || extent.height != combinedRoughnessMetalnessAOExtent.height)
		{
			Debug::sendWarning("AO has not same resolution than roughness, this is not supported and will be set to default value");
		}
		else
		{
			for (uint32_t i = 0; i < pixels.size(); ++i)
			{
				combinedRoughnessMetalnessAOAniso[i].b = pixels[i].r;
			}
			for (uint32_t i = 0; i < mipLevels.size(); ++i)
			{
				for (uint32_t j = 0; j < mipLevels[i].size(); ++j)
				{
					combinedRoughnessMetalnessAOMipLevels[i][j].b = mipLevels[i][j].r;
				}
			}
		}
	}

	// Anisotropy strength
	if (!textureSet.anisoStrength.empty())
	{
		Debug::sendInfo("Loading anisoStrength " + textureSet.anisoStrength);
		Timer albedoTimer("Loading anisoStrength " + textureSet.anisoStrength);

		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(textureSet.anisoStrength, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		if (extent.width != combinedRoughnessMetalnessAOExtent.width || extent.height != combinedRoughnessMetalnessAOExtent.height)
		{
			Debug::sendWarning("Anisotropic strength has not same resolution than roughness, this is not supported and will be set to default value");
		}
		else
		{
			for (uint32_t i = 0; i < pixels.size(); ++i)
			{
				combinedRoughnessMetalnessAOAniso[i].a = pixels[i].r;
			}
			for (uint32_t i = 0; i < mipLevels.size(); ++i)
			{
				for (uint32_t j = 0; j < mipLevels[i].size(); ++j)
				{
					combinedRoughnessMetalnessAOMipLevels[i][j].a = mipLevels[i][j].r;
				}
			}
		}
	}

	if (!textureSet.roughness.empty())
	{
		Debug::sendInfo("Creating combined texture");
		Timer albedoTimer("Creating combined texture");

		std::vector<const unsigned char*> mipsData(combinedRoughnessMetalnessAOMipLevels.size());
		for (uint32_t i = 0; i < combinedRoughnessMetalnessAOMipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAOMipLevels[i].data());
		}
		createImageFromData(combinedRoughnessMetalnessAOExtent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAOAniso.data()), mipsData, 2);
	}
}

Wolf::TextureSetLoader::TextureSetLoader(const TextureSetFileInfoSixWayLighting& textureSet, bool useCache)
{
	if (!textureSet.tex0.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(textureSet.tex0, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 0);
	}

	if (!textureSet.tex1.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(textureSet.tex1, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 1);
	}
}

Wolf::Image* Wolf::TextureSetLoader::releaseImage(uint32_t idx)
{
	return m_outputs[idx].release();
}

void Wolf::TextureSetLoader::assignCache(uint32_t idx, std::vector<unsigned char>& output)
{
	output = std::move(m_imagesData[idx]);
}

void Wolf::TextureSetLoader::loadImageFile(const std::string& filename, VkFormat format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, VkExtent3D& outExtent) const
{
	const ImageFileLoader imageFileLoader(filename);

	if (imageFileLoader.getCompression() == ImageCompression::Compression::BC5)
	{
		__debugbreak(); // deadcode?

		std::vector<ImageCompression::RG8> RG8Pixels;
		ImageCompression::uncompressImage(imageFileLoader.getCompression(), imageFileLoader.getPixels(), { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()) }, RG8Pixels);

		pixels.resize(RG8Pixels.size());
		auto computeZ = [](const ImageCompression::RG8& pixel)
			{
				auto u8ToFloat = [](uint8_t value) { return 2.0f * static_cast<float>(value) / 255.0f - 1.0f; };
				return static_cast<uint8_t>(((glm::sqrt(1.0f - u8ToFloat(pixel.r) * u8ToFloat(pixel.r) - u8ToFloat(pixel.g) * u8ToFloat(pixel.g)) + 1.0f) * 0.5f) * 255.0f);
			};
		for (uint32_t i = 0; i < pixels.size(); ++i)
		{
			pixels[i] = ImageCompression::RGBA8(RG8Pixels[i].r, RG8Pixels[i].g, computeZ(RG8Pixels[i]), 255);
		}
	}
	else if (imageFileLoader.getCompression() != ImageCompression::Compression::NO_COMPRESSION)
	{
		ImageCompression::uncompressImage(imageFileLoader.getCompression(), imageFileLoader.getPixels(), { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()) }, pixels);
	}
	else
	{
		pixels.assign(reinterpret_cast<ImageCompression::RGBA8*>(imageFileLoader.getPixels()),
			reinterpret_cast<ImageCompression::RGBA8*>(imageFileLoader.getPixels()) + static_cast<size_t>(imageFileLoader.getWidth()) * imageFileLoader.getHeight());
	}

	const MipMapGenerator mipmapGenerator(reinterpret_cast<const unsigned char*>(pixels.data()), { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()) }, format);

	mipLevels.resize(mipmapGenerator.getMipLevelCount() - 1);
	for (uint32_t mipLevel = 1; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
	{
		const std::vector<unsigned char>& mipPixels = mipmapGenerator.getMipLevel(mipLevel);
		const ImageCompression::RGBA8* mipPixelsAsColor = reinterpret_cast<const ImageCompression::RGBA8*>(mipPixels.data());
		mipLevels[mipLevel - 1] = std::vector<ImageCompression::RGBA8>(mipPixelsAsColor, mipPixelsAsColor + mipPixels.size() / sizeof(ImageCompression::RGBA8));
	}

	outExtent = { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()), 1 };
}

void Wolf::TextureSetLoader::loadImageFile(const std::string& filename, VkFormat format, std::vector<ImageCompression::RG32F>& pixels, std::vector<std::vector<ImageCompression::RG32F>>& mipLevels, VkExtent3D& outExtent) const
{
	const ImageFileLoader imageFileLoader(filename, false);

	if (imageFileLoader.getCompression() == ImageCompression::Compression::BC5)
	{
		Debug::sendCriticalError("Unhandled");
	}
	else if (imageFileLoader.getCompression() != ImageCompression::Compression::NO_COMPRESSION)
	{
		Debug::sendCriticalError("Unhandled");
	}
	else
	{
		// For some reason, loading a texture as floats (stbi_loadf) implies that texture should be HDR, which is not true for normals...

		//ImageCompression::RGBA32F* fullPixels = reinterpret_cast<ImageCompression::RGBA32F*>(imageFileLoader.getPixels());
		ImageCompression::RGBA8* fullPixels = reinterpret_cast<ImageCompression::RGBA8*>(imageFileLoader.getPixels());
		pixels.resize(static_cast<size_t>(imageFileLoader.getWidth()) * imageFileLoader.getHeight());

		for (uint32_t i = 0; i < pixels.size(); ++i)
		{
			//ImageCompression::RGBA32F fullPixel = fullPixels[i];
			ImageCompression::RGBA8 fullPixel = fullPixels[i];
			glm::vec3 fullPixelAsVec = 2.0f * glm::vec3(static_cast<float>(fullPixel.r) / 255.0f, static_cast<float>(fullPixel.g) / 255.0f, static_cast<float>(fullPixel.b) / 255.0f) - glm::vec3(1.0f);
			fullPixelAsVec = glm::normalize(fullPixelAsVec);

			pixels[i] = ImageCompression::RG32F(fullPixelAsVec.x, fullPixelAsVec.y);
		}
	}

	const MipMapGenerator mipmapGenerator(reinterpret_cast<const unsigned char*>(pixels.data()), { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()) }, format);

	mipLevels.resize(mipmapGenerator.getMipLevelCount() - 1);
	for (uint32_t mipLevel = 1; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
	{
		const std::vector<unsigned char>& mipPixels = mipmapGenerator.getMipLevel(mipLevel);
		const ImageCompression::RG32F* mipPixelsAsColor = reinterpret_cast<const ImageCompression::RG32F*>(mipPixels.data());
		mipLevels[mipLevel - 1] = std::vector<ImageCompression::RG32F>(mipPixelsAsColor, mipPixelsAsColor + mipPixels.size() / sizeof(ImageCompression::RG32F));
	}

	outExtent = { (imageFileLoader.getWidth()), (imageFileLoader.getHeight()), 1 };
}

bool Wolf::TextureSetLoader::createImageFileFromCache(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx)
{
	std::string binFilename = filename + ".bin";
	if (std::filesystem::exists(binFilename))
	{
		std::ifstream input(binFilename, std::ios::in | std::ios::binary);

		uint64_t hash;
		input.read(reinterpret_cast<char*>(&hash), sizeof(hash));
		if (hash != HASH_TEXTURE_SET_LOADER_CPP)
		{
			Debug::sendInfo("Cache found but hash is incorrect");
			return false;
		}

		VkExtent3D extent;
		input.read(reinterpret_cast<char*>(&extent), sizeof(extent));

		uint32_t dataBytesCount;
		input.read(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));

		std::vector<uint8_t> data(dataBytesCount);
		input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));

		uint32_t mipsCount;
		input.read(reinterpret_cast<char*>(&mipsCount), sizeof(mipsCount));

		std::vector<std::vector<uint8_t>> mipsData(mipsCount);
		std::vector<const uint8_t*> mipsDataPtr(mipsData.size());
		for (uint32_t i = 0; i < mipsData.size(); ++i)
		{
			input.read(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));
			mipsData[i].resize(dataBytesCount);
			input.read(reinterpret_cast<char*>(mipsData[i].data()), static_cast<std::streamsize>(mipsData[i].size()));

			mipsDataPtr[i] = mipsData[i].data();
		}

		VkFormat compressedFormat = VK_FORMAT_UNDEFINED;
		if (compression == ImageCompression::Compression::BC1)
			compressedFormat = sRGB ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		else if (compression == ImageCompression::Compression::BC3)
			compressedFormat = sRGB ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
		else if (compression == ImageCompression::Compression::BC5)
			compressedFormat = VK_FORMAT_BC5_UNORM_BLOCK;

		createImageFromData(extent, compressedFormat, data.data(), mipsDataPtr, imageIdx);

		return true;
	}

	Debug::sendInfo("Cache not found");
	return false;
}

template <typename PixelType>
void Wolf::TextureSetLoader::createImageFileFromSource(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx)
{
	VkFormat format = VK_FORMAT_UNDEFINED;
	if (compression == ImageCompression::Compression::BC1 || compression == ImageCompression::Compression::BC3)
		format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	else if (compression == ImageCompression::Compression::BC5)
		format = VK_FORMAT_R32G32_SFLOAT;
	else
		Debug::sendCriticalError("Unhandled case");

	std::vector<PixelType> pixels;
	std::vector<std::vector<PixelType>> mipLevels;
	VkExtent3D extent;
	loadImageFile(filename, format, pixels, mipLevels, extent);

	if (pixels.size() < 4)
	{
		Debug::sendWarning("Image size must be at least 4x4");
		pixels.resize(4 * 4);
		for (uint32_t i = 1; i < 4 * 4; ++i)
		{
			pixels[i] = pixels[0];
		}
		extent.width = extent.height = 4;
	}

	Debug::sendInfo("Creating new cache");

	std::fstream outCacheFile(filename + ".bin", std::ios::out | std::ios::binary);

	/* Hash */
	uint64_t hash = HASH_TEXTURE_SET_LOADER_CPP;
	outCacheFile.write(reinterpret_cast<char*>(&hash), sizeof(hash));
	outCacheFile.write(reinterpret_cast<char*>(&extent), sizeof(extent));

	if (compression == ImageCompression::Compression::BC1)
		compressAndCreateImage<ImageCompression::BC1>(mipLevels, pixels, extent, VK_FORMAT_BC1_RGB_SRGB_BLOCK, filename, outCacheFile, imageIdx);
	else if (compression == ImageCompression::Compression::BC3)
		compressAndCreateImage<ImageCompression::BC3>(mipLevels, pixels, extent, VK_FORMAT_BC3_SRGB_BLOCK, filename, outCacheFile, imageIdx);
	else if (compression == ImageCompression::Compression::BC5)
		compressAndCreateImage<ImageCompression::BC5>(mipLevels, pixels, extent, VK_FORMAT_BC5_UNORM_BLOCK, filename, outCacheFile, imageIdx);
	else
		Debug::sendError("Requested compression is not available");

	outCacheFile.close();
}

void Wolf::TextureSetLoader::createImageFromData(VkExtent3D extent, VkFormat format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = extent;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = format;
	createImageInfo.mipLevelCount = static_cast<uint32_t>(mipLevels.size()) + 1;
	createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_outputs[idx].reset(Image::createImage(createImageInfo));
	m_outputs[idx]->copyCPUBuffer(pixels, Image::SampledInFragmentShader());

	for (uint32_t mipLevel = 1; mipLevel < mipLevels.size() + 1; ++mipLevel)
	{
		m_outputs[idx]->copyCPUBuffer(mipLevels[mipLevel - 1], Image::SampledInFragmentShader(mipLevel), mipLevel);
	}

	if (m_useCache)
	{
		auto computeImageSize = [](VkExtent3D extent, float bpp, uint32_t mipLevel)
			{
				const VkExtent3D adjustedExtent = { extent.width >> mipLevel, extent.height >> mipLevel, extent.depth };
				return static_cast<VkDeviceSize>(static_cast<float>(adjustedExtent.width) * static_cast<float>(adjustedExtent.height) * static_cast<float>(adjustedExtent.depth) * bpp);
			};

		VkDeviceSize imageSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
			imageSize += computeImageSize(m_outputs[idx]->getExtent(), m_outputs[idx]->getBPP(), mipLevel);

		m_imagesData[idx].resize(imageSize);

		VkDeviceSize copyOffset = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
		{
			const VkDeviceSize copySize = computeImageSize(m_outputs[idx]->getExtent(), m_outputs[idx]->getBPP(), mipLevel);
			const unsigned char* dataPtr = pixels;
			if (mipLevel > 0)
				dataPtr = mipLevels[mipLevel - 1];
			memcpy(m_imagesData[idx].data() + copyOffset, dataPtr, copySize);

			copyOffset += copySize;
		}
	}
}

template <typename CompressionType, typename PixelType>
void Wolf::TextureSetLoader::compressAndCreateImage(std::vector<std::vector<PixelType>>& mipLevels, const std::vector<PixelType>& pixels, VkExtent3D& extent, VkFormat format, const std::string& filename,
	std::fstream& outCacheFile, uint32_t imageIdx)
{
	std::vector<CompressionType> compressedBlocks;
	std::vector<std::vector<CompressionType>> mipBlocks(mipLevels.size());
	ImageCompression::compress(extent, pixels, compressedBlocks);
	uint32_t mipWidth = extent.width / 2;
	uint32_t mipHeight = extent.height / 2;
	for (uint32_t i = 0; i < mipLevels.size(); ++i)
	{
		// Can't compress if size can't be divided by 4 (as we take 4x4 blocks)
		if (mipWidth % 4 != 0 || mipHeight % 4 != 0)
		{
			Debug::sendWarning("Image " + filename + " resolution is not a power of 2, not all mips are generated");
			mipLevels.resize(i);
			mipBlocks.resize(i);
			break;
		}

		ImageCompression::compress({ mipWidth, mipHeight, 1 }, mipLevels[i], mipBlocks[i]);

		mipWidth /= 2;
		mipHeight /= 2;
	}

	std::vector<const unsigned char*> mipsData(mipBlocks.size());
	for (uint32_t i = 0; i < mipBlocks.size(); ++i)
	{
		mipsData[i] = reinterpret_cast<const unsigned char*>(mipBlocks[i].data());
	}
	createImageFromData(extent, format, reinterpret_cast<const unsigned char*>(compressedBlocks.data()), mipsData, imageIdx);

	// Add to cache
	uint32_t dataBytesCount = static_cast<uint32_t>(compressedBlocks.size() * sizeof(CompressionType));
	outCacheFile.write(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));
	outCacheFile.write(reinterpret_cast<char*>(compressedBlocks.data()), dataBytesCount);

	uint32_t mipsCount = static_cast<uint32_t>(mipBlocks.size());
	outCacheFile.write(reinterpret_cast<char*>(&mipsCount), sizeof(mipsCount));

	for (uint32_t i = 0; i < mipsCount; ++i)
	{
		dataBytesCount = static_cast<uint32_t>(mipBlocks[i].size() * sizeof(CompressionType));
		outCacheFile.write(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));
		outCacheFile.write(reinterpret_cast<char*>(mipBlocks[i].data()), dataBytesCount);
	}
}
