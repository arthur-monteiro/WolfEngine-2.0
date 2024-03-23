#include "ModelLoader.h"

#include <filesystem>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <thread>

#include "AndroidCacheHelper.h"
#include "CodeFileHashes.h"
#include "ImageFileLoader.h"
#include "MaterialsGPUManager.h"
#include "MipMapGenerator.h"

void Wolf::ModelData::getImages(std::vector<Image*>& outputImages) const
{
	outputImages.reserve(outputImages.size());
	for (const std::unique_ptr<Image>& image : images)
	{
		outputImages.push_back(image.get());
	}
}

void Wolf::ModelLoader::loadObject(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo)
{
	ModelLoader objLoader(outputModel, modelLoadingInfo);
}

Wolf::ModelLoader::ModelLoader(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo) : m_outputModel(outputModel)
{
#ifdef __ANDROID__
    if(modelLoadingInfo.isInAssets)
    {
        const std::string appFolderName = "model_cache";
        copyCompressedFileToStorage(modelLoadingInfo.filename, appFolderName, modelLoadingInfo.filename);
    }
#endif

	Debug::sendInfo("Start loading " + modelLoadingInfo.filename);

	if (modelLoadingInfo.useCache)
	{
		if (loadCache(modelLoadingInfo))
			return;
	}

	Debug::sendInfo("Loading without cache");

	m_useCache = modelLoadingInfo.useCache;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err, warn;

	if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, modelLoadingInfo.filename.c_str(), modelLoadingInfo.mtlFolder.c_str()))
		Debug::sendCriticalError(err);

	if (!err.empty())
		Debug::sendInfo("[Loading object file] Error : " + err + " for " + modelLoadingInfo.filename + " !");
	if (!warn.empty())
		Debug::sendInfo("[Loading object file] Warning : " + warn + " for " + modelLoadingInfo.filename + " !");

	std::unordered_map<Vertex3D, uint32_t> uniqueVertices = {};
	std::vector<Vertex3D> vertices;
	std::vector<uint32_t> indices;
	std::vector<InternalShapeInfo> shapeInfos(shapes.size());

	glm::vec3 minPos(1'000'000.f);
	glm::vec3 maxPos(-1'000'000.f);

	bool hasEncounteredAnInvalidMaterialId = false;
	for(uint32_t shapeIdx = 0; shapeIdx < shapes.size(); ++shapeIdx)
	{
		auto& [name, mesh, lines, points] = shapes[shapeIdx];
		shapeInfos[shapeIdx].indicesOffset = static_cast<uint32_t>(indices.size());

		int numVertex = 0;
		for (const auto& index : mesh.indices)
		{
			Vertex3D vertex = {};

			vertex.pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (vertex.pos.x < minPos.x)
				minPos.x = vertex.pos.x;
			if (vertex.pos.y < minPos.y)
				minPos.y = vertex.pos.y;
			if (vertex.pos.z < minPos.z)
				minPos.z = vertex.pos.z;

			if (vertex.pos.x > maxPos.x)
				maxPos.x = vertex.pos.x;
			if (vertex.pos.y > maxPos.y)
				maxPos.y = vertex.pos.y;
			if (vertex.pos.z > maxPos.z)
				maxPos.z = vertex.pos.z;

			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (attrib.normals.size() <= 3 * index.normal_index + 2)
				vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
			else
			{
				vertex.normal =
				{
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (modelLoadingInfo.materialLayout == ModelLoadingInfo::InputMaterialLayout::NO_MATERIAL)
				vertex.materialID = modelLoadingInfo.materialIdOffset;
			else
			{
				int32_t materialId = mesh.material_ids[numVertex / 3];
				if (materialId < 0)
				{
					hasEncounteredAnInvalidMaterialId = true;
					vertex.materialID = 0;
				}
				else
					vertex.materialID = modelLoadingInfo.materialIdOffset + materialId;
			}

			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			
			indices.push_back(uniqueVertices[vertex]);
			numVertex++;
		}
	}

	if (hasEncounteredAnInvalidMaterialId)
		Debug::sendError("Loading model " + modelLoadingInfo.filename + ", invalid material ID found. Switching to default (0)");

	AABB aabb(minPos, maxPos);

	std::array<Vertex3D, 3> tempTriangle{};
	for (size_t i(0); i <= indices.size(); ++i)
	{
		if (i != 0 && i % 3 == 0)
		{
			glm::vec3 edge1 = tempTriangle[1].pos - tempTriangle[0].pos;
			glm::vec3 edge2 = tempTriangle[2].pos - tempTriangle[0].pos;
			glm::vec2 deltaUV1 = tempTriangle[1].texCoord - tempTriangle[0].texCoord;
			glm::vec2 deltaUV2 = tempTriangle[2].texCoord - tempTriangle[0].texCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = normalize(tangent);

			for (int32_t j(static_cast<int32_t>(i) - 1); j >= static_cast<int32_t>(i - 3); --j)
				vertices[indices[j]].tangent = tangent;
		}

		if (i == indices.size())
			break;

		tempTriangle[i % 3] = vertices[indices[i]];
	}

	m_outputModel.mesh.reset(new Mesh(vertices, indices, aabb, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages));

	for (uint32_t shapeIdx = 0; shapeIdx < shapeInfos.size(); ++shapeIdx)
	{
		const InternalShapeInfo& shapeInfo = shapeInfos[shapeIdx];
		const InternalShapeInfo& nextShapeInfo = shapeIdx == shapeInfos.size() - 1 ? InternalShapeInfo{ static_cast<uint32_t>(indices.size()) } : shapeInfos[shapeIdx + 1];

		m_outputModel.mesh->addSubMesh(shapeInfo.indicesOffset, nextShapeInfo.indicesOffset - shapeInfo.indicesOffset);
	}

	Debug::sendInfo("Model " + modelLoadingInfo.filename + " loaded with " + std::to_string(indices.size() / 3) + " triangles and " + std::to_string(shapeInfos.size()) + " shapes");

	if (modelLoadingInfo.materialLayout != ModelLoadingInfo::InputMaterialLayout::NO_MATERIAL)
	{
		m_outputModel.images.resize(materials.size() * MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL);
		m_imagesData.resize(materials.size() * MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL); // is kept empty if no cache is used
		uint32_t indexTexture = 0;
		for (auto& material : materials)
		{
			loadMaterial(material, modelLoadingInfo.mtlFolder, modelLoadingInfo.materialLayout, indexTexture);
		}
	}

	if (modelLoadingInfo.useCache)
	{
		Debug::sendInfo("Creating new cache");

		std::fstream outCacheFile(modelLoadingInfo.filename + ".bin", std::ios::out | std::ios::binary);

		/* Hash */
		uint64_t hash = HASH_MODEL_LOADER_CPP;
		outCacheFile.write(reinterpret_cast<char*>(&hash), sizeof(hash));

		/* Geometry */
		uint32_t verticesCount = static_cast<uint32_t>(vertices.size());
		outCacheFile.write(reinterpret_cast<char*>(&verticesCount), sizeof(uint32_t));
		for(Vertex3D& vertex : vertices)
		{
			if (vertex.materialID == 0)
				vertex.materialID = static_cast<uint32_t>(-1);
			else
				vertex.materialID -= modelLoadingInfo.materialIdOffset;
		}
		outCacheFile.write(reinterpret_cast<char*>(vertices.data()), verticesCount * sizeof(vertices[0]));
		uint32_t indicesCount = static_cast<uint32_t>(indices.size());
		outCacheFile.write(reinterpret_cast<char*>(&indicesCount), sizeof(uint32_t));
		outCacheFile.write(reinterpret_cast<char*>(indices.data()), indicesCount * sizeof(indices[0]));
		outCacheFile.write(reinterpret_cast<char*>(&aabb), sizeof(AABB));

		uint32_t shapeCount = static_cast<uint32_t>(shapeInfos.size());
		outCacheFile.write(reinterpret_cast<char*>(&shapeCount), sizeof(uint32_t));
		for (const InternalShapeInfo& shapeInfo : shapeInfos)
		{
			outCacheFile.write(reinterpret_cast<const char*>(&shapeInfo), sizeof(InternalShapeInfo));
		}

		/* Textures*/
		uint32_t imageCount = static_cast<uint32_t>(m_outputModel.images.size());
		outCacheFile.write(reinterpret_cast<char*>(&imageCount), sizeof(uint32_t));
		for (uint32_t i = 0; i < m_imagesData.size(); ++i)
		{
			VkExtent3D extent = m_outputModel.images[i]->getExtent();
			outCacheFile.write(reinterpret_cast<char*>(&extent), sizeof(VkExtent3D));
			outCacheFile.write(reinterpret_cast<char*>(m_imagesData[i].data()), m_imagesData[i].size());
		}

		outCacheFile.close();
	}
}

bool Wolf::ModelLoader::loadCache(ModelLoadingInfo& modelLoadingInfo) const
{
	std::string binFilename = modelLoadingInfo.filename + ".bin";
	if (std::filesystem::exists(binFilename))
	{
		std::ifstream input(binFilename, std::ios::in | std::ios::binary);

		uint64_t hash;
		input.read(reinterpret_cast<char*>(&hash), sizeof(hash));
		if (hash != HASH_MODEL_LOADER_CPP)
		{
			Debug::sendInfo("Cache found but hash is incorrect");
			return false;
		}

		uint32_t verticesCount;
		input.read(reinterpret_cast<char*>(&verticesCount), sizeof(verticesCount));
		std::vector<Vertex3D> vertices(verticesCount);
		input.read(reinterpret_cast<char*>(vertices.data()), verticesCount * sizeof(vertices[0]));

		for (Vertex3D& vertex : vertices)
		{
			if (vertex.materialID == static_cast<uint32_t>(-1))
				vertex.materialID = 0;
			else
				vertex.materialID += modelLoadingInfo.materialIdOffset;
		}

		uint32_t indicesCount;
		input.read(reinterpret_cast<char*>(&indicesCount), sizeof(indicesCount));
		std::vector<uint32_t> indices(indicesCount);
		input.read(reinterpret_cast<char*>(indices.data()), indicesCount * sizeof(indices[0]));

		AABB aabb{};
		input.read(reinterpret_cast<char*>(&aabb), sizeof(AABB));

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->lock();
		m_outputModel.mesh.reset(new Mesh(vertices, indices, aabb, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages, VK_FORMAT_R32G32B32_SFLOAT));
		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->unlock();

		uint32_t shapeCount;
		input.read(reinterpret_cast<char*>(&shapeCount), sizeof(shapeCount));
		std::vector<InternalShapeInfo> shapes(shapeCount);
		input.read(reinterpret_cast<char*>(shapes.data()), shapeCount * sizeof(shapes[0]));

		for (uint32_t shapeIdx = 0; shapeIdx < shapes.size(); ++shapeIdx)
		{
			const InternalShapeInfo& shapeInfo = shapes[shapeIdx];
			const InternalShapeInfo& nextShapeInfo = shapeIdx == shapes.size() - 1 ? InternalShapeInfo{ static_cast<uint32_t>(indices.size()) } : shapes[shapeIdx + 1];

			m_outputModel.mesh->addSubMesh(shapeInfo.indicesOffset, nextShapeInfo.indicesOffset - shapeInfo.indicesOffset);
		}

		uint32_t textureCount;
		input.read(reinterpret_cast<char*>(&textureCount), sizeof(textureCount));
		m_outputModel.images.resize(textureCount);

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->lock();

		std::chrono::steady_clock::time_point startTimer = std::chrono::steady_clock::now();

		for (uint32_t i(0); i < textureCount; ++i)
		{
			std::chrono::steady_clock::time_point currentTimer = std::chrono::steady_clock::now();
			long long timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimer - startTimer).count();

			if (timeDiff > 33 && modelLoadingInfo.vulkanQueueLock)
			{
				modelLoadingInfo.vulkanQueueLock->unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				modelLoadingInfo.vulkanQueueLock->lock();

				startTimer = std::chrono::steady_clock::now();
			}

			VkExtent3D extent;
			input.read(reinterpret_cast<char*>(&extent), sizeof(extent));

			CreateImageInfo createImageInfo;
			createImageInfo.extent = { (extent.width), (extent.height), 1 };
			createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
			createImageInfo.format = i % MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL == 0 ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_R8G8B8A8_UNORM;
			createImageInfo.mipLevelCount = MAX_MIP_COUNT;
			createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			m_outputModel.images[i].reset(new Image(createImageInfo));

			auto computeImageSize = [](VkExtent3D extent, float bbp, uint32_t mipLevel)
				{
					const VkExtent3D adjustedExtent = { extent.width >> mipLevel, extent.height >> mipLevel, extent.depth };
					return static_cast<VkDeviceSize>(static_cast<float>(adjustedExtent.width) * static_cast<float>(adjustedExtent.height) * static_cast<float>(adjustedExtent.depth) * bbp);
				};

			for (uint32_t mipLevel = 0; mipLevel < m_outputModel.images[i]->getMipLevelCount(); ++mipLevel)
			{
				VkDeviceSize imageSize = computeImageSize(extent, m_outputModel.images[i]->getBBP(), mipLevel);

				std::vector<unsigned char> pixels(imageSize);
				input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

				m_outputModel.images[i]->copyCPUBuffer(pixels.data(), Image::SampledInFragmentShader(mipLevel), mipLevel);
			}
		}

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->unlock();

		return true;
	}

	Debug::sendInfo("Cache not found");
	return false;
}

