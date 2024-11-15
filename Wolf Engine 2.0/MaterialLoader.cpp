#include "MaterialLoader.h"

#include "ImageFileLoader.h"
#include "MipMapGenerator.h"

Wolf::MaterialLoader::MaterialLoader(const MaterialFileInfoGGX& material, const OutputLayout& outputLayout, bool useCache) : m_useCache(useCache)
{
	Debug::sendInfo("Loading material " + material.name);

	// Albedo
	if (!material.albedo.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.albedo, VK_FORMAT_R8G8B8A8_SRGB, pixels, mipLevels, extent);

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

		if (outputLayout.albedoCompression == ImageCompression::Compression::BC1)
			compressAndCreateImage<ImageCompression::BC1>(mipLevels, pixels, extent, VK_FORMAT_BC1_RGB_SRGB_BLOCK, material);
		else if (outputLayout.albedoCompression == ImageCompression::Compression::BC3)
			compressAndCreateImage<ImageCompression::BC3>(mipLevels, pixels, extent, VK_FORMAT_BC3_SRGB_BLOCK, material);
		else
			Debug::sendError("Requested compression is not available");
	}

	// Normal
	if (!material.normal.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.normal, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 1);
	}

	std::vector<ImageCompression::RGBA8> combinedRoughnessMetalnessAOAniso;
	std::vector<std::vector<ImageCompression::RGBA8>> combinedRoughnessMetalnessAOMipLevels;
	VkExtent3D combinedRoughnessMetalnessAOExtent;

	// Roughness
	if (!material.roughness.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		loadImageFile(material.roughness, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, combinedRoughnessMetalnessAOExtent);

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
	if (!material.metalness.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.metalness, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

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
	if (!material.ao.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.ao, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

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
	if (!material.anisoStrength.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.anisoStrength, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

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

	if (!material.roughness.empty())
	{
		std::vector<const unsigned char*> mipsData(combinedRoughnessMetalnessAOMipLevels.size());
		for (uint32_t i = 0; i < combinedRoughnessMetalnessAOMipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAOMipLevels[i].data());
		}
		createImageFromData(combinedRoughnessMetalnessAOExtent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAOAniso.data()), mipsData, 2);
	}
}

Wolf::MaterialLoader::MaterialLoader(const MaterialFileInfoSixWayLighting& material, bool useCache)
{
	if (!material.tex0.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.tex0, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 0);
	}

	if (!material.tex1.empty())
	{
		std::vector<ImageCompression::RGBA8> pixels;
		std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
		VkExtent3D extent;
		loadImageFile(material.tex1, VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

		std::vector<const unsigned char*> mipsData(mipLevels.size());
		for (uint32_t i = 0; i < mipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
		}
		createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, 1);
	}
}

Wolf::Image* Wolf::MaterialLoader::releaseImage(uint32_t idx)
{
	return m_outputs[idx].release();
}

void Wolf::MaterialLoader::assignCache(uint32_t idx, std::vector<unsigned char>& output)
{
	output = std::move(m_imagesData[idx]);
}

void Wolf::MaterialLoader::loadImageFile(const std::string& filename, VkFormat format,
                                         std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels,
                                         VkExtent3D& outExtent) const
{
	const ImageFileLoader imageFileLoader(filename);

	if (imageFileLoader.getCompression() == ImageCompression::Compression::BC5)
	{
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

void Wolf::MaterialLoader::createImageFromData(VkExtent3D extent, VkFormat format, const unsigned char* pixels,
	const std::vector<const unsigned char*>& mipLevels, uint32_t idx)
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
		auto computeImageSize = [](VkExtent3D extent, float bbp, uint32_t mipLevel)
			{
				const VkExtent3D adjustedExtent = { extent.width >> mipLevel, extent.height >> mipLevel, extent.depth };
				return static_cast<VkDeviceSize>(static_cast<float>(adjustedExtent.width) * static_cast<float>(adjustedExtent.height) * static_cast<float>(adjustedExtent.depth) * bbp);
			};

		VkDeviceSize imageSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
			imageSize += computeImageSize(m_outputs[idx]->getExtent(), m_outputs[idx]->getBBP(), mipLevel);

		m_imagesData[idx].resize(imageSize);

		VkDeviceSize copyOffset = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
		{
			const VkDeviceSize copySize = computeImageSize(m_outputs[idx]->getExtent(), m_outputs[idx]->getBBP(), mipLevel);
			const unsigned char* dataPtr = pixels;
			if (mipLevel > 0)
				dataPtr = mipLevels[mipLevel - 1];
			memcpy(m_imagesData[idx].data() + copyOffset, dataPtr, copySize);

			copyOffset += copySize;
		}
	}
}

template <typename T>
void Wolf::MaterialLoader::compressAndCreateImage(std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, const std::vector<ImageCompression::RGBA8>& pixels, VkExtent3D& extent, VkFormat format, const MaterialFileInfoGGX& material)
{
	std::vector<T> compressedBlocks;
	std::vector<std::vector<T>> mipBlocks(mipLevels.size());
	ImageCompression::compress(extent, pixels, compressedBlocks);
	uint32_t mipWidth = extent.width / 2;
	uint32_t mipHeight = extent.height / 2;
	for (uint32_t i = 0; i < mipLevels.size(); ++i)
	{
		// Can't compress if size can't be divided by 4 (as we take 4x4 blocks)
		if (mipWidth % 4 != 0 || mipHeight % 4 != 0)
		{
			Debug::sendWarning("Image " + material.albedo + " resolution is not a power of 2, not all mips are generated");
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
	createImageFromData(extent, format, reinterpret_cast<const unsigned char*>(compressedBlocks.data()), mipsData, 0);
}