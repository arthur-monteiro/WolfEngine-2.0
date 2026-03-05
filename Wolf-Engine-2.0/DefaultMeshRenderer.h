#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "CommandBuffer.h"
#include "Mesh.h"
#include "PipelineSet.h"

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

	struct AdditionalDescriptorSet
	{
		DescriptorSetBindInfo m_descriptorSetBindInfo;
		uint32_t m_mask = 0;
	};

	class DefaultMeshRenderer
	{
	public:
		DefaultMeshRenderer(ShaderList& shaderList) : m_shaderList(&shaderList) {}

		void moveToNextFrame();
		void clear();

		struct MeshToRender
		{
			ResourceNonOwner<MeshInterface> m_mesh;
			ResourceNonOwner<const PipelineSet> m_pipelineSet;
			std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> m_perPipelineDescriptorSets;

			NullableResourceNonOwner<Buffer> m_overrideIndexBuffer;

			MeshToRender(const ResourceNonOwner<MeshInterface>& mesh, const ResourceNonOwner<const PipelineSet>& pipeline) : m_mesh(mesh), m_pipelineSet(pipeline) {}
		};

		struct InstancedMesh
		{
			MeshToRender m_mesh;

			NullableResourceNonOwner<Buffer> m_instanceBuffer;
			uint32_t m_instanceBufferOffset;
			uint32_t m_instanceSize;
		};

		uint32_t registerMesh(const MeshToRender& mesh);
		void addTransientMesh(const MeshToRender& mesh);

		uint32_t registerInstancedMesh(const InstancedMesh& mesh, uint32_t maxInstanceCount, uint32_t instanceIdx);
		void addInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx);
		void removeInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx);
		void addTransientInstancedMesh(const InstancedMesh& mesh, uint32_t instanceCount);

		void isolateInstanceMesh(uint32_t instancedMeshIdx, uint32_t instanceIdx);
		void removeIsolation();

		static constexpr uint32_t NO_CAMERA_IDX = -1;
		void draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& descriptorSetsToBind,
			const std::vector<PipelineSet::ShaderCodeToAddForStage>& shadersCodeToAdd) const;

	private:
		struct InternalMesh
		{
			MeshToRender m_meshToRender;

			InternalMesh(MeshToRender meshToRender) : m_meshToRender(std::move(meshToRender)) {}
		};
		std::vector<InternalMesh> m_meshes;
		std::vector<InternalMesh> m_transientMeshesCurrentFrame;
		std::vector<InternalMesh> m_transientMeshesNextFrame;

		struct InternalInstancedMesh
		{
			InstancedMesh m_instancedMesh;

			struct OffsetAndCount
			{
				uint32_t m_offset;
				uint32_t m_count;
			};
			std::vector<OffsetAndCount> m_offsetAndCounts; // offset in instance buffer and instance count
			void buildOffsetAndCounts();

			std::vector<bool> activatedInstances;

			InternalInstancedMesh(InstancedMesh instancedMesh) : m_instancedMesh(std::move(instancedMesh)) {}
		};
		std::vector<InternalInstancedMesh> m_instancedMeshes;
		std::vector<InternalInstancedMesh> m_transientInstancedMeshesCurrentFrame;
		std::vector<InternalInstancedMesh> m_transientInstancedMeshesNextFrame;

		struct IsolatedInfo
		{
			bool enabled;

			uint32_t instancedMeshIdx;
			uint32_t instanceIdx;
		};
		IsolatedInfo m_isolatedInfo{};

		ShaderList* m_shaderList;
	};
}
