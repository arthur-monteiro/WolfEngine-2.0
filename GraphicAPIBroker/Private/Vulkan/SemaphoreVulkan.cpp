#include "SemaphoreVulkan.h"

#include <Debug.h>

#include "Vulkan.h"

Wolf::SemaphoreVulkan::SemaphoreVulkan(VkPipelineStageFlags pipelineStage)
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (vkCreateSemaphore(g_vulkanInstance->getDevice(), &semaphoreInfo, nullptr, &m_semaphore) != VK_SUCCESS)
        Debug::sendCriticalError("Error : create semaphore");

    m_pipelineStage = pipelineStage;
}

Wolf::SemaphoreVulkan::~SemaphoreVulkan()
{
    vkDestroySemaphore(g_vulkanInstance->getDevice(), m_semaphore, nullptr);
}
