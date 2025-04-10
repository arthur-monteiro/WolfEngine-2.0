#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <RayTracingShaderGroupGenerator.h>
#include <ShaderBindingTable.h>

#include "Vertex2D.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	m_commandBuffer.reset(CommandBuffer::createCommandBuffer(QueueType::RAY_TRACING, false /* isTransient */));
	m_semaphore.reset(Semaphore::createSemaphore(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR));

	m_rayGenShaderParser.reset(new ShaderParser("Shaders/shader.rgen"));
	m_rayMissShaderParser.reset(new ShaderParser("Shaders/shader.rmiss"));
	m_closestHitShaderParser.reset(new ShaderParser("Shaders/shader.rchit"));

	m_descriptorSetLayoutGenerator.addAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0);
	m_descriptorSetLayoutGenerator.addStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_KHR,                                                1);
	m_descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,                                          2); // vertex buffer
	m_descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,                                          3); // index buffer
	m_descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator.getDescriptorLayouts()));

	createPipeline();

	// Load triangle
	const std::vector<Vertex2D> vertices =
	{
		{ glm::vec2(0.0f, -0.75f) }, // top
		{ glm::vec2(-0.75f, 0.75f) }, // bot left
		{ glm::vec2(0.75f, 0.75f) } // bot right
	};

	const std::vector<uint32_t> indices =
	{
		0, 1, 2
	};

	const VkBufferUsageFlags rayTracingFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_triangle.reset(new Mesh(vertices, indices, {}, {}, rayTracingFlags, rayTracingFlags, Format::R32G32_SFLOAT));

	BottomLevelAccelerationStructureCreateInfo blasCreateInfo;
	blasCreateInfo.buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;

	std::vector<GeometryInfo> geometries(1);
	GeometryInfo& geometryInfo = geometries.back();
	geometryInfo.mesh.vertexBuffer = &*m_triangle->getVertexBuffer();
	geometryInfo.mesh.vertexCount = m_triangle->getVertexCount();
	geometryInfo.mesh.vertexSize = m_triangle->getVertexSize();
	geometryInfo.mesh.vertexFormat = m_triangle->getVertexFormat();
	geometryInfo.mesh.indexBuffer = &m_triangle->getIndexBuffer();
	geometryInfo.mesh.indexCount = m_triangle->getIndexCount();
	blasCreateInfo.geometryInfos = geometries;

	m_blas.reset(BottomLevelAccelerationStructure::createBottomLevelAccelerationStructure(blasCreateInfo));

	BLASInstance blasInstance;
	blasInstance.bottomLevelAS = m_blas.get();
	blasInstance.hitGroupIndex = 0;
	blasInstance.transform = glm::mat4(1.0f);
	blasInstance.instanceID = 0;
	std::vector blasInstances = { blasInstance };
	m_tlas.reset(TopLevelAccelerationStructure::createTopLevelAccelerationStructure(blasInstances));

	m_descriptorSets.resize(context.swapChainImageCount);
	createDescriptorSets(context);
}

void UniquePass::resize(const InitializationContext& context)
{
	createDescriptorSets(context);
}

void UniquePass::record(const RecordContext& context)
{
	uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer();

	context.swapchainImage->transitionImageLayout(*m_commandBuffer, { VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR });

	m_commandBuffer->bindPipeline(m_pipeline.get());
	m_commandBuffer->bindDescriptorSet(m_descriptorSets[context.swapChainImageIdx].get(), 0, *m_pipeline);

	m_commandBuffer->traceRays(m_shaderBindingTable.get(), context.swapchainImage->getExtent());

	context.swapchainImage->transitionImageLayout(*m_commandBuffer, { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT });

	m_commandBuffer->endCommandBuffer();
}

void UniquePass::submit(const SubmitContext& context)
{
	const std::vector waitSemaphores{ context.swapChainImageAvailableSemaphore };
	const std::vector<const Semaphore*> signalSemaphores{ m_semaphore.get() };
	m_commandBuffer->submit(waitSemaphores, signalSemaphores, context.frameFence);

	bool anyShaderModified = m_rayGenShaderParser->compileIfFileHasBeenModified();
	if (m_rayMissShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;
	if (m_closestHitShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;

	if (anyShaderModified)
	{
		context.graphicAPIManager->waitIdle();
		createPipeline();
	}
}

void UniquePass::createPipeline()
{
	RayTracingShaderGroupGenerator shaderGroupGenerator;
	shaderGroupGenerator.addRayGenShaderStage(0);
	shaderGroupGenerator.addMissShaderStage(1);
	HitGroup hitGroup;
	hitGroup.closestHitShaderIdx = 2;
	shaderGroupGenerator.addHitGroup(hitGroup);

	RayTracingPipelineCreateInfo pipelineCreateInfo;

	std::vector<char> rayGenShaderCode;
	m_rayGenShaderParser->readCompiledShader(rayGenShaderCode);
	std::vector<char> rayMissShaderCode;
	m_rayMissShaderParser->readCompiledShader(rayMissShaderCode);
	std::vector<char> closestHitShaderCode;
	m_closestHitShaderParser->readCompiledShader(closestHitShaderCode);

	std::vector<ShaderCreateInfo> shaders(3);
	shaders[0].shaderCode = rayGenShaderCode;
	shaders[0].stage = RAYGEN;
	shaders[1].shaderCode = rayMissShaderCode;
	shaders[1].stage = MISS;
	shaders[2].shaderCode = closestHitShaderCode;
	shaders[2].stage = CLOSEST_HIT;
	pipelineCreateInfo.shaderCreateInfos = shaders;

	pipelineCreateInfo.shaderGroupsCreateInfos = shaderGroupGenerator.getShaderGroups();

	std::vector<const DescriptorSetLayout*> descriptorSetLayouts = { m_descriptorSetLayout.get() };
	m_pipeline.reset(Pipeline::createRayTracingPipeline(pipelineCreateInfo, descriptorSetLayouts));

	m_shaderBindingTable.reset(new ShaderBindingTable(static_cast<uint32_t>(shaders.size()), *m_pipeline));
}

void UniquePass::createDescriptorSets(const InitializationContext& context)
{
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		DescriptorSetGenerator::ImageDescription image(VK_IMAGE_LAYOUT_GENERAL, context.swapChainImages[i]->getDefaultImageView());

		DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator.getDescriptorLayouts());
		descriptorSetGenerator.setAccelerationStructure(0, *m_tlas);
		descriptorSetGenerator.setImage(1, image);
		descriptorSetGenerator.setBuffer(2, *m_triangle->getVertexBuffer());
		descriptorSetGenerator.setBuffer(3, m_triangle->getIndexBuffer());

		if (!m_descriptorSets[i])
			m_descriptorSets[i].reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}
