#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <vector>

#include <DescriptorSet.h>
#include <DescriptorSetLayout.h>
#include <Image.h>
#include <UniformBuffer.h>

#include "DescriptorSetLayoutGenerator.h"
#include "LazyInitSharedResource.h"
#include "ResourceUniqueOwner.h"

namespace Wolf
{
	class LightManager
	{
	public:
		LightManager();

		struct PointLightInfo
		{
			glm::vec3 worldPos;
			glm::vec3 color;
			float intensity;
		};
		void addPointLightForNextFrame(const PointLightInfo& pointLightInfo);

		struct SunLightInfo
		{
			glm::vec3 direction;
			float angle;
			glm::vec3 color;
			float intensity;
		};
		void addSunLightInfoForNextFrame(const SunLightInfo& sunLightInfo);

		void setSkyCubeMap(ResourceNonOwner<Image> cubeMap);
		void resetSkyCubeMap();

		void updateBeforeFrame();

		[[nodiscard]] static ResourceUniqueOwner<DescriptorSetLayout>& getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, LightManager>::getResource(); }
		ResourceUniqueOwner<DescriptorSet>& getDescriptorSet() { return m_descriptorSet; }

		uint32_t getSunLightCount() const { return static_cast<uint32_t>(m_currentSunLights.size()); }
		const SunLightInfo& getSunLightInfo(uint32_t idx) const { return m_currentSunLights[idx]; }

	private:
		void updateDescriptorSet();

		std::vector<PointLightInfo> m_currentPointLights;
		std::vector<PointLightInfo> m_nextFramePointLights;
		std::mutex m_pointLightsMutex;

		std::vector<SunLightInfo> m_currentSunLights;
		std::vector<SunLightInfo> m_nextFrameSunLights;

		// GPU Info
		struct PointLightUBInfo
		{
			glm::vec4 lightPos;
			glm::vec4 lightColor;
		};
		static constexpr uint32_t MAX_POINT_LIGHTS = 16;

		struct SunLightUBInfo
		{
			glm::vec4 sunDirection;
			glm::vec4 sunColor;
		};
		static constexpr uint32_t MAX_SUN_LIGHTS = 1;

		struct LightsUBData
		{
			PointLightUBInfo pointLights[MAX_POINT_LIGHTS];
			uint32_t pointLightsCount;
			glm::vec3 padding;

			SunLightUBInfo sunLights[MAX_SUN_LIGHTS];
			uint32_t sunLightsCount;
			glm::vec3 padding2;
		};
		ResourceUniqueOwner<UniformBuffer> m_uniformBuffer;

		ResourceUniqueOwner<Image> m_defaultSkyCubeMap;
		NullableResourceNonOwner<Image> m_skyCubeMap;
		ResourceUniqueOwner<Sampler> m_cubeMapSampler;

		DescriptorSetLayoutGenerator m_descriptorSetLayoutGenerator;
		ResourceUniqueOwner<DescriptorSet> m_descriptorSet;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, LightManager>> m_descriptorSetLayout;
	};

}
