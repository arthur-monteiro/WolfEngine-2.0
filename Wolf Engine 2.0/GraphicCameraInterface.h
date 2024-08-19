#pragma once

#include <glm/glm.hpp>

#include <Buffer.h>

#include "CameraInterface.h"
#include "DescriptorSet.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSetLayoutGenerator.h"
#include "LazyInitSharedResource.h"

namespace Wolf
{
	class GraphicCameraInterface
	{
	public:
		[[nodiscard]] virtual const glm::mat4& getViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getPreviousViewMatrix() const = 0;
		[[nodiscard]] virtual const glm::mat4& getProjectionMatrix() const = 0;
		[[nodiscard]] virtual float getNear() const = 0;
		[[nodiscard]] virtual float getFar() const = 0;

		static ResourceUniqueOwner<DescriptorSetLayout>& getDescriptorSetLayout() { return LazyInitSharedResource<DescriptorSetLayout, GraphicCameraInterface>::getResource(); }
		virtual const DescriptorSet* getDescriptorSet() const { return m_descriptorSet.get(); }

	protected:
		GraphicCameraInterface();
		void updateGraphic(const glm::vec2& pixelJitter) const;

	private:
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayoutGenerator, GraphicCameraInterface>> m_descriptorSetLayoutGenerator;
		std::unique_ptr<LazyInitSharedResource<DescriptorSetLayout, GraphicCameraInterface>> m_descriptorSetLayout;

		struct UniformBufferData
		{
			glm::mat4 view;

			glm::mat4 projection;

			glm::mat4 invView;

			glm::mat4 invProjection;

			glm::mat4 previousViewMatrix;

			glm::vec2 jitter;
			glm::vec2 projectionParams;

			glm::f32  near;
			glm::f32  far;
		};

		std::unique_ptr<DescriptorSet> m_descriptorSet;
		std::unique_ptr<Buffer> m_matricesUniformBuffer;
	};
}
