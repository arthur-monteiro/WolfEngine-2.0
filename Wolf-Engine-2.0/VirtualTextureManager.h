#pragma once

#include <map>

#include <ResourceUniqueOwner.h>

#include <Formats.h>
#include <Image.h>
#include <queue>
#include <ReadableBuffer.h>

#include "DynamicResourceUniqueOwnerArray.h"
#include "GPUDataTransfersManager.h"

namespace Wolf
{
	class VirtualTextureManager
	{
	public:
		static constexpr uint32_t VIRTUAL_PAGE_SIZE = 256;
		static constexpr uint32_t BORDER_SIZE = 4; // minimum for block compressed formats
		static constexpr uint32_t PAGE_SIZE_WITH_BORDERS = VIRTUAL_PAGE_SIZE + 2 * BORDER_SIZE;

		explicit VirtualTextureManager(Extent2D extent, const ResourceNonOwner<GPUDataTransfersManagerInterface>& pushDataToGPU);

		using AtlasIndex = uint32_t;
		AtlasIndex createAtlas(uint32_t pageCountX, uint32_t pageCountY, Format format);

		void updateBeforeFrame();
		void resize(Extent2D newExtent);

		struct FeedbackInfo
		{
			uint8_t m_sliceY;
			uint8_t m_sliceX;
			uint16_t m_mipLevel : 5;
			uint16_t m_textureId : 11;

			bool operator==(const FeedbackInfo&) const = default;
			bool operator<(const FeedbackInfo& other) const
			{
				return *reinterpret_cast<const uint32_t*>(this) < *reinterpret_cast<const uint32_t*>(&other);
			}

			bool operator>(const FeedbackInfo& other) const
			{
				return *reinterpret_cast<const uint32_t*>(this) > *reinterpret_cast<const uint32_t*>(&other);
			}

			FeedbackInfo() = default;
			explicit FeedbackInfo(uint32_t value)
			{
				*reinterpret_cast<uint32_t*>(this) = value;
			}
		};
		static_assert(sizeof(FeedbackInfo) == sizeof(uint32_t));

		static constexpr uint32_t INVALID_INDIRECTION_OFFSET = -1;
		uint32_t createNewIndirection(uint32_t indirectionCount);
		uint32_t takeEntryId(AtlasIndex atlasIndex, const FeedbackInfo& feedbackInfo, const Extent3D& sliceExtent, FeedbackInfo& removedFeedback, bool neverRemoveEntry = false);
		void uploadData(AtlasIndex atlasIndex, const std::vector<uint8_t>& data, const Extent3D& sliceExtent, uint8_t sliceX, uint8_t sliceY, uint8_t mipLevel, uint8_t sliceCountX, uint8_t sliceCountY, uint32_t indirectionOffset,
		                const FeedbackInfo& feedbackInfo, uint32_t entryId);
		void rejectRequest(const FeedbackInfo& feedbackInfo);
		void removeIndirection(const FeedbackInfo& feedbackInfo, uint8_t sliceCountX, uint8_t sliceCountY, uint32_t indirectionOffset);

		ResourceNonOwner<Image> getAtlasImage(uint32_t atlasIdx);
		ResourceNonOwner<Buffer> getFeedbackBuffer();
		ResourceNonOwner<Buffer> getIndirectionBuffer();

		void getRequestedSlices(std::vector<FeedbackInfo>& outSlicesRequested, uint32_t maxCount);

	private:
		void createFeedbackBuffer(Extent2D extent);
		void readFeedbackBuffer();
		void updateAtlasesAvailabilities();
		void clearReadbackBuffer();
		void requestReadbackCopyRecord();

		const ResourceNonOwner<GPUDataTransfersManagerInterface>& m_pushDataToGPUHandler;

		class AtlasInfo
		{
		public:
			AtlasInfo(uint32_t pageCountX, uint32_t pageCountY, Format format);

			void updateAvailabilities();
			void updateEntryLRU(uint32_t entryId);

			ResourceNonOwner<Image> getImage() { return m_image.createNonOwnerResource(); }

			static constexpr uint32_t INVALID_ENTRY = static_cast<uint32_t>(-1);
			[[nodiscard]] uint32_t getNextEntry(const FeedbackInfo& newSlice, uint32_t pixelCountPerSide, FeedbackInfo& removedFeedbackEntry, bool neverRemoveEntry = false);
			[[nodiscard]] uint32_t getPageCountX() const { return m_pageCountX; }
			[[nodiscard]] uint32_t getPageCountY() const { return m_pageCountY; }

			uint32_t computeEntryId(uint32_t entryIdx, uint32_t subEntryIdx) const;

		private:
			Format m_format;

			uint32_t m_pageCountX;
			uint32_t m_pageCountY;

			ResourceUniqueOwner<Image> m_image;

			struct SubEntry
			{
				uint32_t m_LRU;
				uint16_t m_sliceOffsetX;
				uint16_t m_sliceOffsetY;
			};
			static_assert(sizeof(SubEntry) == 8);

			struct Entry
			{
				uint32_t m_sliceMipLevels;
				std::vector<SubEntry> m_subEntries;
			};
			std::vector<Entry> m_entries;

			struct Availability
			{
				uint32_t m_entryId;
				uint32_t m_subEntryIndex;
			};
			std::array<std::queue<uint32_t>, 7 /* mip count for 256*256 pixels (worst mip is 4*4) */> m_nextAvailabilities;

			struct AvailabilityCandidate
			{
				Availability m_availability;
				uint32_t m_LRU;
				uint32_t m_sliceMipLevel;
				bool m_isAvailable;
			};
			std::vector<AvailabilityCandidate> m_allAvailabilities;
			std::mutex m_entriesMutex;

			struct SliceInAtlasInfo
			{
				FeedbackInfo m_feedbackInfo;
				Availability m_availability;
			};
			std::map<uint32_t, SliceInAtlasInfo> m_currentSlices;
		};

		struct InfoPerLoadedFeedback
		{
			uint32_t m_atlasIdx;
			uint32_t m_entryId;
		};
		std::map<uint32_t, InfoPerLoadedFeedback> m_loadedFeedbacks;

		static constexpr uint32_t MAX_INDIRECTION_COUNT = 65'536;
		static constexpr uint32_t INVALID_INDIRECTION = -1;
		ResourceUniqueOwner<Buffer> m_indirectionBuffer;
		uint32_t m_currentIndirectionOffset = 0;
		bool m_indirectionBufferInitialized = false;

		DynamicResourceUniqueOwnerArray<AtlasInfo, 4> m_atlases;

		static constexpr uint32_t DITHER_PIXEL_COUNT_PER_SIDE = 24;
		uint32_t m_feedbackCountX = 0;
		uint32_t m_feedbackCountY = 0;
		uint32_t m_maxFeedbackCount = 0;
		uint32_t m_feedbackBufferSize = 0;
		ResourceUniqueOwner<Buffer> m_feedbackBuffer;
		ResourceUniqueOwner<ReadableBuffer> m_feedbackReadableBuffer;
		std::vector<uint32_t> m_deduplicatedFeedbacks;
		std::vector<uint32_t> m_debuplicatedFeedbacksTempBuffer;

		std::vector<FeedbackInfo> m_feedbacksToLoad;
	};
}

template<>
struct std::hash<Wolf::VirtualTextureManager::FeedbackInfo>
{
	size_t operator()(Wolf::VirtualTextureManager::FeedbackInfo const& feedback) const noexcept
	{
		return *reinterpret_cast<const uint32_t*>(&feedback);
	}
};
