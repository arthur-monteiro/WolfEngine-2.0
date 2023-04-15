#pragma once

#include "Vulkan.h"

namespace Wolf
{
    class Semaphore
    {
    public:
        Semaphore(VkPipelineStageFlags pipelineStage);
        ~Semaphore();

        // Getters
        [[nodiscard]] VkSemaphore getSemaphore() const { return m_semaphore; }
        [[nodiscard]] VkPipelineStageFlags getPipelineStage() const { return m_pipelineStage; }

    private:
        VkSemaphore m_semaphore;
        VkPipelineStageFlags m_pipelineStage;
    };
}
