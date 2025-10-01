#include "VirtualTextureManager.h"

#include <algorithm>
#include <unordered_set>

#include <Buffer.h>
#include <Configuration.h>
#include <RuntimeContext.h>

#include "ProfilerCommon.h"
#include "PushDataToGPU.h"
#include "ReadbackDataFromGPU.h"
#include "VirtualTextureUtils.h"

Wolf::VirtualTextureManager::VirtualTextureManager(Extent2D extent)
{
	createFeedbackBuffer(extent);
	m_indirectionBuffer.reset(Buffer::createBuffer(MAX_INDIRECTION_COUNT * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
}

Wolf::VirtualTextureManager::AtlasIndex Wolf::VirtualTextureManager::createAtlas(uint32_t pageCountX, uint32_t pageCountY, Wolf::Format format)
{
	m_atlases.emplace_back(new AtlasInfo(pageCountX, pageCountY, format));
	return static_cast<AtlasIndex>(m_atlases.size()) - 1;
}

void Wolf::VirtualTextureManager::updateBeforeFrame()
{
	if (!m_indirectionBufferInitialized)
	{
		fillGPUBuffer(INVALID_INDIRECTION, MAX_INDIRECTION_COUNT * sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), 0);
		m_indirectionBufferInitialized = true;
	}

	readFeedbackBuffer();
	updateAtlasesAvailabilities();

	clearReadbackBuffer(); // will be executed on the beginning of the frame
	requestReadbackCopyRecord(); // will be executed at the end of the frame
}

void Wolf::VirtualTextureManager::resize(Extent2D newExtent)
{
	createFeedbackBuffer(newExtent);
}

uint32_t Wolf::VirtualTextureManager::createNewIndirection(uint32_t indirectionCount)
{
	const uint32_t r = m_currentIndirectionOffset;
	m_currentIndirectionOffset += indirectionCount;

	if (r > MAX_INDIRECTION_COUNT)
		Debug::sendCriticalError("Too many indirections, this can cause a GPU hang");

	return r;
}

void Wolf::VirtualTextureManager::uploadData(AtlasIndex atlasIndex, const std::vector<uint8_t>& data, const Extent3D& sliceExtent, uint8_t sliceX, uint8_t sliceY, uint8_t mipLevel, uint8_t sliceCountX, uint8_t sliceCountY,
	uint32_t indirectionOffset, const FeedbackInfo& feedbackInfo)
{
	if (sliceExtent.width > PAGE_SIZE_WITH_BORDERS || sliceExtent.height > PAGE_SIZE_WITH_BORDERS)
		Debug::sendError("Slice extent is too big");

	AtlasInfo& atlasInfo = *m_atlases[atlasIndex];

	FeedbackInfo removedFeedbackInfo = static_cast<FeedbackInfo>(-1);
	uint32_t entryId = atlasInfo.getNextEntry(feedbackInfo, removedFeedbackInfo);
	if (entryId == AtlasInfo::INVALID_ENTRY)
	{
		rejectRequest(feedbackInfo);
		return;
	}

	if (removedFeedbackInfo != static_cast<FeedbackInfo>(-1))
	{
		m_loadedFeedbacks.erase(*reinterpret_cast<const uint32_t*>(&removedFeedbackInfo));
	}

	glm::ivec2 atlasOffset = { (entryId % atlasInfo.getPageCountY()) * PAGE_SIZE_WITH_BORDERS, (entryId / atlasInfo.getPageCountX()) * PAGE_SIZE_WITH_BORDERS };
	pushDataToGPUImage(data.data(), m_atlases[atlasIndex]->getImage(), Image::SampledInFragmentShader(), 0, { sliceExtent.width, sliceExtent.height }, atlasOffset);

	uint32_t indirectionId = computeVirtualTextureIndirectionId(sliceX, sliceY, sliceCountX, sliceCountY, mipLevel);
	uint32_t indirectionInfo = entryId;
	pushDataToGPUBuffer(&indirectionInfo, sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), (indirectionId + indirectionOffset) * sizeof(uint32_t));

	m_loadedFeedbacks[*reinterpret_cast<const uint32_t*>(&feedbackInfo)] = { atlasIndex, entryId };
}