void Wolf::ModelLoader::loadMaterial(const tinyobj::material_t& material, const std::string& mtlFolder, ModelLoadingInfo::InputMaterialLayout materialLayout, uint32_t& indexTexture)
{
	Debug::sendInfo("Loading material " + material.name);

	if (materialLayout == ModelLoadingInfo::InputMaterialLayout::EACH_TEXTURE_A_FILE)
	{
		// Albedo
		{
			std::vector<ImageCompression::RGBA8> pixels;
			std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
			VkExtent3D extent;
			loadImageFile(getTexName(material.diffuse_texname, mtlFolder, "Textures/no_texture_albedo.png"), VK_FORMAT_R8G8B8A8_SRGB, pixels, mipLevels, extent);

			std::vector<ImageCompression::BC1> compressedBlocks;
			std::vector<std::vector<ImageCompression::BC1>> mipBlocks(mipLevels.size());
			ImageCompression::compressBC1(extent, pixels, compressedBlocks);
			uint32_t mipWidth = extent.width / 2;
			uint32_t mipHeight = extent.height / 2;
			for (uint32_t i = 0; i < mipLevels.size(); ++i)
			{
				ImageCompression::compressBC1({ mipWidth, mipHeight, 1 }, mipLevels[i], mipBlocks[i]);

				mipWidth /= 2;
				mipHeight /= 2;
			}

			std::vector<const unsigned char*> mipsData(mipBlocks.size());
			for (uint32_t i = 0; i < mipBlocks.size(); ++i)
			{
				mipsData[i] = reinterpret_cast<const unsigned char*>(mipBlocks[i].data());
			}
			createImageFromData(extent, VK_FORMAT_BC1_RGB_SRGB_BLOCK, reinterpret_cast<const unsigned char*>(compressedBlocks.data()), mipsData, indexTexture);

			indexTexture++;
		}

		// Normal
		{
			std::vector<ImageCompression::RGBA8> pixels;
			std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
			VkExtent3D extent;
			loadImageFile(getTexName(material.bump_texname, mtlFolder), VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

			std::vector<const unsigned char*> mipsData(mipLevels.size());
			for (uint32_t i = 0; i < mipLevels.size(); ++i)
			{
				mipsData[i] = reinterpret_cast<const unsigned char*>(mipLevels[i].data());
			}
			createImageFromData(extent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(pixels.data()), mipsData, indexTexture);

			indexTexture++;
		}

		std::vector<ImageCompression::RGBA8> combinedRoughnessMetalnessAO;
		std::vector<std::vector<ImageCompression::RGBA8>> combinedRoughnessMetalnessAOMipLevels;
		VkExtent3D combinedRoughnessMetalnessAOExtent;

		// Roughness
		{
			std::vector<ImageCompression::RGBA8> pixels;
			std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
			loadImageFile(getTexName(material.specular_highlight_texname, mtlFolder), VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, combinedRoughnessMetalnessAOExtent);

			combinedRoughnessMetalnessAO.resize(pixels.size());
			for (uint32_t i = 0; i < pixels.size(); ++i)
			{
				combinedRoughnessMetalnessAO[i].r = pixels[i].r;
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
		{
			std::vector<ImageCompression::RGBA8> pixels;
			std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
			VkExtent3D extent;
			loadImageFile(getTexName(material.specular_texname, mtlFolder), VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

			if (extent.width != combinedRoughnessMetalnessAOExtent.width || extent.height != combinedRoughnessMetalnessAOExtent.height)
			{
				Debug::sendWarning("Metalness has not same resolution than roughness, this is not supported and will be set to default value");
			}
			else
			{
				for (uint32_t i = 0; i < pixels.size(); ++i)
				{
					combinedRoughnessMetalnessAO[i].g = pixels[i].r;
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
		{
			std::vector<ImageCompression::RGBA8> pixels;
			std::vector<std::vector<ImageCompression::RGBA8>> mipLevels;
			VkExtent3D extent;
			loadImageFile(getTexName(material.ambient_texname, mtlFolder), VK_FORMAT_R8G8B8A8_UNORM, pixels, mipLevels, extent);

			if (extent.width != combinedRoughnessMetalnessAOExtent.width || extent.height != combinedRoughnessMetalnessAOExtent.height)
			{
				Debug::sendWarning("AO has not same resolution than roughness, this is not supported and will be set to default value");
			}
			else
			{
				for (uint32_t i = 0; i < pixels.size(); ++i)
				{
					combinedRoughnessMetalnessAO[i].b = pixels[i].r;
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

		std::vector<const unsigned char*> mipsData(combinedRoughnessMetalnessAOMipLevels.size());
		for (uint32_t i = 0; i < combinedRoughnessMetalnessAOMipLevels.size(); ++i)
		{
			mipsData[i] = reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAOMipLevels[i].data());
		}
		createImageFromData(combinedRoughnessMetalnessAOExtent, VK_FORMAT_R8G8B8A8_UNORM, reinterpret_cast<const unsigned char*>(combinedRoughnessMetalnessAO.data()), mipsData, indexTexture);
		indexTexture++;
	}
}

inline std::string Wolf::ModelLoader::getTexName(const std::string& texName, const std::string& folder, const std::string& defaultTexture)
{
	return !texName.empty() ? folder + "/" + texName : defaultTexture;
}

void Wolf::ModelLoader::loadImageFile(const std::string& filename, VkFormat format, std::vector<ImageCompression::RGBA8>& pixels, std::vector<std::vector<ImageCompression::RGBA8>>& mipLevels, VkExtent3D& outExtent) const
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

void Wolf::ModelLoader::createImageFromData(VkExtent3D extent, VkFormat format, const unsigned char* pixels, const std::vector<const unsigned char*>& mipLevels, uint32_t idx)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = extent;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = format;
	createImageInfo.mipLevelCount = static_cast<uint32_t>(mipLevels.size()) + 1;
	createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_outputModel.images[idx].reset(new Image(createImageInfo));
	m_outputModel.images[idx]->copyCPUBuffer(pixels, Image::SampledInFragmentShader());

	for (uint32_t mipLevel = 1; mipLevel < mipLevels.size() + 1; ++mipLevel)
	{
		m_outputModel.images[idx]->copyCPUBuffer(mipLevels[mipLevel - 1], Image::SampledInFragmentShader(mipLevel), mipLevel);
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
			imageSize += computeImageSize(m_outputModel.images[idx]->getExtent(), m_outputModel.images[idx]->getBBP(), mipLevel);

		m_imagesData[idx].resize(imageSize);

		VkDeviceSize copyOffset = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipLevels.size() + 1; ++mipLevel)
		{
			const VkDeviceSize copySize = computeImageSize(m_outputModel.images[idx]->getExtent(), m_outputModel.images[idx]->getBBP(), mipLevel);
			const unsigned char* dataPtr = pixels;
			if (mipLevel > 0)
				dataPtr = mipLevels[mipLevel - 1];
			memcpy(m_imagesData[idx].data() + copyOffset, dataPtr, copySize);

			copyOffset += copySize;
		}
	}
}