#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <mutex>
#include <tiny_obj_loader.h>

#include "DAEImporter.h"
#include "Image.h"
#include "ImageCompression.h"
#include "TextureSetLoader.h"
#include "Mesh.h"
#include "PhysicShapes.h"
#include "ResourceUniqueOwner.h"

namespace Wolf
{
	struct Vertex3D
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texCoord;
		glm::uint subMeshIdx;

		static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription, uint32_t binding)
		{
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex3D);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
		{
			const uint32_t attributeDescriptionCountBefore = static_cast<uint32_t>(attributeDescriptions.size());
			attributeDescriptions.resize(attributeDescriptionCountBefore + 6);

			attributeDescriptions[attributeDescriptionCountBefore + 0].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 0].location = 0;
			attributeDescriptions[attributeDescriptionCountBefore + 0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 0].offset = offsetof(Vertex3D, pos);

			attributeDescriptions[attributeDescriptionCountBefore + 1].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 1].location = 1;
			attributeDescriptions[attributeDescriptionCountBefore + 1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 1].offset = offsetof(Vertex3D, color);

			attributeDescriptions[attributeDescriptionCountBefore + 2].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 2].location = 2;
			attributeDescriptions[attributeDescriptionCountBefore + 2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 2].offset = offsetof(Vertex3D, normal);

			attributeDescriptions[attributeDescriptionCountBefore + 3].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 3].location = 3;
			attributeDescriptions[attributeDescriptionCountBefore + 3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 3].offset = offsetof(Vertex3D, tangent);

			attributeDescriptions[attributeDescriptionCountBefore + 4].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 4].location = 4;
			attributeDescriptions[attributeDescriptionCountBefore + 4].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 4].offset = offsetof(Vertex3D, texCoord);

			attributeDescriptions[attributeDescriptionCountBefore + 5].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 5].location = 5;
			attributeDescriptions[attributeDescriptionCountBefore + 5].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[attributeDescriptionCountBefore + 5].offset = offsetof(Vertex3D, subMeshIdx);
		}

		bool operator==(const Vertex3D& other) const
		{
			return pos == other.pos && normal == other.normal && texCoord == other.texCoord && tangent == other.tangent && subMeshIdx == other.subMeshIdx;
		}
	};
}

namespace std
{
	template<> struct hash<Wolf::Vertex3D>
	{
		size_t operator()(Wolf::Vertex3D const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

namespace Wolf
{
	struct ModelLoadingInfo
	{
		// Files paths
		std::string filename;
		std::string mtlFolder;
#ifdef __ANDROID__
        bool isInAssets = false; // false = in storage
#endif

		// Default options
		glm::vec3 defaultNormal = glm::vec3(0.0f, 1.0f, 0.0f);

		// Material options
		TextureSetLoader::InputTextureSetLayout textureSetLayout = TextureSetLoader::InputTextureSetLayout::EACH_TEXTURE_A_FILE;

		// Cache options
		bool useCache = true;

		// LODs
		uint32_t generateDefaultLODCount = 1;
		uint32_t generateSloppyLODCount = 1;

		// Multi-threading options
		std::mutex* vulkanQueueLock = nullptr;

		// Addidional flags
		VkBufferUsageFlags additionalVertexBufferUsages = 0;
		VkBufferUsageFlags additionalIndexBufferUsages = 0;
	};

	struct ModelData
	{
		ResourceUniqueOwner<Mesh> m_mesh;

		// LOD data
		struct LODInfo
		{
			float m_error;
			uint32_t m_indexCount;
		};
		std::vector<ResourceUniqueOwner<Buffer>> m_defaultSimplifiedIndexBuffers;
		std::vector<LODInfo> m_defaultLODsInfo;
		std::vector<ResourceUniqueOwner<Buffer>> m_sloppySimplifiedIndexBuffers;
		std::vector<LODInfo> m_sloppyLODsInfo;

#ifdef MATERIAL_DEBUG
		std::string m_originFilepath;
#endif
		bool m_isMeshCentered;
		std::vector<MaterialsGPUManager::TextureSetInfo> m_textureSets;

		std::unique_ptr<AnimationData> m_animationData;
		std::vector<ResourceUniqueOwner<Physics::Shape>> m_physicsShapes;
	};

	class ModelLoader
	{
	public:
		static void loadObject(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo);

	private:
		ModelLoader(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo);

		struct InternalShapeInfo
		{
			uint32_t indicesOffset;
		};
		bool loadCache(ModelLoadingInfo& modelLoadingInfo);

		// Materials
		TextureSetLoader::TextureSetFileInfoGGX createTextureSetFileInfoGGXFromTinyObjMaterial(const tinyobj::material_t& material, const std::string& mtlFolder) const;
		void loadTextureSet(const TextureSetLoader::TextureSetFileInfoGGX& material, uint32_t indexMaterial);

		float sRGBtoLinear(float component);
		
		bool m_useCache;
		struct InfoForCachePerTextureSet
		{
			TextureSetLoader::TextureSetFileInfoGGX m_textureSetFileInfoGGX;
		};
		std::vector<InfoForCachePerTextureSet> m_infoForCache;

		ModelData* m_outputModel = nullptr;
	};
}