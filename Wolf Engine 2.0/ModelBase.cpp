#include "ModelBase.h"

#include <utility>

#include "BindlessDescriptor.h"
#include "DescriptorSetGenerator.h"
#include "RenderMeshList.h"

Wolf::ModelBase::ModelBase(ModelLoadingInfo& modelLoadingInfo, bool requestAccelerationStructuresBuild, BindlessDescriptor* bindlessDescriptor)
{
	ModelLoader::loadObject(m_modelData, modelLoadingInfo);

	if (modelLoadingInfo.loadMaterials)
	{
		std::vector<DescriptorSetGenerator::ImageDescription> imageDescriptions;
		for (const std::unique_ptr<Image>& image : m_modelData.images)
		{
			imageDescriptions.resize(imageDescriptions.size() + 1);
			imageDescriptions.back().imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageDescriptions.back().imageView = image->getDefaultImageView();
		}
		if (bindlessDescriptor->addImages(imageDescriptions) != 5 * modelLoadingInfo.materialIdOffset)
			Debug::sendError("Material ID offset seems wrong");
	}

	m_descriptorSetLayoutGenerator.reset(new LazyInitSharedResource<DescriptorSetLayoutGenerator, ModelBase>([this](std::unique_ptr<DescriptorSetLayoutGenerator>& descriptorSetLayoutGenerator)
		{
			descriptorSetLayoutGenerator.reset(new DescriptorSetLayoutGenerator);
			descriptorSetLayoutGenerator->addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0);
		}));

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, ModelBase>([this](std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(new DescriptorSetLayout(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts()));
		}));

	m_uniformBuffer.reset(new Buffer(sizeof(UniformBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, UpdateRate::NEVER));
	m_descriptorSet.reset(new DescriptorSet(m_descriptorSetLayout->getResource()->getDescriptorSetLayout(), UpdateRate::NEVER));
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

void Wolf::ModelBase::addMeshToRenderList(RenderMeshList& renderMeshList, const RenderMeshList::MeshToRenderInfo::InstanceInfos& instanceInfos) const
{
	RenderMeshList::MeshToRenderInfo meshToRenderInfo(m_modelData.mesh.get(), m_pipelineSet);
	meshToRenderInfo.descriptorSets.push_back({ m_descriptorSet.get(), 0 });
	meshToRenderInfo.instanceInfos = instanceInfos;
	renderMeshList.addMeshToRender(meshToRenderInfo);
}

void Wolf::ModelBase::updateGraphic() const
{
	UniformBufferData uniformBufferData;
	uniformBufferData.model = m_transform;

	m_uniformBuffer->transferCPUMemory(&uniformBufferData, sizeof(uniformBufferData), 0);
}

void Wolf::ModelBase::buildAccelerationStructures()
{
	BottomLevelAccelerationStructureCreateInfo blasCreateInfo;
	blasCreateInfo.buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	std::vector<GeometryInfo> geometries(1);
	geometries[0].mesh = m_modelData.mesh.get(); // to fix
	blasCreateInfo.geometryInfos = geometries;
	m_blas.reset(new BottomLevelAccelerationStructure(blasCreateInfo));
}
