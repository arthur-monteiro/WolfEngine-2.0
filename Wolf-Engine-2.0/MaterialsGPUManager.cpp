#include "MaterialsGPUManager.h"

#include <Configuration.h>

#include <CommandBuffer.h>
#include <fstream>

#include "ConfigurationHelper.h"
#include "DescriptorSetLayoutGenerator.h"
#include "MipMapGenerator.h"
#include "ProfilerCommon.h"
#include "Timer.h"

std::vector<std::string> Wolf::MaterialsGPUManager::MaterialInfo::SHADING_MODE_STRING_LIST = { "GGX", "Aniso GGX", "Six ways lighting", "Alpha only" };

Wolf::MaterialsGPUManager::MaterialsGPUManager(const std::vector<DescriptorSetGenerator::ImageDescription>& firstImages, const ResourceNonOwner<GPUDataTransfersManagerInterface>& pushDataToGPU) : m_pushDataToGPUHandler(pushDataToGPU)
{
	DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;

	descriptorSetLayoutGenerator.reset();
	descriptorSetLayoutGenerator.addImages(DescriptorType::SAMPLED_IMAGE, ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT, MAX_IMAGES,
		VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);
	descriptorSetLayoutGenerator.addSampler(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 1);
	descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 2); // Texture sets
	descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 3); // Materials
	if (g_configuration->getUseVirtualTexture())
	{
		descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 4); // Textures info
		descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 5); // Virtual texture feedbacks
		descriptorSetLayoutGenerator.addImages(DescriptorType::SAMPLED_IMAGE, ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 6, 3); // Atlases
		descriptorSetLayoutGenerator.addSampler(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 7); // Albedo atlas
		descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 8); // Indirection buffer
	}

	m_sampler.reset(Sampler::createSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, 11, VK_FILTER_LINEAR, 4.0f));

	if (g_configuration->getUseVirtualTexture())
	{
		m_virtualTextureManager.reset(new VirtualTextureManager({ g_configuration->getWindowWidth(), g_configuration->getWindowHeight() }, m_pushDataToGPUHandler));
		m_albedoAtlasIdx = m_virtualTextureManager->createAtlas(16, 16, Format::BC1_RGB_SRGB_BLOCK); // note that page count per side is a constant in shader
		m_normalAtlasIdx = m_virtualTextureManager->createAtlas(16, 16, Format::BC5_UNORM_BLOCK);
		m_combinedAtlasIdx = m_virtualTextureManager->createAtlas(16, 16, Format::BC3_UNORM_BLOCK);

		m_virtualTextureSampler.reset(Sampler::createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_FILTER_LINEAR));
	}

	m_materialsBuffer.reset(Buffer::createBuffer(MAX_MATERIAL_COUNT * sizeof(MaterialGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	constexpr MaterialGPUInfo initMaterialInfo{ { 0, 0, 0, 0 }, { 1.0f, 0.0f, 0.0f, 0.0}, 0 };
	m_materialsBuffer->transferCPUMemoryWithStagingBuffer(&initMaterialInfo, sizeof(MaterialGPUInfo), 0, 0);
	m_currentMaterialCount = 1;

	m_textureSetsBuffer.reset(Buffer::createBuffer(MAX_TEXTURE_SET_COUNT * sizeof(TextureSetGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	const TextureSetGPUInfo initTextureSetInfo{ 0, 1, 2, 0, glm::vec4(1.0) };
	m_textureSetsBuffer->transferCPUMemoryWithStagingBuffer(&initTextureSetInfo, sizeof(TextureSetGPUInfo), 0, 0);
	m_currentTextureSetCount = 1;

	if (g_configuration->getUseVirtualTexture())
	{
		m_texturesInfoBuffer.reset(Buffer::createBuffer(MAX_TEXTURE_COUNT * sizeof(TextureGPUInfo), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	}

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
	if (g_configuration->getUseVirtualTexture())
	{
		descriptorSetGenerator.setBuffer(BINDING_SLOT + 4, *m_texturesInfoBuffer);
		descriptorSetGenerator.setBuffer(BINDING_SLOT + 5, *m_virtualTextureManager->getFeedbackBuffer());

		std::vector<DescriptorSetGenerator::ImageDescription> atlases(3);
		atlases[0] = { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_virtualTextureManager->getAtlasImage(m_albedoAtlasIdx)->getDefaultImageView()};
		atlases[1] = { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_virtualTextureManager->getAtlasImage(m_normalAtlasIdx)->getDefaultImageView()};
		atlases[2] = { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_virtualTextureManager->getAtlasImage(m_combinedAtlasIdx)->getDefaultImageView() };

		descriptorSetGenerator.setImages(BINDING_SLOT + 6, atlases);
		descriptorSetGenerator.setSampler(BINDING_SLOT + 7, *m_virtualTextureSampler);
		descriptorSetGenerator.setBuffer(BINDING_SLOT + 8, *m_virtualTextureManager->getIndirectionBuffer());
	}
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());

	m_currentBindlessCount = static_cast<uint32_t>(firstImages.size());
	m_texturesCPUInfo.resize(firstImages.size()); // needed to increment the indices
	m_currentTextureInfoCount = static_cast<uint32_t>(m_texturesCPUInfo.size());

	// Idk why, but resizing m_newMaterialInfo sometimes causes a crash in memory delete
	m_newMaterialInfo.reserve(256);
}

void Wolf::MaterialsGPUManager::addNewTextureSet(const TextureSetInfo& textureSetInfo)
{
	if (textureSetInfo.images2.size() != TEXTURE_COUNT_PER_MATERIAL)
	{
		Debug::sendCriticalError("Texture set doesn't have the right number of images");
	}

	TextureSetGPUInfo& newTextureSetInfo = m_newTextureSetsInfo.emplace_back();

	if (g_configuration->getUseVirtualTexture())
	{
		if (!textureSetInfo.slicesFolders[0].empty())
		{
			newTextureSetInfo.albedoIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
			addSlicedImage(textureSetInfo.slicesFolders[0], TextureCPUInfo::TextureType::ALBEDO);
		}

		if (!textureSetInfo.slicesFolders[1].empty())
		{
			newTextureSetInfo.normalIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
			addSlicedImage(textureSetInfo.slicesFolders[1], TextureCPUInfo::TextureType::NORMAL);
		}

		if (!textureSetInfo.slicesFolders[2].empty())
		{
			newTextureSetInfo.roughnessMetalnessAOIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
			addSlicedImage(textureSetInfo.slicesFolders[2], TextureCPUInfo::TextureType::COMBINED_ROUGHNESS_METALNESS_AO);
		}
	}
	else
	{
		// Clean images to remove nullptr
		std::vector<DescriptorSetGenerator::ImageDescription> imagesCleaned;
		imagesCleaned.reserve(textureSetInfo.images2.size());

		for (uint32_t i = 0; i < textureSetInfo.images2.size(); i++)
		{
			if (const NullableResourceNonOwner<Image>& image = textureSetInfo.images2[i])
			{
				imagesCleaned.emplace_back(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image->getDefaultImageView());
			}
		}

		uint32_t currentBindlessOffset = addImagesToBindless(imagesCleaned);

		if (textureSetInfo.images2[0])
			newTextureSetInfo.albedoIdx = currentBindlessOffset++;
		if (textureSetInfo.images2[1])
			newTextureSetInfo.normalIdx = currentBindlessOffset++;
		if (textureSetInfo.images2[2])
			newTextureSetInfo.roughnessMetalnessAOIdx = currentBindlessOffset++;
	}

#ifdef MATERIAL_DEBUG
	TextureSetCacheInfo& textureSetCacheInfo = m_textureSetsCacheInfo.emplace_back();
	textureSetCacheInfo.textureSetIdx = m_currentTextureSetCount + static_cast<uint32_t>(m_newTextureSetsInfo.size()) - 1;
	textureSetCacheInfo.albedoIdx = newTextureSetInfo.albedoIdx;
	textureSetCacheInfo.normalIdx = newTextureSetInfo.normalIdx;
	textureSetCacheInfo.roughnessMetalnessAOIdx = newTextureSetInfo.roughnessMetalnessAOIdx;
	textureSetCacheInfo.materialName = textureSetInfo.name;
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

void Wolf::MaterialsGPUManager::updateBeforeFrame()
{
	PROFILE_FUNCTION

	if (!m_newMaterialInfo.empty())
	{
		m_pushDataToGPUHandler->pushDataToGPUBuffer(m_newMaterialInfo.data(), static_cast<uint32_t>(m_newMaterialInfo.size()) * sizeof(MaterialGPUInfo), m_materialsBuffer.createNonOwnerResource(),
			m_currentMaterialCount * sizeof(MaterialGPUInfo));
		m_currentMaterialCount += static_cast<uint32_t>(m_newMaterialInfo.size());
		m_newMaterialInfo.clear();
	}

	if (!m_newTextureSetsInfo.empty())
	{
		m_pushDataToGPUHandler->pushDataToGPUBuffer(m_newTextureSetsInfo.data(), static_cast<uint32_t>(m_newTextureSetsInfo.size()) * sizeof(TextureSetGPUInfo), m_textureSetsBuffer.createNonOwnerResource(),
			m_currentTextureSetCount * sizeof(TextureSetGPUInfo));
		m_currentTextureSetCount += static_cast<uint32_t>(m_newTextureSetsInfo.size());
		m_newTextureSetsInfo.clear();
	}

	if (!m_newTextureInfo.empty())
	{
		m_pushDataToGPUHandler->pushDataToGPUBuffer(m_newTextureInfo.data(), static_cast<uint32_t>(m_newTextureInfo.size()) * sizeof(TextureGPUInfo), m_texturesInfoBuffer.createNonOwnerResource(),
			m_currentTextureInfoCount * sizeof(TextureGPUInfo));
		m_currentTextureInfoCount += static_cast<uint32_t>(m_newTextureInfo.size());
		m_newTextureInfo.clear();
	}

	if (g_configuration->getUseVirtualTexture())
	{
		m_virtualTextureManager->updateBeforeFrame();

		std::vector<VirtualTextureManager::FeedbackInfo> requestedSlices;
		m_virtualTextureManager->getRequestedSlices(requestedSlices, 4);

		for (const VirtualTextureManager::FeedbackInfo& requestedSlice : requestedSlices)
		{
			uint16_t textureId = requestedSlice.m_textureId;
			if (textureId <= 2)
			{
				m_virtualTextureManager->rejectRequest(requestedSlice);
				continue;
			}

			const std::string& sliceFolder = m_texturesCPUInfo[textureId].m_slicesFolder;
			uint8_t sliceX = requestedSlice.m_sliceX;
			uint8_t sliceY = requestedSlice.m_sliceY;
			uint8_t mipLevel = requestedSlice.m_mipLevel;

			// Virtual texture manager will create request for the mip above (for trilinear sampling), we may get out range mip level
			if (mipLevel >= MipMapGenerator::computeMipCount({ m_texturesCPUInfo[textureId].m_width, m_texturesCPUInfo[textureId].m_width }))
			{
				m_virtualTextureManager->rejectRequest(requestedSlice);
				continue;
			}

			float pixelSizeInBytes = 0.0f;
			if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::ALBEDO)
			{
				pixelSizeInBytes = 0.5f; // BC1
			}
			else if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::NORMAL)
			{
				pixelSizeInBytes = 1.0f; // BC5
			}
			else if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::COMBINED_ROUGHNESS_METALNESS_AO)
			{
				pixelSizeInBytes = 1.0f; // BC3
			}
 			else
 			{
 				m_virtualTextureManager->rejectRequest(requestedSlice);
 				continue;
 			}

			std::string binFilename = sliceFolder + "mip" + std::to_string(mipLevel) + "_sliceX" + std::to_string(sliceX) + "_sliceY" + std::to_string(sliceY) + ".bin";
			std::ifstream input(binFilename, std::ios::in | std::ios::binary);
			if (!input.is_open())
			{
				Debug::sendError("Unable to open file");
				m_virtualTextureManager->rejectRequest(requestedSlice);
				continue;
			}

			uint64_t hash;
			input.read(reinterpret_cast<char*>(&hash), sizeof(hash));
			// TODO: check hash

			uint32_t dataBytesCount;
			input.read(reinterpret_cast<char*>(&dataBytesCount), sizeof(dataBytesCount));

			Extent3D maxSliceExtent{ VirtualTextureManager::VIRTUAL_PAGE_SIZE, VirtualTextureManager::VIRTUAL_PAGE_SIZE, 1 };
			Extent3D extentForMip = { m_texturesCPUInfo[textureId].m_width >> mipLevel, m_texturesCPUInfo[textureId].m_height >> mipLevel, 1 };
			Extent3D sliceExtent{ std::min(extentForMip.width, maxSliceExtent.width) + 2 * VirtualTextureManager::BORDER_SIZE,
				std::min(extentForMip.height, maxSliceExtent.height) + 2 * VirtualTextureManager::BORDER_SIZE, 1 };

			if (static_cast<float>(sliceExtent.width) * static_cast<float>(sliceExtent.height) * pixelSizeInBytes != static_cast<float>(dataBytesCount))
			{
				Debug::sendError("Wrong slice size");
			}

			std::vector<uint8_t> data(dataBytesCount);
			input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));

			input.close();

			if (m_texturesCPUInfo[textureId].m_virtualTextureIndirectionOffset == VirtualTextureManager::INVALID_INDIRECTION_OFFSET)
			{
				uint32_t indirectionCount = computeSliceCount(m_texturesCPUInfo[textureId].m_width, m_texturesCPUInfo[textureId].m_height);
				uint32_t virtualTextureIndirectionOffset = m_virtualTextureManager->createNewIndirection(indirectionCount);

				m_texturesCPUInfo[textureId].m_virtualTextureIndirectionOffset = virtualTextureIndirectionOffset;
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&virtualTextureIndirectionOffset, sizeof(uint32_t), m_texturesInfoBuffer.createNonOwnerResource(), textureId * sizeof(TextureGPUInfo) + offsetof(TextureGPUInfo, virtualTextureIndirectionOffset));
			}

			uint8_t sliceCountX = m_texturesCPUInfo[textureId].m_height / VirtualTextureManager::VIRTUAL_PAGE_SIZE;
			uint8_t sliceCountY = m_texturesCPUInfo[textureId].m_height / VirtualTextureManager::VIRTUAL_PAGE_SIZE;

			uint32_t atlasIdx = -1;
			if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::ALBEDO)
				atlasIdx = m_albedoAtlasIdx;
			else if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::NORMAL)
				atlasIdx = m_normalAtlasIdx;
			else if (m_texturesCPUInfo[textureId].m_textureType == TextureCPUInfo::TextureType::COMBINED_ROUGHNESS_METALNESS_AO)
				atlasIdx = m_combinedAtlasIdx;

			if (atlasIdx == -1)
				Debug::sendCriticalError("Wrong atlas index");

			m_virtualTextureManager->uploadData(atlasIdx, data, sliceExtent, sliceX, sliceY, mipLevel, sliceCountX, sliceCountY, m_texturesCPUInfo[textureId].m_virtualTextureIndirectionOffset, requestedSlice);
		}
	}
}

