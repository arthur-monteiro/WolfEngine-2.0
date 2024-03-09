#include "MaterialsGPUManager.h"

#include "DescriptorSetLayoutGenerator.h"

Wolf::MaterialsGPUManager::MaterialsGPUManager(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages)
{
	DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;

	descriptorSetLayoutGenerator.reset();
	descriptorSetLayoutGenerator.addImages(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT, MAX_IMAGES,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	descriptorSetLayoutGenerator.addSampler(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 1);
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 2);

	m_sampler.reset(new Sampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, 11, VK_FILTER_LINEAR));
	m_materialsBuffer.reset(new Buffer(MAX_MATERIAL_COUNT * sizeof(MaterialInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, UpdateRate::NEVER));

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>([&descriptorSetLayoutGenerator](std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(new DescriptorSetLayout(descriptorSetLayoutGenerator.getDescriptorLayouts(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
		}));
	m_descriptorSet.reset(new DescriptorSet(m_descriptorSetLayout->getResource()->getDescriptorSetLayout(), UpdateRate::NEVER));

	DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setImages(BINDING_SLOT, firstImages);
	descriptorSetGenerator.setSampler(BINDING_SLOT + 1, *m_sampler);
	descriptorSetGenerator.setBuffer(BINDING_SLOT + 2, *m_materialsBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	m_currentBindlessCount = static_cast<uint32_t>(firstImages.size());
}

void Wolf::MaterialsGPUManager::addNewMaterials(const std::vector<DescriptorSetGenerator::ImageDescription>& images)
{
	if (images.size() % 5 != 0)
	{
		Debug::sendError("Material doesn't have right number of images");
	}

	const uint32_t materialCountToAdd = static_cast<uint32_t>(images.size()) / 5;
	const uint32_t oldNewMaterialsInfoCount = static_cast<uint32_t>(m_newMaterialsInfo.size());
	m_newMaterialsInfo.resize(m_newMaterialsInfo.size() + materialCountToAdd);

	const uint32_t firstBindlessOffset = addImagesToBindless(images);
	for (uint32_t newMaterialIdx = 0; newMaterialIdx < materialCountToAdd; ++newMaterialIdx)
	{
		MaterialInfo& newMaterialInfo = m_newMaterialsInfo[oldNewMaterialsInfoCount + newMaterialIdx];

		newMaterialInfo.albedoIdx = firstBindlessOffset + newMaterialIdx * 5;
		newMaterialInfo.normalIdx = newMaterialInfo.albedoIdx + 1;
		newMaterialInfo.roughnessIdx = newMaterialInfo.albedoIdx + 2;
		newMaterialInfo.metalnessIdx = newMaterialInfo.albedoIdx + 3;
		newMaterialInfo.aoIdx = newMaterialInfo.albedoIdx + 4;
	}
}

void Wolf::MaterialsGPUManager::pushMaterialsToGPU()
{
	if (m_newMaterialsInfo.empty())
		return;

	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(m_newMaterialsInfo.data(), m_newMaterialsInfo.size() * sizeof(MaterialInfo), 0, m_currentMaterialCount * sizeof(MaterialInfo));

	m_currentMaterialCount += static_cast<uint32_t>(m_newMaterialsInfo.size());
	m_newMaterialsInfo.clear();
}

uint32_t Wolf::MaterialsGPUManager::addImagesToBindless(const std::vector<DescriptorSetGenerator::ImageDescription>& images)
{
	const uint32_t previousCounter = m_currentBindlessCount;

	DescriptorSetUpdateInfo descriptorSetUpdateInfo;
	descriptorSetUpdateInfo.descriptorImages.resize(images.size());

	for (uint32_t i = 0; i < images.size(); ++i)
	{
		descriptorSetUpdateInfo.descriptorImages[i].images.resize(1);
		DescriptorSetUpdateInfo::ImageData& imageData = descriptorSetUpdateInfo.descriptorImages[i].images.back();
		imageData.imageLayout = images[i].imageLayout;
		imageData.imageView = images[i].imageView;

		DescriptorLayout& descriptorLayout = descriptorSetUpdateInfo.descriptorImages[i].descriptorLayout;
		descriptorLayout.binding = BINDING_SLOT;
		descriptorLayout.arrayIndex = m_currentBindlessCount++;
		descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	}

	m_descriptorSet->update(descriptorSetUpdateInfo);

	return previousCounter;
}

void Wolf::MaterialsGPUManager::bind(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t descriptorSlot) const
{
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, descriptorSlot, 1, m_descriptorSet->getDescriptorSet(), 0, nullptr);
}
