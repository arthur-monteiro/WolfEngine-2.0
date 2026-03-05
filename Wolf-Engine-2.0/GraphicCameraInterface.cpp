#include "GraphicCameraInterface.h"

#include "Buffer.h"
#include "CameraInterface.h"
#include "DescriptorSetGenerator.h"

uint32_t Wolf::GraphicCameraInterface::s_instanceCount = 0;
Wolf::ResourceUniqueOwner<Wolf::DescriptorSetLayoutGenerator> Wolf::GraphicCameraInterface::s_descriptorSetLayoutGenerator;
Wolf::ResourceUniqueOwner<Wolf::DescriptorSetLayout> Wolf::GraphicCameraInterface::s_descriptorSetLayout;

Wolf::GraphicCameraInterface::~GraphicCameraInterface()
{
	s_instanceCount--;
	if (s_instanceCount == 0)
	{
		s_descriptorSetLayoutGenerator.reset(nullptr);
		s_descriptorSetLayout.reset(nullptr);
	}
}

Wolf::ResourceUniqueOwner<Wolf::DescriptorSetLayout>& Wolf::GraphicCameraInterface::getDescriptorSetLayout()
{
	initDescriptorSetLayoutIfNeeded();
	return s_descriptorSetLayout;
}

Wolf::GraphicCameraInterface::GraphicCameraInterface()
{
	initDescriptorSetLayoutIfNeeded();
	s_instanceCount++;

	m_descriptorSet.reset(DescriptorSet::createDescriptorSet(*s_descriptorSetLayout));
	DescriptorSetGenerator descriptorSetGenerator(s_descriptorSetLayoutGenerator->getDescriptorLayouts());
	m_matricesUniformBuffer.reset(new UniformBuffer(sizeof(UniformBufferData)));
	descriptorSetGenerator.setUniformBuffer(0, *m_matricesUniformBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
}

void Wolf::GraphicCameraInterface::updateGraphic(const glm::vec2& pixelJitter, const CameraUpdateContext& context)
{
	UniformBufferData ubData;
	ubData.projection = getProjectionMatrix();
	ubData.view = getViewMatrix();
	ubData.invView = glm::inverse(ubData.view);
	ubData.invProjection = glm::inverse(ubData.projection);
	ubData.previousViewMatrix = getPreviousViewMatrix();
	ubData.jitter = pixelJitter;
	ubData.near = getNear();
	ubData.far = getFar();
	ubData.projectionParams.x = ubData.far / (ubData.far - ubData.near);
	ubData.projectionParams.y = (-ubData.far * ubData.near) / (ubData.far - ubData.near);
	ubData.frameIndex = m_currentFrameIndex;
	ubData.extentWidth = context.m_swapChainExtent.width;

	const glm::mat4 transposedViewProj = glm::transpose(ubData.projection * ubData.view);
	ubData.frustumPlanes[0] = transposedViewProj[3] + transposedViewProj[0]; // left
	ubData.frustumPlanes[1] = transposedViewProj[3] - transposedViewProj[0]; // right
	ubData.frustumPlanes[2] = transposedViewProj[3] + transposedViewProj[1]; // bottom
	ubData.frustumPlanes[3] = transposedViewProj[3] - transposedViewProj[1]; // top
	ubData.frustumPlanes[4] = transposedViewProj[2]; // near
	ubData.frustumPlanes[5] = transposedViewProj[3] - transposedViewProj[2]; // far

	for (glm::vec4& frustumPlane : ubData.frustumPlanes)
	{
		const float length = glm::length(glm::vec3(frustumPlane));
		frustumPlane /= length;
	}

	m_matricesUniformBuffer->transferCPUMemory(&ubData, sizeof(ubData), 0);

	m_currentFrameIndex++;
}

void Wolf::GraphicCameraInterface::initDescriptorSetLayoutIfNeeded()
{
	if (s_descriptorSetLayoutGenerator)
		return;

	s_descriptorSetLayoutGenerator.reset(new DescriptorSetLayoutGenerator);
	s_descriptorSetLayoutGenerator->addUniformBuffer(
		ShaderStageFlagBits::VERTEX | ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::COMPUTE | ShaderStageFlagBits::RAYGEN | ShaderStageFlagBits::TESSELLATION_CONTROL | ShaderStageFlagBits::TESSELLATION_EVALUATION | ShaderStageFlagBits::GEOMETRY,
		0);

	s_descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(s_descriptorSetLayoutGenerator->getDescriptorLayouts()));
}
