#include "ModelLoader.h"

#include <filesystem>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <thread>

#include "AndroidCacheHelper.h"
#include "CodeFileHashes.h"
#include "ImageFileLoader.h"
#include "MaterialLoader.h"
#include "MaterialsGPUManager.h"
#include "MipMapGenerator.h"

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

			if (modelLoadingInfo.materialLayout == MaterialLoader::InputMaterialLayout::NO_MATERIAL)
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

	if (modelLoadingInfo.materialLayout != MaterialLoader::InputMaterialLayout::NO_MATERIAL)
	{
		m_outputModel.materials.resize(materials.size());
		m_imagesData.resize(materials.size() * MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL); // is kept empty if no cache is used
		uint32_t indexMaterial = 0;
		for (auto& material : materials)
		{
			loadMaterial(material, modelLoadingInfo.mtlFolder, modelLoadingInfo.materialLayout, indexMaterial);
			indexMaterial++;
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

		/* Materials */
		uint32_t materialCount = static_cast<uint32_t>(m_outputModel.materials.size());
		outCacheFile.write(reinterpret_cast<char*>(&materialCount), sizeof(uint32_t));

		for (uint32_t materialIdx = 0; materialIdx < materialCount; ++materialIdx)
		{
			MaterialsGPUManager::MaterialInfo& currentMaterial = m_outputModel.materials[materialIdx];

			uint32_t imageCount = MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL;
			outCacheFile.write(reinterpret_cast<char*>(&imageCount), sizeof(uint32_t));
			for (uint32_t imageIdx = 0; imageIdx < imageCount; ++imageIdx)
			{
				ResourceUniqueOwner<Image>& currentImage = currentMaterial.images[imageIdx];

				VkExtent3D extent = currentImage->getExtent();
				outCacheFile.write(reinterpret_cast<char*>(&extent), sizeof(VkExtent3D));
				const std::vector<unsigned char>& imageData = m_imagesData[materialIdx * MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL + imageIdx];
				outCacheFile.write(reinterpret_cast<const char*>(imageData.data()), static_cast<uint32_t>(imageData.size()));
			}

			uint32_t shadingMode = currentMaterial.shadingMode;
			outCacheFile.write(reinterpret_cast<char*>(&shadingMode), sizeof(uint32_t));

#ifdef MATERIAL_DEBUG
			uint32_t materialNameSize = static_cast<uint32_t>(currentMaterial.materialName.size());
			outCacheFile.write(reinterpret_cast<char*>(&materialNameSize), sizeof(uint32_t));
			outCacheFile.write(currentMaterial.materialName.c_str(), materialNameSize);

			uint32_t imageNameCount = static_cast<uint32_t>(currentMaterial.imageNames.size());
			outCacheFile.write(reinterpret_cast<char*>(&imageNameCount), sizeof(uint32_t));

			for (uint32_t imageNameIdx = 0; imageNameIdx < imageNameCount; ++imageNameIdx)
			{
				uint32_t imageNameSize = static_cast<uint32_t>(currentMaterial.imageNames[imageNameIdx].size());
				outCacheFile.write(reinterpret_cast<char*>(&imageNameSize), sizeof(uint32_t));
				outCacheFile.write(currentMaterial.imageNames[imageNameIdx].c_str(), imageNameSize);
			}

			uint32_t materialFolderSize = static_cast<uint32_t>(modelLoadingInfo.mtlFolder.size());
			outCacheFile.write(reinterpret_cast<char*>(&materialFolderSize), sizeof(uint32_t));
			outCacheFile.write(modelLoadingInfo.mtlFolder.c_str(), materialFolderSize);
#endif
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

		uint32_t materialCount;
		input.read(reinterpret_cast<char*>(&materialCount), sizeof(materialCount));
		m_outputModel.materials.resize(materialCount);

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->lock();

		std::chrono::steady_clock::time_point startTimer = std::chrono::steady_clock::now();

		for (uint32_t materialIdx(0); materialIdx < materialCount; ++materialIdx)
		{
			uint32_t imageCount;
			input.read(reinterpret_cast<char*>(&imageCount), sizeof(uint32_t));

			if (imageCount != MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL)
			{
				Debug::sendError("Image count is wrong, cache is maybe corrupted");
				return false;
			}

			for (uint32_t imageIdx = 0; imageIdx < imageCount; ++imageIdx)
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

				ResourceUniqueOwner<Image>& currentImage = m_outputModel.materials[materialIdx].images[imageIdx];

				CreateImageInfo createImageInfo;
				createImageInfo.extent = { (extent.width), (extent.height), 1 };
				createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
				createImageInfo.format = imageIdx == 0 ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_R8G8B8A8_UNORM;
				createImageInfo.mipLevelCount = MAX_MIP_COUNT;
				createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				currentImage.reset(Image::createImage(createImageInfo));

				auto computeImageSize = [](VkExtent3D extent, float bbp, uint32_t mipLevel)
					{
						const VkExtent3D adjustedExtent = { extent.width >> mipLevel, extent.height >> mipLevel, extent.depth };
						return static_cast<VkDeviceSize>(static_cast<float>(adjustedExtent.width) * static_cast<float>(adjustedExtent.height) * static_cast<float>(adjustedExtent.depth) * bbp);
					};

				for (uint32_t mipLevel = 0; mipLevel < currentImage->getMipLevelCount(); ++mipLevel)
				{
					VkDeviceSize imageSize = computeImageSize(extent, currentImage->getBBP(), mipLevel);

					std::vector<unsigned char> pixels(imageSize);
					input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

					currentImage->copyCPUBuffer(pixels.data(), Image::SampledInFragmentShader(mipLevel), mipLevel);
				}
			}

			input.read(reinterpret_cast<char*>(&m_outputModel.materials[materialIdx].shadingMode), sizeof(uint32_t));

#ifdef MATERIAL_DEBUG
			uint32_t materialNameSize;
			input.read(reinterpret_cast<char*>(&materialNameSize), sizeof(uint32_t));
			m_outputModel.materials[materialIdx].materialName.resize(materialNameSize);
			input.read(m_outputModel.materials[materialIdx].materialName.data(), materialNameSize);

			uint32_t imageNameCount;
			input.read(reinterpret_cast<char*>(&imageNameCount), sizeof(uint32_t));
			m_outputModel.materials[materialIdx].imageNames.resize(imageNameCount);

			for (uint32_t imageNameIdx = 0; imageNameIdx < imageNameCount; ++imageNameIdx)
			{
				uint32_t imageNameSize;
				input.read(reinterpret_cast<char*>(&imageNameSize), sizeof(uint32_t));
				m_outputModel.materials[materialIdx].imageNames[imageNameIdx].resize(imageNameSize);
				input.read(m_outputModel.materials[materialIdx].imageNames[imageNameIdx].data(), imageNameSize);
			}

			uint32_t materialFolderSize;
			input.read(reinterpret_cast<char*>(&materialFolderSize), sizeof(uint32_t));
			m_outputModel.materials[materialIdx].materialFolder.resize(materialFolderSize);
			input.read(m_outputModel.materials[materialIdx].materialFolder.data(), materialFolderSize);
#endif
		}

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->unlock();

		return true;
	}

	Debug::sendInfo("Cache not found");
	return false;
}

