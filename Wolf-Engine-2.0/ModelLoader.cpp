#include "ModelLoader.h"

#include <filesystem>
#ifndef __ANDROID__
#include <meshoptimizer.h>
#endif
#define TINYOBJLOADER_IMPLEMENTATION
#include "ModelLoader.h"

#include <tiny_obj_loader.h>
#include <thread>

#include "AndroidCacheHelper.h"
#include "CodeFileHashes.h"
#include "ConfigurationHelper.h"
#include "DAEImporter.h"
#include "ImageFileLoader.h"
#include "JSONReader.h"
#include "TextureSetLoader.h"
#include "MaterialsGPUManager.h"

inline void writeStringToCache(const std::string& str, std::fstream& outCacheFile)
{
	uint32_t strLength = static_cast<uint32_t>(str.size());
	outCacheFile.write(reinterpret_cast<char*>(&strLength), sizeof(uint32_t));
	outCacheFile.write(str.c_str(), strLength);
}

inline std::string readStringFromCache(std::ifstream& input)
{
	std::string str;
	uint32_t strLength;
	input.read(reinterpret_cast<char*>(&strLength), sizeof(uint32_t));
	str.resize(strLength);
	input.read(str.data(), strLength);

	return str;
}

void Wolf::ModelLoader::loadObject(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo)
{
#ifdef MATERIAL_DEBUG
	outputModel.m_originFilepath = modelLoadingInfo.filename;
#endif

	std::string filenameExtension = modelLoadingInfo.filename.substr(modelLoadingInfo.filename.find_last_of(".") + 1);
	if (filenameExtension == "obj")
	{
		ModelLoader objLoader(outputModel, modelLoadingInfo);
	}
	else if (filenameExtension == "dae")
	{
		DAEImporter daeImporter(outputModel, modelLoadingInfo);
	}
	else
	{
		Debug::sendCriticalError("Unhandled filename extension");
	}

	// Load physics
	std::string physicsConfigFilename = modelLoadingInfo.filename + ".physics.json";
	if (std::filesystem::exists(physicsConfigFilename))
	{
		JSONReader physicsJSONReader(JSONReader::FileReadInfo{ physicsConfigFilename });

		uint32_t physicsMeshesCount = static_cast<uint32_t>(physicsJSONReader.getRoot()->getPropertyFloat("physicsMeshesCount"));
		outputModel.m_physicsShapes.resize(physicsMeshesCount);

		for (uint32_t i = 0; i < physicsMeshesCount; ++i)
		{
			JSONReader::JSONObjectInterface* physicsMeshObject = physicsJSONReader.getRoot()->getArrayObjectItem("physicsMeshes", i);
			std::string type = physicsMeshObject->getPropertyString("type");

			if (type == "Rectangle")
			{
				glm::vec3 p0 = glm::vec3(physicsMeshObject->getPropertyFloat("defaultP0X"), physicsMeshObject->getPropertyFloat("defaultP0Y"), physicsMeshObject->getPropertyFloat("defaultP0Z"));
				glm::vec3 s1 = glm::vec3(physicsMeshObject->getPropertyFloat("defaultS1X"), physicsMeshObject->getPropertyFloat("defaultS1Y"), physicsMeshObject->getPropertyFloat("defaultS1Z"));
				glm::vec3 s2 = glm::vec3(physicsMeshObject->getPropertyFloat("defaultS2X"), physicsMeshObject->getPropertyFloat("defaultS2Y"), physicsMeshObject->getPropertyFloat("defaultS2Z"));

				outputModel.m_physicsShapes[i].reset(new Physics::Rectangle(physicsMeshObject->getPropertyString("name"), p0, s1, s2));
			}
			else if (type == "Box")
			{
				glm::vec3 aabbMin = glm::vec3(physicsMeshObject->getPropertyFloat("defaultAABBMinX"), physicsMeshObject->getPropertyFloat("defaultAABBMinY"), physicsMeshObject->getPropertyFloat("defaultAABBMinZ"));
				glm::vec3 aabbMax = glm::vec3(physicsMeshObject->getPropertyFloat("defaultAABBMaxX"), physicsMeshObject->getPropertyFloat("defaultAABBMaxY"), physicsMeshObject->getPropertyFloat("defaultAABBMaxZ"));
				glm::vec3 rotation = glm::vec3(physicsMeshObject->getPropertyFloat("defaultRotationX"), physicsMeshObject->getPropertyFloat("defaultRotationY"), physicsMeshObject->getPropertyFloat("defaultRotationZ"));

				outputModel.m_physicsShapes[i].reset(new Physics::Box(physicsMeshObject->getPropertyString("name"), aabbMin, aabbMax, rotation));
			}
			else
			{
				Debug::sendCriticalError("Unhandled physics mesh type");
			}
		}
	}
}

