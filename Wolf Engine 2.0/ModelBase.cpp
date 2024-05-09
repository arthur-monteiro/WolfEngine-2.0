#include "ModelBase.h"

#include "BindlessDescriptor.h"
#include "DescriptorSetGenerator.h"
#include "RenderMeshList.h"

Wolf::ModelBase::ModelBase(ModelLoadingInfo& modelLoadingInfo, bool requestAccelerationStructuresBuild)
{
	ModelLoader::loadObject(m_modelData, modelLoadingInfo);

	m_descriptorSetLayoutGenerator.reset(new LazyInitSharedResource<DescriptorSetLayoutGenerator, ModelBase>([this](std::unique_ptr<DescriptorSetLayoutGenerator>& descriptorSetLayoutGenerator)
		{
			descriptorSetLayoutGenerator.reset(new DescriptorSetLayoutGenerator);
			descriptorSetLayoutGenerator->addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		}));

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, ModelBase>([this](std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts()));
		}));

	m_uniformBuffer.reset(Buffer::createBuffer(sizeof(UniformBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	m_descriptorSet.reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));
	DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts());
	descriptorSetGenerator.setBuffer(0, *m_uniformBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	if (requestAccelerationStructuresBuild)
	{
		modelLoadingInfo.vulkanQueueLock->lock();
		buildAccelerationStructures();
		modelLoadingInfo.vulkanQueueLock->unlock();
	}
}

Wolf::ModelBase::ModelBase(ModelLoadingInfo& modelLoadingInfo, bool requestAccelerationStructuresBuild, const ResourceNonOwner<MaterialsGPUManager>& materialsGPUManager)
	: ModelBase(modelLoadingInfo, requestAccelerationStructuresBuild)
{
	if (modelLoadingInfo.materialLayout != MaterialLoader::InputMaterialLayout::NO_MATERIAL)
	{
		std::vector<DescriptorSetGenerator::ImageDescription> imageDescriptions;
		for (const ResourceUniqueOwner<Image>& image : m_modelData.images)
		{
			imageDescriptions.resize(imageDescriptions.size() + 1);
			imageDescriptions.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDescriptions.back().imageView = image->getDefaultImageView();
		}
		materialsGPUManager->addNewMaterials(imageDescriptions);
	}
}

void Wolf::ModelBase::addMeshToRenderList(RenderMeshList& renderMeshList, const RenderMeshList::MeshToRenderInfo::InstanceInfos& instanceInfos)
{
	RenderMeshList::MeshToRenderInfo meshToRenderInfo(m_modelData.mesh.createNonOwnerResource(), m_pipelineSet, m_transform);
	meshToRenderInfo.descriptorSets.push_back({ m_descriptorSet.createConstNonOwnerResource(), 0 });
	meshToRenderInfo.instanceInfos = instanceInfos;
	renderMeshList.addMeshToRender(meshToRenderInfo);
}

void Wolf::ModelBase::updateGraphic()
{
	UniformBufferData uniformBufferData;
	uniformBufferData.model = m_transform;
	uniformBufferData.previousModel = m_previousTransform;

	m_uniformBuffer->transferCPUMemory(&uniformBufferData, sizeof(uniformBufferData), 0);

	m_previousTransform = m_transform;
}

void Wolf::ModelBase::buildAccelerationStructures()
{
	BottomLevelAccelerationStructureCreateInfo blasCreateInfo;
	blasCreateInfo.buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	std::vector<GeometryInfo> geometries(1);

	GeometryInfo& geometryInfo = geometries.back();
	geometryInfo.mesh.vertexBuffer = &m_modelData.mesh->getVertexBuffer();
	geometryInfo.mesh.vertexCount = m_modelData.mesh->getVertexCount();
	geometryInfo.mesh.vertexSize = m_modelData.mesh->getVertexSize();
	geometryInfo.mesh.vertexFormat = m_modelData.mesh->getVertexFormat();

	geometryInfo.mesh.indexBuffer = &m_modelData.mesh->getIndexBuffer();
	geometryInfo.mesh.indexCount = m_modelData.mesh->getIndexCount();
	
	blasCreateInfo.geometryInfos = geometries;
	m_blas.reset(BottomLevelAccelerationStructure::createBottomLevelAccelerationStructure(blasCreateInfo));
}
