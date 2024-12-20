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
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 2); // Texture sets
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_FRAGMENT_BIT, BINDING_SLOT + 3); // Materials

	m_sampler.reset(Sampler::createSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, 11, VK_FILTER_LINEAR));

	m_textureSetsBuffer.reset(Buffer::createBuffer(MAX_TEXTURE_SET_COUNT * sizeof(TextureSetGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	const TextureSetGPUInfo initTextureSetInfo{ 0, 1, 2, 0, glm::vec4(1.0) };
	m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&initTextureSetInfo, sizeof(TextureSetGPUInfo), 0, 0);
	m_currentTextureSetCount = 1;

	m_materialsBuffer.reset(Buffer::createBuffer(MAX_MATERIAL_COUNT * sizeof(MaterialGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	constexpr MaterialGPUInfo initMaterialInfo{ { 0, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0}, 0 };
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
	descriptorSetGenerator.setBuffer(BINDING_SLOT + 2, *m_textureSetsBuffer);
	descriptorSetGenerator.setBuffer(BINDING_SLOT + 3, *m_materialsBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	m_currentBindlessCount = static_cast<uint32_t>(firstImages.size());

	// Idk why, but resizing m_newMaterialInfo sometimes causes a crash in memory delete
	m_newMaterialInfo.reserve(256);
}

void Wolf::MaterialsGPUManager::addNewTextureSet(const TextureSetInfo& textureSetInfo)
{
	if (textureSetInfo.images.size() != TEXTURE_COUNT_PER_MATERIAL)
	{
		Debug::sendCriticalError("Texture set doesn't have the right number of images");
	}

	// Clean images to remove nullptr
	std::vector<DescriptorSetGenerator::ImageDescription> imagesCleaned;
	imagesCleaned.reserve(textureSetInfo.images.size());
	for (const ResourceUniqueOwner<Image>& image : textureSetInfo.images)
	{
		if (image)
		{
			imagesCleaned.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image->getDefaultImageView());
		}
	}

	uint32_t currentBindlessOffset = addImagesToBindless(imagesCleaned);

	TextureSetGPUInfo& newTextureSetInfo = m_newTextureSetsInfo.emplace_back();
	if (textureSetInfo.images[0])
		newTextureSetInfo.albedoIdx = currentBindlessOffset++;
	if (textureSetInfo.images[1])
		newTextureSetInfo.normalIdx = currentBindlessOffset++;
	if (textureSetInfo.images[2])
		newTextureSetInfo.roughnessMetalnessAOIdx = currentBindlessOffset++;

#ifdef MATERIAL_DEBUG
	TextureSetCacheInfo& textureSetCacheInfo = m_textureSetsCacheInfo.emplace_back();
	textureSetCacheInfo.textureSetIdx = m_currentTextureSetCount + static_cast<uint32_t>(m_newTextureSetsInfo.size()) - 1;
	textureSetCacheInfo.albedoIdx = newTextureSetInfo.albedoIdx;
	textureSetCacheInfo.normalIdx = newTextureSetInfo.normalIdx;
	textureSetCacheInfo.roughnessMetalnessAOIdx = newTextureSetInfo.roughnessMetalnessAOIdx;
	textureSetCacheInfo.materialName = textureSetInfo.materialName;
	textureSetCacheInfo.imageNames = textureSetInfo.imageNames;
	textureSetCacheInfo.materialFolder = textureSetInfo.materialFolder;
#endif
}

void Wolf::MaterialsGPUManager::addNewMaterial(const MaterialInfo& material)
{
	MaterialGPUInfo& newMaterialInfo = m_newMaterialInfo.emplace_back();

	for (uint32_t i = 0; i < MaterialInfo::MAX_TEXTURE_SET_PER_MATERIAL; ++i)
	{
		newMaterialInfo.textureSetIndices[i] = material.textureSetInfos[i].textureSetIdx;
		newMaterialInfo.strengths[i] = material.textureSetInfos[i].strength;
	}
	newMaterialInfo.shadingMode = static_cast<uint32_t>(material.shadingMode);
}

void Wolf::MaterialsGPUManager::pushMaterialsToGPU()
{
	if (!m_newTextureSetsInfo.empty())
	{
		m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(m_newTextureSetsInfo.data(), m_newTextureSetsInfo.size() * sizeof(TextureSetGPUInfo), 0, m_currentTextureSetCount * sizeof(TextureSetGPUInfo));
		m_currentTextureSetCount += static_cast<uint32_t>(m_newTextureSetsInfo.size());
		m_newTextureSetsInfo.clear();
	}

	if (!m_newMaterialInfo.empty())
	{
		m_materialsBuffer->transferCPUMemoryWithStagingBuffer(m_newMaterialInfo.data(), m_newMaterialInfo.size() * sizeof(MaterialGPUInfo), 0, m_currentMaterialCount * sizeof(MaterialGPUInfo));
		m_currentMaterialCount += static_cast<uint32_t>(m_newMaterialInfo.size());
		m_newMaterialInfo.clear();
	}
}

#ifdef MATERIAL_DEBUG
void Wolf::MaterialsGPUManager::changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode) const
{
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&newShadingMode, sizeof(uint32_t), 0, materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, shadingMode));
}

void Wolf::MaterialsGPUManager::changeTextureSetIdxBeforeFrame(uint32_t materialIdx,uint32_t indexOfTextureSetInMaterial, uint32_t newTextureSetIdx) const
{
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&newTextureSetIdx, sizeof(uint32_t), 0, 
		materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, textureSetIndices) + indexOfTextureSetInMaterial * sizeof(uint32_t));
}

