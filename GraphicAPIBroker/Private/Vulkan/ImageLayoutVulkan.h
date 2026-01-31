#pragma once

#include <vulkan/vulkan_core.h>

#include <Debug.h>

#include "../../Public/ImageLayout.h"

namespace Wolf
{
    inline VkImageLayout wolfImageLayoutToVulkanImageLayout(ImageLayout imageLayout)
    {
        switch (imageLayout)
        {
            case ImageLayout::UNDEFINED:
                return VK_IMAGE_LAYOUT_UNDEFINED;
            case ImageLayout::GENERAL:
                return VK_IMAGE_LAYOUT_GENERAL;
            case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case ImageLayout::TRANSFER_SRC_OPTIMAL:
                return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case ImageLayout::TRANSFER_DST_OPTIMAL:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case ImageLayout::PREINITIALIZED:
                return VK_IMAGE_LAYOUT_PREINITIALIZED;
            case ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            case ImageLayout::DEPTH_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            case ImageLayout::DEPTH_READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            case ImageLayout::STENCIL_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            case ImageLayout::STENCIL_READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
            case ImageLayout::READ_ONLY_OPTIMAL:
                return VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            case ImageLayout::ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            case ImageLayout::PRESENT_SRC_KHR:
                return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            case ImageLayout::FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL:
                return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
            default:
                Wolf::Debug::sendCriticalError("Undefined ImageLayout");
                return VK_IMAGE_LAYOUT_UNDEFINED;
        }

    }
}
