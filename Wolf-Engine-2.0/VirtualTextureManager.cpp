#include "VirtualTextureManager.h"

#include <algorithm>
#include <unordered_set>

#include <Buffer.h>
#include <Configuration.h>
#include <RuntimeContext.h>

#include "ProfilerCommon.h"
#include "GPUDataTransfersManager.h"
#include "VirtualTextureUtils.h"

Wolf::VirtualTextureManager::VirtualTextureManager(Extent2D extent, const ResourceNonOwner<GPUDataTransfersManagerInterface>& pushDataToGPU) : m_pushDataToGPUHandler(pushDataToGPU)
{
	createFeedbackBuffer(extent);
	m_indirectionBuffer.reset(Buffer::createBuffer(MAX_INDIRECTION_COUNT * sizeof(uint32_t), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_indirectionBuffer->setName("Virtual texture indirections (VirtualTextureManager::m_indirectionBuffer)");
}

Wolf::VirtualTextureManager::AtlasIndex Wolf::VirtualTextureManager::createAtlas(uint32_t pageCountX, uint32_t pageCountY, Wolf::Format format)
{
	m_atlases.emplace_back(new AtlasInfo(pageCountX, pageCountY, format));
	return static_cast<AtlasIndex>(m_atlases.size()) - 1;
}

void Wolf::VirtualTextureManager::updateBeforeFrame()
{
	PROFILE_FUNCTION

	if (!m_indirectionBufferInitialized)
	{
		m_pushDataToGPUHandler->fillGPUBuffer(INVALID_INDIRECTION, MAX_INDIRECTION_COUNT * sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), 0);
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

	if (m_currentIndirectionOffset > MAX_INDIRECTION_COUNT)
		Debug::sendCriticalError("Too many indirections, this can cause a GPU hang");

	return r;
}
uint32_t Wolf::VirtualTextureManager::takeEntryId(AtlasIndex atlasIndex, const FeedbackInfo& feedbackInfo, const Extent3D& sliceExtent, FeedbackInfo& removedFeedback, bool neverRemoveEntry)
{
	if (sliceExtent.width != sliceExtent.height)
	{
		Debug::sendCriticalError("Only square slices are supported");
	}

	AtlasInfo& atlasInfo = *m_atlases[atlasIndex];

	removedFeedback = static_cast<FeedbackInfo>(-1);
	uint32_t entryId = atlasInfo.getNextEntry(feedbackInfo, sliceExtent.width, removedFeedback, neverRemoveEntry);
	if (entryId == static_cast<uint32_t>(-1))
	{
		rejectRequest(feedbackInfo);
		return AtlasInfo::INVALID_ENTRY;
	}

	if (removedFeedback != static_cast<FeedbackInfo>(-1))
	{
		m_loadedFeedbacks.erase(*reinterpret_cast<const uint32_t*>(&removedFeedback));
	}

	return entryId;
}

void Wolf::VirtualTextureManager::uploadData(AtlasIndex atlasIndex, const std::vector<uint8_t>& data, const Extent3D& sliceExtent, uint8_t sliceX, uint8_t sliceY, uint8_t mipLevel, uint8_t sliceCountX, uint8_t sliceCountY,
                                             uint32_t indirectionOffset, const FeedbackInfo& feedbackInfo, uint32_t entryId)
{
	if (entryId == AtlasInfo::INVALID_ENTRY)
	{
		Debug::sendCriticalError("Invalid entry");
	}

	if (sliceExtent.width > PAGE_SIZE_WITH_BORDERS || sliceExtent.height > PAGE_SIZE_WITH_BORDERS)
		Debug::sendError("Slice extent is too big");

	AtlasInfo& atlasInfo = *m_atlases[atlasIndex];

	uint32_t entryIdx = entryId & 0xFFFF;
	uint32_t subEntryOffsetX = (entryId >> 16) & 0xFF;
	uint32_t subEntryOffsetY = (entryId >> 24) & 0xFF;
	glm::ivec2 atlasOffset = { (entryIdx % atlasInfo.getPageCountY()) * PAGE_SIZE_WITH_BORDERS, (entryIdx / atlasInfo.getPageCountX()) * PAGE_SIZE_WITH_BORDERS };
	atlasOffset.x += subEntryOffsetX;
	atlasOffset.y += subEntryOffsetY;
	GPUDataTransfersManagerInterface::PushDataToGPUImageInfo pushDataToGpuImageInfo(data.data(), m_atlases[atlasIndex]->getImage(), Image::SampledInFragmentShader(), 0, { sliceExtent.width, sliceExtent.height }, atlasOffset);
	m_pushDataToGPUHandler->pushDataToGPUImage(pushDataToGpuImageInfo);

	uint32_t indirectionId = computeVirtualTextureIndirectionId(sliceX, sliceY, sliceCountX, sliceCountY, mipLevel);
	uint32_t indirectionInfo = entryId;
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&indirectionInfo, sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), (indirectionId + indirectionOffset) * sizeof(uint32_t));

	m_loadedFeedbacks[*reinterpret_cast<const uint32_t*>(&feedbackInfo)] = { atlasIndex, entryId };
}

