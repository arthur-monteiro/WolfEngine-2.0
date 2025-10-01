#include "TextureSetLoader.h"

#include <filesystem>
#include <fstream>

#include <Configuration.h>

#include "CodeFileHashes.h"
#include "ConfigurationHelper.h"
#include "ImageFileLoader.h"
#include "MipMapGenerator.h"
#include "PushDataToGPU.h"
#include "Timer.h"
#include "VirtualTextureManager.h"

Wolf::TextureSetLoader::TextureSetLoader(const TextureSetFileInfoGGX& textureSet, const OutputLayout& outputLayout, bool useCache) : m_useCache(useCache)
{
	Debug::sendInfo("Loading material " + textureSet.name);
	Timer globalTimer("Loading material " + textureSet.name);

	// Albedo
	if (!textureSet.albedo.empty())
	{
		Debug::sendInfo("Loading albedo " + textureSet.albedo);
		Timer albedoTimer("Loading albedo " + textureSet.albedo);

		if (g_configuration->getUseVirtualTexture())
		{
			if (outputLayout.albedoCompression != DEFAULT_ALBEDO_COMPRESSION)
			{
				Debug::sendError("Albedo compression must be BC1");
			}
			createSlicedCache<ImageCompression::RGBA8, ImageCompression::BC1>(textureSet.albedo, true, outputLayout.albedoCompression, 0);
		}
		else
		{
			if (!createImageFileFromCache(textureSet.albedo, true, outputLayout.albedoCompression, 0))
			{
				createImageFileFromSource<ImageCompression::RGBA8>(textureSet.albedo, true, outputLayout.albedoCompression, 0);
			}
		}
	}

	// Normal
	if (!textureSet.normal.empty())
	{
		Debug::sendInfo("Loading normal " + textureSet.normal);
		Timer albedoTimer("Loading normal " + textureSet.normal);

		if (g_configuration->getUseVirtualTexture())
		{
			if (outputLayout.normalCompression != DEFAULT_NORMAL_COMPRESSION)
			{
				Debug::sendError("Normal compression must be BC5");
			}
			createSlicedCache<ImageCompression::RG32F, ImageCompression::BC5>(textureSet.normal, false, outputLayout.normalCompression, 1);
		}
		else
		{
			if (!createImageFileFromCache(textureSet.normal, false, outputLayout.normalCompression, 1))
			{
				createImageFileFromSource<ImageCompression::RG32F>(textureSet.normal, false, outputLayout.normalCompression, 1);
			}
		}
	}

	createCombinedTexture(textureSet);
}

Wolf::TextureSetLoader::TextureSetLoader(const TextureSetFileInfoSixWayLighting& textureSet, bool useCache)
{
	if (!textureSet.tex0.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		Extent3D extent;
		loadImageFile(textureSet.tex0, Format::R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, Format::R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 0);
	}

	if (!textureSet.tex1.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		Extent3D extent;
		loadImageFile(textureSet.tex1, Format::R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, Format::R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 1);
	}
}

void Wolf::TextureSetLoader::transferImageTo(uint32_t idx, ResourceUniqueOwner<Image>& output)
{
	return output.transferFrom(m_outputImages[idx]);
}

