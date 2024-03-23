#pragma once

#include "BottomLevelAccelerationStructure.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSetLayoutGenerator.h"
#include "LazyInitSharedResource.h"
#include "MaterialsGPUManager.h"
#include "ModelLoader.h"
#include "RenderMeshList.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	class PipelineSet;
	class RenderMeshList;
	class BindlessDescriptor;

	class ModelBase
	{
	public:
		ModelBase(ModelLoadingInfo& modelLoadingInfo, bool requestAccelerationStructuresBuild);
		ModelBase(ModelLoadingInfo& modelLoadingInfo, bool requestAccelerationStructuresBuild, const ResourceNonOwner<MaterialsGPUManager>& materialsGPUManager);
		ModelBase(const ModelBase&) = delete;
		virtual ~ModelBase() = default;

		virtual void addMeshToRenderList(RenderMeshList& renderMeshList, const RenderMeshList::MeshToRenderInfo::InstanceInfos& instanceInfos = { nullptr, 1 });
		void updateGraphic();

		const BottomLevelAccelerationStructure* getBLAS() const { return m_blas.get(); }
		const glm::mat4& getTransform() const { return m_transform; }
		VkDescriptorSetLayout getDescriptorSetLayout() const { return m_descriptorSetLayout->getResource()->getDescriptorSetLayout(); }

		void setTransform(const glm::mat4& transform) { m_transform = transform; }
		void setPosition(glm::vec3 position) { m_transform[3] = glm::vec4(position, 1.0f); }
		void setPipelineSet(PipelineSet* pipelineSet) { m_pipelineSet = pipelineSet; }

	private:
		void buildAccelerationStructures();

		ModelData m_modelData;
		PipelineSet* m_pipelineSet = nullptr;

		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayoutGenerator, ModelBase>> m_descriptorSetLayoutGenerator;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, ModelBase>> m_descriptorSetLayout;

		glm::mat4 m_transform;
		glm::mat4 m_previousTransform;
		struct UniformBufferData
		{
			glm::mat4 model;
			glm::mat4 previousModel;
		};
		ResourceUniqueOwner<DescriptorSet> m_descriptorSet;
		std::unique_ptr<Buffer> m_uniformBuffer;

		std::unique_ptr<BottomLevelAccelerationStructure> m_blas;
	};
}
