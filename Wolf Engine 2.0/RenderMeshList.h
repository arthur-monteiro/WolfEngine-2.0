#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "CommandBuffer.h"
#include "DescriptorSetBindInfo.h"
#include "DescriptorSetLayout.h"
#include "PipelineSet.h"
#include "ResourceNonOwner.h"

namespace Wolf
{
	class CameraList;
	struct RecordContext;
	class Buffer;
	class DescriptorSet;
	class Mesh;
	class Pipeline;
	class PipelineSet;
	class RenderPass;
	class ShaderList;
	class WolfEngine;

	class RenderMeshList
	{
	public:
		struct MeshToRenderInfo
		{
			ResourceNonOwner<Mesh> mesh;
			const glm::mat4& transform;
			ResourceNonOwner<const PipelineSet> pipelineSet;
			std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> perPipelineDescriptorSets;
			struct InstanceInfos
			{
				const Buffer* instanceBuffer = nullptr;
				uint32_t instanceCount = 1;
			};
			InstanceInfos instanceInfos;

			MeshToRenderInfo(const ResourceNonOwner<Mesh>& mesh, const ResourceNonOwner<const PipelineSet>& pipelineSet, const glm::mat4& transform = glm::mat4(1.0f)) : mesh(mesh), transform(transform), pipelineSet(pipelineSet) {}
		};
		void addMeshToRender(const MeshToRenderInfo& meshToRenderInfo);

		static constexpr uint32_t NO_CAMERA_IDX = -1;
		struct AdditionalDescriptorSet
		{
			DescriptorSetBindInfo descriptorSetBindInfo;
			uint32_t mask = 0;
		};
		void draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& descriptorSetsToBind) const;

		void clear();

	private:
		friend WolfEngine;

		RenderMeshList(ShaderList& shaderList) : m_shaderList(shaderList) {}

		void moveToNextFrame(const CameraList& cameraList);

		class RenderMesh
		{
		public:
			RenderMesh(ResourceNonOwner<Mesh> mesh, const glm::mat4& transform, const ResourceNonOwner<const PipelineSet>& pipelineSet, std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> perPipelineDescriptorSets, const MeshToRenderInfo::InstanceInfos& instanceInfos) :
				m_mesh(std::move(mesh)), m_transform(transform), m_pipelineSet(pipelineSet), m_perPipelineDescriptorSets(std::move(perPipelineDescriptorSets)), m_instanceInfos(instanceInfos) { }

			void draw(const CommandBuffer& commandBuffer, const Pipeline* pipeline, uint32_t cameraIdx, uint32_t pipelineIdx) const;

			ResourceNonOwner<const PipelineSet> getPipelineSet() const { return m_pipelineSet; }
			ResourceNonOwner<Mesh> getMesh() const { return m_mesh; }
			const glm::mat4& getTransform() const { return m_transform; }
			bool isInstanced() const { return m_instanceInfos.instanceCount > 1; }
			const std::vector<DescriptorSetBindInfo>& getDescriptorSets(uint32_t pipelineIdx) const { return m_perPipelineDescriptorSets[pipelineIdx]; }

		private:
			ResourceNonOwner<Mesh> m_mesh;
			glm::mat4 m_transform;
			ResourceNonOwner<const PipelineSet> m_pipelineSet;
			std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> m_perPipelineDescriptorSets;
			MeshToRenderInfo::InstanceInfos m_instanceInfos;
		};

		std::vector<std::vector<RenderMesh*>> m_meshesToRenderByPipelineIdx;
		std::vector<std::unique_ptr<RenderMesh>> m_currentFrameMeshesToRender;
		std::vector<std::unique_ptr<RenderMesh>> m_nextFrameMeshesToRender;

		uint32_t m_pipelineIdxCount = 0;
		std::vector<uint64_t> m_uniquePipelinesHash; // list of all pipelines info hash

		ShaderList& m_shaderList;
	};
}
