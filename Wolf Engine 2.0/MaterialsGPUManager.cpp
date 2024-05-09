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
	m_materialsBuffer.reset(Buffer::createBuffer(MAX_MATERIAL_COUNT * sizeof(MaterialInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	constexpr MaterialInfo initMaterialInfo{ 0, 1, 2 };
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&initMaterialInfo, sizeof(MaterialInfo), 0, 0);
	m_currentMaterialCount = 1;

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, BindlessDescriptor>([&descriptorSetLayoutGenerator](std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
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

void Wolf::MaterialsGPUManager::addNewMaterials(const std::vector<DescriptorSetGenerator::ImageDescription>& images)
{
	if (images.size() % TEXTURE_COUNT_PER_MATERIAL != 0)
	{
		Debug::sendError("Material doesn't have right number of images");
	}

	const uint32_t materialCountToAdd = static_cast<uint32_t>(images.size()) / TEXTURE_COUNT_PER_MATERIAL;
	const uint32_t oldNewMaterialsInfoCount = static_cast<uint32_t>(m_newMaterialsInfo.size());
	m_newMaterialsInfo.resize(m_newMaterialsInfo.size() + materialCountToAdd);

	// Clean images to remove nullptr
	std::vector<DescriptorSetGenerator::ImageDescription> imagesCleaned;
	imagesCleaned.reserve(images.size());
	for (const DescriptorSetGenerator::ImageDescription& imageDescription : images)
	{
		if (imageDescription.imageView)
		{
			imagesCleaned.push_back(imageDescription);
		}
	}
	uint32_t currentBindlessOffset = addImagesToBindless(imagesCleaned);

	for (uint32_t newMaterialIdx = 0; newMaterialIdx < materialCountToAdd; ++newMaterialIdx)
	{
		MaterialInfo& newMaterialInfo = m_newMaterialsInfo[oldNewMaterialsInfoCount + newMaterialIdx];

		if (images[newMaterialIdx / TEXTURE_COUNT_PER_MATERIAL].imageView)
			newMaterialInfo.albedoIdx = currentBindlessOffset++;
		if (images[newMaterialIdx / TEXTURE_COUNT_PER_MATERIAL + 1].imageView)
			newMaterialInfo.normalIdx = currentBindlessOffset++;
		if (images[newMaterialIdx / TEXTURE_COUNT_PER_MATERIAL + 2].imageView)
			newMaterialInfo.roughnessMetalnessAOIdx = currentBindlessOffset++;
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

void Wolf::MaterialsGPUManager::bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const
{
	commandBuffer.bindDescriptorSet(m_descriptorSet.get(), descriptorSlot, pipeline);
}