void Wolf::TextureSetLoader::loadImageFile(const std::string& filename, Format format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, Extent3D& outExtent) const
{
	const ImageFileLoader imageFileLoader(filename);

	if (imageFileLoader.getCompression() == ImageCompression::Compression::BC5)
	{
#ifndef __ANDROID__
		__debugbreak(); // deadcode?
#endif

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

void Wolf::TextureSetLoader::loadImageFile(const std::string& filename, Format format, std::vector<ImageCompression::RG32F>& pixels, std::vector<std::vector<ImageCompression::RG32F>>& mipLevels, Extent3D& outExtent) const
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

Wolf::Format Wolf::TextureSetLoader::findFormatFromCompression(ImageCompression::Compression compression, bool sRGB)
{
	if (compression == ImageCompression::Compression::BC1 || compression == ImageCompression::Compression::BC3)
		return sRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;
	else if (compression == ImageCompression::Compression::BC5)
		return Format::R32G32_SFLOAT;
	else
	{
		Debug::sendCriticalError("Unhandled case");
		return Format::UNDEFINED;
	}
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

		Extent3D extent;
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

		Format compressedFormat = Format::UNDEFINED;
		if (compression == ImageCompression::Compression::BC1)
			compressedFormat = sRGB ? Format::BC1_RGB_SRGB_BLOCK : Format::BC1_RGBA_UNORM_BLOCK;
		else if (compression == ImageCompression::Compression::BC3)
			compressedFormat = sRGB ? Format::BC3_SRGB_BLOCK : Format::BC3_UNORM_BLOCK;
		else if (compression == ImageCompression::Compression::BC5)
			compressedFormat = Format::BC5_UNORM_BLOCK;
		else if (compression == ImageCompression::Compression::NO_COMPRESSION)
			compressedFormat = sRGB ? Format::R8G8B8A8_SRGB : Format::R8G8B8A8_UNORM;

		createImageFromData(extent, compressedFormat, data.data(), mipsDataPtr, imageIdx);

		return true;
	}
	else if (filename[filename.size() - 4] == '.' && filename[filename.size() - 3] == 'd' && filename[filename.size() - 2] == 'd' && filename[filename.size() - 1] == 's')
	{
		const ImageFileLoader imageFileLoader(filename, false);

		Extent3D extent = { imageFileLoader.getWidth(), imageFileLoader.getHeight(), 1 };
		Format format = imageFileLoader.getFormat();

		const std::vector<std::vector<uint8_t>> mipPixels = imageFileLoader.getMipPixels();
		std::vector<const unsigned char*> mipPtrs(mipPixels.size());
		for (uint32_t i = 0; i < mipPixels.size(); ++i)
		{
			mipPtrs[i] = mipPixels[i].data();
		}
		createImageFromData(extent, format, imageFileLoader.getPixels(), mipPtrs, imageIdx);

		return true;
	}

	Debug::sendInfo("Cache not found");
	return false;
}

template <typename PixelType>
void Wolf::TextureSetLoader::createImageFileFromSource(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx)
{
	Format format = findFormatFromCompression(compression, sRGB);

	std::vector<PixelType> pixels;
	std::vector<std::vector<PixelType>> mipLevels;
	Extent3D extent;
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
		compressAndCreateImage<ImageCompression::BC1>(mipLevels, pixels, extent, Format::BC1_RGB_SRGB_BLOCK, filename, outCacheFile, imageIdx);
	else if (compression == ImageCompression::Compression::BC3)
		compressAndCreateImage<ImageCompression::BC3>(mipLevels, pixels, extent, Format::BC3_SRGB_BLOCK, filename, outCacheFile, imageIdx);
	else if (compression == ImageCompression::Compression::BC5)
		compressAndCreateImage<ImageCompression::BC5>(mipLevels, pixels, extent, Format::BC5_UNORM_BLOCK, filename, outCacheFile, imageIdx);
	else
		Debug::sendError("Requested compression is not available");

	outCacheFile.close();
}

template <typename PixelType, typename CompressionType>
void Wolf::TextureSetLoader::createSlicedCache(const std::string& filename, bool sRGB, ImageCompression::Compression compression, uint32_t imageIdx)
{
	std::string filenameNoExtension = filename.substr(filename.find_last_of("\\") + 1, filename.find_last_of("."));
	std::string folder = filename.substr(0, filename.find_last_of("\\"));
	std::string binFolder = folder + '\\' + filenameNoExtension + "_bin" + "\\";

	std::string binFolderFixed;
	for (const char character : binFolder)
	{
		if (character == '/')
			binFolderFixed += "\\";
		else
			binFolderFixed += character;
	}

	if (!std::filesystem::is_directory(binFolderFixed) || !std::filesystem::exists(binFolderFixed))
	{
		std::filesystem::create_directory(binFolderFixed);
	}

	m_outputFolders[imageIdx] = binFolderFixed;

	// Check 1st file to avoid loading pixels if we don't need
	std::string binFilename = binFolderFixed + "mip0_sliceX0_sliceY0.bin";
	if (std::filesystem::exists(binFilename))
	{
		std::ifstream input(binFilename, std::ios::in | std::ios::binary);

		uint64_t hash;
		input.read(reinterpret_cast<char*>(&hash), sizeof(hash));
		if (hash == (HASH_TEXTURE_SET_LOADER_CPP ^ HASH_VIRTUAL_TEXTURE_MANAGER_H))
		{
			return; // we assume cache is ready
		}
	}

	Format format = findFormatFromCompression(compression, sRGB);

	std::vector<PixelType> pixels;
	std::vector<std::vector<PixelType>> mipLevels;
	Extent3D extent{};
	loadImageFile(filename, format, pixels, mipLevels, extent);

	if (extent.width % VirtualTextureManager::VIRTUAL_PAGE_SIZE != 0 || extent.height % VirtualTextureManager::VIRTUAL_PAGE_SIZE != 0 || extent.depth != 1)
	{
		Debug::sendCriticalError("Wrong texture extent for virtual texture slicing");
	}

	ConfigurationHelper::writeInfoToFile(binFolderFixed + "info.txt", "width", extent.width);
	ConfigurationHelper::writeInfoToFile(binFolderFixed + "info.txt", "height", extent.height);

	Extent3D maxSliceExtent{ VirtualTextureManager::VIRTUAL_PAGE_SIZE, VirtualTextureManager::VIRTUAL_PAGE_SIZE, 1 };

	for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
	{
		Extent3D extentForMip = { extent.width >> mipLevel, extent.height >> mipLevel, 1 };

		const uint32_t sliceCountX = std::max(extentForMip.width / maxSliceExtent.width, 1u);
		const uint32_t sliceCountY = std::max(extentForMip.height / maxSliceExtent.height, 1u);

		for (uint32_t sliceX = 0; sliceX < sliceCountX; ++sliceX)
		{
			for (uint32_t sliceY = 0; sliceY < sliceCountY; ++sliceY)
			{
				std::string binFilename = binFolderFixed + "mip" + std::to_string(mipLevel) + "_sliceX" + std::to_string(sliceX) + "_sliceY" + std::to_string(sliceY) + ".bin";
				if (std::filesystem::exists(binFilename))
				{
					std::ifstream input(binFilename, std::ios::in | std::ios::binary);

					uint64_t hash;
					input.read(reinterpret_cast<char*>(&hash), sizeof(hash));
					if (hash == (HASH_TEXTURE_SET_LOADER_CPP ^ HASH_VIRTUAL_TEXTURE_MANAGER_H))
					{
						continue;
					}

					Debug::sendInfo(binFilename + " found but hash is incorrect");
				}
				else
				{
					Debug::sendInfo(binFilename + " not found");
				}

				std::fstream outCacheFile(binFilename, std::ios::out | std::ios::binary);

				/* Hash */
				uint64_t hash = (HASH_TEXTURE_SET_LOADER_CPP ^ HASH_VIRTUAL_TEXTURE_MANAGER_H);
				outCacheFile.write(reinterpret_cast<char*>(&hash), sizeof(hash));

				Extent3D pixelsToCompressExtent{ std::min(extentForMip.width, maxSliceExtent.width), std::min(extentForMip.height, maxSliceExtent.height), 1 };
				pixelsToCompressExtent.width += 2 * VirtualTextureManager::BORDER_SIZE;
				pixelsToCompressExtent.height += 2 * VirtualTextureManager::BORDER_SIZE;
				std::vector<PixelType> pixelsToCompress(pixelsToCompressExtent.width * pixelsToCompressExtent.height);

				std::vector<PixelType>& allPixelsSrc = mipLevel == 0 ? pixels : mipLevels[mipLevel - 1];
				for (uint32_t line = 0; line < pixelsToCompressExtent.height; ++line)
				{
					uint32_t srcY = sliceY * maxSliceExtent.width + line - VirtualTextureManager::BORDER_SIZE;
					if (sliceY == 0 && line < VirtualTextureManager::BORDER_SIZE)
					{
						srcY = extentForMip.height - VirtualTextureManager::BORDER_SIZE + line;
					}
					else if (sliceY == sliceCountY - 1 && line >= pixelsToCompressExtent.height - VirtualTextureManager::BORDER_SIZE)
					{
						srcY = line - std::min(extentForMip.width, maxSliceExtent.width) - VirtualTextureManager::BORDER_SIZE;
					}

					uint32_t srcX = sliceX * maxSliceExtent.width;

					const PixelType* src = &allPixelsSrc[srcY * extentForMip.width + srcX];
					PixelType* dst = &pixelsToCompress[line * pixelsToCompressExtent.width + VirtualTextureManager::BORDER_SIZE];

					memcpy(dst, src, std::min(extentForMip.width, maxSliceExtent.width) * sizeof(PixelType)); // copy without left and right border

					// Left border
					for (uint32_t i = 0; i < VirtualTextureManager::BORDER_SIZE; ++i)
					{
						// Left border
						PixelType& leftBorderPixelSrc = pixelsToCompress[line * pixelsToCompressExtent.width + i];
						if (sliceX == 0)
						{
							leftBorderPixelSrc = allPixelsSrc[srcY * extentForMip.width + extentForMip.width - VirtualTextureManager::BORDER_SIZE + i];
						}
						else
						{
							leftBorderPixelSrc = allPixelsSrc[srcY * extentForMip.width + srcX - VirtualTextureManager::BORDER_SIZE + i];
						}

						// Right border
						PixelType& rightBorderPixelSrc = pixelsToCompress[line * pixelsToCompressExtent.width + pixelsToCompressExtent.width - VirtualTextureManager::BORDER_SIZE + i];
						if (sliceX == sliceCountX - 1)
						{
							rightBorderPixelSrc = allPixelsSrc[srcY * extentForMip.width + i];
						}
						else
						{
							rightBorderPixelSrc = allPixelsSrc[srcY * extentForMip.width + srcX + maxSliceExtent.width + i];
						}
					}
				}

				std::vector<CompressionType> compressedBlocks;
				ImageCompression::compress(pixelsToCompressExtent, pixelsToCompress, compressedBlocks);

				uint32_t dataBytesCount = static_cast<uint32_t>(compressedBlocks.size() * sizeof(CompressionType));
				outCacheFile.write(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));
				outCacheFile.write(reinterpret_cast<char*>(compressedBlocks.data()), dataBytesCount);

				outCacheFile.close();
			}
		}
	}
}

