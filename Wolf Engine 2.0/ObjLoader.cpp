#include "ObjLoader.h"

#include <filesystem>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <thread>

#include "AndroidCacheHelper.h"
#include "ImageFileLoader.h"
#include "MipMapGenerator.h"

Wolf::ObjLoader::ObjLoader(ModelLoadingInfo& modelLoadingInfo)
{
#ifdef __ANDROID__
    if(modelLoadingInfo.isInAssets)
    {
        const std::string appFolderName = "model_cache";
        copyCompressedFileToStorage(modelLoadingInfo.filename, appFolderName, modelLoadingInfo.filename);
    }
#endif

	if (modelLoadingInfo.useCache)
	{
		std::string binFilename = modelLoadingInfo.filename + ".bin";

		if (std::filesystem::exists(binFilename))
		{
			std::ifstream input(binFilename, std::ios::in | std::ios::binary);

			uint32_t verticesCount;
			input.read(reinterpret_cast<char*>(&verticesCount), sizeof(verticesCount));
			std::vector<Vertex3D> vertices(verticesCount);
			input.read(reinterpret_cast<char*>(vertices.data()), verticesCount * sizeof(vertices[0]));

			uint32_t indicesCount;
			input.read(reinterpret_cast<char*>(&indicesCount), sizeof(indicesCount));
			std::vector<uint32_t> indices(indicesCount);
			input.read(reinterpret_cast<char*>(indices.data()), indicesCount * sizeof(indices[0]));

			m_mesh.reset(new Mesh(vertices, indices, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages, VK_FORMAT_R32G32B32_SFLOAT));

			uint32_t textureCount;
			input.read(reinterpret_cast<char*>(&textureCount), sizeof(textureCount));
			m_images.resize(textureCount);

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
				createImageInfo.format = i % 5 == 0 ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
				createImageInfo.mipLevelCount = MAX_MIP_COUNT;
				createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				m_images[i].reset(new Image(createImageInfo));

				for (uint32_t mipLevel = 0; mipLevel < m_images[i]->getMipLevelCount(); ++mipLevel)
				{
					VkDeviceSize imageSize = (extent.width * extent.height * 4) >> mipLevel;

					std::vector<unsigned char> pixels(imageSize);
					input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());

					m_images[i]->copyCPUBuffer(pixels.data(), Image::SampledInFragmentShader(mipLevel), mipLevel);
				}
			}

			if (modelLoadingInfo.vulkanQueueLock)
				modelLoadingInfo.vulkanQueueLock->unlock();

			return;
		}
	}

	m_useCache = modelLoadingInfo.useCache;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err, warn;

	if (!LoadObj(&attrib, &shapes, &materials, &warn, &err, modelLoadingInfo.filename.c_str(), modelLoadingInfo.mtlFolder.c_str()))
		Debug::sendCriticalError(err);

	if (!err.empty())
		Debug::sendInfo("[Loading objet file] Error : " + err + " for " + modelLoadingInfo.filename + " !");
	if (!warn.empty())
		Debug::sendInfo("[Loading objet file]  Warning : " + warn + " for " + modelLoadingInfo.filename + " !");

	std::unordered_map<Vertex3D, uint32_t> uniqueVertices = {};
	std::vector<Vertex3D> vertices;
	std::vector<uint32_t> indices;

	std::vector<uint32_t> lastIndices;

	for (const auto& shape : shapes)
	{
		int numVertex = 0;
		for (const auto& index : shape.mesh.indices)
		{
			Vertex3D vertex = {};

			int materialID = shape.mesh.material_ids[numVertex / 3];

			if (modelLoadingInfo.loadMaterials && materialID < 0)
				continue;

			vertex.pos =
			{
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
			};

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

			if (!modelLoadingInfo.loadMaterials)
				vertex.materialID = modelLoadingInfo.materialIdOffset;
			else vertex.materialID = modelLoadingInfo.materialIdOffset + shape.mesh.material_ids[numVertex / 3];

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			if (std::find(m_toBeLast.begin(), m_toBeLast.end(), materialID) == m_toBeLast.end())
				indices.push_back(uniqueVertices[vertex]);
			else
				lastIndices.push_back(uniqueVertices[vertex]);

			numVertex++;
		}
	}

	for (int i(0); i < lastIndices.size(); ++i)
		indices.push_back(lastIndices[i]);

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

	if (modelLoadingInfo.loadMaterials)
	{
		m_images.resize(materials.size() * 5);
		if (m_useCache)
			m_imagesData.resize(materials.size() * 5);
		int indexTexture = 0;
		for (auto& material : materials)
		{
			setImage(getTexName(material.diffuse_texname, modelLoadingInfo.mtlFolder), indexTexture++, true);
			setImage(getTexName(material.bump_texname, modelLoadingInfo.mtlFolder), indexTexture++, false);
			setImage(getTexName(material.specular_highlight_texname, modelLoadingInfo.mtlFolder), indexTexture++, false);
			setImage(getTexName(material.ambient_texname, modelLoadingInfo.mtlFolder), indexTexture++, false);
			setImage(getTexName(material.ambient_texname, modelLoadingInfo.mtlFolder), indexTexture++, false);
		}
	}

	m_mesh.reset(new Mesh(vertices, indices, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages));

	Debug::sendInfo("Model loaded with " + std::to_string(indices.size() / 3) + " triangles");

	if (modelLoadingInfo.useCache)
	{
		std::fstream outCacheFile(modelLoadingInfo.filename + ".bin", std::ios::out | std::ios::binary);

		/* Geometry */
		uint32_t verticesCount = vertices.size();
		outCacheFile.write(reinterpret_cast<char*>(&verticesCount), sizeof(uint32_t));
		outCacheFile.write(reinterpret_cast<char*>(vertices.data()), verticesCount * sizeof(vertices[0]));
		uint32_t indicesCount = indices.size();
		outCacheFile.write(reinterpret_cast<char*>(&indicesCount), sizeof(uint32_t));
		outCacheFile.write(reinterpret_cast<char*>(indices.data()), indicesCount * sizeof(indices[0]));

		/* Textures*/
		uint32_t imageCount = m_images.size();
		outCacheFile.write(reinterpret_cast<char*>(&imageCount), sizeof(uint32_t));
		for (uint32_t i = 0; i < m_imagesData.size(); ++i)
		{
			VkExtent3D extent = m_images[i]->getExtent();
			outCacheFile.write(reinterpret_cast<char*>(&extent), sizeof(VkExtent3D));
			outCacheFile.write(reinterpret_cast<char*>(m_imagesData[i].data()), m_imagesData[i].size());
		}

		outCacheFile.close();
	}
}

