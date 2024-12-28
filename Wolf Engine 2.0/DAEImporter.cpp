#include "DAEImporter.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <glm/gtx/quaternion.hpp>

#include "Debug.h"
#include "ModelLoader.h"
#include "Timer.h"

Wolf::DAEImporter::DAEImporter(ModelData& outputModel, ModelLoadingInfo& modelLoadingInfo) : m_outputModel(&outputModel)
{
	Debug::sendInfo("Loading DAE file " + modelLoadingInfo.filename);
	Timer albedoTimer("Loading DAE file " + modelLoadingInfo.filename);

	outputModel.animationData.reset(new AnimationData);

	std::ifstream file(modelLoadingInfo.filename);

	std::string line;
	unsigned int currentLineIdx = 1;
	std::getline(file, line); // ignore first line

	Node* currentNode = nullptr;
	while (std::getline(file, line))
	{
		currentLineIdx++;

		bool prevCharIsStartNode = false;
		bool isStartingNewNode = false;
		bool isClosingNode = false;
		bool isGettingNodeName = false;
		unsigned int currentPropertyId = 0;
		bool isGettingPropertyValue = false;

		for (char character : line)
		{
			if (character == '<')
			{
				prevCharIsStartNode = true;
				continue;
			}

			if (character == '>')
			{
				isStartingNewNode = false;
				isGettingNodeName = false;
				isClosingNode = false;
				currentPropertyId = 0;
				continue;
			}

			if (isClosingNode)
			{
				continue;
			}

			if (prevCharIsStartNode)
			{
				if (character == '/')
				{
					isClosingNode = true;
					currentNode = currentNode->parent;
					continue;
				}
				else
				{
					isStartingNewNode = true;
					isGettingNodeName = true;
					currentNode = createNode(currentNode);
				}
				prevCharIsStartNode = false;
			}

			if (isStartingNewNode && !isGettingPropertyValue && character == '/')
			{
				isClosingNode = true;
				currentNode = currentNode->parent;
				continue;
			}

			if (isGettingNodeName)
			{
				if (character == ' ')
				{
					isGettingNodeName = false;
				}
				else
				{
					currentNode->name.push_back(character);
				}
				continue;
			}

			if (isStartingNewNode && !isGettingNodeName) // getting properties
			{
				if (currentPropertyId >= currentNode->properties.size())
				{
					currentNode->properties.emplace_back();
				}

				if (character == ' ')
				{
					if (!currentNode->properties[currentPropertyId].first.empty()) // allowing 2 or more spaces
					{
						currentPropertyId++;
					}
					continue;
				}

				if (isGettingPropertyValue)
				{
					currentNode->properties[currentPropertyId].second.push_back(character);
					if (character == '"' && currentNode->properties[currentPropertyId].second.size() > 1)
					{
						isGettingPropertyValue = false;
					}
				}
				else
				{
					if (character == '=')
					{
						isGettingPropertyValue = true;
						continue;
					}
					else
					{
						currentNode->properties[currentPropertyId].first.push_back(character);
					}
				}
				continue;
			}
			else
			{
				currentNode->body.push_back(character);
			}
		}
		if (currentNode)
			currentNode->body.push_back(' ');
	}

	// Getting vertices
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> texCoords;
	std::vector<glm::uvec3> indices;

	Node* meshNode = m_rootNodes[0]->getFirstChildByName("library_geometries")->getFirstChildByName("geometry")->getFirstChildByName("mesh");

	unsigned int uvCount = 0;
	for (std::unique_ptr<Node>& meshChild : meshNode->children)
	{
		bool isPosition = false;
		bool isUV = false;

		if (meshChild->getProperty("name") == "\"position\"")
			isPosition = true;
		else if (meshChild->getProperty("name") == "\"map1\"")
			isUV = true;

		if (isUV)
			uvCount++;

		if (isPosition || (isUV && uvCount == 1))
		{
			for (std::unique_ptr<Node>& sourceChild : meshChild->children)
			{
				if (sourceChild->name == "float_array")
				{
					std::string count = sourceChild->getProperty("count");
					std::erase(count, '"');
					if (isPosition)
						positions.resize(std::stoi(count) / 3);
					else if (isUV)
						texCoords.resize(std::stoi(count) / 2);

					std::string floatString;
					int currentId = 0;
					for (char c : sourceChild->body)
					{
						if (c == ' ')
						{
							if (!floatString.empty())
							{
								float f = std::stof(floatString);
								if (isPosition)
									positions[currentId / 3][currentId % 3] = f;
								else if (isUV)
									texCoords[currentId / 2][currentId % 2] = f;

								floatString = "";
								currentId++;
							}
						}
						else
						{
							floatString.push_back(c);
						}
					}
				}
			}
		}

		if (meshChild->name == "polylist")
		{
			std::string count = meshChild->getProperty("count");
			std::erase(count, '"');
			indices.resize(std::stoi(count) * 3);

			Node* polylistChild = meshChild->getFirstChildByName("p");
			std::string intString;
			int currentId = 0;
			for (char c : polylistChild->body)
			{
				if (c == ' ')
				{
					if (!intString.empty())
					{
						unsigned int u = std::stoi(intString);

						if (currentId % (uvCount + 2) < 3)
							indices[currentId / (uvCount + 2)][currentId % (uvCount + 2)] = u;

						intString = "";
						currentId++;
					}
				}
				else
				{
					intString.push_back(c);
				}
			}
		}
	}

	Node* skinNode = m_rootNodes[0]->getFirstChildByName("library_controllers")->getFirstChildByName("controller")->getFirstChildByName("skin");
	uint32_t jointsCount = 0; // will be found afterward

	Node* jointsNode = skinNode->getFirstChildByName("joints");
	std::string jointsName;
	std::string matricesName;
	for (std::unique_ptr<Node>& vertexWeightChild : jointsNode->children)
	{
		if (vertexWeightChild->getProperty("semantic") == "\"JOINT\"")
		{
			jointsName = vertexWeightChild->getProperty("source");
		}
		if (vertexWeightChild->getProperty("semantic") == "\"INV_BIND_MATRIX\"")
		{
			matricesName = vertexWeightChild->getProperty("source");
		}
	}
	std::erase(jointsName, '#');
	std::erase(matricesName, '#');

	// Debug names
	for (std::unique_ptr<Node>& skinChild : skinNode->children)
	{
		if (skinChild->getProperty("id") == jointsName)
		{
			if (Node* jointsNamesNode = skinChild->getFirstChildByName("Name_array"))
			{
				std::string count = jointsNamesNode->getProperty("count");
				std::erase(count, '"');
				jointsCount = std::stoi(count);

				m_bonesInfo.resize(jointsCount);
				uint32_t currentIdx = 0;
				for (char c : jointsNamesNode->body)
				{
					if (c == ' ')
					{
						if (m_bonesInfo[currentIdx].name.empty())
							continue;
						currentIdx++;
					}
					else
					{
						m_bonesInfo[currentIdx].name.push_back(c);
					}
				}
			}
		}
	}

	// Matrices
	for (std::unique_ptr<Node>& skinChild : skinNode->children)
	{
		if (skinChild->getProperty("id") == matricesName)
		{
			Node* matricesValuesNode = skinChild->getFirstChildByName("float_array");

			std::string matricesValuesCount = matricesValuesNode->getProperty("count");
			std::erase(matricesValuesCount, '"');
			/*if (jointsCount == 0)
				jointsCount = std::stoi(matricesValuesCount) / 16u;
			else*/
			{
				if (jointsCount != std::stoi(matricesValuesCount) / 16u)
				{
					Debug::sendError("Matrices count seems wrong");
				}
			}

			std::vector<float> floatValues;
			extractFloatValues(matricesValuesNode->body, floatValues);

			if (floatValues.size() != static_cast<size_t>(jointsCount) * 16)
			{
				Debug::sendError("Got wrong floats count");
			}
			else
			{
				for (uint32_t i = 0; i < floatValues.size(); ++i)
				{
					float floatValue = floatValues[i];
					uint32_t valueIdxInMat = i % 16;
					uint32_t columnIdx = valueIdxInMat / 4;
					uint32_t lineIdx = valueIdxInMat % 4;

					m_bonesInfo[i / 16].offsetMatrix[static_cast<int>(columnIdx)][static_cast<int>(lineIdx)] = floatValue;
				}

				for (InternalInfoPerBone& infoForBone : m_bonesInfo)
				{
					infoForBone.offsetMatrix = glm::transpose(infoForBone.offsetMatrix);
				}
			}
		}
	}

	Node* vertexWeightsNode = skinNode->getFirstChildByName("vertex_weights");

	std::vector<uint32_t> boneCountPerVertex;

	std::string intString;
	for (char c : vertexWeightsNode->getFirstChildByName("vcount")->body)
	{
		if (c == ' ')
		{
			if (!intString.empty())
			{
				unsigned int u = std::stoi(intString);
				boneCountPerVertex.push_back(u);

				intString = "";
			}
		}
		else
		{
			intString.push_back(c);
		}
	}
	if (!intString.empty())
	{
		unsigned int u = std::stoi(intString);
		boneCountPerVertex.push_back(u);
		intString = "";
	}

	std::vector<std::vector<uint32_t>> boneIndicesAndWeightIndices(boneCountPerVertex.size());

	uint32_t currentVertexIdx = 0;
	for (char c : vertexWeightsNode->getFirstChildByName("v")->body)
	{
		if (c == ' ')
		{
			if (!intString.empty())
			{
				uint32_t u = std::stoi(intString);
				boneIndicesAndWeightIndices[currentVertexIdx].push_back(u);

				if (boneIndicesAndWeightIndices[currentVertexIdx].size() == 2 * boneCountPerVertex[currentVertexIdx])
					currentVertexIdx++;

				intString = "";
			}
		}
		else
		{
			intString.push_back(c);
		}
	}
	if (!intString.empty())
	{
		uint32_t u = std::stoi(intString);
		boneIndicesAndWeightIndices[currentVertexIdx].push_back(u);

		if (boneIndicesAndWeightIndices[currentVertexIdx].size() == 2 * boneCountPerVertex[currentVertexIdx])
			currentVertexIdx++;
	}

	std::string weightName;
	for (std::unique_ptr<Node>& vertexWeightChild : vertexWeightsNode->children)
	{
		if (vertexWeightChild->getProperty("semantic") == "\"WEIGHT\"")
		{
			weightName = vertexWeightChild->getProperty("source");
		}
	}
	std::erase(weightName, '#');

	std::vector<float> weights;
	for (std::unique_ptr<Node>& skinChild : skinNode->children)
	{
		if (skinChild->getProperty("id") == weightName)
		{
			extractFloatValues(skinChild->getFirstChildByName("float_array")->body, weights);
		}
	}

	std::unordered_map<SkeletonVertex, uint32_t> uniqueVertices = {};
	glm::vec3 minPos(1'000.0f), maxPos(-1000.0f);
	for (glm::uvec3 index : indices)
	{
		SkeletonVertex vertex;
		vertex.pos = positions[index.x];

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

		vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // TODO: read normals
		vertex.texCoords = glm::vec2(texCoords[index.z].x, 1.0f - texCoords[index.z].y);
		vertex.bonesIds = glm::ivec4(-1, -1, -1, -1);
		vertex.bonesWeights = glm::vec4(0.0f);

		if (boneIndicesAndWeightIndices[index.x].size() % 2 != 0)
		{
			Debug::sendError("There not weight for all indices");
		}

		for (uint32_t i = 0; i < std::min(boneIndicesAndWeightIndices[index.x].size(), static_cast<size_t>(8)); ++i)
		{
			if (i % 2u == 0)
			{
				vertex.bonesIds[static_cast<int>(i / 2u)] = static_cast<int>(boneIndicesAndWeightIndices[index.x][i]);
				if (vertex.bonesIds[static_cast<int>(i / 2u)] > static_cast<int>(jointsCount))
				{
					Debug::sendError("Bone idx out of range");
				}
			}
			else
				vertex.bonesWeights[static_cast<int>(i / 2u)] = weights[boneIndicesAndWeightIndices[index.x][i]];
		}

		if (!uniqueVertices.contains(vertex))
		{
			uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
			m_vertices.push_back(vertex);
		}

		m_indices.push_back(uniqueVertices[vertex]);
	}
	AABB aabb(minPos, maxPos);
	m_outputModel->mesh.reset(new Mesh(m_vertices, m_indices, aabb, modelLoadingInfo.additionalVertexBufferUsages, modelLoadingInfo.additionalIndexBufferUsages));

	// Animation

	Node* animationNode = m_rootNodes[0]->getFirstChildByName("library_animations");
	uint32_t animationIdx = 0;
	while (Node* boneAnimationNode = animationNode->getChildByName("animation", animationIdx))
	{
		animationIdx++;

		std::string boneAnimationName = boneAnimationNode->getProperty("name");
		std::erase(boneAnimationName, '"');
		uint32_t boneIdx = 0;
		for (boneIdx = 0; boneIdx < m_bonesInfo.size(); ++boneIdx)
		{
			if (m_bonesInfo[boneIdx].name == boneAnimationName)
				break;
		}

		if (boneIdx == m_bonesInfo.size())
		{
			Debug::sendError("Bone not found for animation " + boneAnimationName);
		}

		uint32_t sourceIdx = 0;
		while (Node* boneAnimationSource = boneAnimationNode->getChildByName("source", sourceIdx))
		{
			sourceIdx++;

			std::string techniqueType = boneAnimationSource->getFirstChildByName("technique_common")->getFirstChildByName("accessor")->getFirstChildByName("param")->getProperty("type");
			std::erase(techniqueType, '"');
			if (techniqueType != "float" && techniqueType != "float4x4")
				continue;

			std::vector<float> floatValues;
			extractFloatValues(boneAnimationSource->getFirstChildByName("float_array")->body, floatValues);

			if (techniqueType == "float") // time
			{
				if (m_bonesInfo[boneIdx].poses.empty())
					m_bonesInfo[boneIdx].poses.resize(floatValues.size());
				else if (floatValues.size() != m_bonesInfo[boneIdx].poses.size())
					Debug::sendError("Mismatch sizes");

				for (uint32_t i = 0; i < floatValues.size(); ++i)
				{
					m_bonesInfo[boneIdx].poses[i].time = floatValues[i];
				}
			}
			else if (techniqueType == "float4x4") // transforms
			{
				if (m_bonesInfo[boneIdx].poses.empty())
					m_bonesInfo[boneIdx].poses.resize(floatValues.size() / 16);
				else if (floatValues.size() != m_bonesInfo[boneIdx].poses.size() * 16)
					Debug::sendError("Mismatch sizes");

				for (uint32_t i = 0; i < floatValues.size(); ++i)
				{
					uint32_t valueIdxInMat = i % 16;
					uint32_t columnIdx = valueIdxInMat / 4;
					uint32_t lineIdx = valueIdxInMat % 4;

					m_bonesInfo[boneIdx].poses[i / 16].transform[static_cast<int>(columnIdx)][static_cast<int>(lineIdx)] = floatValues[i];
				}

				for (InternalInfoPerBone::Pose& pose : m_bonesInfo[boneIdx].poses)
				{
					pose.transform = glm::transpose(pose.transform);
				}
			}
		}
	}

	m_outputModel->animationData->boneCount = jointsCount;

	// Hierarchy
	Node* visualSceneNode = m_rootNodes[0]->getFirstChildByName("library_visual_scenes")->getFirstChildByName("visual_scene");
	findNodesInHierarchy(visualSceneNode, m_outputModel->animationData->rootBones);
}

void Wolf::DAEImporter::extractFloatValues(const std::string& input, std::vector<float>& outValues)
{
	std::string floatString;
	for (char c : input)
	{
		if (c == ' ')
		{
			if (!floatString.empty())
			{
				float f = std::stof(floatString);
				outValues.push_back(f);

				floatString = "";
			}
		}
		else
		{
			floatString.push_back(c);
		}
	}
	if (!floatString.empty())
	{
		float f = std::stof(floatString);
		outValues.push_back(f);

		floatString = "";
	}
}

Wolf::DAEImporter::Node* Wolf::DAEImporter::createNode(Node* currentNode)
{
	Node* r = nullptr;
	if (currentNode)
	{
		currentNode->children.push_back(std::make_unique<Node>());
		r = currentNode->children.back().get();
		r->parent = currentNode;
	}
	else
	{
		m_rootNodes.push_back(std::make_unique<Node>());
		r = m_rootNodes.back().get();
	}

	return r;
}

void Wolf::DAEImporter::findNodesInHierarchy(Node* currentNode, std::vector<AnimationData::Bone>& currentBoneArray)
{
	uint32_t idx = 0;
	while (Node* node = currentNode->getChildByName("node", idx))
	{
		idx++;

		AnimationData::Bone bone;
		bone.name = node->getProperty("name");
		std::erase(bone.name, '"');

		const InternalInfoPerBone* boneInfo = findBoneInfoByName(bone.name, bone.idx);
		if (!boneInfo)
			continue;

		bone.offsetMatrix = boneInfo->offsetMatrix;
		bone.poses.resize(boneInfo->poses.size());
		for (uint32_t i = 0; i < boneInfo->poses.size(); ++i)
		{
			const InternalInfoPerBone::Pose& pose = boneInfo->poses[i];
			bone.poses[i].time = pose.time;

			glm::vec3 skew;
			glm::vec4 perspective;
			glm::quat orientation;
			glm::decompose(pose.transform, bone.poses[i].scale, orientation, bone.poses[i].translation, skew, perspective);
			bone.poses[i].orientation = glm::quat_cast(pose.transform);
		}

		currentBoneArray.push_back(bone);
		findNodesInHierarchy(node, currentBoneArray.back().children);
	}
}

const Wolf::DAEImporter::InternalInfoPerBone* Wolf::DAEImporter::findBoneInfoByName(const std::string& name, uint32_t& outIdx) const
{
	for (uint32_t i = 0; i < m_bonesInfo.size(); ++i)
	{
		const InternalInfoPerBone& boneInfo = m_bonesInfo[i];

		if (boneInfo.name == name)
		{
			outIdx = i;
			return &boneInfo;
		}
	}
	return nullptr;
}

std::string Wolf::DAEImporter::Node::getProperty(const std::string& propertyName)
{
	for (std::pair<std::string, std::string>& property : properties)
	{
		if (property.first == propertyName)
			return property.second;
	}

	return "";
}

Wolf::DAEImporter::Node* Wolf::DAEImporter::Node::getFirstChildByName(const std::string& childName)
{
	return getChildByName(childName, 0);
}

Wolf::DAEImporter::Node* Wolf::DAEImporter::Node::getChildByName(const std::string& childName, uint32_t idx)
{
	uint32_t counter = 0;
	for (std::unique_ptr<Node>& child : children)
	{
		if (child->name == childName)
		{
			if (counter == idx)
				return child.get();
			counter++;
		}
	}

	return nullptr;
}
