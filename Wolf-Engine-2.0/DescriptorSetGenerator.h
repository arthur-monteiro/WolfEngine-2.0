#pragma once

#include <map>
#include <span>

#include <ResourceNonOwner.h>

#include "DescriptorSet.h"
#include "Image.h"
#include "ImageView.h"

namespace Wolf
{
	class Buffer;
	class UniformBuffer;

	class DescriptorSetGenerator
	{
	public:
		DescriptorSetGenerator(const std::span<const DescriptorLayout> descriptorLayouts);
		DescriptorSetGenerator(const DescriptorSetGenerator&) = delete;

		void setBuffer(uint32_t binding, const Buffer& buffer);
		void setBuffers(uint32_t binding, const std::vector<ResourceNonOwner<Buffer>>& buffers);
		void setUniformBuffer(uint32_t binding, const UniformBuffer& buffer);
		void setCombinedImageSampler(uint32_t binding, ImageLayout imageLayout, ImageView imageView, const Sampler& sampler);
		struct ImageDescription
		{
			ImageLayout imageLayout;
			ImageView imageView;

			ImageDescription(ImageLayout imageLayout, ImageView imageView) : imageLayout(imageLayout), imageView(imageView) {}
			ImageDescription() {}
		};
		void setImage(uint32_t binding, const ImageDescription& imageDescription);
		void setImages(uint32_t binding, const std::vector<ImageDescription>& imageDescriptions);
		void setSampler(uint32_t binding, const Sampler& sampler);
		void setAccelerationStructure(uint32_t binding, const TopLevelAccelerationStructure& accelerationStructure);

		[[nodiscard]] const DescriptorSetUpdateInfo& getDescriptorSetCreateInfo() const { return m_descriptorSetCreateInfo; }

	private:
		DescriptorSetUpdateInfo m_descriptorSetCreateInfo;

		std::map<uint32_t /* binding */, std::pair<DescriptorType, uint32_t /* descriptor index */>> m_mapBindingCreateInfo;
	};
}
