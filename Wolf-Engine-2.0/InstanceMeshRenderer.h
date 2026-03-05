#pragma once

#include <glm/glm.hpp>

#include "CommandRecordBase.h"
#include "DefaultMeshRenderer.h"
#include "DescriptorSetLayoutGenerator.h"
#include "DynamicResourceUniqueOwnerArray.h"
#include "DynamicStableArray.h"
#include "GPUDataTransfersManager.h"
#include "MeshInterface.h"
#include "PipelineSet.h"
#include "ResourceUniqueOwner.h"
#include "ShaderList.h"
#include "UniformBuffer.h"

namespace Wolf
{
    class CommandBuffer;

    class InstanceMeshRenderer : public CommandRecordBase
    {
    public:
        InstanceMeshRenderer(ShaderList& shaderList, const ResourceNonOwner<GPUDataTransfersManagerInterface>& gpuDataTransfersManager);

        void moveToNextFrame();
        void clear();

        // Command record base overrides
        void initializeResources(const InitializationContext& context) override;
        void resize(const InitializationContext& context) override;
        void record(const RecordContext& context) override;
        void submit(const SubmitContext& context) override;

        struct MeshToRender
        {
            ResourceNonOwner<MeshInterface> m_mesh;
            ResourceNonOwner<const PipelineSet> m_pipelineSet;
            std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> m_perPipelineDescriptorSets;

            MeshToRender(const ResourceNonOwner<MeshInterface>& mesh, const ResourceNonOwner<const PipelineSet>& pipeline) : m_mesh(mesh), m_pipelineSet(pipeline) {}
        };

        [[nodiscard]] uint32_t registerMesh(const MeshToRender& mesh);
        [[nodiscard]] uint32_t addInstance(uint32_t meshIdx, const glm::mat4& transform, uint32_t materialIdx, uint32_t customData, const ResourceNonOwner<const PipelineSet>& pipelineSet,
            const std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT>& perPipelineDescriptorSets);
        void removeInstance(uint32_t instanceIdx);

        struct OverrideInstance
        {
            uint32_t m_instanceIdx;
        };
        void overrideCullingInstances(const std::vector<OverrideInstance>& instances);
        void stopOverridingCullingInstances();

        void activateCameraForThisFrame(uint32_t cameraIdx, uint32_t pipelineIdx);

        static constexpr uint32_t NO_CAMERA_IDX = -1;
        void draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& additionalDescriptorSetsToBind,
            const std::vector<PipelineSet::ShaderCodeToAddForStage>& shadersCodeToAdd) const;

    private:
        static constexpr uint32_t MAX_INSTANCE_COUNT = 16'184;
        static constexpr uint32_t MAX_MESH_COUNT = 1024;
        static constexpr uint32_t MAX_BATCH_COUNT = 32;

        static uint64_t computeHash(const ResourceNonOwner<MeshInterface>& meshInterface);
        struct MeshCacheData;
        uint32_t createMissingBatchesAndComputeBatchMask(const ResourceNonOwner<const PipelineSet>& pipelineSet, const MeshCacheData& meshCacheData,
            const std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT>& perPipelineDescriptorSets);

        ShaderList* m_shaderList;
        ResourceNonOwner<GPUDataTransfersManagerInterface> m_gpuDataTransfersManager;

        // GPU Scene description
        struct CullingInstanceInfo
        {
            glm::mat4 m_transform;
            uint32_t m_meshIdx;
            uint32_t m_materialIdx;
            uint32_t m_customData;
            uint32_t m_batchesMask;
        };
        ResourceUniqueOwner<Buffer> m_cullingInstancesBuffer;
        uint32_t m_currentInstanceCount = 0;

        ResourceUniqueOwner<Buffer> m_overrideCullingInstancesBuffer;
        ResourceUniqueOwner<UniformBuffer> m_copyInstancesUniformBuffer;
        uint32_t m_overrideCullingInstancesCount = 0;

        struct MeshInfo
        {
            glm::vec4 m_boundingSphere;
            uint32_t m_vertexOffset;
            uint32_t m_indexOffset;
            uint32_t m_indexCount;
            uint32_t padding;
        };
        ResourceUniqueOwner<Buffer> m_meshesInfoBuffer;
        uint32_t m_currentMeshCount = 0;

        struct CullingUniformData
        {
            uint32_t m_instanceCount;
        };
        ResourceUniqueOwner<UniformBuffer> m_cullingUniformsBuffer;

        struct InstanceDataLayout
        {
            glm::mat4 m_transform;
            uint32_t m_materialIdx;
            uint32_t m_customData;
            uint32_t pad0;
            uint32_t pad1;
        };

        ResourceUniqueOwner<DescriptorSetLayout> m_instancesDataDescriptorSetLayout;

        struct ActiveCamera
        {
            uint32_t m_cameraIdx;
            uint32_t m_batchesMask;
        };
        std::vector<ActiveCamera> m_activeCamerasNextFrame;
        std::vector<ActiveCamera> m_activeCamerasThisFrame;

