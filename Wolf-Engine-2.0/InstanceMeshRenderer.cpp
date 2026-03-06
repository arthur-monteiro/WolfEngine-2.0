#include "InstanceMeshRenderer.h"

#include <fstream>
#include <xxh64.hpp>

#include "BoundingSphere.h"
#include "CameraList.h"
#include "CommandRecordBase.h"
#include "DebugMarker.h"
#include "DescriptorSetGenerator.h"
#include "GraphicCameraInterface.h"
#include "LightManager.h"
#include "ProfilerCommon.h"

Wolf::InstanceMeshRenderer::InstanceMeshRenderer(ShaderList& shaderList, const ResourceNonOwner<GPUDataTransfersManagerInterface>& gpuDataTransfersManager) : m_shaderList(&shaderList), m_gpuDataTransfersManager(gpuDataTransfersManager)
{
    m_cullInstancesDescriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::COMPUTE, 0); // cullingInstancesInfo
    m_cullInstancesDescriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::COMPUTE, 1); // meshesInfo
    m_cullInstancesDescriptorSetLayoutGenerator.addStorageBuffers(ShaderStageFlagBits::COMPUTE, 2, MAX_BATCH_COUNT); // outInstancesData
    m_cullInstancesDescriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::COMPUTE, 3); // drawCounts
    m_cullInstancesDescriptorSetLayoutGenerator.addStorageBuffers(ShaderStageFlagBits::COMPUTE, 4, MAX_BATCH_COUNT); // outDrawCommands
    m_cullInstancesDescriptorSetLayoutGenerator.addUniformBuffer(ShaderStageFlagBits::COMPUTE, 5); // uniform buffer
    m_cullInstancesDescriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_cullInstancesDescriptorSetLayoutGenerator.getDescriptorLayouts()));

#ifdef __ANDROID__
    std::ifstream appFile("/proc/self/cmdline");
    std::string processName;
    std::getline(appFile, processName);

    std::string appFolderName = "/data/data/" + processName.substr(0, processName.find('\0'));
