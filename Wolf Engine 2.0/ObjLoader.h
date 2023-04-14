#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
			attributeDescriptions.resize(5);

			attributeDescriptions[0].binding = binding;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

			attributeDescriptions[1].binding = binding;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex3D, normal);

			attributeDescriptions[2].binding = binding;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex3D, tangent);

			attributeDescriptions[3].binding = binding;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(Vertex3D, texCoord);

			attributeDescriptions[4].binding = binding;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
			attributeDescriptions[4].offset = offsetof(Vertex3D, materialID);
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

		// Cache options
		bool useCache = true;

		// Multi-threading options
		std::mutex* vulkanQueueLock = nullptr;

		// Addidional flags
		VkBufferUsageFlags additionalVertexBufferUsages = 0;
		VkBufferUsageFlags additionalIndexBufferUsages = 0;
	};

	class ObjLoader
	{
	public:
		ObjLoader(ModelLoadingInfo& modelLoadingInfo);
		ObjLoader(const ObjLoader&) = delete;

		const Mesh* getMesh() const { return m_mesh.get(); }
		const void getImages(std::vector<Image*>& images);

	private:
		static std::string getTexName(const std::string& texName, const std::string& folder);
		void setImage(const std::string& filename, uint32_t idx, bool sRGB);

	private:
		std::vector<int> m_toBeLast = { 2, 19, 0 }; // flower contains alpha blending
		bool m_useCache;

		std::unique_ptr<Mesh> m_mesh;
		std::vector<std::unique_ptr<Image>> m_images;
		std::vector<std::vector<unsigned char>> m_imagesData;
	};
}