void Wolf::ObjLoader::getImages(std::vector<Image*>& images)
{
	images.reserve(m_images.size());
	for (std::unique_ptr<Image>& image : m_images)
	{
		images.push_back(image.get());
	}
}

inline std::string Wolf::ObjLoader::getTexName(const std::string& texName, const std::string& folder)
{
	return !texName.empty() ? folder + "/" + texName : "Textures/white_pixel.jpg";
}

void Wolf::ObjLoader::setImage(const std::string& filename, uint32_t idx, bool sRGB)
{
	const VkFormat format = sRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	const ImageFileLoader imageFileLoader(filename);
	MipMapGenerator mipmapGenerator(imageFileLoader.getPixels(), { (uint32_t)imageFileLoader.getWidth(), (uint32_t)imageFileLoader.getHeight() }, format);

	CreateImageInfo createImageInfo;
	createImageInfo.extent = { (uint32_t)imageFileLoader.getWidth(), (uint32_t)imageFileLoader.getHeight(), 1 };
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = format;
	createImageInfo.mipLevelCount = mipmapGenerator.getMipLevelCount();
	createImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_images[idx].reset(new Image(createImageInfo));
	m_images[idx]->copyCPUBuffer(imageFileLoader.getPixels(), Image::SampledInFragmentShader());

	for (uint32_t mipLevel = 1; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
	{
		m_images[idx]->copyCPUBuffer(mipmapGenerator.getMipLevel(mipLevel), Image::SampledInFragmentShader(mipLevel), mipLevel);
	}

	if (m_useCache)
	{
		VkDeviceSize imageSize = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
			imageSize += (m_images[idx]->getExtent().width * m_images[idx]->getExtent().height * m_images[idx]->getExtent().depth * m_images[idx]->getBBP()) >> mipLevel;

		m_imagesData[idx].resize(imageSize);

		VkDeviceSize copyOffset = 0;
		for (uint32_t mipLevel = 0; mipLevel < mipmapGenerator.getMipLevelCount(); ++mipLevel)
		{
			const VkDeviceSize copySize = (m_images[idx]->getExtent().width * m_images[idx]->getExtent().height * m_images[idx]->getExtent().depth * m_images[idx]->getBBP()) >> mipLevel;
			const void* dataPtr = (void*)imageFileLoader.getPixels();
			if (mipLevel > 0)
				dataPtr = (void*)mipmapGenerator.getMipLevel(mipLevel);
			memcpy(m_imagesData[idx].data() + copyOffset, dataPtr, copySize);

			copyOffset += copySize;
		}
	}
}