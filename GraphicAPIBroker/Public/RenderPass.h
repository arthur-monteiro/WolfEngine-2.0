#pragma once

#include <functional>
#include <vector>

#include "Attachment.h"

namespace Wolf
{
	class FrameBuffer;

	class RenderPass
	{
	public:
		static RenderPass* createRenderPass(const std::vector<Attachment>& attachments);
		virtual ~RenderPass() = default;

		[[nodiscard]] const Extent2D& getExtent() const { return m_extent; }

		void setExtent(const Extent2D& extent);
		void registerNewExtentChangedCallback(const std::function<void(const RenderPass*)>& callback) { m_callbackWhenExtentChanged.push_back(callback); }

	protected:
		std::vector<std::function<void(const RenderPass*)>> m_callbackWhenExtentChanged;
		Extent2D m_extent;
	};
}
