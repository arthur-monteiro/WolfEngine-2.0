#pragma once

#include <ResourceUniqueOwner.h>

#include <Formats.h>
#include <Image.h>
#include <ReadableBuffer.h>

#include "DynamicResourceUniqueOwnerArray.h"

namespace Wolf
{
	class VirtualTextureManager
	{
	public:
		static constexpr uint32_t PAGE_SIZE = 256;
		static constexpr uint32_t BORDER_SIZE = 4; // minimum for block compressed formats
		static constexpr uint32_t PAGE_SIZE_WITH_BORDERS = PAGE_SIZE + 2 * BORDER_SIZE;
		static constexpr uint32_t MAX_FEEDBACK_COUNT = 255;

		VirtualTextureManager();

		using AtlasIndex = uint32_t;
		AtlasIndex createAtlas(uint32_t pageCountX, uint32_t pageCountY, Format format);

		void updateBeforeFrame();

		static constexpr uint32_t INVALID_INDIRECTION_OFFSET = -1;
		uint32_t createNewIndirection(uint32_t indirectionCount);
		void uploadData(AtlasIndex atlasIndex, const std::vector<uint8_t>& data, const Extent3D& sliceExtent, uint8_t sliceX, uint8_t sliceY, uint8_t mipLevel, uint8_t sliceCountX, uint8_t sliceCountY, uint32_t indirectionOffset);

		ResourceNonOwner<Image> getAtlasImage(uint32_t atlasIdx);
		ResourceNonOwner<Buffer> getFeedbackBuffer();
		ResourceNonOwner<Buffer> getIndirectionBuffer();

		struct FeedbackInfo
		{
			uint8_t m_sliceY;
			uint8_t m_sliceX;
			uint16_t m_mipLevel : 5;
			uint16_t m_textureId : 11;

			bool operator==(const FeedbackInfo&) const = default;
		};
		static_assert(sizeof(FeedbackInfo) == sizeof(uint32_t));
		void getRequestedSlices(std::vector<FeedbackInfo>& outSlicesRequested, uint32_t maxCount);

	private:
		void readFeedbackBuffer();
		void clearReadbackBuffer();
		void requestReadbackCopyRecord();

		class AtlasInfo
		{
		public:
			AtlasInfo(uint32_t pageCountX, uint32_t pageCountY, Format format);

			ResourceNonOwner<Image> getImage() { return m_image.createNonOwnerResource(); }

			static constexpr uint32_t INVALID_ENTRY = -1;
			[[nodiscard]] uint32_t getNextEntry();
			[[nodiscard]] uint32_t getPageCountX() const { return m_pageCountX; }
			[[nodiscard]] uint32_t getPageCountY() const { return m_pageCountY; }

		private:
			Format m_format;

			uint32_t m_pageCountX;
			uint32_t m_pageCountY;

			ResourceUniqueOwner<Image> m_image;

			std::vector<uint32_t> m_LRUs;
			std::vector<uint32_t> m_sortedAvailabilities;
		};

		static constexpr uint32_t MAX_INDIRECTION_COUNT = 16384;
		static constexpr uint32_t INVALID_INDIRECTION = -1;
		ResourceUniqueOwner<Buffer> m_indirectionBuffer;
		uint32_t m_currentIndirectionOffset = 0;
		bool m_indirectionBufferInitialized = false;

		DynamicResourceUniqueOwnerArray<AtlasInfo, 4> m_atlases;

		static constexpr uint32_t FEEDBACK_MAX_COUNT = MAX_FEEDBACK_COUNT + 1;
		static constexpr uint64_t FEEDBACK_BUFFER_SIZE = FEEDBACK_MAX_COUNT * sizeof(uint32_t);
		ResourceUniqueOwner<Buffer> m_feedbackBuffer;
		ResourceUniqueOwner<ReadableBuffer> m_feedbackReadableBuffer;

		std::vector<FeedbackInfo> m_feedbacksToLoad;
	};
}