void Wolf::VirtualTextureManager::rejectRequest(const FeedbackInfo& feedbackInfo)
{
	m_loadedFeedbacks[*reinterpret_cast<const uint32_t*>(&feedbackInfo)] = { static_cast<uint32_t>(-1), static_cast<uint32_t>(-1) };
}

void Wolf::VirtualTextureManager::clearReadbackBuffer()
{
	fillGPUBuffer(-1, m_feedbackBufferSize, m_feedbackBuffer.createNonOwnerResource(), 0);
}

void Wolf::VirtualTextureManager::requestReadbackCopyRecord()
{
	requestGPUBufferReadbackRecord(m_feedbackBuffer.createNonOwnerResource(), 0, m_feedbackReadableBuffer.createNonOwnerResource(), m_feedbackBufferSize);
}

Wolf::ResourceNonOwner<Wolf::Image> Wolf::VirtualTextureManager::getAtlasImage(uint32_t atlasIdx)
{
	return m_atlases[atlasIdx]->getImage();
}

Wolf::ResourceNonOwner<Wolf::Buffer> Wolf::VirtualTextureManager::getFeedbackBuffer()
{
	return m_feedbackBuffer.createNonOwnerResource();
}

Wolf::ResourceNonOwner<Wolf::Buffer> Wolf::VirtualTextureManager::getIndirectionBuffer()
{
	return m_indirectionBuffer.createNonOwnerResource();
}

void Wolf::VirtualTextureManager::readFeedbackBuffer()
{
	PROFILE_FUNCTION

	const uint32_t bufferIdx = g_runtimeContext->getCurrentCPUFrameNumber() % g_configuration->getMaxCachedFrames();

	const uint32_t* feedbackData = static_cast<const uint32_t*>(m_feedbackReadableBuffer->getBuffer(bufferIdx).map());

	FeedbackInfo leftValue = static_cast<uint32_t>(-1);
	std::vector<std::array<FeedbackInfo, 3>> topValues(m_feedbackCountX, { static_cast<uint32_t>(-1), static_cast<uint32_t>(-1), static_cast<uint32_t>(-1) });
	std::unordered_set<FeedbackInfo> deduplicatedValues;
	{
		PROFILE_SCOPED("De-duplication")
		for (uint32_t feedbackIdx = 0; feedbackIdx < m_maxFeedbackCount; ++feedbackIdx)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				uint32_t feedbackAsUint = feedbackData[feedbackIdx * 3 + i];
				if (feedbackAsUint == -1)
					continue;

				FeedbackInfo feedback = feedbackAsUint;

				if (feedback == leftValue || feedback == topValues[feedbackIdx % m_feedbackCountX][i])
					continue;

				deduplicatedValues.insert(feedback);

				FeedbackInfo mipAboveFeedback = feedback;
				mipAboveFeedback.m_mipLevel++;
				mipAboveFeedback.m_sliceX >>= 1;
				mipAboveFeedback.m_sliceY >>= 1;
				deduplicatedValues.insert(mipAboveFeedback);

				leftValue = feedback;
				topValues[feedbackIdx % m_feedbackCountX][i] = feedback;
			}
		}
	}

	for (FeedbackInfo feedback : deduplicatedValues)
	{
		bool alreadyAddedToLoad = std::find(m_feedbacksToLoad.begin(), m_feedbacksToLoad.end(), feedback) != m_feedbacksToLoad.end();
		if (!alreadyAddedToLoad && !m_loadedFeedbacks.contains(*reinterpret_cast<uint32_t*>(&feedback)))
		{
			m_feedbacksToLoad.push_back(feedback);
		}
		else if (!alreadyAddedToLoad)
		{
			const InfoPerLoadedFeedback& infoForLoadedFeedback = m_loadedFeedbacks[*reinterpret_cast<uint32_t*>(&feedback)];
			if (infoForLoadedFeedback.atlasIdx != -1)
			{
				m_atlases[infoForLoadedFeedback.atlasIdx]->updateEntryLRU(infoForLoadedFeedback.entryIdx);
			}
		}
	}

	m_feedbackReadableBuffer->getBuffer(bufferIdx).unmap();
}

