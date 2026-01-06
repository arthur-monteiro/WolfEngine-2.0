#include "SemaphoreVulkan.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::SemaphoreVulkan::SemaphoreVulkan(VkPipelineStageFlags pipelineStage, Type type)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (type == Type::TIMELINE)
    {
        VkSemaphoreTypeCreateInfo timelineCreateInfo;
        timelineCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineCreateInfo.pNext = nullptr;
        timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineCreateInfo.initialValue = 0;
        semaphoreInfo.pNext = &timelineCreateInfo;
    }

    if (vkCreateSemaphore(g_vulkanInstance->getDevice(), &semaphoreInfo, nullptr, &m_semaphore) != VK_SUCCESS)
        Debug::sendCriticalError("Error : create semaphore");

    m_pipelineStage = pipelineStage;
    m_type = type;
}

Wolf::SemaphoreVulkan::~SemaphoreVulkan()
{
    vkDestroySemaphore(g_vulkanInstance->getDevice(), m_semaphore, nullptr);
}
