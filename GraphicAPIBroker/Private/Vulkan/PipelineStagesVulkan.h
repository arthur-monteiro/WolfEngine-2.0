#pragma once

#include <vulkan/vulkan_core.h>

#include <Debug.h>

#include "../../Public/Enums.h"

namespace Wolf
{
    inline VkPipelineStageFlagBits2 pipelineStageToVkType2(PipelineStage in)
    {
        switch (in)
        {
            case PipelineStage::VERTEX_INPUT:
                return VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
            case PipelineStage::VERTEX_SHADER:
                return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
            case PipelineStage::COMPUTE_SHADER:
                return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
            case PipelineStage::RAY_TRACING_SHADER:
                return VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
            default:
                Debug::sendCriticalError("Unhandled pipeline stage");
        }

        return VK_PIPELINE_STAGE_2_NONE;
    }
}