void Wolf::MaterialsGPUManager::resize(Extent2D newExtent)
{
	if (g_configuration->getUseVirtualTexture())
	{
		m_virtualTextureManager->resize(newExtent);

		DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;
		descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::CLOSEST_HIT, BINDING_SLOT + 5); // Virtual texture feedbacks

		DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
		descriptorSetGenerator.setBuffer(BINDING_SLOT + 5, *m_virtualTextureManager->getFeedbackBuffer());
		m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}

void Wolf::MaterialsGPUManager::lockTextureSets()
{
	m_textureSetsMutex.lock();
}

void Wolf::MaterialsGPUManager::unlockTextureSets()
{
	m_textureSetsMutex.unlock();
}

void Wolf::MaterialsGPUManager::lockMaterials()
{
	m_materialsMutex.lock();
}

void Wolf::MaterialsGPUManager::unlockMaterials()
{
	m_materialsMutex.unlock();
}

#ifdef MATERIAL_DEBUG
void Wolf::MaterialsGPUManager::changeMaterialShadingModeBeforeFrame(uint32_t materialIdx, uint32_t newShadingMode)
{
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&newShadingMode, sizeof(uint32_t), m_materialsBuffer.createNonOwnerResource(), materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, shadingMode));
}

void Wolf::MaterialsGPUManager::changeTextureSetIdxBeforeFrame(uint32_t materialIdx,uint32_t indexOfTextureSetInMaterial, uint32_t newTextureSetIdx)
{
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&newTextureSetIdx, sizeof(uint32_t), m_materialsBuffer.createNonOwnerResource(),
		materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, textureSetIndices) + indexOfTextureSetInMaterial * sizeof(uint32_t));
}

