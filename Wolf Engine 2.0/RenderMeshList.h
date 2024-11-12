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
		RenderMeshList(ShaderList& shaderList) : m_shaderList(&shaderList) {}

		void moveToNextFrame();
		void clear();

		struct MeshToRender
		{
			ResourceNonOwner<Mesh> mesh;
			ResourceNonOwner<const PipelineSet> pipelineSet;
			std::array<std::vector<DescriptorSetBindInfo>, PipelineSet::MAX_PIPELINE_COUNT> perPipelineDescriptorSets;
		};
		uint32_t registerMesh(const MeshToRender& mesh);
		void addTransientMesh(const MeshToRender& mesh);

		struct InstancedMesh
		{
			MeshToRender mesh;

			NullableResourceNonOwner<Buffer> instanceBuffer;
			uint32_t instanceSize;
		};
		uint32_t registerInstancedMesh(const InstancedMesh& mesh, uint32_t maxInstanceCount, uint32_t instanceIdx);
		void addInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx);
		void removeInstance(uint32_t instancedMeshIdx, uint32_t instanceIdx);
		void addTransientInstancedMesh(const InstancedMesh& mesh, uint32_t instanceCount);

		static constexpr uint32_t NO_CAMERA_IDX = -1;
		struct AdditionalDescriptorSet
		{
			DescriptorSetBindInfo descriptorSetBindInfo;
			uint32_t mask = 0;
		};
		void draw(const RecordContext& context, const CommandBuffer& commandBuffer, RenderPass* renderPass, uint32_t pipelineIdx, uint32_t cameraIdx, const std::vector<AdditionalDescriptorSet>& descriptorSetsToBind) const;

	private:
		struct InternalMesh
		{
			MeshToRender meshToRender;
		};
		std::vector<InternalMesh> m_meshes;
		std::vector<InternalMesh> m_transientMeshesCurrentFrame;
		std::vector<InternalMesh> m_transientMeshesNextFrame;

		struct InternalInstancedMesh
		{
			InstancedMesh instancedMesh;

			struct OffsetAndCount
			{
				uint32_t offset;
				uint32_t count;
			};
			std::vector<OffsetAndCount> offsetAndCounts; // offset in instance buffer and instance count
			void buildOffsetAndCounts();

			std::vector<bool> activatedInstances;
		};
		std::vector<InternalInstancedMesh> m_instancedMeshes;
		std::vector<InternalInstancedMesh> m_transientInstancedMeshesCurrentFrame;
		std::vector<InternalInstancedMesh> m_transientInstancedMeshesNextFrame;

		ShaderList* m_shaderList;
	};
}