inline std::string getTexName(const std::string& texName, const std::string& folder, const std::string& defaultTexture = "Textures/white_pixel.jpg")
{
	return !texName.empty() ? folder + "/" + texName : defaultTexture;
}


void Wolf::ModelLoader::loadMaterial(const tinyobj::material_t& material, const std::string& mtlFolder, MaterialLoader::InputMaterialLayout materialLayout, uint32_t indexMaterial)
{
	MaterialLoader::MaterialFileInfoGGX materialFileInfo{};
	materialFileInfo.name = material.name;
	materialFileInfo.albedo = getTexName(material.diffuse_texname, mtlFolder, "Textures/no_texture_albedo.png");
	materialFileInfo.normal = getTexName(material.bump_texname, mtlFolder);
	materialFileInfo.roughness = getTexName(material.specular_highlight_texname, mtlFolder);
	materialFileInfo.metalness = getTexName(material.specular_texname, mtlFolder);
	materialFileInfo.ao = getTexName(material.ambient_texname, mtlFolder);
	materialFileInfo.anisoStrength = getTexName(material.sheen_texname, mtlFolder);

#ifdef MATERIAL_DEBUG
	m_outputModel.materials[indexMaterial].materialName = materialFileInfo.name;
	m_outputModel.materials[indexMaterial].imageNames =
	{
		material.diffuse_texname,
		material.bump_texname,
		material.specular_highlight_texname,
		material.specular_texname,
		material.ambient_texname,
		material.sheen_texname
	};
	m_outputModel.materials[indexMaterial].materialFolder = mtlFolder;
#endif

	MaterialLoader::OutputLayout outputLayout;
	outputLayout.albedoCompression = ImageCompression::Compression::BC1;

	MaterialLoader materialLoader(materialFileInfo, outputLayout, m_useCache);
	for (uint32_t i = 0; i < MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL; ++i)
	{
		if (m_useCache)
			materialLoader.assignCache(i, m_imagesData[indexMaterial * MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL + i]);
		m_outputModel.materials[indexMaterial].images[i].reset(materialLoader.releaseImage(i));
	}
	m_outputModel.materials[indexMaterial].shadingMode = material.illum;
}