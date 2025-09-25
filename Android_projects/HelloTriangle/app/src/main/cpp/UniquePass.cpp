#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <Attachment.h>
#include <Image.h>
#include <FrameBuffer.h>
#include <GPUSemaphore.h>

#include "Vertex2D.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
    const Wolf::Format outputFormat = context.swapChainFormat;

    createDepthImage(context);

    Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, SAMPLE_COUNT_1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, AttachmentStoreOp::DONT_CARE, Wolf::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
                     m_depthImage->getDefaultImageView());
    Attachment color({ context.swapChainWidth, context.swapChainHeight }, outputFormat, SAMPLE_COUNT_1, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, AttachmentStoreOp::STORE, Wolf::ImageUsageFlagBits::COLOR_ATTACHMENT,
                     IMAGE_VIEW_NULL);

    m_renderPass.reset(RenderPass::createRenderPass({ depth, color }));

    m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::GRAPHIC, false /* isTransient */));

    m_frameBuffers.resize(context.swapChainImageCount);
    for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
    {
        color.imageView = context.swapChainImages[i]->getDefaultImageView();
        m_frameBuffers[i].reset(FrameBuffer::createFrameBuffer(*m_renderPass, { depth, color }));
    }

    createSemaphores(context, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, true);

    m_vertexShaderParser.reset(new ShaderParser("shaders/shader.vert"));
    m_fragmentShaderParser.reset(new ShaderParser("shaders/shader.frag"));

    createPipeline(context.swapChainWidth, context.swapChainHeight);

    // Load triangle
    std::vector<Vertex2D> vertices =
            {
                    { glm::vec2(0.0f, -0.75f) }, // top
                    { glm::vec2(-0.75f, 0.75f) }, // bot left
                    { glm::vec2(0.75f, 0.75f) } // bot right
            };

    std::vector<uint32_t> indices =
            {
                    0, 1, 2
            };

    m_vertexBuffer.reset(Buffer::createBuffer(sizeof(Vertex2D) * vertices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    m_vertexBuffer->transferCPUMemoryWithStagingBuffer(vertices.data(), sizeof(Vertex2D) * vertices.size());

    m_indexBuffer.reset(Buffer::createBuffer(sizeof(uint32_t) * indices.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    m_indexBuffer->transferCPUMemoryWithStagingBuffer(indices.data(), sizeof(uint32_t) * indices.size());
}

void UniquePass::resize(const InitializationContext& context)
{
    createDepthImage(context);
    m_renderPass->setExtent({ context.swapChainWidth, context.swapChainHeight });

    m_frameBuffers.clear();
    m_frameBuffers.resize(context.swapChainImageCount);

    Attachment depth({ context.swapChainWidth, context.swapChainHeight }, context.depthFormat, SAMPLE_COUNT_1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, AttachmentStoreOp::DONT_CARE, Wolf::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT,
                     m_depthImage->getDefaultImageView());
    Attachment color({ context.swapChainWidth, context.swapChainHeight }, context.swapChainFormat, SAMPLE_COUNT_1, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, AttachmentStoreOp::STORE, Wolf::ImageUsageFlagBits::COLOR_ATTACHMENT,
                     IMAGE_VIEW_NULL);

    for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
    {
        color.imageView = context.swapChainImages[i]->getDefaultImageView();
        m_frameBuffers[i].reset(FrameBuffer::createFrameBuffer(*m_renderPass, { depth, color }));
    }

    createPipeline(context.swapChainWidth, context.swapChainHeight);
}

void UniquePass::record(const RecordContext& context)
{
    const uint32_t frameBufferIdx = context.swapChainImageIdx;

    m_commandBuffer->beginCommandBuffer();

    std::vector<Wolf::ClearValue> clearValues(2);
    clearValues[0] = {{{1.0f}}};
    clearValues[1] = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    m_commandBuffer->beginRenderPass(*m_renderPass, *m_frameBuffers[frameBufferIdx], clearValues);

    m_commandBuffer->bindPipeline(m_pipeline.get());

    m_commandBuffer->bindVertexBuffer(*m_vertexBuffer);
    m_commandBuffer->bindIndexBuffer(*m_indexBuffer, IndexType::U32);

    m_commandBuffer->drawIndexed(3, 1, 0, 0, 0);

    m_commandBuffer->endRenderPass();

    m_commandBuffer->endCommandBuffer();
}

void UniquePass::submit(const SubmitContext& context)
{
    const std::vector<const Semaphore*> waitSemaphores{ context.swapChainImageAvailableSemaphore };
    const std::vector<const Semaphore*> signalSemaphores{ getSemaphore(context.swapChainImageIndex) };
    m_commandBuffer->submit(waitSemaphores, signalSemaphores, context.frameFence);

    bool anyShaderModified = m_vertexShaderParser->compileIfFileHasBeenModified();
    if (m_fragmentShaderParser->compileIfFileHasBeenModified())
        anyShaderModified = true;

    if (anyShaderModified)
    {
        context.graphicAPIManager->waitIdle();
        createPipeline(m_swapChainWidth, m_swapChainHeight);
    }
}

void UniquePass::createDepthImage(const InitializationContext& context)
{
    CreateImageInfo depthImageCreateInfo;
    depthImageCreateInfo.format = context.depthFormat;
    depthImageCreateInfo.extent.width = context.swapChainWidth;
    depthImageCreateInfo.extent.height = context.swapChainHeight;
    depthImageCreateInfo.extent.depth = 1;
    depthImageCreateInfo.mipLevelCount = 1;
    depthImageCreateInfo.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    depthImageCreateInfo.usage = DEPTH_STENCIL_ATTACHMENT;
    m_depthImage.reset(Image::createImage(depthImageCreateInfo));
}

void UniquePass::createPipeline(uint32_t width, uint32_t height)
{
    RenderingPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.renderPass = m_renderPass.get();

    // Programming stages
    pipelineCreateInfo.shaderCreateInfos.resize(2);
    m_vertexShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[0].shaderCode);
    pipelineCreateInfo.shaderCreateInfos[0].stage = VERTEX;
    m_fragmentShaderParser->readCompiledShader(pipelineCreateInfo.shaderCreateInfos[1].shaderCode);
    pipelineCreateInfo.shaderCreateInfos[1].stage = FRAGMENT;

    // IA
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    Vertex2D::getAttributeDescriptions(attributeDescriptions, 0);
    pipelineCreateInfo.vertexInputAttributeDescriptions = attributeDescriptions;

    std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0] = {};
    Vertex2D::getBindingDescription(bindingDescriptions[0], 0);
    pipelineCreateInfo.vertexInputBindingDescriptions = bindingDescriptions;

    // Viewport
    pipelineCreateInfo.extent = { width, height };

    // Color Blend
    std::vector blendModes = { RenderingPipelineCreateInfo::BLEND_MODE::OPAQUE };
    pipelineCreateInfo.blendModes = blendModes;

    m_pipeline.reset(Pipeline::createRenderingPipeline(pipelineCreateInfo));

    m_swapChainWidth = width;
    m_swapChainHeight = height;
}