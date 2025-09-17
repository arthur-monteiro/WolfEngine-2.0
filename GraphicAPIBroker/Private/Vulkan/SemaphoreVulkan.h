#pragma once

#include "../../Public/GPUSemaphore.h"
#include "Vulkan.h"

namespace Wolf
{
    class SemaphoreVulkan : public Semaphore
    {
    public:
        SemaphoreVulkan(VkPipelineStageFlags pipelineStage);
        ~SemaphoreVulkan() override;
        
        [[nodiscard]] VkSemaphore getSemaphore() const { return m_semaphore; }
        [[nodiscard]] VkPipelineStageFlags getPipelineStage() const { return m_pipelineStage; }

    private:
        VkSemaphore m_semaphore;
        VkPipelineStageFlags m_pipelineStage;
    };
}