        struct PerCullingCamera
        {
            ResourceUniqueOwner<Buffer> m_drawCommandsCountsBuffer; // contains MAX_BATCH_COUNT uint32
            std::array<ResourceUniqueOwner<Buffer>, MAX_BATCH_COUNT> m_drawCommandsBuffers; // 1 buffer per batch
            std::array<ResourceUniqueOwner<Buffer>, MAX_BATCH_COUNT> m_instancesDataBuffers; // 1 buffer per batch
            std::array<ResourceUniqueOwner<DescriptorSet>, MAX_BATCH_COUNT> m_instancesDataDescriptorSets; // 1 buffer per batch
            ResourceUniqueOwner<DescriptorSet> m_cullingDescriptorSet;
        };
        static constexpr uint32_t MAX_CAMERA_COUNT = 16;
        std::array<PerCullingCamera, MAX_CAMERA_COUNT> m_cullingCamerasData;

        // CPU caches
        struct MeshCacheData
        {
            uint32_t m_bufferSetHash;
            NullableResourceNonOwner<Buffer> m_vertexBuffer;
            NullableResourceNonOwner<Buffer> m_indexBuffer;
        };
        std::vector<MeshCacheData> m_meshesCacheData;

        // Add lists
        std::vector<MeshInfo> m_meshesToAdd;
        std::vector<CullingInstanceInfo> m_instancesToAdd;
        std::vector<uint32_t> m_instancesToRemove;

        ResourceUniqueOwner<Pipeline> m_cullInstancesPipeline;
        DescriptorSetLayoutGenerator m_cullInstancesDescriptorSetLayoutGenerator;
        ResourceUniqueOwner<DescriptorSetLayout> m_cullInstancesDescriptorSetLayout;

        ResourceUniqueOwner<Pipeline> m_copyInstancesPipeline;
        DescriptorSetLayoutGenerator m_copyInstancesDescriptorSetLayoutGenerator;
        ResourceUniqueOwner<DescriptorSetLayout> m_copyInstancesDescriptorSetLayout;
        ResourceUniqueOwner<DescriptorSet> m_copyDescriptorSet;

        class PerBatchData
        {
        public:
            PerBatchData(const MeshCacheData& meshCacheData, uint32_t pipelineIdx, const std::vector<DescriptorSetBindInfo>& descriptorSetsToBindForDraw, const ResourceNonOwner<const PipelineSet>& pipelineSet,
                uint32_t maxInstanceCount, uint32_t maxMeshCount, const DescriptorSetLayoutGenerator& cullInstancesDescriptorSetLayoutGenerator, const ResourceNonOwner<DescriptorSetLayout>& cullingDescriptorSetLayout);

            void activateCamera(uint32_t cameraIdx);

            bool isSame(uint64_t bufferSetHash, uint64_t pipelineHash, uint32_t pipelineIdx, const std::vector<DescriptorSetBindInfo>& descriptorSetsToBindForDraw, uint32_t customMask) const;

            const Pipeline* getOrCreatePipeline(RenderPass* renderPass, const std::vector<DescriptorSetBindInfo>& descriptorSetsBindInfo, const std::vector<PipelineSet::ShaderCodeToAddForStage>& shadersCodeToAdd,
                ShaderList& shaderList) const;
            uint32_t getPipelineIndex() const { return m_pipelineIdx; }
            uint32_t hasCamera(uint32_t cameraIdx) const;
            uint32_t getMaxDescriptorSetBindingSlot() const { return m_maxDescriptorSetBindingSlot; }
            uint32_t getCustomMask() const { return m_customMask; }
            const ResourceNonOwner<const PipelineSet>& getPipelineSet() const { return m_renderingPipelineSet; }
            const std::vector<DescriptorSetBindInfo>& getDescriptorSetsToBindForDraw() const { return m_descriptorSetsToBindForDraw; }
            const NullableResourceNonOwner<Buffer>& getVertexBuffer() const { return m_vertexBuffer; }
            const NullableResourceNonOwner<Buffer>& getIndexBuffer() const { return m_indexBuffer; }

        private:
            uint64_t m_bufferSetHash; // hash of the buffer pointers, not of the data
            uint64_t m_pipelineHash;
            uint32_t m_maxInstanceCount;
            uint32_t m_pipelineIdx;
            uint32_t m_cameraDescriptorSlot;
            uint32_t m_materialsDescriptorSlot;
            uint32_t m_lightDescriptorSlot;
            uint32_t m_customMask;

            ResourceNonOwner<DescriptorSetLayout> m_cullingDescriptorSetLayout;
            const DescriptorSetLayoutGenerator* m_cullInstancesDescriptorSetLayoutGenerator;
            ResourceNonOwner<const PipelineSet> m_renderingPipelineSet;
            std::vector<DescriptorSetBindInfo> m_descriptorSetsToBindForDraw;
            NullableResourceNonOwner<Buffer> m_vertexBuffer;
            NullableResourceNonOwner<Buffer> m_indexBuffer;

            // CPU data
            struct InternalInstancedMesh
            {
                MeshToRender m_mesh;

                struct InstanceData
                {
                    glm::mat4 m_transform;
                    uint32_t m_materialIdx;
                    uint32_t m_customData;
                };
                std::vector<InstanceData> m_instancesData;

                InternalInstancedMesh(MeshToRender mesh) : m_mesh(std::move(mesh)) {}
            };

            struct PerCameraData
            {
                uint32_t m_cameraIdx = -1;
            };
            void emplaceNewCamera(uint32_t cameraIdx);
            std::vector<PerCameraData> m_dataForCameras;

            uint32_t m_maxDescriptorSetBindingSlot = -1;

        };
        DynamicResourceUniqueOwnerArray<PerBatchData, 16> m_batchesData;

        std::mutex m_mutex;
    };
}
