#pragma once

#include "Attachment.h"
#include "Vulkan.h"

namespace Wolf
{
	class Framebuffer
	{
	public:
		Framebuffer(VkRenderPass renderPass, const std::vector<Attachment>& attachments);
		~Framebuffer();

		[[nodiscard]] VkFramebuffer getFramebuffer() const { return m_framebuffer; }

	private:
		VkFramebuffer m_framebuffer;
	};
}