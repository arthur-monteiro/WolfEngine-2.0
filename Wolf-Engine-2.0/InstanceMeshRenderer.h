#pragma once

#include <atomic>
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
            struct LOD
            {
                NullableResourceNonOwner<MeshInterface> m_mesh;
                float m_maxDistance = 0.0f;

                struct Cluster
                {
                    uint32_t m_indexOffset;
                    uint32_t m_indexCount;
                    uint32_t m_vertexOffset;
                    uint32_t m_vertexCount;
                    BoundingSphere m_boundingSphere;
                };
                static_assert(std::is_trivially_copyable_v<Cluster>); // needed for editor mesh formatter caches
                static_assert(std::is_standard_layout_v<BoundingSphere>);
                std::vector<Cluster> m_clusters;

                uint32_t m_indexCount = 0; // for the stats

                LOD(const ResourceNonOwner<MeshInterface>& mesh, float maxDistance, uint32_t indexCount, const std::vector<Cluster>& clusterRanges)
                    : m_mesh(mesh), m_maxDistance(maxDistance), m_indexCount(indexCount), m_clusters(clusterRanges) {}
                LOD(const ResourceNonOwner<MeshInterface>& mesh) : m_mesh(mesh) {}
                LOD() = default;
            };
            std::vector<LOD> m_lods;
            BoundingSphere m_boundingSphere;
            ResourceNonOwner<const PipelineSet> m_pipelineSet;
            std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> m_perPipelineDescriptorSets;

            MeshToRender(const ResourceNonOwner<const PipelineSet>& pipeline) : m_pipelineSet(pipeline) {}
        };

        [[nodiscard]] uint32_t registerMesh(const MeshToRender& mesh);
        void registerLODData(uint32_t meshIdx, uint32_t lodIdx, const MeshToRender::LOD& lod);
        void unregisterLODData(uint32_t meshIdx, uint32_t lodIdx) const;
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

        static float computeLODDistance(float radius, uint32_t indexCount, float quality);

        struct Feedback
        {
            uint32_t m_meshIdx;
            uint32_t m_lod;
        };
        void swapFeedbacks(std::vector<Feedback>& outFeedbacks);
        uint32_t getLastUsedFrameIdx(uint32_t meshIdx, uint32_t lodIdx) const;

        uint32_t getUniqueTriangleRegisteredCount() const { return m_uniqueTriangleRegisteredCount; }
        uint32_t getTotalTriangleRegisteredCount() const { return m_totalTriangleRegisteredCount; }

    private:
        static constexpr uint32_t MAX_INSTANCE_COUNT = 32'768;
        static constexpr uint32_t MAX_MESH_COUNT = 4096;
        static constexpr uint32_t MAX_BATCH_COUNT = 32;
        static constexpr uint32_t MAX_FEEDBACK_COUNT = 32;

        static uint64_t computeHash(const ResourceNonOwner<MeshInterface>& meshInterface);
        struct MeshCacheData;
        uint32_t createMissingBatchesAndComputeBatchMask(const ResourceNonOwner<const PipelineSet>& pipelineSet, const MeshCacheData& meshCacheData,
            const std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT>& perPipelineDescriptorSets);
        void readFeedbackBuffer();
        void readLastFrameIndicesBuffer();
        uint32_t registerClusters(const std::vector<MeshToRender::LOD::Cluster>& clusters);

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

        struct LODInfo
        {
            uint32_t m_vertexOffset;
            uint32_t m_indexOffset;
            uint32_t m_indexCount;
            uint32_t m_clusterOffset;

            uint32_t m_clusterCount;
            float m_maxDistance; // distance must be last
        };
        static constexpr uint32_t MAX_LOD_COUNT = 20;

        struct MeshInfo
        {
            LODInfo m_lods[MAX_LOD_COUNT];
            glm::vec4 m_boundingSphere;
        };
        ResourceUniqueOwner<Buffer> m_meshesInfoBuffer;
        uint32_t m_currentMeshCount = 0;

        struct ClusterInfo
        {
            glm::vec4 m_boundingSphere;

            uint32_t m_indexOffset;
            uint32_t m_indexCount;
            uint32_t m_vertexOffset;
            uint32_t m_vertexCount;
        };
        // a separate buffer containing all the clusters
        // LODs contain an offset in that buffer, all the clusters for the same LOD are contiguous
        ResourceUniqueOwner<Buffer> m_clustersInfoBuffer;
        static constexpr uint32_t MAX_CLUSTER_COUNT = 4'194'304;
        std::atomic<uint32_t> m_currentClusterCount = 0;

        struct CullingUniformData
        {
            uint32_t m_instanceCount;
            uint32_t m_frameIdx;
        };
        ResourceUniqueOwner<UniformBuffer> m_cullingUniformsBuffer;

        struct InstanceDataLayout
        {
            glm::mat4 m_transform;
            uint32_t m_materialIdx;
            uint32_t m_customData;
            uint32_t m_lod;
            uint32_t pad1;
        };

        ResourceUniqueOwner<DescriptorSetLayout> m_instancesDataDescriptorSetLayout;

        struct ActiveCamera
        {
            uint32_t m_cameraIdx;
            uint32_t m_batchesMask;

            ActiveCamera(uint32_t cameraIdx, uint32_t batchesMask) : m_cameraIdx(cameraIdx), m_batchesMask(batchesMask) {}
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
        std::array<ResourceUniqueOwner<PerCullingCamera>, MAX_CAMERA_COUNT> m_cullingCamerasData;

        ResourceUniqueOwner<Buffer> m_feedbackBuffer;
        ResourceUniqueOwner<ReadableBuffer> m_feedbackReadableBuffer;
        std::vector<Feedback> m_lastFrameFeedbacks;
        std::mutex m_lastFrameFeedbacksMutex;

        struct LastFrameIndexUsageMeshInfo
        {
            uint32_t m_frameIdx[MAX_LOD_COUNT];
        };
        ResourceUniqueOwner<Buffer> m_latestFrameIdxUsedPerLODBuffer;
        ResourceUniqueOwner<ReadableBuffer> m_latestFrameIdxUsedPerLODReadableBuffer;
        std::vector<LastFrameIndexUsageMeshInfo> m_lastFrameIndexUsageMeshInfos;

        // CPU caches
        struct MeshCacheData
        {
            uint64_t m_bufferSetHash;
            NullableResourceNonOwner<Buffer> m_vertexBuffer;
            NullableResourceNonOwner<Buffer> m_indexBuffer;

            uint32_t m_indexCount;
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

        // Stats
        uint32_t m_uniqueTriangleRegisteredCount = 0; // LOD 0 triangles registered
        uint32_t m_totalTriangleRegisteredCount = 0; // multiplied by instance count
    };
}