Wolf::ModelLoader::ModelLoader(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo) : m_outputModel(&outputModel)
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
	std::map<int32_t /* materialId */, uint32_t /* subMeshIdx */> m_materialIdToSubMeshIdx;

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

			vertex.color =
			{
				sRGBtoLinear(attrib.colors[3 * index.vertex_index + 0]),
				sRGBtoLinear(attrib.colors[3 * index.vertex_index + 1]),
				sRGBtoLinear(attrib.colors[3 * index.vertex_index + 2])
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

			int32_t materialId = mesh.material_ids[numVertex / 3];
			if (materialId < 0)
			{
				materialId = -1;
				hasEncounteredAnInvalidMaterialId = true;
			}

			if (!m_materialIdToSubMeshIdx.contains(materialId))
			{
				m_materialIdToSubMeshIdx[materialId] = static_cast<uint32_t>(m_materialIdToSubMeshIdx.size());
			}
			
			vertex.subMeshIdx = m_materialIdToSubMeshIdx[materialId];

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
		Debug::sendWarning("Loading model " + modelLoadingInfo.filename + ", invalid material ID found. Switching to default (0)");

	glm::vec3 center = (maxPos + minPos) * 0.5f;
	if (glm::length(center) > glm::length(maxPos) * 0.1f)
	{
		if (ConfigurationHelper::readInfoFromFile(modelLoadingInfo.filename + ".config", "forceCenter") == "true")
		{
			Debug::sendInfo("Mesh is forced to be centered");

			for (Vertex3D& vertex : vertices)
			{
				vertex.pos -= center;
			}

			maxPos -= center;
			minPos -= center;

			m_outputModel->m_isMeshCentered = true;
		}
		else
		{
			Debug::sendWarning("Model " + modelLoadingInfo.filename + " is not centered");
			m_outputModel->m_isMeshCentered = false;
		}
	}
	else
	{
		m_outputModel->m_isMeshCentered = true;
	}

	AABB aabb(minPos, maxPos);
	BoundingSphere boundingSphere(glm::vec3(0.0f), glm::max(glm::length(minPos), glm::length(maxPos)));

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

#ifndef __ANDROID__
	// Optimize mesh
	std::vector<unsigned int> remap(indices.size()); // temporary remap table
	size_t meshOptVertexCount = meshopt_generateVertexRemap(&remap[0], indices.data(), indices.size(),
		vertices.data(), vertices.size(), sizeof(Vertex3D));
	std::vector<uint32_t> newIndices(indices.size());
	meshopt_remapIndexBuffer(newIndices.data(), indices.data(), indices.size(), &remap[0]);
	std::vector<Vertex3D> newVertices(meshOptVertexCount);
	meshopt_remapVertexBuffer(newVertices.data(), vertices.data(), vertices.size(), sizeof(Vertex3D), &remap[0]);

	vertices = std::move(newVertices);
	indices = std::move(newIndices);

	meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());

	std::vector<std::vector<uint32_t>> savedDefaultLODs(modelLoadingInfo.generateDefaultLODCount);
	std::vector<std::vector<uint32_t>> savedSloppyLODs(modelLoadingInfo.generateSloppyLODCount);

	size_t targetIndexCount = indices.size();
	float targetError = 1.0;
	for (uint32_t lodIdx = 0; lodIdx < modelLoadingInfo.generateDefaultLODCount; ++lodIdx)
	{
		if (targetIndexCount <= 16)
		{
			modelLoadingInfo.generateDefaultLODCount = lodIdx;
			break;
		}

		targetIndexCount *= 0.5f;

		std::vector<uint32_t>& lod = savedDefaultLODs[lodIdx];
		lod.resize(indices.size());
		float lodError = 0.0f;
		static_assert(offsetof(Vertex3D, pos) == 0);
		lod.resize(meshopt_simplify(&lod[0], indices.data(), indices.size(), reinterpret_cast<float*>(vertices.data() /* note that it makes the assumption that pos is the first data */),
			vertices.size(), sizeof(Vertex3D), targetIndexCount, targetError, 0, &lodError));

		if (lod.size() > targetIndexCount * 1.5f) // target not reached
		{
			modelLoadingInfo.generateDefaultLODCount = lodIdx;
			break;
		}

		m_outputModel->defaultSimplifiedIndexBuffers.push_back(Buffer::createBuffer(lod.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | modelLoadingInfo.additionalIndexBufferUsages,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		m_outputModel->defaultSimplifiedIndexBuffers.back()->transferCPUMemoryWithStagingBuffer(lod.data(), lod.size() * sizeof(uint32_t));

		ModelData::LODInfo lodInfo;
		lodInfo.m_error = lodError;
		lodInfo.m_indexCount = lod.size();
		m_outputModel->m_defaultLODsInfo.push_back(lodInfo);
	}

	targetIndexCount = indices.size();
	targetError = 1.0;
	for (uint32_t lodIdx = 0; lodIdx < modelLoadingInfo.generateSloppyLODCount; ++lodIdx)
	{
		if (targetIndexCount <= 16)
		{
			modelLoadingInfo.generateSloppyLODCount = lodIdx;
			break;
		}

		targetIndexCount *= 0.5f;

		std::vector<uint32_t>& lod = savedSloppyLODs[lodIdx];
		lod.resize(indices.size());
		float lodError = 0.0f;
		static_assert(offsetof(Vertex3D, pos) == 0);
		lod.resize(meshopt_simplifySloppy(&lod[0], indices.data(), indices.size(), reinterpret_cast<float*>(vertices.data() /* note that it makes the assumption that pos is the first data */),
			vertices.size(), sizeof(Vertex3D), targetIndexCount, targetError, &lodError));

		if (lod.size() <= 16)
		{
			modelLoadingInfo.generateSloppyLODCount = lodIdx;
			break;
		}

		m_outputModel->sloppySimplifiedIndexBuffers.push_back(Buffer::createBuffer(lod.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | modelLoadingInfo.additionalIndexBufferUsages,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		m_outputModel->sloppySimplifiedIndexBuffers.back()->transferCPUMemoryWithStagingBuffer(lod.data(), lod.size() * sizeof(uint32_t));

		ModelData::LODInfo lodInfo;
		lodInfo.m_error = lodError;
		lodInfo.m_indexCount = lod.size();
		m_outputModel->m_sloppyLODsInfo.push_back(lodInfo);
	}
#endif

	m_outputModel->mesh.reset(new Mesh(vertices, indices, aabb, boundingSphere, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages));

	for (uint32_t shapeIdx = 0; shapeIdx < shapeInfos.size(); ++shapeIdx)
	{
		const InternalShapeInfo& shapeInfo = shapeInfos[shapeIdx];
		const InternalShapeInfo& nextShapeInfo = shapeIdx == shapeInfos.size() - 1 ? InternalShapeInfo{ static_cast<uint32_t>(indices.size()) } : shapeInfos[shapeIdx + 1];

		m_outputModel->mesh->addSubMesh(shapeInfo.indicesOffset, nextShapeInfo.indicesOffset - shapeInfo.indicesOffset);
	}

	Debug::sendInfo("Model " + modelLoadingInfo.filename + " loaded with " + std::to_string(indices.size() / 3) + " triangles and " + std::to_string(shapeInfos.size()) + " shapes");

	if (modelLoadingInfo.textureSetLayout != TextureSetLoader::InputTextureSetLayout::NO_MATERIAL)
	{
		// Special case when there's no material (obj loader creates a 'none' one)
		if (materials.size() == 1 && materials[0].diffuse_texname.empty())
		{
			materials.clear();
		}

		m_outputModel->m_textureSets.resize(materials.size());
		if (m_useCache)
		{
			m_infoForCache.resize(materials.size());
		}

		uint32_t textureSetIdx = 0;
		for (uint32_t subMeshIdx = 0; subMeshIdx < m_materialIdToSubMeshIdx.size(); ++subMeshIdx)
		{
			int32_t tinyObjMaterialIdx = 0;
			for (; tinyObjMaterialIdx < static_cast<int32_t>(materials.size()); ++tinyObjMaterialIdx)
			{
				if (m_materialIdToSubMeshIdx[tinyObjMaterialIdx] == subMeshIdx)
					break;
			}

			if (tinyObjMaterialIdx == static_cast<int32_t>(materials.size()))
				continue; // the submesh probably don't have a material

#ifdef MATERIAL_DEBUG
			m_outputModel->m_textureSets[textureSetIdx].name = materials[tinyObjMaterialIdx].name;
			m_outputModel->m_textureSets[textureSetIdx].imageNames =
			{
				materials[tinyObjMaterialIdx].diffuse_texname,
				materials[tinyObjMaterialIdx].bump_texname,
				materials[tinyObjMaterialIdx].specular_highlight_texname,
				materials[tinyObjMaterialIdx].specular_texname,
				materials[tinyObjMaterialIdx].ambient_texname,
				materials[tinyObjMaterialIdx].sheen_texname
			};
			m_outputModel->m_textureSets[textureSetIdx].materialFolder = modelLoadingInfo.mtlFolder;
#endif

			TextureSetLoader::TextureSetFileInfoGGX textureSetFileInfoGGX = createTextureSetFileInfoGGXFromTinyObjMaterial(materials[tinyObjMaterialIdx], modelLoadingInfo.mtlFolder);

			if (m_useCache)
			{
				m_infoForCache[textureSetIdx].m_textureSetFileInfoGGX = textureSetFileInfoGGX;
			}

			loadTextureSet(textureSetFileInfoGGX, textureSetIdx);

			textureSetIdx++;
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

		/* Texture sets */
		uint32_t textureSetCount = static_cast<uint32_t>(m_outputModel->m_textureSets.size());
		outCacheFile.write(reinterpret_cast<char*>(&textureSetCount), sizeof(uint32_t));

		for (uint32_t textureSetIdx = 0; textureSetIdx < textureSetCount; ++textureSetIdx)
		{
			MaterialsGPUManager::TextureSetInfo& currentTextureSet = m_outputModel->m_textureSets[textureSetIdx];
			InfoForCachePerTextureSet& currentInfoForCache = m_infoForCache[textureSetIdx];

			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.name, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.albedo, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.normal, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.roughness, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.metalness, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.ao, outCacheFile);
			writeStringToCache(currentInfoForCache.m_textureSetFileInfoGGX.anisoStrength, outCacheFile);

#ifdef MATERIAL_DEBUG
			writeStringToCache(currentTextureSet.name, outCacheFile);

			uint32_t imageNameCount = static_cast<uint32_t>(currentTextureSet.imageNames.size());
			outCacheFile.write(reinterpret_cast<char*>(&imageNameCount), sizeof(uint32_t));

			for (uint32_t imageNameIdx = 0; imageNameIdx < imageNameCount; ++imageNameIdx)
			{
				writeStringToCache(currentTextureSet.imageNames[imageNameIdx], outCacheFile);
			}

			writeStringToCache(modelLoadingInfo.mtlFolder, outCacheFile);
#endif
		}

		/* LODs */
		uint32_t defaultLODCount = modelLoadingInfo.generateDefaultLODCount;
		outCacheFile.write(reinterpret_cast<char*>(&defaultLODCount), sizeof(uint32_t));

#ifndef __ANDROID__
		for (uint32_t lodIdx = 0; lodIdx < defaultLODCount; ++lodIdx)
		{
			uint32_t indexCount = static_cast<uint32_t>(savedDefaultLODs[lodIdx].size());
			outCacheFile.write(reinterpret_cast<char*>(&indexCount), sizeof(uint32_t));
			outCacheFile.write(reinterpret_cast<char*>(savedDefaultLODs[lodIdx].data()), indexCount * sizeof(uint32_t));
			outCacheFile.write(reinterpret_cast<char*>(&m_outputModel->m_defaultLODsInfo[lodIdx]), sizeof(ModelData::LODInfo));
		}
#endif

		uint32_t sloppyLODCount = modelLoadingInfo.generateSloppyLODCount;
		outCacheFile.write(reinterpret_cast<char*>(&sloppyLODCount), sizeof(uint32_t));

#ifndef __ANDROID__
		for (uint32_t lodIdx = 0; lodIdx < sloppyLODCount; ++lodIdx)
		{
			uint32_t indexCount = static_cast<uint32_t>(savedSloppyLODs[lodIdx].size());
			outCacheFile.write(reinterpret_cast<char*>(&indexCount), sizeof(uint32_t));
			outCacheFile.write(reinterpret_cast<char*>(savedSloppyLODs[lodIdx].data()), indexCount * sizeof(uint32_t));
			outCacheFile.write(reinterpret_cast<char*>(&m_outputModel->m_sloppyLODsInfo[lodIdx]), sizeof(ModelData::LODInfo));
		}
#endif

		outCacheFile.close();
	}
}

bool Wolf::ModelLoader::loadCache(ModelLoadingInfo& modelLoadingInfo)
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

		uint32_t indicesCount;
		input.read(reinterpret_cast<char*>(&indicesCount), sizeof(indicesCount));
		std::vector<uint32_t> indices(indicesCount);
		input.read(reinterpret_cast<char*>(indices.data()), indicesCount * sizeof(indices[0]));

		AABB aabb{};
		input.read(reinterpret_cast<char*>(&aabb), sizeof(AABB));

		glm::vec3 minPos = aabb.getMin();
		glm::vec3 maxPos = aabb.getMax();
		BoundingSphere boundingSphere((minPos + maxPos) * 0.5f, glm::distance(minPos, maxPos) * 0.5f);

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->lock();
		m_outputModel->mesh.reset(new Mesh(vertices, indices, aabb, boundingSphere, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages, Format::R32G32B32_SFLOAT));
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

			m_outputModel->mesh->addSubMesh(shapeInfo.indicesOffset, nextShapeInfo.indicesOffset - shapeInfo.indicesOffset);
		}

		uint32_t textureSetCount;
		input.read(reinterpret_cast<char*>(&textureSetCount), sizeof(textureSetCount));
		m_outputModel->m_textureSets.resize(textureSetCount);

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->lock();

		for (uint32_t textureSetIdx(0); textureSetIdx < textureSetCount; ++textureSetIdx)
		{
			TextureSetLoader::TextureSetFileInfoGGX textureSetFileInfoGGX;
			textureSetFileInfoGGX.name = readStringFromCache(input);
			textureSetFileInfoGGX.albedo = readStringFromCache(input);
			textureSetFileInfoGGX.normal = readStringFromCache(input);
			textureSetFileInfoGGX.roughness = readStringFromCache(input);
			textureSetFileInfoGGX.metalness = readStringFromCache(input);
			textureSetFileInfoGGX.ao = readStringFromCache(input);
			textureSetFileInfoGGX.anisoStrength = readStringFromCache(input);

			loadTextureSet(textureSetFileInfoGGX, textureSetIdx);

#ifdef MATERIAL_DEBUG
			uint32_t textureSetNameSize;
			input.read(reinterpret_cast<char*>(&textureSetNameSize), sizeof(uint32_t));
			m_outputModel->m_textureSets[textureSetIdx].name.resize(textureSetNameSize);
			input.read(m_outputModel->m_textureSets[textureSetIdx].name.data(), textureSetNameSize);

			uint32_t imageNameCount;
			input.read(reinterpret_cast<char*>(&imageNameCount), sizeof(uint32_t));
			m_outputModel->m_textureSets[textureSetIdx].imageNames.resize(imageNameCount);

			for (uint32_t imageNameIdx = 0; imageNameIdx < imageNameCount; ++imageNameIdx)
			{
				uint32_t imageNameSize;
				input.read(reinterpret_cast<char*>(&imageNameSize), sizeof(uint32_t));
				m_outputModel->m_textureSets[textureSetIdx].imageNames[imageNameIdx].resize(imageNameSize);
				input.read(m_outputModel->m_textureSets[textureSetIdx].imageNames[imageNameIdx].data(), imageNameSize);
			}

			uint32_t materialFolderSize;
			input.read(reinterpret_cast<char*>(&materialFolderSize), sizeof(uint32_t));
			m_outputModel->m_textureSets[textureSetIdx].materialFolder.resize(materialFolderSize);
			input.read(m_outputModel->m_textureSets[textureSetIdx].materialFolder.data(), materialFolderSize);
#endif
		}

		uint32_t defaultLODCount;
		input.read(reinterpret_cast<char*>(&defaultLODCount), sizeof(defaultLODCount));

		m_outputModel->m_defaultLODsInfo.resize(defaultLODCount);
		for (uint32_t lodIdx = 0; lodIdx < defaultLODCount; ++lodIdx)
		{
			input.read(reinterpret_cast<char*>(&indicesCount), sizeof(indicesCount));
			std::vector<uint32_t> lod(indicesCount);
			input.read(reinterpret_cast<char*>(lod.data()), indicesCount * sizeof(lod[0]));

			m_outputModel->defaultSimplifiedIndexBuffers.push_back(Buffer::createBuffer(lod.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | modelLoadingInfo.additionalIndexBufferUsages,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
			m_outputModel->defaultSimplifiedIndexBuffers.back()->transferCPUMemoryWithStagingBuffer(lod.data(), lod.size() * sizeof(uint32_t));

			input.read(reinterpret_cast<char*>(&m_outputModel->m_defaultLODsInfo[lodIdx]), sizeof(ModelData::LODInfo));
		}

		uint32_t sloppyLODCount;
		input.read(reinterpret_cast<char*>(&sloppyLODCount), sizeof(sloppyLODCount));

		m_outputModel->m_sloppyLODsInfo.resize(sloppyLODCount);
		for (uint32_t lodIdx = 0; lodIdx < sloppyLODCount; ++lodIdx)
		{
			input.read(reinterpret_cast<char*>(&indicesCount), sizeof(indicesCount));
			std::vector<uint32_t> lod(indicesCount);
			input.read(reinterpret_cast<char*>(lod.data()), indicesCount * sizeof(lod[0]));

			m_outputModel->sloppySimplifiedIndexBuffers.push_back(Buffer::createBuffer(lod.size() * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | modelLoadingInfo.additionalIndexBufferUsages,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
			m_outputModel->sloppySimplifiedIndexBuffers.back()->transferCPUMemoryWithStagingBuffer(lod.data(), lod.size() * sizeof(uint32_t));

			input.read(reinterpret_cast<char*>(&m_outputModel->m_sloppyLODsInfo[lodIdx]), sizeof(ModelData::LODInfo));
		}

		if (modelLoadingInfo.vulkanQueueLock)
			modelLoadingInfo.vulkanQueueLock->unlock();

		return true;
	}

	Debug::sendInfo("Cache not found");
	return false;
}

inline std::string getTexName(const std::string& texName, const std::string& folder, const std::string& defaultTexture = "Textures\\white_pixel.jpg")
{
	return !texName.empty() ? folder + "/" + texName : defaultTexture;
}

Wolf::TextureSetLoader::TextureSetFileInfoGGX Wolf::ModelLoader::createTextureSetFileInfoGGXFromTinyObjMaterial(const tinyobj::material_t& material, const std::string& mtlFolder) const
{
	TextureSetLoader::TextureSetFileInfoGGX materialFileInfo{};
	materialFileInfo.name = material.name;
	materialFileInfo.albedo = getTexName(material.diffuse_texname, mtlFolder, "Textures\\no_texture_albedo.png");
	materialFileInfo.normal = getTexName(material.bump_texname, mtlFolder);
	materialFileInfo.roughness = getTexName(material.specular_highlight_texname, mtlFolder);
	materialFileInfo.metalness = getTexName(material.specular_texname, mtlFolder);
	materialFileInfo.ao = getTexName(material.ambient_texname, mtlFolder);
	materialFileInfo.anisoStrength = getTexName(material.sheen_texname, mtlFolder);

	return materialFileInfo;
}

void Wolf::ModelLoader::loadTextureSet(const TextureSetLoader::TextureSetFileInfoGGX& material, uint32_t indexMaterial)
{
	TextureSetLoader::OutputLayout outputLayout;
	outputLayout.albedoCompression = ImageCompression::Compression::BC1;
	outputLayout.normalCompression = ImageCompression::Compression::BC5;

	TextureSetLoader materialLoader(material, outputLayout, m_useCache);
	for (uint32_t i = 0; i < MaterialsGPUManager::TEXTURE_COUNT_PER_MATERIAL; ++i)
	{
		materialLoader.transferImageTo(i, m_outputModel->m_textureSets[indexMaterial].images[i]);
		m_outputModel->m_textureSets[indexMaterial].slicesFolders[i] = materialLoader.getOutputSlicesFolder(i);
	}
}

float Wolf::ModelLoader::sRGBtoLinear(float component)
{
	// Clamp the input to the valid range [0.0, 1.0]
	component = std::max(0.0f, std::min(1.0f, component));

	if (component <= 0.04045f)
	{
		// Linear segment for dark colors
		return component / 12.92f;
	}
	else
	{
		// Gamma segment for bright colors (power function)
		return std::pow((component + 0.055f) / 1.055f, 2.4f);
	}
}