void Wolf::MaterialsGPUManager::changeStrengthBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial,
	float newStrength) const
{
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&newStrength, sizeof(float), 0,
		materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, strengths) + indexOfTextureSetInMaterial * sizeof(float));
}

void Wolf::MaterialsGPUManager::changeExistingTextureSetBeforeFrame(TextureSetCacheInfo& textureSetCacheInfo, const TextureSetInfo& textureSetInfo)
{
	if (textureSetCacheInfo.textureSetIdx == 0)
		return;

	if (textureSetInfo.images[0])
	{
		if (textureSetCacheInfo.albedoIdx == 0)
		{
			textureSetCacheInfo.albedoIdx = m_currentBindlessCount++;
			m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&textureSetCacheInfo.albedoIdx, sizeof(uint32_t), 0, textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, albedoIdx));
		}

		DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images[0]->getDefaultImageView() };
		updateImageInBindless(imageToUpdate, textureSetCacheInfo.albedoIdx);
	}

	if (textureSetInfo.images[1])
	{
		if (textureSetCacheInfo.normalIdx == 1)
		{
			textureSetCacheInfo.normalIdx = m_currentBindlessCount++;
			m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&textureSetCacheInfo.normalIdx, sizeof(uint32_t), 0, textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, normalIdx));
		}

		DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images[1]->getDefaultImageView() };
		updateImageInBindless(imageToUpdate, textureSetCacheInfo.normalIdx);
	}

	if (textureSetInfo.images[2])
	{
		if (textureSetCacheInfo.roughnessMetalnessAOIdx == 2)
		{
			textureSetCacheInfo.roughnessMetalnessAOIdx = m_currentBindlessCount++;
			m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&textureSetCacheInfo.roughnessMetalnessAOIdx, sizeof(uint32_t), 0, textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, roughnessMetalnessAOIdx));
		}

		DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images[2]->getDefaultImageView() };
		updateImageInBindless(imageToUpdate, textureSetCacheInfo.roughnessMetalnessAOIdx);
	}
}

void Wolf::MaterialsGPUManager::changeSamplingModeBeforeFrame(uint32_t textureSetIdx, TextureSetInfo::SamplingMode newSamplingMode) const
{
	uint32_t newSamplingModeUInt = static_cast<uint32_t>(newSamplingMode);
	m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&newSamplingModeUInt, sizeof(uint32_t), 0, textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, samplingMode));
}

void Wolf::MaterialsGPUManager::changeScaleBeforeFrame(uint32_t textureSetIdx, glm::vec3 newScale) const
{
	m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&newScale, sizeof(glm::vec3), 0, textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, scale));
}
#endif

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
