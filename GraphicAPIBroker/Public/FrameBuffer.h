#pragma once

#include <vector>

#include "Attachment.h"

namespace Wolf
{
	class RenderPass;

	class FrameBuffer
	{
	public:
		static FrameBuffer* createFrameBuffer(const RenderPass& renderPass, const std::vector<Attachment>& attachments);
		virtual ~FrameBuffer() = default;
	};
}
