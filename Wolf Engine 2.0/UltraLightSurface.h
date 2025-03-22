#pragma once

#include <AppCore/Platform.h>
#include <memory>
#include <Ultralight/Ultralight.h>
#include <Ultralight/platform/Logger.h>

#include <Image.h>
#include <ResourceUniqueOwner.h>

namespace Wolf
{
	class UltraLightSurface : public ultralight::Surface
	{
	public:
		static constexpr uint32_t IMAGE_COUNT = 1;

		void beforeRender() const;
		void moveToNextFrame();
		uint32_t getReadyImageIdx() const { return (m_currentImageIdx + IMAGE_COUNT - 1) % IMAGE_COUNT; }

		[[nodiscard]] uint32_t width() const override { return m_width; }
		[[nodiscard]] uint32_t height() const override { return m_height; }
		[[nodiscard]] uint32_t row_bytes() const override { return m_rowBytes; }
		[[nodiscard]] size_t size() const override { return m_size; }

		void* LockPixels() override;
		void UnlockPixels() override;

		void Resize(uint32_t width, uint32_t height) override;

		const Image& getImage(uint32_t imageIdx) const { return *m_images[imageIdx]; }

	private:
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_rowBytes = 0;
		uint32_t m_size = 0;

		uint32_t m_currentImageIdx = 0;
		std::array<ResourceUniqueOwner<Image>, IMAGE_COUNT> m_images;
		VkSubresourceLayout m_imageResourceLayout{};
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
