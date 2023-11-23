#pragma once

#include <memory>
#include <vector>

#include "CommandBuffer.h"

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
			Mesh* mesh;
			PipelineSet* pipelineSet;
			struct DescriptorSetBindInfo
			{
				const DescriptorSet* descriptorSet;
				uint32_t descriptorSetBindingSlot;
			};
			std::vector<DescriptorSetBindInfo> descriptorSets;
			struct InstanceInfos
			{
				const Buffer* instanceBuffer = nullptr;
				uint32_t instanceCount = 1;
			};
			InstanceInfos instanceInfos;

			MeshToRenderInfo(Mesh* mesh, PipelineSet* pipelineSet) : mesh(mesh), pipelineSet(pipelineSet) {}
		};
		void addMeshToRender(const MeshToRenderInfo& meshToRenderInfo);

		static constexpr uint32_t NO_CAMERA_IDX = -1;
		void draw(const RecordContext& context, VkCommandBuffer commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<std::pair<uint32_t, const DescriptorSet*>>& descriptorSetsToBind) const;

	private:
		friend WolfEngine;

		RenderMeshList(ShaderList& shaderList) : m_shaderList(shaderList) {}

		void moveToNextFrame(const CameraList& cameraList);

		class RenderMesh
		{
		public:
			RenderMesh(Mesh* mesh, PipelineSet* pipelineSet, std::vector<MeshToRenderInfo::DescriptorSetBindInfo> descriptorSets, const MeshToRenderInfo::InstanceInfos& instanceInfos) :
				m_mesh(mesh), m_pipelineSet(pipelineSet), m_descriptorSets(std::move(descriptorSets)), m_instanceInfos(instanceInfos) { }

			void draw(VkCommandBuffer commandBuffer, const Pipeline* pipeline, uint32_t cameraIdx) const;

			PipelineSet* getPipelineSet() const { return m_pipelineSet; }
			Mesh* getMesh() const { return m_mesh; }

		private:
			Mesh* m_mesh;
			PipelineSet* m_pipelineSet;
			std::vector<MeshToRenderInfo::DescriptorSetBindInfo> m_descriptorSets;
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