void Wolf::VirtualTextureManager::updateAtlasesAvailabilities()
{
	DYNAMIC_RESOURCE_UNIQUE_OWNER_ARRAY_CONST_RANGE_LOOP(m_atlases, atlas, atlas->updateAvailabilities();)
}

Wolf::VirtualTextureManager::AtlasInfo::AtlasInfo(uint32_t pageCountX, uint32_t pageCountY, Wolf::Format format)
	: m_format(format), m_pageCountX(pageCountX), m_pageCountY(pageCountY)
{
	CreateImageInfo createImageInfo;
	createImageInfo.extent = { m_pageCountX * PAGE_SIZE_WITH_BORDERS, m_pageCountY * PAGE_SIZE_WITH_BORDERS, 1 } ;
	createImageInfo.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	createImageInfo.format = m_format;
	createImageInfo.mipLevelCount = 1;
	createImageInfo.usage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::SAMPLED;
	m_image.reset(Image::createImage(createImageInfo));

	m_LRUs.resize(m_pageCountX * m_pageCountY);
	for (uint32_t& lru : m_LRUs)
	{
		lru = 0;
	}

	m_sortedAvailabilities.resize(m_pageCountX * m_pageCountY);
	for (uint32_t i = 0; i < m_sortedAvailabilities.size(); ++i)
	{
		m_sortedAvailabilities[i] = i;
	}

	m_currentSlices.resize(m_pageCountX * m_pageCountY);
	for (FeedbackInfo& currentSlice : m_currentSlices)
	{
		currentSlice = FeedbackInfo(-1);
	}
}

void Wolf::VirtualTextureManager::AtlasInfo::updateAvailabilities()
{
	std::sort(m_sortedAvailabilities.begin(), m_sortedAvailabilities.end(), [this](uint32_t i1, uint32_t i2) { return m_LRUs[i1] < m_LRUs[i2]; });
	m_nextSortedEntryIdx = 0;
}

void Wolf::VirtualTextureManager::AtlasInfo::updateEntryLRU(uint32_t entryIdx)
{
	m_LRUs[entryIdx] = g_runtimeContext->getCurrentCPUFrameNumber();
}

uint32_t Wolf::VirtualTextureManager::AtlasInfo::getNextEntry(const FeedbackInfo& newSlice, FeedbackInfo& removedSlice)
{
	if (m_sortedAvailabilities.empty())
		return INVALID_ENTRY;

	uint32_t newEntry = m_sortedAvailabilities[m_nextSortedEntryIdx];
	m_nextSortedEntryIdx++;

	m_LRUs[newEntry] = g_runtimeContext->getCurrentCPUFrameNumber();
	removedSlice = m_currentSlices[newEntry];
	m_currentSlices[newEntry] = newSlice;

	return newEntry;
}

void Wolf::VirtualTextureManager::getRequestedSlices(std::vector<FeedbackInfo>& outSlicesRequested, uint32_t maxCount)
{
	if (m_feedbacksToLoad.empty())
		return;

	if (!outSlicesRequested.empty())
		Debug::sendError("Out slices requested must be sent empty");

	if (m_feedbacksToLoad.size() < maxCount)
	{
		m_feedbacksToLoad.swap(outSlicesRequested);
	}
	else
	{
		outSlicesRequested.reserve(maxCount);
		for (uint32_t i = 0; i < maxCount; ++i)
		{
			outSlicesRequested.push_back(m_feedbacksToLoad[i]);
		}

		m_feedbacksToLoad.clear();
	}
}

void Wolf::VirtualTextureManager::createFeedbackBuffer(Extent2D extent)
{
	m_feedbackCountX = (extent.width / DITHER_PIXEL_COUNT_PER_SIDE) + 1;
	m_feedbackCountY = (extent.height / DITHER_PIXEL_COUNT_PER_SIDE) + 1;
	m_maxFeedbackCount = m_feedbackCountX * m_feedbackCountY;
	m_feedbackBufferSize = m_maxFeedbackCount * sizeof(glm::uvec3);

	m_feedbackBuffer.reset(Buffer::createBuffer(m_feedbackBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_feedbackReadableBuffer.reset(new ReadableBuffer(m_feedbackBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
}
