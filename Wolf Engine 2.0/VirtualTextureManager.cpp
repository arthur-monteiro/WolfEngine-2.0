#include "VirtualTextureManager.h"

#include <Buffer.h>
#include <Configuration.h>
#include <RuntimeContext.h>

#include "PushDataToGPU.h"
#include "ReadbackDataFromGPU.h"
#include "VirtualTextureUtils.h"

Wolf::VirtualTextureManager::VirtualTextureManager()
{
	m_feedbackBuffer.reset(Buffer::createBuffer(FEEDBACK_BUFFER_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
	m_feedbackReadableBuffer.reset(new ReadableBuffer(FEEDBACK_BUFFER_SIZE, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

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

	clearReadbackBuffer(); // will be executed on the beginning of the frame
	requestReadbackCopyRecord(); // will be executed at the end of the frame
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
	uint32_t indirectionOffset)
{
	if (sliceExtent.width > PAGE_SIZE_WITH_BORDERS || sliceExtent.height > PAGE_SIZE_WITH_BORDERS)
		Debug::sendError("Slice extent is too big");

	AtlasInfo& atlasInfo = *m_atlases[atlasIndex];

	uint32_t entryId = atlasInfo.getNextEntry();
	if (entryId == AtlasInfo::INVALID_ENTRY)
		return;

	glm::ivec2 atlasOffset = { (entryId % atlasInfo.getPageCountY()) * PAGE_SIZE_WITH_BORDERS, (entryId / atlasInfo.getPageCountX()) * PAGE_SIZE_WITH_BORDERS };
	pushDataToGPUImage(data.data(), m_atlases[atlasIndex]->getImage(), Image::SampledInFragmentShader(), 0, { sliceExtent.width, sliceExtent.height }, atlasOffset);

	uint32_t indirectionId = computeVirtualTextureIndirectionId(sliceX, sliceY, sliceCountX, sliceCountY, mipLevel);
	uint32_t indirectionInfo = entryId;
	pushDataToGPUBuffer(&indirectionInfo, sizeof(uint32_t), m_indirectionBuffer.createNonOwnerResource(), (indirectionId + indirectionOffset) * sizeof(uint32_t));
}

void Wolf::VirtualTextureManager::clearReadbackBuffer()
{
	fillGPUBuffer(0, FEEDBACK_BUFFER_SIZE, m_feedbackBuffer.createNonOwnerResource(), 0);
}

void Wolf::VirtualTextureManager::requestReadbackCopyRecord()
{
	requestGPUBufferReadbackRecord(m_feedbackBuffer.createNonOwnerResource(), 0, m_feedbackReadableBuffer.createNonOwnerResource(), FEEDBACK_BUFFER_SIZE);
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
	const uint32_t bufferIdx = g_runtimeContext->getCurrentCPUFrameNumber() % g_configuration->getMaxCachedFrames();

	if (bufferIdx != 0) // next frame is likely to be the same as requests has not been processed yet
		return;

	const uint32_t* feedbackData = static_cast<const uint32_t*>(m_feedbackReadableBuffer->getBuffer(bufferIdx).map());
	const uint32_t feedbackCount = feedbackData[0];

	for (uint32_t feedbackIdx = 1; feedbackIdx < std::min(feedbackCount, FEEDBACK_MAX_COUNT); ++feedbackIdx)
	{
		FeedbackInfo feedback = *reinterpret_cast<const FeedbackInfo*>(&feedbackData[feedbackIdx]);
		if (std::find(m_feedbacksToLoad.begin(), m_feedbacksToLoad.end(), feedback) == m_feedbacksToLoad.end())
		{
			m_feedbacksToLoad.push_back(feedback);
		}
	}

	m_feedbackReadableBuffer->getBuffer(bufferIdx).unmap();
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
}

uint32_t Wolf::VirtualTextureManager::AtlasInfo::getNextEntry()
{
	if (m_sortedAvailabilities.empty())
		return INVALID_ENTRY;

	uint32_t newEntry = m_sortedAvailabilities[0];
	m_sortedAvailabilities.erase(m_sortedAvailabilities.begin());

	m_LRUs[newEntry] = g_runtimeContext->getCurrentCPUFrameNumber();

	return newEntry;
}

void Wolf::VirtualTextureManager::getRequestedSlices(std::vector<FeedbackInfo>& outSlicesRequested, uint32_t maxCount)
{
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

		m_feedbacksToLoad.erase(m_feedbacksToLoad.begin(), m_feedbacksToLoad.begin() + maxCount);
	}
}