void Wolf::MaterialsGPUManager::changeStrengthBeforeFrame(uint32_t materialIdx, uint32_t indexOfTextureSetInMaterial, float newStrength)
{
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&newStrength, sizeof(float), m_materialsBuffer.createNonOwnerResource(),
		materialIdx * sizeof(MaterialGPUInfo) + offsetof(MaterialGPUInfo, strengths) + indexOfTextureSetInMaterial * sizeof(float));
}

void Wolf::MaterialsGPUManager::changeExistingTextureSetBeforeFrame(TextureSetCacheInfo& textureSetCacheInfo, const TextureSetInfo& textureSetInfo)
{
	if (textureSetCacheInfo.textureSetIdx == 0)
		return;

	if (g_configuration->getUseVirtualTexture())
	{
		auto updateInfo = [this](uint32_t textureIdx, const std::string& slicesFolder)
		{
			m_texturesCPUInfo[textureIdx].m_slicesFolder = slicesFolder;

			if (!slicesFolder.empty())
			{
				uint32_t width = std::stoi(ConfigurationHelper::readInfoFromFile(slicesFolder + "info.txt", "width"));
				uint32_t height = std::stoi(ConfigurationHelper::readInfoFromFile(slicesFolder + "info.txt", "height"));

				m_pushDataToGPUHandler->pushDataToGPUBuffer(&width, sizeof(uint32_t), m_texturesInfoBuffer.createNonOwnerResource(), textureIdx * sizeof(TextureGPUInfo) + offsetof(TextureGPUInfo, width));
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&height, sizeof(uint32_t), m_texturesInfoBuffer.createNonOwnerResource(), textureIdx * sizeof(TextureGPUInfo) + offsetof(TextureGPUInfo, height));
			}
		};

		if (!textureSetInfo.slicesFolders[0].empty())
		{
			if (textureSetCacheInfo.albedoIdx == 0)
			{
				textureSetCacheInfo.albedoIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.albedoIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
						textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, albedoIdx));
				addSlicedImage(textureSetInfo.slicesFolders[0], TextureCPUInfo::TextureType::ALBEDO);
			}
			else
			{
				updateInfo(textureSetCacheInfo.albedoIdx, textureSetInfo.slicesFolders[0]);
			}
		}

		if (!textureSetInfo.slicesFolders[1].empty())
		{
			if (textureSetCacheInfo.normalIdx == 1)
			{
				textureSetCacheInfo.normalIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.normalIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
						textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, normalIdx));
				addSlicedImage(textureSetInfo.slicesFolders[1], TextureCPUInfo::TextureType::NORMAL);
			}
			else
			{
				updateInfo(textureSetCacheInfo.normalIdx, textureSetInfo.slicesFolders[1]);
			}
		}

		if (!textureSetInfo.slicesFolders[2].empty())
		{
			if (textureSetCacheInfo.roughnessMetalnessAOIdx == 2)
			{
				textureSetCacheInfo.roughnessMetalnessAOIdx = static_cast<uint32_t>(m_texturesCPUInfo.size());
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.roughnessMetalnessAOIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
						textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, roughnessMetalnessAOIdx));
				addSlicedImage(textureSetInfo.slicesFolders[2], TextureCPUInfo::TextureType::COMBINED_ROUGHNESS_METALNESS_AO);
			}
			else
			{
				updateInfo(textureSetCacheInfo.roughnessMetalnessAOIdx, textureSetInfo.slicesFolders[2]);
			}
		}

		// TODO: reload VT where textures are written
	}
	else
	{
		if (textureSetInfo.images2[0])
		{
			if (textureSetCacheInfo.albedoIdx == 0)
			{
				textureSetCacheInfo.albedoIdx = m_currentBindlessCount++;
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.albedoIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
					textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, albedoIdx));
			}

			DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images2[0]->getDefaultImageView() };
			updateImageInBindless(imageToUpdate, textureSetCacheInfo.albedoIdx);
		}

		if (textureSetInfo.images2[1])
		{
			if (textureSetCacheInfo.normalIdx == 1)
			{
				textureSetCacheInfo.normalIdx = m_currentBindlessCount++;
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.normalIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
					textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, normalIdx));
			}

			DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images2[1]->getDefaultImageView() };
			updateImageInBindless(imageToUpdate, textureSetCacheInfo.normalIdx);
		}

		if (textureSetInfo.images2[2])
		{
			if (textureSetCacheInfo.roughnessMetalnessAOIdx == 2)
			{
				textureSetCacheInfo.roughnessMetalnessAOIdx = m_currentBindlessCount++;
				m_pushDataToGPUHandler->pushDataToGPUBuffer(&textureSetCacheInfo.roughnessMetalnessAOIdx, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(),
					textureSetCacheInfo.textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, roughnessMetalnessAOIdx));
			}

			DescriptorSetGenerator::ImageDescription imageToUpdate{ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, textureSetInfo.images2[2]->getDefaultImageView() };
			updateImageInBindless(imageToUpdate, textureSetCacheInfo.roughnessMetalnessAOIdx);
		}
	}
}