#endif

    {
#ifdef __ANDROID__
        std::string cacheFilename = appFolderName + "/" + "instanceMeshRendererCullInstancesComputeShaderCache.comp";
#else
        std::string cacheFilename = "instanceMeshRendererCullInstancesComputeShaderCache.comp";
#endif
        {
            std::ofstream outputFile(cacheFilename);
            outputFile <<
                #include "InstanceRendererCulling.comp"
                ;
            outputFile.close();
        }

        ShaderParser cullInstancesComputeShaderParser(cacheFilename, {}, 1);

        ShaderCreateInfo computeShaderInfo;
        computeShaderInfo.stage = Wolf::ShaderStageFlagBits::COMPUTE;
        cullInstancesComputeShaderParser.readCompiledShader(computeShaderInfo.shaderCode);

        std::vector<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts = { m_cullInstancesDescriptorSetLayout.createConstNonOwnerResource(),
            GraphicCameraInterface::getDescriptorSetLayout().createConstNonOwnerResource() };
        m_cullInstancesPipeline.reset(Pipeline::createComputePipeline(computeShaderInfo, descriptorSetLayouts));

        std::remove(cacheFilename.c_str());
        std::remove("instanceMeshRendererCullInstancesComputeShaderCacheParsed.comp");
        std::remove("instanceMeshRendererCullInstancesComputeShaderCacheComp.spv");
    }

    m_copyInstancesDescriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::COMPUTE, 0); // copyInstanceIndices
    m_copyInstancesDescriptorSetLayoutGenerator.addUniformBuffer(ShaderStageFlagBits::COMPUTE, 1); // ubCopy
    m_copyInstancesDescriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_copyInstancesDescriptorSetLayoutGenerator.getDescriptorLayouts()));

    {
#ifdef __ANDROID__
        std::string cacheFilename = appFolderName + "/" + "instanceMeshRendererCopyInstancesComputeShaderCache.comp";
#else
        std::string cacheFilename = "instanceMeshRendererCopyInstancesComputeShaderCache.comp";
#endif

        {
            std::ofstream outputFile(cacheFilename);
            outputFile <<
                #include "InstanceRendererCopy.comp"
                ;
            outputFile.close();
        }

        ShaderParser copyInstancesComputeShaderParser(cacheFilename, {}, 1);

        ShaderCreateInfo computeShaderInfo;
        computeShaderInfo.stage = Wolf::ShaderStageFlagBits::COMPUTE;
        copyInstancesComputeShaderParser.readCompiledShader(computeShaderInfo.shaderCode);

        std::vector<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts = { m_cullInstancesDescriptorSetLayout.createConstNonOwnerResource(),
            GraphicCameraInterface::getDescriptorSetLayout().createConstNonOwnerResource(), m_copyInstancesDescriptorSetLayout.createConstNonOwnerResource() };
        m_copyInstancesPipeline.reset(Pipeline::createComputePipeline(computeShaderInfo, descriptorSetLayouts));

        std::remove(cacheFilename.c_str());
        std::remove("instanceMeshRendererCopyInstancesComputeShaderCacheParsed.comp");
        std::remove("instanceMeshRendererCopyInstancesComputeShaderCacheComp.spv");
    }



    m_cullingInstancesBuffer.reset(Buffer::createBuffer(MAX_INSTANCE_COUNT * sizeof(CullingInstanceInfo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    m_meshesInfoBuffer.reset(Buffer::createBuffer(MAX_MESH_COUNT * sizeof(MeshInfo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
    m_cullingUniformsBuffer.reset(new UniformBuffer(sizeof(CullingUniformData)));

    m_copyInstancesUniformBuffer.reset(new UniformBuffer(sizeof(uint32_t)));

    m_overrideCullingInstancesBuffer.reset(Buffer::createBuffer(MAX_INSTANCE_COUNT * sizeof(CullingInstanceInfo), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    {
        DescriptorSetGenerator descriptorSetGenerator(m_copyInstancesDescriptorSetLayoutGenerator.getDescriptorLayouts());
        descriptorSetGenerator.setBuffer(0, *m_overrideCullingInstancesBuffer);
        descriptorSetGenerator.setUniformBuffer(1, *m_copyInstancesUniformBuffer);

        m_copyDescriptorSet.reset(DescriptorSet::createDescriptorSet(*m_copyInstancesDescriptorSetLayout));
        m_copyDescriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
    }

    // TODO: this is overkill because we won't use all the possible cameras
    for (uint32_t cullingCameraIdx = 0; cullingCameraIdx < MAX_CAMERA_COUNT; cullingCameraIdx++)
    {
        PerCullingCamera& perCullingCamera = m_cullingCamerasData[cullingCameraIdx];

        perCullingCamera.m_drawCommandsCountsBuffer.reset(Buffer::createBuffer(MAX_BATCH_COUNT * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        {
            DescriptorSetGenerator descriptorSetGenerator(m_cullInstancesDescriptorSetLayoutGenerator.getDescriptorLayouts());
            descriptorSetGenerator.setBuffer(0, *m_cullingInstancesBuffer);
            descriptorSetGenerator.setBuffer(1, *m_meshesInfoBuffer);

            std::vector<ResourceNonOwner<Buffer>> instanceDataBuffers;
            for (uint32_t i = 0; i < MAX_BATCH_COUNT; i++) // TODO: not all cameras will use all batches, this should be initialized lazily
            {
                perCullingCamera.m_instancesDataBuffers[i].reset(Buffer::createBuffer(MAX_INSTANCE_COUNT * sizeof(InstanceDataLayout), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
                instanceDataBuffers.push_back(perCullingCamera.m_instancesDataBuffers[i].createNonOwnerResource());
            }
            descriptorSetGenerator.setBuffers(2, instanceDataBuffers);

            descriptorSetGenerator.setBuffer(3, *perCullingCamera.m_drawCommandsCountsBuffer);

            std::vector<ResourceNonOwner<Buffer>> drawCommandsBuffers;
            for (uint32_t i = 0; i < MAX_BATCH_COUNT; i++) // TODO: not all cameras will use all batches, this should be initialized lazily
            {
                perCullingCamera.m_drawCommandsBuffers[i].reset(Buffer::createBuffer(MAX_INSTANCE_COUNT * CommandBuffer::getDrawIndexedIndirectCommandStructureSize(),
                    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
                drawCommandsBuffers.push_back(perCullingCamera.m_drawCommandsBuffers[i].createNonOwnerResource());
            }
            descriptorSetGenerator.setBuffers(4, drawCommandsBuffers);
            descriptorSetGenerator.setUniformBuffer(5, *m_cullingUniformsBuffer);

            perCullingCamera.m_cullingDescriptorSet.reset(DescriptorSet::createDescriptorSet(*m_cullInstancesDescriptorSetLayout));
            perCullingCamera.m_cullingDescriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
        }

        {
            DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;
            descriptorSetLayoutGenerator.addStorageBuffer(ShaderStageFlagBits::VERTEX, 0);
            m_instancesDataDescriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(descriptorSetLayoutGenerator.getDescriptorLayouts()));

            for (uint32_t batchIdx = 0; batchIdx < MAX_BATCH_COUNT; batchIdx++)
            {
                DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
                descriptorSetGenerator.setBuffer(0, *perCullingCamera.m_instancesDataBuffers[batchIdx]);
                perCullingCamera.m_instancesDataDescriptorSets[batchIdx].reset(DescriptorSet::createDescriptorSet(*m_instancesDataDescriptorSetLayout));
                perCullingCamera.m_instancesDataDescriptorSets[batchIdx]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
            }
        }
    }
}

void Wolf::InstanceMeshRenderer::moveToNextFrame()
{
    std::vector<MeshInfo> meshesToAdd;
    std::vector<CullingInstanceInfo> instancesToAdd;
    std::vector<uint32_t> instancesToRemove;

    m_mutex.lock();
    m_meshesToAdd.swap(meshesToAdd);
    uint32_t meshCountBeforeAdd = m_currentMeshCount;
    m_currentMeshCount += meshesToAdd.size();

    m_instancesToAdd.swap(instancesToAdd);
    uint32_t instanceCountBeforeAdd = m_currentInstanceCount;
    m_currentInstanceCount += instancesToAdd.size();

    m_instancesToRemove.swap(instancesToRemove);

    m_activeCamerasThisFrame.clear();
    m_activeCamerasThisFrame.swap(m_activeCamerasNextFrame);
    m_mutex.unlock();

    if (!meshesToAdd.empty())
    {
        m_gpuDataTransfersManager->pushDataToGPUBuffer(meshesToAdd.data(), meshesToAdd.size() * sizeof(meshesToAdd[0]), m_meshesInfoBuffer.createNonOwnerResource(),
        meshCountBeforeAdd * sizeof(meshesToAdd[0]));
    }
    if (!instancesToAdd.empty())
    {
        m_gpuDataTransfersManager->pushDataToGPUBuffer(instancesToAdd.data(), instancesToAdd.size() * sizeof(instancesToAdd[0]), m_cullingInstancesBuffer.createNonOwnerResource(),
           instanceCountBeforeAdd * sizeof(instancesToAdd[0]));
    }

    // TODO: make the removed instances usable
    for (uint32_t instanceIdxToRemove : instancesToRemove)
    {
        float overrideData = 0.0f;
        uint32_t bufferOffset = instanceIdxToRemove * sizeof(CullingInstanceInfo) + offsetof(CullingInstanceInfo, m_transform);
        m_gpuDataTransfersManager->pushDataToGPUBuffer(&overrideData, sizeof(overrideData), m_cullingInstancesBuffer.createNonOwnerResource(), bufferOffset);
    }

    if (!m_overrideCullingInstancesCount)
    {
        CullingUniformData cullingUniformData{};
        cullingUniformData.m_instanceCount = m_currentInstanceCount;
        m_cullingUniformsBuffer->transferCPUMemory(&cullingUniformData, sizeof(CullingUniformData));
    }
    else
    {
        uint32_t overrideCount = m_overrideCullingInstancesCount;
        m_copyInstancesUniformBuffer->transferCPUMemory(&overrideCount, sizeof(overrideCount));
    }
}

void Wolf::InstanceMeshRenderer::clear()
{
    m_mutex.lock();

    m_currentMeshCount = 0;

    m_currentInstanceCount = 0;
    CullingUniformData cullingUniformData{};
    cullingUniformData.m_instanceCount = m_currentInstanceCount;
    m_cullingUniformsBuffer->transferCPUMemory(&cullingUniformData, sizeof(CullingUniformData));

    m_meshesCacheData.clear();
    m_batchesData.clear();

    m_mutex.unlock();
}

void Wolf::InstanceMeshRenderer::initializeResources(const InitializationContext& context)
{
    m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::COMPUTE, false));
    createSemaphores(context, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, false);
}

void Wolf::InstanceMeshRenderer::resize(const InitializationContext& context)
{
    // Nothing to do
}

void Wolf::InstanceMeshRenderer::record(const RecordContext& context)
{
    m_commandBuffer->beginCommandBuffer();

    DebugMarker::beginRegion(m_commandBuffer.get(), DebugMarker::renderPassDebugColor, "Instance mesh renderer culling pass");

    if (!m_overrideCullingInstancesCount)
    {
        m_commandBuffer->bindPipeline(m_cullInstancesPipeline.createConstNonOwnerResource());
    }
    else
    {
        m_commandBuffer->bindPipeline(m_copyInstancesPipeline.createConstNonOwnerResource());
    }

    for (const ActiveCamera& activeCamera : m_activeCamerasThisFrame)
    {
        if (!m_overrideCullingInstancesCount)
        {
            DebugMarker::beginRegion(m_commandBuffer.get(), DebugMarker::commandRegionDebugColor, "Culling for camera " + std::to_string(activeCamera.m_cameraIdx));

            for (uint32_t batchIdx = 0; batchIdx < MAX_BATCH_COUNT; ++batchIdx)
            {
                if (activeCamera.m_batchesMask & (1 << batchIdx))
                {
                    m_commandBuffer->fillBuffer(*m_cullingCamerasData[activeCamera.m_cameraIdx].m_drawCommandsCountsBuffer, batchIdx * sizeof(uint32_t), sizeof(uint32_t), 0);
                }
            }

            m_commandBuffer->bindDescriptorSet(m_cullingCamerasData[activeCamera.m_cameraIdx].m_cullingDescriptorSet.createConstNonOwnerResource(), 0, *m_cullInstancesPipeline);
            m_commandBuffer->bindDescriptorSet(context.m_cameraList->getCamera(activeCamera.m_cameraIdx)->getDescriptorSet(), 1, *m_cullInstancesPipeline);

            constexpr Extent3D dispatchGroups = { 256, 1, 1 };
            const uint32_t groupSizeX = m_currentInstanceCount % dispatchGroups.width != 0 ? m_currentInstanceCount / dispatchGroups.width + 1 : m_currentInstanceCount / dispatchGroups.width;
            m_commandBuffer->dispatch(groupSizeX, 1, 1);

            DebugMarker::endRegion(m_commandBuffer.get());
        }
        else
        {
            DebugMarker::beginRegion(m_commandBuffer.get(), DebugMarker::commandRegionDebugColor, "Copy instances for camera " + std::to_string(activeCamera.m_cameraIdx));

            m_commandBuffer->fillBuffer(*m_cullingCamerasData[activeCamera.m_cameraIdx].m_drawCommandsCountsBuffer, 0, MAX_BATCH_COUNT * sizeof(uint32_t), 0);

            m_commandBuffer->bindDescriptorSet(m_cullingCamerasData[activeCamera.m_cameraIdx].m_cullingDescriptorSet.createConstNonOwnerResource(), 0, *m_copyInstancesPipeline);
            m_commandBuffer->bindDescriptorSet(context.m_cameraList->getCamera(activeCamera.m_cameraIdx)->getDescriptorSet(), 1, *m_copyInstancesPipeline);
            m_commandBuffer->bindDescriptorSet(m_copyDescriptorSet.createConstNonOwnerResource(), 2, *m_copyInstancesPipeline);

            constexpr Extent3D dispatchGroups = { 256, 1, 1 };
            const uint32_t groupSizeX = m_overrideCullingInstancesCount % dispatchGroups.width != 0 ? m_overrideCullingInstancesCount / dispatchGroups.width + 1 : m_overrideCullingInstancesCount / dispatchGroups.width;
            m_commandBuffer->dispatch(groupSizeX, 1, 1);

            DebugMarker::endRegion(m_commandBuffer.get());
        }
    }

    DebugMarker::endRegion(m_commandBuffer.get());

    m_commandBuffer->endCommandBuffer();
}

void Wolf::InstanceMeshRenderer::submit(const SubmitContext& context)
{
    std::vector<const Semaphore*> waitSemaphores;
    const std::vector<const Semaphore*> signalSemaphores{ getSemaphore(context.swapChainImageIndex) };
    m_commandBuffer->submit(waitSemaphores, signalSemaphores, nullptr);
}

uint32_t Wolf::InstanceMeshRenderer::registerMesh(const MeshToRender& mesh)
{
    m_mutex.lock();

    uint32_t meshIdx = m_currentMeshCount + m_meshesToAdd.size();
    if (meshIdx >= MAX_MESH_COUNT)
    {
        Debug::sendCriticalError("Mesh count limit reached");
    }

    MeshInfo& meshInfo = m_meshesToAdd.emplace_back();
    meshInfo.m_indexCount = mesh.m_mesh->getIndexCount();
    meshInfo.m_vertexOffset = mesh.m_mesh->getVertexBufferOffset() / std::max(mesh.m_mesh->getVertexSize(), 1u);
    meshInfo.m_indexOffset = mesh.m_mesh->getIndexBufferOffset() / std::max(mesh.m_mesh->getIndexSize(), 1u);
    meshInfo.m_boundingSphere = glm::vec4(mesh.m_mesh->getBoundingSphere().getCenter(), mesh.m_mesh->getBoundingSphere().getRadius());

    MeshCacheData& meshCacheData = m_meshesCacheData.emplace_back();
    meshCacheData.m_bufferSetHash = computeHash(mesh.m_mesh);
    meshCacheData.m_vertexBuffer = mesh.m_mesh->getVertexBuffer();
    meshCacheData.m_indexBuffer = mesh.m_mesh->getIndexBuffer();
    if (meshIdx != m_meshesCacheData.size() - 1)
    {
        Debug::sendCriticalError("Missmatch bewteen GPU scene description and CPU cache data");
    }

    m_mutex.unlock();
    return meshIdx;
}

uint32_t Wolf::InstanceMeshRenderer::addInstance(uint32_t meshIdx, const glm::mat4& transform, uint32_t materialIdx, uint32_t customData, const ResourceNonOwner<const PipelineSet>& pipelineSet,
    const std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT>& perPipelineDescriptorSets)
{
    m_mutex.lock();

    uint32_t instanceIdx = m_currentInstanceCount + m_instancesToAdd.size();
    if (instanceIdx >= MAX_INSTANCE_COUNT)
    {
        Debug::sendCriticalError("Instance count limit reached");
    }

    CullingInstanceInfo& cullingInstanceInfo = m_instancesToAdd.emplace_back();
    cullingInstanceInfo.m_meshIdx = meshIdx;
    cullingInstanceInfo.m_transform = transform;
    cullingInstanceInfo.m_materialIdx = materialIdx;
    cullingInstanceInfo.m_customData = customData;
    cullingInstanceInfo.m_batchesMask = createMissingBatchesAndComputeBatchMask(pipelineSet, m_meshesCacheData[meshIdx], perPipelineDescriptorSets);

    m_mutex.unlock();

    return instanceIdx;
}

void Wolf::InstanceMeshRenderer::removeInstance(uint32_t instanceIdx)
{
    m_mutex.lock();

    m_instancesToRemove.push_back(instanceIdx);

    m_mutex.unlock();
}

void Wolf::InstanceMeshRenderer::overrideCullingInstances(const std::vector<OverrideInstance>& instances)
{
    if (m_overrideCullingInstancesCount)
    {
        Debug::sendCriticalError("Override culling instances is already enabled");
    }

    m_overrideCullingInstancesCount = instances.size();

    m_gpuDataTransfersManager->pushDataToGPUBuffer(instances.data(), instances.size() * sizeof(instances[0]), m_overrideCullingInstancesBuffer.createNonOwnerResource(), 0);
}

void Wolf::InstanceMeshRenderer::stopOverridingCullingInstances()
{
    if (!m_overrideCullingInstancesCount)
    {
        Debug::sendCriticalError("Override culling instances hasn't been enabled");
    }
    m_overrideCullingInstancesCount = 0;
}

void Wolf::InstanceMeshRenderer::activateCameraForThisFrame(uint32_t cameraIdx, uint32_t pipelineIdx)
{
    m_mutex.lock();

    uint32_t batchesMask = 0;
    for (uint32_t i = 0; i < m_batchesData.size(); i++)
    {
        if (m_batchesData[i]->getPipelineIndex() == pipelineIdx)
        {
            batchesMask |= (1u << i);
            m_batchesData[i]->activateCamera(cameraIdx);
        }
    }

    bool cameraFound = false;
    for (ActiveCamera& activeCamera : m_activeCamerasNextFrame)
    {
        if (activeCamera.m_cameraIdx == cameraIdx)
        {
            activeCamera.m_batchesMask |= batchesMask;
            cameraFound = true;
            break;
        }
    }

    if (!cameraFound)
    {
        m_activeCamerasNextFrame.emplace_back(cameraIdx, batchesMask);
    }

    m_mutex.unlock();
}

void Wolf::InstanceMeshRenderer::draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& additionalDescriptorSetsToBind,
                                      const std::vector<PipelineSet::ShaderCodeToAddForStage>& shadersCodeToAdd) const
{
    PROFILE_FUNCTION

    for (uint32_t batchIdx = 0; batchIdx < m_batchesData.size(); batchIdx++)
    {
        const PerBatchData& batchData = *m_batchesData[batchIdx];

        if (batchData.getPipelineIndex() == pipelineIdx && batchData.hasCamera(cameraIdx))
        {
            /* Get max descriptor slot to bind instance data */
            uint32_t drawInstancesDescriptorSetSlot = batchData.getMaxDescriptorSetBindingSlot() + 1;
            for (const AdditionalDescriptorSet& additionalDescriptorSet : additionalDescriptorSetsToBind)
            {
                if ((additionalDescriptorSet.m_mask == 0 || additionalDescriptorSet.m_mask & batchData.getCustomMask()) && drawInstancesDescriptorSetSlot <= additionalDescriptorSet.m_descriptorSetBindInfo.getBindingSlot())
                {
                    drawInstancesDescriptorSetSlot = additionalDescriptorSet.m_descriptorSetBindInfo.getBindingSlot() + 1;
                }
            }

            /* Compute shader code to add */
            std::vector<PipelineSet::ShaderCodeToAddForStage> realShadersCodeToAdd;
            for (const PipelineSet::ShaderCodeToAddForStage& requestedShaderCode : shadersCodeToAdd)
            {
                if (requestedShaderCode.requiredMask == 0 || requestedShaderCode.requiredMask & batchData.getCustomMask())
                {
                    realShadersCodeToAdd.push_back(requestedShaderCode);
                }
            }

            PipelineSet::ShaderCodeToAddForStage getInstanceInfoShaderCode{};
            getInstanceInfoShaderCode.stage = ShaderStageFlagBits::VERTEX;

            std::string instanceRendererShaderCode =
                #include "InstanceRendererHelper.glsl"
            ;
            const std::string& descriptorSlotToken = "@DESCRIPTOR_SLOT";
            if (const size_t descriptorSlotTokenPos = instanceRendererShaderCode.find(descriptorSlotToken); descriptorSlotTokenPos != std::string::npos)
            {
                instanceRendererShaderCode.replace(descriptorSlotTokenPos, descriptorSlotToken.length(), std::to_string(drawInstancesDescriptorSetSlot));
            }
            getInstanceInfoShaderCode.shaderCodeToAdd.codeString = std::move(instanceRendererShaderCode);
            realShadersCodeToAdd.push_back(getInstanceInfoShaderCode);

            /* Compute descriptor sets to bind */
            std::vector<DescriptorSetBindInfo> additionalDescriptorSetsBindInfo;
            additionalDescriptorSetsBindInfo.reserve(additionalDescriptorSetsToBind.size());

            for (const AdditionalDescriptorSet& additionalDescriptorSet : additionalDescriptorSetsToBind)
            {
                if (additionalDescriptorSet.m_mask == 0 || additionalDescriptorSet.m_mask & batchData.getCustomMask())
                {
                    additionalDescriptorSetsBindInfo.push_back(additionalDescriptorSet.m_descriptorSetBindInfo);
                }
            }
            DescriptorSetBindInfo instanceDataDescriptorSet(m_cullingCamerasData[cameraIdx].m_instancesDataDescriptorSets[batchIdx].createConstNonOwnerResource(), m_instancesDataDescriptorSetLayout.createConstNonOwnerResource(), drawInstancesDescriptorSetSlot);
            additionalDescriptorSetsBindInfo.push_back(instanceDataDescriptorSet);

            const Pipeline* pipeline = batchData.getOrCreatePipeline(renderPass, additionalDescriptorSetsBindInfo, realShadersCodeToAdd, *m_shaderList);
            commandBuffer.bindPipeline(pipeline);

            for (const DescriptorSetBindInfo& descriptorSetToBind : additionalDescriptorSetsBindInfo)
            {
                commandBuffer.bindDescriptorSet(descriptorSetToBind.getDescriptorSet(), descriptorSetToBind.getBindingSlot(), *pipeline);
            }

            if (cameraIdx != NO_CAMERA_IDX)
            {
                const uint32_t cameraDescriptorSlot = batchData.getPipelineSet()->getCameraDescriptorSlot(pipelineIdx);
                if (cameraDescriptorSlot == static_cast<uint32_t>(-1))
                    Debug::sendError("Trying to bind camera descriptor set but slot hasn't been defined");

                commandBuffer.bindDescriptorSet(context.m_cameraList->getCamera(cameraIdx)->getDescriptorSet(), cameraDescriptorSlot, *pipeline);
            }

            const uint32_t materialsDescriptorSlot = batchData.getPipelineSet()->getMaterialsDescriptorSlot(pipelineIdx);
            if (materialsDescriptorSlot != static_cast<uint32_t>(-1))
            {
                commandBuffer.bindDescriptorSet(context.m_materialGPUManagerDescriptorSet, materialsDescriptorSlot, *pipeline);
            }

            const uint32_t lightDescriptorSlot = batchData.getPipelineSet()->getLightDescriptorSlot(pipelineIdx);
            if (lightDescriptorSlot != static_cast<uint32_t>(-1))
            {
                commandBuffer.bindDescriptorSet(context.m_lightManager->getDescriptorSet().createConstNonOwnerResource(), lightDescriptorSlot, *pipeline);
            }

            for (const DescriptorSetBindInfo& descriptorSetBindInfo : batchData.getDescriptorSetsToBindForDraw())
            {
                commandBuffer.bindDescriptorSet(descriptorSetBindInfo.getDescriptorSet(), descriptorSetBindInfo.getBindingSlot(), *pipeline);
            }

            if (const NullableResourceNonOwner<Buffer>& vertexBuffer = batchData.getVertexBuffer())
            {
                commandBuffer.bindVertexBuffer(*vertexBuffer, 0, 0);
            }

            if (const NullableResourceNonOwner<Buffer>& indexBuffer = batchData.getIndexBuffer())
            {
                commandBuffer.bindIndexBuffer(*indexBuffer, 0, IndexType::U32);
                commandBuffer.drawIndexedIndirectCount(*m_cullingCamerasData[cameraIdx].m_drawCommandsBuffers[batchIdx], 0, *m_cullingCamerasData[cameraIdx].m_drawCommandsCountsBuffer,
                    batchIdx * sizeof(uint32_t), MAX_INSTANCE_COUNT);
            }
            else
            {
                commandBuffer.drawIndirectCount(*m_cullingCamerasData[cameraIdx].m_drawCommandsBuffers[batchIdx], 0, *m_cullingCamerasData[cameraIdx].m_drawCommandsCountsBuffer,
                    batchIdx * sizeof(uint32_t), MAX_INSTANCE_COUNT);
            }
        }
    }
}

uint64_t Wolf::InstanceMeshRenderer::computeHash(const ResourceNonOwner<MeshInterface>& meshInterface)
{
    std::array<const void*, 2> buffers = { meshInterface->getVertexBuffer() ? &*meshInterface->getVertexBuffer() : nullptr, meshInterface->getIndexBuffer() ? &*meshInterface->getIndexBuffer() : nullptr };
    return xxh64::hash(reinterpret_cast<const char*>(buffers.data()), buffers.size() * sizeof(buffers[0]), 0);
}

uint32_t Wolf::InstanceMeshRenderer::createMissingBatchesAndComputeBatchMask(const ResourceNonOwner<const PipelineSet>& pipelineSet, const MeshCacheData& meshCacheData,
    const std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT>& perPipelineDescriptorSets)
{
    uint32_t batchMask = 0;

    uint32_t pipelineCount = pipelineSet->getPipelineCount();
    for (uint32_t pipelineIdx = 0; pipelineIdx < pipelineCount; ++pipelineIdx)
    {
        uint64_t pipelineHash = pipelineSet->getPipelineHash(pipelineIdx);
        if (pipelineHash == 0)
        {
            continue;
        }

        uint32_t batchIdx = -1;
        for (uint32_t i = 0; i < m_batchesData.size(); i++)
        {
            if (m_batchesData[i]->isSame(meshCacheData.m_bufferSetHash, pipelineHash, pipelineIdx, perPipelineDescriptorSets[pipelineIdx], pipelineSet->getCustomMask(pipelineIdx)))
            {
                batchIdx = i;
                break;
            }
        }

        if (batchIdx == -1)
        {
            batchIdx = m_batchesData.size();
            m_batchesData.emplace_back(new PerBatchData(meshCacheData, pipelineIdx, perPipelineDescriptorSets[pipelineIdx], pipelineSet, 16'184, 1024, m_cullInstancesDescriptorSetLayoutGenerator,
                m_cullInstancesDescriptorSetLayout.createNonOwnerResource()));
        }

        batchMask |= (1u << batchIdx);
    }
    return batchMask;
}

Wolf::InstanceMeshRenderer::PerBatchData::PerBatchData(const MeshCacheData& meshCacheData, uint32_t pipelineIdx, const std::vector<DescriptorSetBindInfo>& descriptorSetsToBindForDraw,
    const ResourceNonOwner<const PipelineSet>& pipelineSet,
    uint32_t maxInstanceCount, uint32_t maxMeshCount, const DescriptorSetLayoutGenerator& cullInstancesDescriptorSetLayoutGenerator, const ResourceNonOwner<DescriptorSetLayout>& cullingDescriptorSetLayout)
: m_maxInstanceCount(maxInstanceCount), m_pipelineIdx(pipelineIdx), m_cullingDescriptorSetLayout(cullingDescriptorSetLayout), m_renderingPipelineSet(pipelineSet)
{
    m_bufferSetHash = meshCacheData.m_bufferSetHash;
    m_pipelineHash = pipelineSet->getPipelineHash(pipelineIdx);
    m_customMask = pipelineSet->getCustomMask(pipelineIdx);
    m_descriptorSetsToBindForDraw = descriptorSetsToBindForDraw;
    m_cameraDescriptorSlot = pipelineSet->getCameraDescriptorSlot(m_pipelineIdx);
    m_materialsDescriptorSlot = pipelineSet->getMaterialsDescriptorSlot(m_pipelineIdx);
    m_lightDescriptorSlot = pipelineSet->getLightDescriptorSlot(m_pipelineIdx);
    m_vertexBuffer = meshCacheData.m_vertexBuffer;
    m_indexBuffer = meshCacheData.m_indexBuffer;
    m_cullInstancesDescriptorSetLayoutGenerator = &cullInstancesDescriptorSetLayoutGenerator;

    uint32_t maxSlot = pipelineSet->getCameraDescriptorSlot(pipelineIdx);
    if (pipelineSet->getLightDescriptorSlot(pipelineIdx) != -1 && maxSlot < pipelineSet->getLightDescriptorSlot(pipelineIdx))
        maxSlot = pipelineSet->getLightDescriptorSlot(pipelineIdx);
    if (pipelineSet->getMaterialsDescriptorSlot(pipelineIdx) != -1 && maxSlot < pipelineSet->getMaterialsDescriptorSlot(pipelineIdx))
        maxSlot = pipelineSet->getMaterialsDescriptorSlot(pipelineIdx);
    for (const DescriptorSetBindInfo& descriptorSetBindInfo : m_descriptorSetsToBindForDraw)
    {
        if (maxSlot < descriptorSetBindInfo.getBindingSlot())
        {
            maxSlot = descriptorSetBindInfo.getBindingSlot();
        }
    }
    m_maxDescriptorSetBindingSlot = maxSlot;
}

void Wolf::InstanceMeshRenderer::PerBatchData::activateCamera(uint32_t cameraIdx)
{
    for (PerCameraData& perCameraData : m_dataForCameras)
    {
        if (perCameraData.m_cameraIdx == cameraIdx)
        {
            // TODO
            //perCameraData.m_enabled = true;
            return;
        }
    }

    emplaceNewCamera(cameraIdx);
}

bool Wolf::InstanceMeshRenderer::PerBatchData::isSame(uint64_t bufferSetHash, uint64_t pipelineHash, uint32_t pipelineIdx, const std::vector<DescriptorSetBindInfo>& descriptorSetsToBindForDraw, uint32_t customMask) const
{
    return bufferSetHash == m_bufferSetHash && pipelineHash == m_pipelineHash && pipelineIdx == m_pipelineIdx && descriptorSetsToBindForDraw == m_descriptorSetsToBindForDraw && m_customMask == customMask;
}

const Wolf::Pipeline* Wolf::InstanceMeshRenderer::PerBatchData::getOrCreatePipeline(RenderPass* renderPass, const std::vector<DescriptorSetBindInfo>& descriptorSetsBindInfo,
    const std::vector<PipelineSet::ShaderCodeToAddForStage>& shadersCodeToAdd, ShaderList& shaderList) const
{
    return m_renderingPipelineSet->getOrCreatePipeline(m_pipelineIdx, renderPass, m_descriptorSetsToBindForDraw, descriptorSetsBindInfo, shadersCodeToAdd, shaderList);
}

uint32_t Wolf::InstanceMeshRenderer::PerBatchData::hasCamera(uint32_t cameraIdx) const
{
    for (const PerCameraData& perCameraData : m_dataForCameras)
    {
        if (perCameraData.m_cameraIdx == cameraIdx)
            return true;
    }
    return false;
}

void Wolf::InstanceMeshRenderer::PerBatchData::emplaceNewCamera(uint32_t cameraIdx)
{
    PerCameraData& drawData = m_dataForCameras.emplace_back();
    drawData.m_cameraIdx = cameraIdx;
}
