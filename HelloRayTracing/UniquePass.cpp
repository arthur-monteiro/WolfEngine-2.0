#include "UniquePass.h"

#include <filesystem>
#include <fstream>

#include <Attachment.h>
#include <DescriptorSetGenerator.h>
#include <DescriptorSetLayoutGenerator.h>
#include <Image.h>
#include <FrameBuffer.h>
#include <RayTracingShaderGroupGenerator.h>
#include <ShaderBindingTable.h>

#include "Vertex2D.h"

using namespace Wolf;

void UniquePass::initializeResources(const InitializationContext& context)
{
	m_commandBuffer.reset(new CommandBuffer(QueueType::RAY_TRACING, false /* isTransient */));
	m_semaphore.reset(new Semaphore(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR));

	m_rayGenShaderParser.reset(new ShaderParser("Shaders/shader.rgen"));
	m_rayMissShaderParser.reset(new ShaderParser("Shaders/shader.rmiss"));
	m_closestHitShaderParser.reset(new ShaderParser("Shaders/shader.rchit"));

	RayTracingShaderGroupGenerator shaderGroupGenerator;
	shaderGroupGenerator.addRayGenShaderStage(0);
	shaderGroupGenerator.addMissShaderStage(  1);
	HitGroup hitGroup;
	hitGroup.closestHitShaderIdx =            2;
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
	shaders[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	shaders[1].shaderCode = rayMissShaderCode;
	shaders[1].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
	shaders[2].shaderCode = closestHitShaderCode;
	shaders[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	pipelineCreateInfo.shaderCreateInfos = shaders;

	pipelineCreateInfo.shaderGroupsCreateInfos = shaderGroupGenerator.getShaderGroups();

	DescriptorSetLayoutGenerator descriptorSetLayoutGenerator;
	descriptorSetLayoutGenerator.addAccelerationStructure(VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0);
	descriptorSetLayoutGenerator.addStorageImage(VK_SHADER_STAGE_RAYGEN_BIT_KHR,                                                1);
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,                                          2); // vertex buffer
	descriptorSetLayoutGenerator.addStorageBuffer(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,                                          3); // index buffer
	m_descriptorSetLayout.reset(new DescriptorSetLayout(descriptorSetLayoutGenerator.getDescriptorLayouts()));

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts = { m_descriptorSetLayout->getDescriptorSetLayout() };
	m_pipeline.reset(new Pipeline(pipelineCreateInfo, descriptorSetLayouts));

	std::vector<uint32_t> t(3);
	m_shaderBindingTable.reset(new ShaderBindingTable(t, m_pipeline->getPipeline()));

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

	VkBufferUsageFlags rayTracingFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	m_triangle.reset(new Mesh(vertices, indices, rayTracingFlags, rayTracingFlags, VK_FORMAT_R32G32_SFLOAT));

	BottomLevelAccelerationStructureCreateInfo blasCreateInfo;
	blasCreateInfo.buildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	std::vector<GeometryInfo> geometries(1);
	geometries[0].mesh = m_triangle.get();
	blasCreateInfo.geometryInfos = geometries;
	m_blas.reset(new BottomLevelAccelerationStructure(blasCreateInfo));

	BLASInstance blasInstance;
	blasInstance.bottomLevelAS = m_blas.get();
	blasInstance.hitGroupIndex = 0;
	blasInstance.transform = glm::mat4(1.0f);
	blasInstance.instanceID = 0;
	std::vector<BLASInstance> blasInstances = { blasInstance };
	m_tlas.reset(new TopLevelAccelerationStructure(blasInstances));

	m_descriptorSets.resize(context.swapChainImageCount);
	for (uint32_t i = 0; i < context.swapChainImageCount; ++i)
	{
		DescriptorSetGenerator::ImageDescription image(VK_IMAGE_LAYOUT_GENERAL, context.swapChainImages[i]->getDefaultImageView());

		DescriptorSetGenerator descriptorSetGenerator(descriptorSetLayoutGenerator.getDescriptorLayouts());
		descriptorSetGenerator.setAccelerationStructure(0, *m_tlas.get());
		descriptorSetGenerator.setImage(                1, image);
		descriptorSetGenerator.setBuffer(               2, m_triangle->getVertexBuffer());
		descriptorSetGenerator.setBuffer(               3, m_triangle->getIndexBuffer());

		if (!m_descriptorSets[i])
			m_descriptorSets[i].reset(new DescriptorSet(m_descriptorSetLayout->getDescriptorSetLayout(), UpdateRate::NEVER));
		m_descriptorSets[i]->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
	}
}

void UniquePass::resize(const Wolf::InitializationContext& context)
{
	
}

void UniquePass::record(const Wolf::RecordContext& context)
{
	uint32_t frameBufferIdx = context.swapChainImageIdx;

	m_commandBuffer->beginCommandBuffer(context.commandBufferIdx);

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_IMAGE_LAYOUT_GENERAL, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR);

	vkCmdBindPipeline(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline->getPipeline());
	vkCmdBindDescriptorSets(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline->getPipelineLayout(), 0, 1, m_descriptorSets[context.swapChainImageIdx]->getDescriptorSet(), 0, nullptr);

	VkStridedDeviceAddressRegionKHR rgenRegion{};
	rgenRegion.deviceAddress = m_shaderBindingTable->getBuffer().getBufferDeviceAddress();
	rgenRegion.stride = m_shaderBindingTable->getBaseAlignment();
	rgenRegion.size = m_shaderBindingTable->getBaseAlignment();

	VkStridedDeviceAddressRegionKHR rmissRegion{};
	rmissRegion.deviceAddress = m_shaderBindingTable->getBuffer().getBufferDeviceAddress() + rgenRegion.size;
	rmissRegion.stride = m_shaderBindingTable->getBaseAlignment();
	rmissRegion.size = m_shaderBindingTable->getBaseAlignment();

	VkStridedDeviceAddressRegionKHR rhitRegion{};
	rhitRegion.deviceAddress = m_shaderBindingTable->getBuffer().getBufferDeviceAddress() + rgenRegion.size + rmissRegion.size;
	rhitRegion.stride = m_shaderBindingTable->getBaseAlignment();
	rhitRegion.size = m_shaderBindingTable->getBaseAlignment();

	VkStridedDeviceAddressRegionKHR callRegion{};

	vkCmdTraceRaysKHR(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), &rgenRegion,
		&rmissRegion,
		&rhitRegion,
		&callRegion, 1920, 1080, 1);

	context.swapchainImage->transitionImageLayout(m_commandBuffer->getCommandBuffer(context.commandBufferIdx), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

	m_commandBuffer->endCommandBuffer(context.commandBufferIdx);
}

void UniquePass::submit(const Wolf::SubmitContext& context)
{
	std::vector<const Semaphore*> waitSemaphores{ context.imageAvailableSemaphore };
	std::vector<VkSemaphore> signalSemaphores{ m_semaphore->getSemaphore() };
	m_commandBuffer->submit(context.commandBufferIdx, waitSemaphores, signalSemaphores, context.frameFence);

	/*bool anyShaderModified = m_vertexShaderParser->compileIfFileHasBeenModified();
	if (m_fragmentShaderParser->compileIfFileHasBeenModified())
		anyShaderModified = true;

	if (anyShaderModified)
	{
		vkDeviceWaitIdle(context.device);
		createPipeline(m_swapChainWidth, m_swapChainHeight);
	}*/
}