#include "LightManager.h"

#include <DescriptorSetLayout.h>

#include "DescriptorSetGenerator.h"

Wolf::LightManager::LightManager()
{
	m_uniformBuffer.reset(Wolf::Buffer::createBuffer(sizeof(LightsUBData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

	m_descriptorSetLayoutGenerator.addUniformBuffer(ShaderStageFlagBits::FRAGMENT, 0);

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, LightManager>([this](ResourceUniqueOwner<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT));
		}));

	Wolf::DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator.getDescriptorLayouts());
	descriptorSetGenerator.setBuffer(0, *m_uniformBuffer);

	m_descriptorSet.reset(Wolf::DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
}

void Wolf::LightManager::addPointLightForNextFrame(const PointLightInfo& pointLightInfo)
{
	m_nextFramePointLights.emplace_back(pointLightInfo);
}

void Wolf::LightManager::addSunLightInfoForNextFrame(const SunLightInfo& sunLightInfo)
{
	m_nextFrameSunLights.emplace_back(sunLightInfo);
}

void Wolf::LightManager::updateBeforeFrame()
{
	m_currentPointLights.swap(m_nextFramePointLights);
	m_nextFramePointLights.clear();

	m_currentSunLights.swap(m_nextFrameSunLights);
	m_nextFrameSunLights.clear();

	LightsUBData lightsUbData{};

	uint32_t pointLightIdx = 0;
	for (const PointLightInfo& pointLightInfo : m_currentPointLights)
	{
		lightsUbData.pointLights[pointLightIdx].lightPos = glm::vec4(pointLightInfo.worldPos, 1.0f);
		lightsUbData.pointLights[pointLightIdx].lightColor = glm::vec4(pointLightInfo.color * pointLightInfo.intensity * 0.01f, 1.0f);

		pointLightIdx++;

		if (pointLightIdx == MAX_POINT_LIGHTS)
		{
			Wolf::Debug::sendMessageOnce("Max point lights reached, next will be ignored", Wolf::Debug::Severity::WARNING, this);
			break;
		}
	}
	lightsUbData.pointLightsCount = pointLightIdx;

	uint32_t sunLightIdx = 0;
	for (const SunLightInfo& sunLightInfo : m_currentSunLights)
	{
		lightsUbData.sunLights[sunLightIdx].sunDirection = glm::vec4(sunLightInfo.direction, 1.0f);
		lightsUbData.sunLights[sunLightIdx].sunColor = glm::vec4(sunLightInfo.color * sunLightInfo.intensity * 0.01f, 1.0f);

		sunLightIdx++;

		if (sunLightIdx == MAX_SUN_LIGHTS)
		{
			Wolf::Debug::sendMessageOnce("Max sun lights reached, next will be ignored", Wolf::Debug::Severity::WARNING, this);
			break;
		}
	}
	lightsUbData.sunLightsCount = sunLightIdx;

	m_uniformBuffer->transferCPUMemory(&lightsUbData, sizeof(LightsUBData));
}
