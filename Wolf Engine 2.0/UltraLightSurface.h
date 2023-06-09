#pragma once

#include <AppCore/Platform.h>
#include <memory>
#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Logger.h>

#include "Image.h"

namespace Wolf
{
	class UltraLightSurface : public ultralight::Surface
	{
	public:
		[[nodiscard]] uint32_t width() const override { return m_width; }
		[[nodiscard]] uint32_t height() const override { return m_height; }
		[[nodiscard]] uint32_t row_bytes() const override { return m_rowBytes; }
		[[nodiscard]] size_t size() const override { return m_size; }

		void* LockPixels() override;
		void UnlockPixels() override;

		void Resize(uint32_t width, uint32_t height) override;

		Image* getImage() const { return m_image.get(); }

	private:
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_rowBytes;
		uint32_t m_size;

		std::unique_ptr<Image> m_image;
		VkSubresourceLayout m_imageResourceLayout;
	};

	class UltraLightSurfaceFactory : public ultralight::SurfaceFactory
	{
	public:
		ultralight::Surface* CreateSurface(uint32_t width, uint32_t height) override
		{
			ultralight::Surface* surface = new UltraLightSurface();
			surface->Resize(width, height);
			return surface;
		}

		void DestroySurface(ultralight::Surface* surface) override
		{
			delete dynamic_cast<UltraLightSurface*>(surface);
		}
	};
}