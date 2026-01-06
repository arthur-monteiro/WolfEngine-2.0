#include "LightManager.h"

#include <DescriptorSetLayout.h>
#include <Sampler.h>

#include "DescriptorSetGenerator.h"
#include "ProfilerCommon.h"

Wolf::LightManager::LightManager()
{
	m_uniformBuffer.reset(new UniformBuffer(sizeof(LightsUBData)));
	m_cubeMapSampler.reset(Sampler::createSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1.0f, VK_FILTER_LINEAR, 1));

	CreateImageInfo createCubeMapInfo{};
	createCubeMapInfo.extent = { 4, 4, 1};
	createCubeMapInfo.format = Format::R32G32B32A32_SFLOAT;
	createCubeMapInfo.arrayLayerCount = 6;
	createCubeMapInfo.aspectFlags = ImageAspectFlagBits::COLOR;
	createCubeMapInfo.mipLevelCount = 1;
	createCubeMapInfo.usage = Wolf::ImageUsageFlagBits::TRANSFER_DST | Wolf::ImageUsageFlagBits::SAMPLED;
	m_defaultSkyCubeMap.reset(Wolf::Image::createImage(createCubeMapInfo));
	m_skyCubeMap = m_defaultSkyCubeMap.createNonOwnerResource();

	m_descriptorSetLayoutGenerator.addUniformBuffer(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::RAYGEN | ShaderStageFlagBits::MISS | ShaderStageFlagBits::CLOSEST_HIT, 0); // lights info
	m_descriptorSetLayoutGenerator.addCombinedImageSampler(ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::RAYGEN | ShaderStageFlagBits::MISS | ShaderStageFlagBits::CLOSEST_HIT, 1); // sky cube map

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, LightManager>([this](ResourceUniqueOwner<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
		}));

	m_descriptorSet.reset(Wolf::DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));

	updateDescriptorSet();
}

void Wolf::LightManager::addPointLightForNextFrame(const PointLightInfo& pointLightInfo)
{
	m_pointLightsMutex.lock();
	m_nextFramePointLights.emplace_back(pointLightInfo);
	m_pointLightsMutex.unlock();
}

void Wolf::LightManager::addSunLightInfoForNextFrame(const SunLightInfo& sunLightInfo)
{
	m_nextFrameSunLights.emplace_back(sunLightInfo);
}

void Wolf::LightManager::setSkyCubeMap(ResourceNonOwner<Image> cubeMap)
{
	m_skyCubeMap = cubeMap;
	updateDescriptorSet();
}

void Wolf::LightManager::resetSkyCubeMap()
{
	m_skyCubeMap.release();
}

void Wolf::LightManager::updateBeforeFrame()
{
	PROFILE_FUNCTION

	m_currentPointLights.swap(m_nextFramePointLights);
	m_nextFramePointLights.clear();

	m_currentSunLights.swap(m_nextFrameSunLights);
	m_nextFrameSunLights.clear();

	LightsUBData lightsUbData{};

	if (m_currentPointLights.size() > MAX_POINT_LIGHTS)
	{
		Wolf::Debug::sendMessageOnce("Max point lights reached, next will be ignored", Wolf::Debug::Severity::WARNING, this);
	}
	uint32_t pointLightIdx = 0;
	for (const PointLightInfo& pointLightInfo : m_currentPointLights)
	{
		lightsUbData.pointLights[pointLightIdx].lightPos = glm::vec4(pointLightInfo.worldPos, 1.0f);
		lightsUbData.pointLights[pointLightIdx].lightColor = glm::vec4(pointLightInfo.color * pointLightInfo.intensity * 0.01f, 1.0f);

		pointLightIdx++;

		if (pointLightIdx == MAX_POINT_LIGHTS)
			break;
	}
	lightsUbData.pointLightsCount = pointLightIdx;

	if (m_currentSunLights.size() > MAX_SUN_LIGHTS)
	{
		Wolf::Debug::sendMessageOnce("Max sun lights reached, next will be ignored", Wolf::Debug::Severity::WARNING, this);
	}
	uint32_t sunLightIdx = 0;
	for (const SunLightInfo& sunLightInfo : m_currentSunLights)
	{
		lightsUbData.sunLights[sunLightIdx].sunDirection = glm::vec4(sunLightInfo.direction, 1.0f);
		lightsUbData.sunLights[sunLightIdx].sunColor = glm::vec4(sunLightInfo.color * sunLightInfo.intensity * 0.01f, 1.0f);

		sunLightIdx++;

		if (sunLightIdx == MAX_SUN_LIGHTS)
			break;
	}
	lightsUbData.sunLightsCount = sunLightIdx;

	m_uniformBuffer->transferCPUMemory(&lightsUbData, sizeof(LightsUBData));
}

void Wolf::LightManager::updateDescriptorSet()
{
	Wolf::DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setUniformBuffer(0, *m_uniformBuffer);
	descriptorSetGenerator.setCombinedImageSampler(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_skyCubeMap->getDefaultImageView(), *m_cubeMapSampler);

	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
}
