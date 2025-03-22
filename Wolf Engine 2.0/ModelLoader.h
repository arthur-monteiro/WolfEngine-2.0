#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
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
			attributeDescriptions.resize(attributeDescriptionCountBefore + 5);

			attributeDescriptions[attributeDescriptionCountBefore + 0].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 0].location = 0;
			attributeDescriptions[attributeDescriptionCountBefore + 0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 0].offset = offsetof(Vertex3D, pos);

			attributeDescriptions[attributeDescriptionCountBefore + 1].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 1].location = 1;
			attributeDescriptions[attributeDescriptionCountBefore + 1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 1].offset = offsetof(Vertex3D, normal);

			attributeDescriptions[attributeDescriptionCountBefore + 2].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 2].location = 2;
			attributeDescriptions[attributeDescriptionCountBefore + 2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 2].offset = offsetof(Vertex3D, tangent);

			attributeDescriptions[attributeDescriptionCountBefore + 3].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 3].location = 3;
			attributeDescriptions[attributeDescriptionCountBefore + 3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 3].offset = offsetof(Vertex3D, texCoord);

			attributeDescriptions[attributeDescriptionCountBefore + 4].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 4].location = 4;
			attributeDescriptions[attributeDescriptionCountBefore + 4].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[attributeDescriptionCountBefore + 4].offset = offsetof(Vertex3D, subMeshIdx);
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

		// Multi-threading options
		std::mutex* vulkanQueueLock = nullptr;

		// Addidional flags
		VkBufferUsageFlags additionalVertexBufferUsages = 0;
		VkBufferUsageFlags additionalIndexBufferUsages = 0;
	};

	struct ModelData
	{
		ResourceUniqueOwner<Mesh> mesh;
		bool isMeshCentered;
		std::vector<MaterialsGPUManager::TextureSetInfo> textureSets;

		std::unique_ptr<AnimationData> animationData;
		std::vector<ResourceUniqueOwner<Physics::Shape>> physicsShapes;
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
		bool loadCache(ModelLoadingInfo& modelLoadingInfo) const;

		// Materials
		void loadTextureSet(const tinyobj::material_t& material, const std::string& mtlFolder, uint32_t indexMaterial);
		
		bool m_useCache;
		std::vector<std::vector<unsigned char>> m_imagesData;

		ModelData* m_outputModel = nullptr;
	};
}