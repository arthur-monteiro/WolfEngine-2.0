#pragma once

#include <AppCore/Platform.h>
#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Logger.h>

#include "Image.h"

namespace Wolf
{
	class UltraLightSurface : public ultralight::Surface
	{
	public:
		virtual uint32_t width() const override { return m_width; }
		virtual uint32_t height() const override { return m_height; }
		virtual uint32_t row_bytes() const override { return m_rowBytes; }
		virtual size_t size() const override { return m_size; }

		virtual void* LockPixels() override;
		virtual void UnlockPixels() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		Image* getImage() { return m_image.get(); }

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
		virtual ultralight::Surface* CreateSurface(uint32_t width, uint32_t height) override
		{
			ultralight::Surface* surface = new UltraLightSurface();
			surface->Resize(width, height);
			return surface;
		}

		virtual void DestroySurface(ultralight::Surface* surface) override
		{
			delete static_cast<UltraLightSurface*>(surface);
		}
	};
}