void Wolf::TextureSetLoader::createImageFromData(Extent3D extent, Format format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = extent;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = format;
	createImageInfo.mipLevelCount = static_cast<uint32_t>(mipLevels.size()) + 1;
	createImageInfo.usage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::SAMPLED;
	m_outputImages[idx].reset(Image::createImage(createImageInfo));
	pushDataToGPUImage(pixels, m_outputImages[idx].createNonOwnerResource(), Image::SampledInFragmentShader());

	for (uint32_t mipLevel = 1; mipLevel < mipLevels.size() + 1; ++mipLevel)
	{
		pushDataToGPUImage(mipLevels[mipLevel - 1], m_outputImages[idx].createNonOwnerResource(), Image::SampledInFragmentShader(mipLevel), mipLevel);
	}
}

std::string extractFilenameFromPath(const std::string& fullPath)
{
	return fullPath.substr(fullPath.find_last_of("/\\") + 1);
}

std::string extractFolderFromFullPath(const std::string& fullPath)
{
	return fullPath.substr(0, fullPath.find_last_of("/\\"));
}

void Wolf::TextureSetLoader::createCombinedTexture(const TextureSetFileInfoGGX& textureSet)
{
	if (g_configuration->getUseVirtualTexture())
	{
		return;
	}

	bool useCache = m_useCache && !textureSet.roughness.empty();

	std::string combinedTexturePath = extractFolderFromFullPath(textureSet.roughness) + "combined_";
	combinedTexturePath += extractFilenameFromPath(textureSet.roughness) + "_";
	combinedTexturePath += extractFilenameFromPath(textureSet.metalness) + "_";
	combinedTexturePath += extractFilenameFromPath(textureSet.ao) + "_";
	combinedTexturePath += extractFilenameFromPath(textureSet.anisoStrength);

	if (useCache)
	{
		if (createImageFileFromCache(combinedTexturePath, false, ImageCompression::Compression::BC3, 2))
		{
			return;
		}
	}

	std::vector<ImageCompression::RGBA8> combinedRoughnessMetalnessAOAniso;
	std::vector<std::vector<ImageCompression::RGBA8>> combinedRoughnessMetalnessAOMipLevels;
	Extent3D combinedRoughnessMetalnessAOExtent = { 1, 1, 1};
	combinedRoughnessMetalnessAOAniso.resize(1);

	// Roughness
	if (!textureSet.roughness.empty())
	{
		Debug::sendInfo("Loading roughness " + textureSet.roughness);
		Timer albedoTimer("Loading roughness " + textureSet.roughness);

		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		loadImageFile(textureSet.roughness, Format::R8G8B8A8_UNORM, pixels, mipLevels, combinedRoughnessMetalnessAOExtent);

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
		Extent3D extent;
		loadImageFile(textureSet.metalness, Format::R8G8B8A8_UNORM, pixels, mipLevels, extent);

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
		Extent3D extent;
		loadImageFile(textureSet.ao, Format::R8G8B8A8_UNORM, pixels, mipLevels, extent);

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
		Extent3D extent;
		loadImageFile(textureSet.anisoStrength, Format::R8G8B8A8_UNORM, pixels, mipLevels, extent);

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

		std::fstream outCacheFile(combinedTexturePath + ".bin", std::ios::out | std::ios::binary);

		/* Hash */
		uint64_t hash = HASH_TEXTURE_SET_LOADER_CPP;
		outCacheFile.write(reinterpret_cast<char*>(&hash), sizeof(hash));
		outCacheFile.write(reinterpret_cast<char*>(&combinedRoughnessMetalnessAOExtent), sizeof(combinedRoughnessMetalnessAOExtent));

		compressAndCreateImage<ImageCompression::BC3>(combinedRoughnessMetalnessAOMipLevels, combinedRoughnessMetalnessAOAniso, combinedRoughnessMetalnessAOExtent, Format::BC3_UNORM_BLOCK,
			combinedTexturePath, outCacheFile, 2);
	}
}

template <typename CompressionType, typename PixelType>
void Wolf::TextureSetLoader::compressAndCreateImage(std::vector<std::vector<PixelType>>& mipLevels, const std::vector<PixelType>& pixels, Extent3D& extent, Format format, const std::string& filename, std::fstream& outCacheFile, uint32_t imageIdx)
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
