#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "Buffer.h"
#include "Image.h"
#include "Mesh.h"
#include "Sampler.h"

namespace Wolf
{
	struct Vertex3D
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texCoord;
		glm::uint materialID;

		static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription, uint32_t binding)
		{
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(Vertex3D);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
		{
			const uint32_t attributeDescriptionCountBefore = attributeDescriptions.size();
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
			attributeDescriptions[attributeDescriptionCountBefore + 4].offset = offsetof(Vertex3D, materialID);
		}

		bool operator==(const Vertex3D& other) const
		{
			return pos == other.pos && normal == other.normal && texCoord == other.texCoord && tangent == other.tangent && materialID == other.materialID;
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
		bool loadMaterials = true;
		uint32_t materialIdOffset = 0;

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
		std::unique_ptr<Mesh> mesh;
		std::vector<std::unique_ptr<Image>> images;

		void getImages(std::vector<Image*>& outputImages) const;
	};

	class ObjLoader
	{
	public:
		static void loadObject(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo);

	private:
		ObjLoader(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo);

		static std::string getTexName(const std::string& texName, const std::string& folder);
		void setImage(const std::string& filename, uint32_t idx, bool sRGB);

		std::vector<int> m_toBeLast = { 2, 19, 0 }; // flower contains alpha blending
		bool m_useCache;
		std::vector<std::vector<unsigned char>> m_imagesData;

		ModelData& m_outputModel;
	};
}