void Wolf::MaterialsGPUManager::changeSamplingModeBeforeFrame(uint32_t textureSetIdx, TextureSetInfo::SamplingMode newSamplingMode)
{
	uint32_t newSamplingModeUInt = static_cast<uint32_t>(newSamplingMode);
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&newSamplingModeUInt, sizeof(uint32_t), m_textureSetsBuffer.createNonOwnerResource(), textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, samplingMode));
}

void Wolf::MaterialsGPUManager::changeScaleBeforeFrame(uint32_t textureSetIdx, glm::vec3 newScale)
{
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&newScale, sizeof(glm::vec3), m_textureSetsBuffer.createNonOwnerResource(), textureSetIdx * sizeof(TextureSetGPUInfo) + offsetof(TextureSetGPUInfo, scale));
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
		descriptorLayout.descriptorType = DescriptorType::SAMPLED_IMAGE;
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
	descriptorLayout.descriptorType = DescriptorType::SAMPLED_IMAGE;

	m_descriptorSet->update(descriptorSetUpdateInfo);
}

uint32_t Wolf::MaterialsGPUManager::computeSliceCount(uint32_t textureWidth, uint32_t textureHeight)
{
	uint32_t sliceCount = 0;

	Extent3D maxSliceExtent{ VirtualTextureManager::VIRTUAL_PAGE_SIZE, VirtualTextureManager::VIRTUAL_PAGE_SIZE, 1 };

	uint32_t textureMipCount = MipMapGenerator::computeMipCount({ textureWidth, textureHeight });
	for (uint32_t mipLevel = 0; mipLevel < textureMipCount; mipLevel++)
	{
		Extent3D extentForMip = { textureWidth >> mipLevel, textureHeight >> mipLevel, 1 };

		const uint32_t sliceCountX = std::max(extentForMip.width / maxSliceExtent.width, 1u);
		const uint32_t sliceCountY = std::max(extentForMip.height / maxSliceExtent.height, 1u);

		sliceCount += sliceCountX * sliceCountY;
	}

	return sliceCount;
}

void Wolf::MaterialsGPUManager::addSlicedImage(const std::string& folder, TextureCPUInfo::TextureType textureType)
{
	TextureGPUInfo& textureInfo = m_newTextureInfo.emplace_back();

	textureInfo.width = std::stoi(ConfigurationHelper::readInfoFromFile(folder + "info.txt", "width"));
	textureInfo.height = std::stoi(ConfigurationHelper::readInfoFromFile(folder + "info.txt", "height"));
	textureInfo.virtualTextureIndirectionOffset = -1;

	m_texturesCPUInfo.emplace_back(folder, textureType, textureInfo.width, textureInfo.height, textureInfo.virtualTextureIndirectionOffset);
}

void Wolf::MaterialsGPUManager::bind(const CommandBuffer& commandBuffer, const Pipeline& pipeline, uint32_t descriptorSlot) const
{
	commandBuffer.bindDescriptorSet(m_descriptorSet.get(), descriptorSlot, pipeline);
}
