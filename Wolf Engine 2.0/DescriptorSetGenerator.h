#pragma once

#include <map>
#include <span>

#include "DescriptorSet.h"
#include "Image.h"
#include "Sampler.h"

namespace Wolf
{
	class DescriptorSetGenerator
	{
	public:
		DescriptorSetGenerator(const std::span<const DescriptorLayout> descriptorLayouts);
		DescriptorSetGenerator(const DescriptorSetGenerator&) = delete;

		void setBuffer(uint32_t binding, const Buffer& buffer);
		void setCombinedImageSampler(uint32_t binding, VkImageLayout imageLayout, VkImageView imageView, const Sampler& sampler);
		struct ImageDescription
		{
			VkImageLayout imageLayout;
			VkImageView imageView;

			ImageDescription(VkImageLayout imageLayout, VkImageView imageView) : imageLayout(imageLayout), imageView(imageView) {}
			ImageDescription() {}
		};
		void setImage(uint32_t binding, ImageDescription& imageDescription);
		void setImages(uint32_t binding, std::vector<ImageDescription>& imageDescriptions);
		void setSampler(uint32_t binding, const Sampler& sampler);

		const DescriptorSetCreateInfo& getDescriptorSetCreateInfo() const { return m_descriptorSetCreateInfo; }

	private:
		DescriptorSetCreateInfo m_descriptorSetCreateInfo;

		enum class DescriptorType { BUFFER, IMAGE, ACCELERATION_STRUCTURE };
		std::map<uint32_t /* binding */, std::pair<DescriptorType, uint32_t /* descriptor index */>> m_mapBindingCreateInfo;
	};
}
