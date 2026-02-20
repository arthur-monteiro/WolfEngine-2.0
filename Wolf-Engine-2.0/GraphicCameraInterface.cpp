#include "GraphicCameraInterface.h"

#include "Buffer.h"
#include "CameraInterface.h"
#include "DescriptorSetGenerator.h"

Wolf::GraphicCameraInterface::GraphicCameraInterface()
{
	m_descriptorSetLayoutGenerator.reset(new LazyInitSharedResource<DescriptorSetLayoutGenerator, GraphicCameraInterface>([](ResourceUniqueOwner<DescriptorSetLayoutGenerator>& descriptorSetLayoutGenerator)
		{
			descriptorSetLayoutGenerator.reset(new DescriptorSetLayoutGenerator);
			descriptorSetLayoutGenerator->addUniformBuffer(
				ShaderStageFlagBits::VERTEX | ShaderStageFlagBits::FRAGMENT | ShaderStageFlagBits::COMPUTE | ShaderStageFlagBits::RAYGEN | ShaderStageFlagBits::TESSELLATION_CONTROL | ShaderStageFlagBits::TESSELLATION_EVALUATION | ShaderStageFlagBits::GEOMETRY,
				0); // matrices
		}));

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, GraphicCameraInterface>([this](ResourceUniqueOwner<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts()));
		}));

	m_descriptorSet.reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));
	DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts());
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

	const glm::mat4 transposedViewProjection = glm::transpose(ubData.projection * ubData.view);
	ubData.frustumPlanes[0] = transposedViewProjection[3] + transposedViewProjection[0]; // left
	ubData.frustumPlanes[1] = transposedViewProjection[3] - transposedViewProjection[0]; // right
	ubData.frustumPlanes[2] = transposedViewProjection[3] + transposedViewProjection[1]; // bottom
	ubData.frustumPlanes[3] = transposedViewProjection[3] - transposedViewProjection[1]; // top
	ubData.frustumPlanes[4] = transposedViewProjection[3] + transposedViewProjection[2]; // near
	ubData.frustumPlanes[5] = transposedViewProjection[3] - transposedViewProjection[2]; // far

	m_matricesUniformBuffer->transferCPUMemory(&ubData, sizeof(ubData), 0);

	m_currentFrameIndex++;
}
