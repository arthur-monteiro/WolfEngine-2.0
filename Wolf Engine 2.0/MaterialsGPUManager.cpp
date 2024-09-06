#include "MaterialsGPUManager.h"

#include "Buffer.h"
#include "CommandBuffer.h"
#include "DescriptorSetLayoutGenerator.h"

Wolf::MaterialsGPUManager::MaterialsGPUManager(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages)
{
	DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;

	descriptorSetLayoutGenerator.reset();
	descriptorSetLayoutGenerator.addImages(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT, MAX_IMAGES,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	descriptorSetLayoutGenerator.addSampler(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 1);
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 2);

	m_sampler.reset(Sampler::createSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, 11, VK_FILTER_LINEAR));
	m_materialsBuffer.reset(Buffer::createBuffer(MAX_MATERIAL_COUNT * sizeof(MaterialGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	constexpr MaterialGPUInfo initMaterialInfo{ 0, 1, 2 };
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&initMaterialInfo, sizeof(MaterialGPUInfo), 0, 0);
	m_currentMaterialCount = 1;

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, MaterialsGPUManager>([&descriptorSetLayoutGenerator](ResourceUniqueOwner<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(descriptorSetLayoutGenerator.getDescriptorLayouts(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
		}));
	m_descriptorSet.reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));

	DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setImages(BINDING_SLOT, firstImages);
	descriptorSetGenerator.setSampler(BINDING_SLOT + 1, *m_sampler);
	descriptorSetGenerator.setBuffer(BINDING_SLOT + 2, *m_materialsBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	m_currentBindlessCount = static_cast<uint32_t>(firstImages.size());
}

void Wolf::MaterialsGPUManager::addNewMaterials(std::vector<MaterialInfo>& materials)
{
	const size_t materialCountToAdd = materials.size();
	const uint32_t oldNewMaterialsInfoCount = static_cast<uint32_t>(m_newMaterialsInfo.size());
	m_newMaterialsInfo.resize(m_newMaterialsInfo.size() + materialCountToAdd);

	// Clean images to remove nullptr
	std::vector<DescriptorSetGenerator::ImageDescription> imagesCleaned;
	imagesCleaned.reserve(materialCountToAdd * TEXTURE_COUNT_PER_MATERIAL);
	for (MaterialInfo& materialInfo : materials)
	{
		for (ResourceUniqueOwner<Image>& image : materialInfo.images)
		{
			if (image)
			{
				imagesCleaned.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image->getDefaultImageView());
			}
		}
	}
	
	uint32_t currentBindlessOffset = addImagesToBindless(imagesCleaned);

	for (uint32_t newMaterialIdx = 0; newMaterialIdx < materialCountToAdd; ++newMaterialIdx)
	{
		MaterialGPUInfo& newMaterialInfo = m_newMaterialsInfo[oldNewMaterialsInfoCount + newMaterialIdx];

		if (materials[newMaterialIdx].images[0])
			newMaterialInfo.albedoIdx = currentBindlessOffset++;
		if (materials[newMaterialIdx].images[1])
			newMaterialInfo.normalIdx = currentBindlessOffset++;
		if (materials[newMaterialIdx].images[2])
			newMaterialInfo.roughnessMetalnessAOIdx = currentBindlessOffset++;

		newMaterialInfo.shadingMode = materials[newMaterialIdx].shadingMode;
	}

#ifdef MATERIAL_DEBUG
	for (uint32_t i = 0; i < materials.size(); ++i)
	{
		MaterialInfo& material = materials[i];

		MaterialCacheInfo& materialCacheInfo = m_materialsInfoCache.emplace_back();
		materialCacheInfo.materialInfo = &material;

		const MaterialGPUInfo& materialGPUInfo = m_newMaterialsInfo[oldNewMaterialsInfoCount + i];
		materialCacheInfo.albedoIdx = materialGPUInfo.albedoIdx;
		materialCacheInfo.normalIdx = materialGPUInfo.normalIdx;
		materialCacheInfo.roughnessMetalnessAOIdx = materialGPUInfo.roughnessMetalnessAOIdx;
	}
#endif
}

void Wolf::MaterialsGPUManager::pushMaterialsToGPU()
{
	if (m_newMaterialsInfo.empty())
		return;

	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(m_newMaterialsInfo.data(), m_newMaterialsInfo.size() * sizeof(MaterialGPUInfo), 0, m_currentMaterialCount * sizeof(MaterialGPUInfo));

	m_currentMaterialCount += static_cast<uint32_t>(m_newMaterialsInfo.size());
	m_newMaterialsInfo.clear();
}

void Wolf::MaterialsGPUManager::changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode) const
{
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&newShadingMode, sizeof(uint32_t), 0, materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, shadingMode));
}

void Wolf::MaterialsGPUManager::changeExistingMaterialBeforeFrame(uint32_t materialIdx, MaterialCacheInfo& materialCacheInfo)
{
	if (materialIdx == 0)
		return;

	if (!materialCacheInfo.materialInfo->images[0])
		return;

	if (materialCacheInfo.albedoIdx == 0)
	{
		materialCacheInfo.albedoIdx = m_currentBindlessCount++;
		m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&materialCacheInfo.albedoIdx, sizeof(uint32_t), 0, materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, albedoIdx));
	}

	DescriptorSetGenerator::ImageDescription imageToUpdate { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, materialCacheInfo.materialInfo->images[0]->getDefaultImageView()};
	updateImageInBindless(imageToUpdate, materialCacheInfo.albedoIdx);
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

void Wolf::MaterialsGPUManager::updateImageInBindless(const DescriptorSetGenerator::ImageDescription& image, uint32_t bindlessOffset) const
{
	DescriptorSetUpdateInfo descriptorSetUpdateInfo;
	descriptorSetUpdateInfo.descriptorImages.resize(1);

	descriptorSetUpdateInfo.descriptorImages[0].images.resize(1);
	DescriptorSetUpdateInfo::ImageData& imageData = descriptorSetUpdateInfo.descriptorImages[0].images.back();
	imageData.imageLayout = image.imageLayout;
	imageData.imageView = image.imageView;

	DescriptorLayout& descriptorLayout = descriptorSetUpdateInfo.descriptorImages[0].descriptorLayout;
	descriptorLayout.binding = BINDING_SLOT;
	descriptorLayout.arrayIndex = bindlessOffset;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;

	m_descriptorSet->update(descriptorSetUpdateInfo);
}

void Wolf::MaterialsGPUManager::bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const
{
	commandBuffer.bindDescriptorSet(m_descriptorSet.get(), descriptorSlot, pipeline);
}
