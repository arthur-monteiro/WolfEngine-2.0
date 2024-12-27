#pragma once

#include <memory>
#include <string>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <GraphicAPIManager.h>

namespace Wolf
{
	struct ModelLoadingInfo;
	struct ModelData;

	struct SkeletonVertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texCoords;

		glm::uvec4 bonesIds;
		glm::vec4 bonesWeights;

		static void getBindingDescription(VkVertexInputBindingDescription& bindingDescription, uint32_t binding)
		{
			bindingDescription.binding = binding;
			bindingDescription.stride = sizeof(SkeletonVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		}

		static void getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions, uint32_t binding)
		{
			const uint32_t attributeDescriptionCountBefore = static_cast<uint32_t>(attributeDescriptions.size());
			attributeDescriptions.resize(attributeDescriptionCountBefore + 6);

			attributeDescriptions[attributeDescriptionCountBefore + 0].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 0].location = 0;
			attributeDescriptions[attributeDescriptionCountBefore + 0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 0].offset = offsetof(SkeletonVertex, pos);

			attributeDescriptions[attributeDescriptionCountBefore + 1].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 1].location = 1;
			attributeDescriptions[attributeDescriptionCountBefore + 1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 1].offset = offsetof(SkeletonVertex, normal);

			attributeDescriptions[attributeDescriptionCountBefore + 2].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 2].location = 2;
			attributeDescriptions[attributeDescriptionCountBefore + 2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 2].offset = offsetof(SkeletonVertex, tangent);

			attributeDescriptions[attributeDescriptionCountBefore + 3].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 3].location = 3;
			attributeDescriptions[attributeDescriptionCountBefore + 3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 3].offset = offsetof(SkeletonVertex, texCoords);

			attributeDescriptions[attributeDescriptionCountBefore + 4].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 4].location = 4;
			attributeDescriptions[attributeDescriptionCountBefore + 4].format = VK_FORMAT_R32G32B32A32_UINT;
			attributeDescriptions[attributeDescriptionCountBefore + 4].offset = offsetof(SkeletonVertex, bonesIds);

			attributeDescriptions[attributeDescriptionCountBefore + 5].binding = binding;
			attributeDescriptions[attributeDescriptionCountBefore + 5].location = 5;
			attributeDescriptions[attributeDescriptionCountBefore + 5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[attributeDescriptionCountBefore + 5].offset = offsetof(SkeletonVertex, bonesWeights);
		}

		bool operator==(const SkeletonVertex& other) const
		{
			return pos == other.pos && normal == other.normal && texCoords == other.texCoords && tangent == other.tangent;
		}
	};
}

namespace std
{
	template<> struct hash<Wolf::SkeletonVertex>
	{
		size_t operator()(Wolf::SkeletonVertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoords) << 1);
		}
	};
}

namespace Wolf
{
	struct AnimationData
	{
		struct Bone
		{
			uint32_t idx;
			std::string name;
			glm::mat4 offsetMatrix;

			struct Pose
			{
				float time;
				glm::vec3 translation;
				glm::quat orientation;
				glm::vec3 scale;
			};
			std::vector<Pose> poses;

			std::vector<Bone> children;
		};
		std::vector<Bone> rootBones;

		uint32_t boneCount;
	};

	class DAEImporter
	{
	public:
		DAEImporter(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo);

		std::vector<SkeletonVertex>& getVertices() { return m_vertices; }
		std::vector<uint32_t>& getIndices() { return m_indices; }

	private:
		static void extractFloatValues(const std::string& input, std::vector<float>& outValues);

		struct InternalInfoPerBone
		{
			std::string name;
			glm::mat4 offsetMatrix;
			struct Pose
			{
				float time;
				glm::mat4 transform;
			};
			std::vector<Pose> poses;
		};
		std::vector<InternalInfoPerBone> m_bonesInfo;
		const InternalInfoPerBone* findBoneInfoByName(const std::string& name, uint32_t& outIdx) const;

		struct Node
		{
			std::string name;
			std::vector<std::pair<std::string, std::string>> properties;
			std::string body;

			std::vector<std::unique_ptr<Node>> children;
			Node* parent;

			std::string getProperty(const std::string& propertyName);
			Node* getFirstChildByName(const std::string& childName);
			Node* getChildByName(const std::string& childName, uint32_t idx);
		};
		Node* createNode(Node* currentNode);
		void findNodesInHierarchy(Node* currentNode, std::vector<AnimationData::Bone>& currentBoneArray);

		std::vector<std::unique_ptr<Node>> m_rootNodes;

		std::vector<SkeletonVertex> m_vertices;
		std::vector<uint32_t> m_indices;

		ModelData* m_outputModel = nullptr;
	};
}