void Wolf::VirtualTextureManager::rejectRequest(const FeedbackInfo& feedbackInfo)
{
	m_loadedFeedbacks[*reinterpret_cast<const uint32_t*>(&feedbackInfo)] = { static_cast<uint32_t>(-1), static_cast<uint32_t>(-1) };
}

void Wolf::VirtualTextureManager::removeIndirection(const FeedbackInfo& feedbackInfo, uint8_t sliceCountX, uint8_t sliceCountY, uint32_t indirectionOffset)
{
	uint32_t indirectionId = computeVirtualTextureIndirectionId(feedbackInfo.m_sliceX, feedbackInfo.m_sliceY, sliceCountX, sliceCountY, feedbackInfo.m_mipLevel);
	uint32_t indirectionInfo = -1;
	m_pushDataToGPUHandler->pushDataToGPUBuffer(&indirectionInfo, sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), (indirectionId + indirectionOffset) * sizeof(uint32_t));
}

void Wolf::VirtualTextureManager::clearReadbackBuffer()
{
	m_pushDataToGPUHandler->fillGPUBuffer(-1, m_feedbackBufferSize, m_feedbackBuffer.createNonOwnerResource(), 0);
}

void Wolf::VirtualTextureManager::requestReadbackCopyRecord()
{
	m_pushDataToGPUHandler->requestGPUBufferReadbackRecord(m_feedbackBuffer.createNonOwnerResource(), 0, m_feedbackReadableBuffer.createNonOwnerResource(), m_feedbackBufferSize);
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

void radixSort(std::vector<uint32_t>& data, std::vector<uint32_t>& temp)
{
	PROFILE_FUNCTION

	for (uint32_t shift = 0; shift < 32; shift += 8)
	{
		uint32_t counts[256] = { 0 };
		for (uint32_t x : data) counts[(x >> shift) & 0xFF]++;

		uint32_t offset = 0;
		for (uint32_t& c : counts)
		{
			uint32_t old = c;
			c = offset;
			offset += old;
		}

		temp.resize(data.size());
		for (uint32_t x : data) temp[counts[(x >> shift) & 0xFF]++] = x;
		data = temp;
	}
}

void Wolf::VirtualTextureManager::readFeedbackBuffer()
{
	PROFILE_FUNCTION

	const uint32_t bufferIdx = g_runtimeContext->getCurrentCPUFrameNumber() % g_configuration->getMaxCachedFrames();

	const uint32_t* feedbackData = static_cast<const uint32_t*>(m_feedbackReadableBuffer->getBuffer(bufferIdx).map());

	FeedbackInfo leftValue(static_cast<uint32_t>(-1));
	std::vector<std::array<FeedbackInfo, 3>> topValues(m_feedbackCountX, { FeedbackInfo(static_cast<uint32_t>(-1)), FeedbackInfo(static_cast<uint32_t>(-1)), FeedbackInfo(static_cast<uint32_t>(-1)) });

	{
		PROFILE_SCOPED("De-duplication")
		m_deduplicatedFeedbacks.clear();

		const uint32_t* __restrict rawData = feedbackData;
		for (uint32_t feedbackIdx = 0; feedbackIdx < m_maxFeedbackCount; ++feedbackIdx)
		{
			for (uint32_t i = 0; i < 3; ++i)
			{
				uint32_t val = rawData[feedbackIdx * 3 + i];
				if (val == -1)
					continue;

				FeedbackInfo feedback(val);
				if (feedback == leftValue || feedback == topValues[feedbackIdx % m_feedbackCountX][i])
				{
					continue;
				}

				m_deduplicatedFeedbacks.push_back(val);

				FeedbackInfo mipAbove = feedback;
				while (mipAbove.m_mipLevel <= feedback.m_mipLevel + 3)
				{
					mipAbove.m_mipLevel++;
					mipAbove.m_sliceX >>= 1;
					mipAbove.m_sliceY >>= 1;
					m_deduplicatedFeedbacks.push_back(*reinterpret_cast<uint32_t*>(&mipAbove));
				}

				leftValue = feedback;
				topValues[feedbackIdx % m_feedbackCountX][i] = feedback;
			}
		}

		if (!m_deduplicatedFeedbacks.empty())
		{
			radixSort(m_deduplicatedFeedbacks, m_debuplicatedFeedbacksTempBuffer);

			PROFILE_SCOPED("Erase duplicates")
			auto it = std::unique(m_deduplicatedFeedbacks.begin(), m_deduplicatedFeedbacks.end());
			m_deduplicatedFeedbacks.erase(it, m_deduplicatedFeedbacks.end());
		}
	}

	for (uint32_t feedback : m_deduplicatedFeedbacks)
	{
		bool alreadyAddedToLoad = std::find(m_feedbacksToLoad.begin(), m_feedbacksToLoad.end(), static_cast<FeedbackInfo>(feedback)) != m_feedbacksToLoad.end();
		if (!alreadyAddedToLoad && !m_loadedFeedbacks.contains(feedback))
		{
			m_feedbacksToLoad.push_back(static_cast<FeedbackInfo>(feedback));
		}
		else if (!alreadyAddedToLoad)
		{
			const InfoPerLoadedFeedback& infoForLoadedFeedback = m_loadedFeedbacks[feedback];
			if (infoForLoadedFeedback.m_atlasIdx != -1)
			{
				m_atlases[infoForLoadedFeedback.m_atlasIdx]->updateEntryLRU(infoForLoadedFeedback.m_entryId);
			}
		}
	}

	std::sort(m_feedbacksToLoad.begin(), m_feedbacksToLoad.end(), [](const FeedbackInfo& a, const FeedbackInfo& b) { return a.m_mipLevel > b.m_mipLevel; });

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
	createImageInfo.aspectFlags = ImageAspectFlagBits::COLOR;
	createImageInfo.format = m_format;
	createImageInfo.mipLevelCount = 1;
	createImageInfo.usage = ImageUsageFlagBits::TRANSFER_DST | ImageUsageFlagBits::SAMPLED;
	m_image.reset(Image::createImage(createImageInfo));
	m_image->setName("Texture atlas (VirtualTextureManager::AtlasInfo::m_image)");

	m_entries.resize(m_pageCountX * m_pageCountY);
	for (Entry& entry : m_entries)
	{
		entry.m_sliceMipLevels = 0;
		entry.m_subEntries.emplace_back(0);
	}

	m_allAvailabilities.reserve(m_pageCountX * m_pageCountY);
	updateAvailabilities();
}

void Wolf::VirtualTextureManager::AtlasInfo::updateAvailabilities()
{
	PROFILE_FUNCTION

	m_entriesMutex.lock();

	uint32_t frameIdx = g_runtimeContext->getCurrentCPUFrameNumber();

	bool needsRefill[7] = { false };
	bool anyRefillNeeded = false;
	for (uint32_t i = 0; i < m_nextAvailabilities.size(); ++i)
	{
		if (m_nextAvailabilities[i].size() < 8 || frameIdx - m_allAvailabilities[m_nextAvailabilities[i].front()].m_LRU < 16)
		{
			needsRefill[i] = true;
			anyRefillNeeded = true;
		}
	}
	// if (!anyRefillNeeded)
	// {
	// 	m_entriesMutex.unlock();
	//   	return;
	// }

	{
		PROFILE_SCOPED("Add availabilities")

		m_allAvailabilities.clear();
		for (uint32_t entryIdx = 0; entryIdx < m_entries.size(); entryIdx++)
		{
			Entry& entry = m_entries[entryIdx];
			for (uint32_t subEntryIdx = 0; subEntryIdx < entry.m_subEntries.size(); subEntryIdx++)
			{
				SubEntry& subEntry = entry.m_subEntries[subEntryIdx];
				if (subEntry.m_LRU < frameIdx) // don't add entries marked to stay forever (or at least until a specific frame idx)
				{
					m_allAvailabilities.push_back({ { computeEntryId(entryIdx, subEntryIdx), subEntryIdx }, subEntry.m_LRU, entry.m_sliceMipLevels, true });
				}
			}
		}
	}
	{
		PROFILE_SCOPED("Sort availabilities")

		std::sort(m_allAvailabilities.begin(), m_allAvailabilities.end(), [](AvailabilityCandidate& a, AvailabilityCandidate& b)
		{
			return a.m_LRU < b.m_LRU;
		});
	}

	{
		PROFILE_SCOPED("Filter availabilities")

		for (uint32_t sliceMip = 0; sliceMip < m_nextAvailabilities.size(); sliceMip++)
		{
			std::queue<uint32_t>& queue = m_nextAvailabilities[sliceMip];

			std::queue<uint32_t> empty;
			std::swap(queue, empty );
		}

		for (uint32_t i = 0; i < m_allAvailabilities.size(); i++)
		{
			m_nextAvailabilities[m_allAvailabilities[i].m_sliceMipLevel].emplace(i);
			if (m_allAvailabilities[i].m_sliceMipLevel == 0)
			{
				for (uint32_t sliceMipLevel = 1; sliceMipLevel < m_nextAvailabilities.size(); ++sliceMipLevel)
				{
					m_nextAvailabilities[sliceMipLevel].emplace(i);
				}
			}
		}
	}

	m_entriesMutex.unlock();
}

void Wolf::VirtualTextureManager::AtlasInfo::updateEntryLRU(uint32_t entryId)
{
	SliceInAtlasInfo sliceInAtlasInfo = m_currentSlices[entryId];

	uint32_t entryIdx = sliceInAtlasInfo.m_availability.m_entryId & 0xFFFF;
	uint32_t subEntryIdx = sliceInAtlasInfo.m_availability.m_subEntryIndex;

	m_entries[entryIdx].m_subEntries[subEntryIdx].m_LRU = std::max(m_entries[entryIdx].m_subEntries[subEntryIdx].m_LRU, g_runtimeContext->getCurrentCPUFrameNumber());
}

uint32_t Wolf::VirtualTextureManager::AtlasInfo::getNextEntry(const FeedbackInfo& newSlice, uint32_t pixelCountPerSide, FeedbackInfo& removedFeedbackEntry, bool neverRemoveEntry)
{
	uint32_t pixelCountPerSideNoBorder = pixelCountPerSide - 2 * BORDER_SIZE;
	if (pixelCountPerSideNoBorder < 4 || (pixelCountPerSideNoBorder & (pixelCountPerSideNoBorder - 1)) != 0)
	{
		Debug::sendCriticalError("Size must be a power of 2");
		return INVALID_ENTRY;
	}

#if defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse(&index, pixelCountPerSideNoBorder);
	uint32_t log2Size = static_cast<uint32_t>(index);
#else
	uint32_t log2Size = 31 - __builtin_clz(pixelCountPerSideNoBorder);
#endif

	// 256 is mip 0 (log2(256) = 8)
	uint32_t sliceMipLevel = 8 - log2Size;

	m_entriesMutex.lock();

	Availability newEntry = { INVALID_ENTRY, 0 };
	while (newEntry.m_entryId == INVALID_ENTRY)
	{
		if (m_nextAvailabilities[sliceMipLevel].empty())
		{
			Debug::sendCriticalError("No availability, we may have too much slices split");
			return INVALID_ENTRY;
		}

		AvailabilityCandidate& availabilityCandidate = m_allAvailabilities[m_nextAvailabilities[sliceMipLevel].front()];
		m_nextAvailabilities[sliceMipLevel].pop();
		if (availabilityCandidate.m_isAvailable)
		{
			newEntry = availabilityCandidate.m_availability;
			availabilityCandidate.m_isAvailable = false;
		}
	}

	uint32_t entryIdx = newEntry.m_entryId & 0xFFFF;
	Entry& entry = m_entries[entryIdx];
	if (sliceMipLevel > entry.m_sliceMipLevels) // TODO: un-split when all slices become too old
	{
		if (entry.m_sliceMipLevels != 0)
		{
			Debug::sendCriticalError("Can't split a slice if not level 0");
		}

		entry.m_sliceMipLevels = sliceMipLevel;

		std::queue<uint32_t>& previousQueue = m_nextAvailabilities[sliceMipLevel];
		std::queue<uint32_t> newQueue;

		uint32_t subSliceCountPerSide = PAGE_SIZE_WITH_BORDERS / pixelCountPerSide;
		entry.m_subEntries.resize(subSliceCountPerSide * subSliceCountPerSide);
		for (uint32_t subSliceIdx = 0; subSliceIdx < entry.m_subEntries.size(); subSliceIdx++)
		{
			entry.m_subEntries[subSliceIdx].m_LRU = 0;
			entry.m_subEntries[subSliceIdx].m_sliceOffsetX = subSliceIdx % subSliceCountPerSide;
			entry.m_subEntries[subSliceIdx].m_sliceOffsetY = subSliceIdx / subSliceCountPerSide;

			uint32_t allAvailabilitiesIdx = m_allAvailabilities.size();
			m_allAvailabilities.push_back({ { computeEntryId(entryIdx, subSliceIdx), subSliceIdx }, 0, sliceMipLevel, true });
			newQueue.emplace(allAvailabilitiesIdx);
		}

		for (uint32_t previousQueueIdx = 0; previousQueueIdx < previousQueue.size(); previousQueueIdx++)
		{
			newQueue.emplace(previousQueue.front());
			previousQueue.pop();
		}
		previousQueue.swap(newQueue);
	}

	m_entries[entryIdx].m_subEntries[newEntry.m_subEntryIndex].m_LRU = neverRemoveEntry ? static_cast<uint32_t>(-1) : g_runtimeContext->getCurrentCPUFrameNumber();

	removedFeedbackEntry = m_currentSlices[newEntry.m_entryId].m_feedbackInfo;
	m_currentSlices[newEntry.m_entryId] = { newSlice, newEntry };

	m_entriesMutex.unlock();

	return newEntry.m_entryId;
}

// 8 bits offsetY - 8 bits offsetX - 16 bits entryIdx
uint32_t Wolf::VirtualTextureManager::AtlasInfo::computeEntryId(uint32_t entryIdx, uint32_t subEntryIdx) const
{
	const Entry& entry = m_entries[entryIdx];
	const SubEntry& subEntry = entry.m_subEntries[subEntryIdx];

	uint32_t subEntryPixelCountPerSide = (VIRTUAL_PAGE_SIZE >> entry.m_sliceMipLevels) + 2 * BORDER_SIZE;
	uint32_t subEntryOffsetX = subEntry.m_sliceOffsetX * subEntryPixelCountPerSide;
	uint32_t subEntryOffsetY = subEntry.m_sliceOffsetY * subEntryPixelCountPerSide;

	return ((subEntryOffsetY & 0xFF) << 24) | ((subEntryOffsetX & 0xFF) << 16) | (entryIdx & 0xFFFF);
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
