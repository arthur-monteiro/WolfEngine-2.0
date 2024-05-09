#include "GraphicCameraInterface.h"

#include "Buffer.h"
#include "DescriptorSetGenerator.h"

Wolf::GraphicCameraInterface::GraphicCameraInterface()
{
	m_descriptorSetLayoutGenerator.reset(new LazyInitSharedResource<DescriptorSetLayoutGenerator, GraphicCameraInterface>([](std::unique_ptr<DescriptorSetLayoutGenerator>& descriptorSetLayoutGenerator)
		{
			descriptorSetLayoutGenerator.reset(new DescriptorSetLayoutGenerator);
			descriptorSetLayoutGenerator->addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0); // matrices
		}));

	m_descriptorSetLayout.reset(new LazyInitSharedResource<DescriptorSetLayout, GraphicCameraInterface>([this](std::unique_ptr<DescriptorSetLayout>& descriptorSetLayout)
		{
			descriptorSetLayout.reset(DescriptorSetLayout::createDescriptorSetLayout(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts()));
		}));

	m_descriptorSet.reset(DescriptorSet::createDescriptorSet(*m_descriptorSetLayout->getResource()));
	DescriptorSetGenerator descriptorSetGenerator(m_descriptorSetLayoutGenerator->getResource()->getDescriptorLayouts());
	m_matricesUniformBuffer.reset(Buffer::createBuffer(sizeof(UniformBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
	descriptorSetGenerator.setBuffer(0, *m_matricesUniformBuffer);
	m_descriptorSet->update(descriptorSetGenerator.getDescriptorSetCreateInfo());
}

void Wolf::GraphicCameraInterface::updateGraphic(const glm::vec2& pixelJitter) const
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

	m_matricesUniformBuffer->transferCPUMemory(&ubData, sizeof(ubData), 0);
}
