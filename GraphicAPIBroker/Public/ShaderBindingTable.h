#pragma once

#include <memory>

#include "Buffer.h"

namespace Wolf
{
	class Pipeline;

	class ShaderBindingTable
	{
	public:
		ShaderBindingTable(uint32_t shaderCount, const Pipeline& pipeline);

		[[nodiscard]] uint32_t getBaseAlignment() const { return m_baseAlignement; }
		[[nodiscard]] const Buffer& getBuffer() const { return *m_buffer; }

	private:
		uint32_t m_baseAlignement;
		std::unique_ptr<Buffer> m_buffer;
	};
}

