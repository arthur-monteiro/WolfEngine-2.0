#include "PipelineVulkan.h"

#include <Debug.h>

#include "DescriptorSetLayoutVulkan.h"
#include "RenderPassVulkan.h"
#include "Vulkan.h"

Wolf::PipelineVulkan::PipelineVulkan(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo)
{
	m_type = Type::RENDERING;

	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(renderingPipelineCreateInfo.descriptorSetLayouts.size());
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts(pipelineLayoutInfo.setLayoutCount);
	for (uint32_t i = 0; i < pipelineLayoutInfo.setLayoutCount; ++i)
		descriptorSetLayouts[i] = renderingPipelineCreateInfo.descriptorSetLayouts[i].operator-><const DescriptorSetLayoutVulkan>()->getDescriptorSetLayout();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(g_vulkanInstance->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");

	/* Shaders */
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	std::vector<VkShaderModule> shaderModules;
	for (auto& shaderCreateInfo : renderingPipelineCreateInfo.shaderCreateInfos)
	{
		// Create shader module
		shaderModules.push_back(createShaderModule(shaderCreateInfo.shaderCode));

		// Add stage
		VkPipelineShaderStageCreateInfo shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderCreateInfo.stage;
		shaderStageInfo.module = shaderModules.back();
		shaderStageInfo.pName = shaderCreateInfo.entryPointName.data();

		shaderStages.push_back(shaderStageInfo);
	}

	/* Input */
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(renderingPipelineCreateInfo.vertexInputBindingDescriptions.size());
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(renderingPipelineCreateInfo.vertexInputAttributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = renderingPipelineCreateInfo.vertexInputBindingDescriptions.data();
	vertexInputInfo.pVertexAttributeDescriptions = renderingPipelineCreateInfo.vertexInputAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = renderingPipelineCreateInfo.topology;
	inputAssembly.primitiveRestartEnable = renderingPipelineCreateInfo.primitiveRestartEnable;

	/* Viewport */
	VkViewport viewport = {};
	viewport.x = renderingPipelineCreateInfo.extent.width * renderingPipelineCreateInfo.viewportOffset[0];
	viewport.y = renderingPipelineCreateInfo.extent.height * renderingPipelineCreateInfo.viewportOffset[1];
	viewport.width = renderingPipelineCreateInfo.extent.width * renderingPipelineCreateInfo.viewportScale[0];
	viewport.height = renderingPipelineCreateInfo.extent.height * renderingPipelineCreateInfo.viewportScale[1];
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = renderingPipelineCreateInfo.extent;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	/* Rasterization */
	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = renderingPipelineCreateInfo.polygonMode;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = renderingPipelineCreateInfo.cullMode;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = renderingPipelineCreateInfo.depthBiasConstantFactor > 0.0f || renderingPipelineCreateInfo.depthBiasSlopeFactor > 0.0f;
	rasterizer.depthBiasConstantFactor = renderingPipelineCreateInfo.depthBiasConstantFactor;
	rasterizer.depthBiasSlopeFactor = renderingPipelineCreateInfo.depthBiasSlopeFactor;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = renderingPipelineCreateInfo.msaaSamples;

	/* Color blend */
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderingPipelineCreateInfo.blendModes.size());
	for (int i(0); i < renderingPipelineCreateInfo.blendModes.size(); ++i)
	{
		if (renderingPipelineCreateInfo.blendModes[i] == RenderingPipelineCreateInfo::BLEND_MODE::TRANS_ADD)
		{
			colorBlendAttachments[i].colorWriteMask = 0xf;
			colorBlendAttachments[i].blendEnable = VK_TRUE;
			colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
		}
		else if (renderingPipelineCreateInfo.blendModes[i] == RenderingPipelineCreateInfo::BLEND_MODE::TRANS_ALPHA)
		{
			colorBlendAttachments[i].colorWriteMask = 0xf;
			colorBlendAttachments[i].blendEnable = VK_TRUE;
			colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
		}
		else
		{
			colorBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
				VK_COLOR_COMPONENT_A_BIT;
			colorBlendAttachments[i].blendEnable = VK_FALSE;
		}
	}

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	/* Create pipeline */
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.renderPass = static_cast<const RenderPassVulkan*>(renderingPipelineCreateInfo.renderPass)->getRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = renderingPipelineCreateInfo.enableDepthTesting;
	depthStencil.depthWriteEnable = renderingPipelineCreateInfo.enableDepthWrite;
	depthStencil.depthCompareOp = renderingPipelineCreateInfo.depthCompareOp;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	pipelineInfo.pDepthStencilState = &depthStencil;

	// Conservative rasterization
	VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterStateCI{};
	if (renderingPipelineCreateInfo.enableConservativeRasterization)
	{
		conservativeRasterStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
		conservativeRasterStateCI.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
		conservativeRasterStateCI.extraPrimitiveOverestimationSize = renderingPipelineCreateInfo.maxExtraPrimitiveOverestimationSize;

		rasterizer.pNext = &conservativeRasterStateCI;
	}

	// Tessellation
	VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};
	if (renderingPipelineCreateInfo.patchControlPoint > 0)
	{
		tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationStateCreateInfo.patchControlPoints = renderingPipelineCreateInfo.patchControlPoint;

		pipelineInfo.pTessellationState = &tessellationStateCreateInfo;
	}

	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	if (!renderingPipelineCreateInfo.dynamicStates.empty())
	{
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pDynamicStates = renderingPipelineCreateInfo.dynamicStates.data();
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(renderingPipelineCreateInfo.dynamicStates.size());

		pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
	}

	if (vkCreateGraphicsPipelines(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error : graphic pipeline creation");

	for (auto& shaderModule : shaderModules)
		vkDestroyShaderModule(g_vulkanInstance->getDevice(), shaderModule, nullptr);
}

Wolf::PipelineVulkan::PipelineVulkan(const ShaderCreateInfo& computeShaderInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts)
{
	m_type = Type::COMPUTE;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(pipelineLayoutInfo.setLayoutCount);
	for (uint32_t i = 0; i < pipelineLayoutInfo.setLayoutCount; ++i)
		vkDescriptorSetLayouts[i] = descriptorSetLayouts[i].operator-><const DescriptorSetLayoutVulkan>()->getDescriptorSetLayout();
	pipelineLayoutInfo.pSetLayouts = vkDescriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(g_vulkanInstance->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");

	/* Shader */
	const VkShaderModule computeShaderModule = createShaderModule(computeShaderInfo.shaderCode);

	VkPipelineShaderStageCreateInfo compShaderStageInfo = {};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = computeShaderModule;
	compShaderStageInfo.pName = computeShaderInfo.entryPointName.data();

	/* Pipeline */
	VkComputePipelineCreateInfo pipelineInfo;
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.stage = compShaderStageInfo;
	pipelineInfo.layout = m_pipelineLayout;
	pipelineInfo.flags = 0;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateComputePipelines(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error : create compute pipeline");

	vkDestroyShaderModule(g_vulkanInstance->getDevice(), computeShaderModule, nullptr);
}

Wolf::PipelineVulkan::PipelineVulkan(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<const DescriptorSetLayout*> descriptorSetLayouts)
{
	m_type = Type::RAY_TRACING;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(pipelineLayoutCreateInfo.setLayoutCount);
	for (uint32_t i = 0; i < pipelineLayoutCreateInfo.setLayoutCount; ++i)
		vkDescriptorSetLayouts[i] = static_cast<const DescriptorSetLayoutVulkan*>(descriptorSetLayouts[i])->getDescriptorSetLayout();
	pipelineLayoutCreateInfo.pSetLayouts = vkDescriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(g_vulkanInstance->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;
	for (auto& shaderCreateInfo : rayTracingPipelineCreateInfo.shaderCreateInfos)
	{
		// Create shader module
		shaderModules.push_back(createShaderModule(shaderCreateInfo.shaderCode));

		// Add stage
		VkPipelineShaderStageCreateInfo shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = shaderCreateInfo.stage;
		shaderStageInfo.module = shaderModules.back();
		shaderStageInfo.pName = shaderCreateInfo.entryPointName.data();

		shaderStages.push_back(shaderStageInfo);
	}

	// Assemble the shader stages and recursion depth info into the raytracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{};
	rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayPipelineInfo.pNext = nullptr;
	rayPipelineInfo.flags = 0;
	rayPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayPipelineInfo.pStages = shaderStages.data();
	rayPipelineInfo.groupCount = static_cast<uint32_t>(rayTracingPipelineCreateInfo.shaderGroupsCreateInfos.size());
	rayPipelineInfo.pGroups = rayTracingPipelineCreateInfo.shaderGroupsCreateInfos.data();
	rayPipelineInfo.maxPipelineRayRecursionDepth = 2;
	rayPipelineInfo.layout = m_pipelineLayout;
	rayPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	rayPipelineInfo.basePipelineIndex = 0;

	if (vkCreateRayTracingPipelinesKHR(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error: create ray tracing pipeline");

	for (const auto& shaderModule : shaderModules)
		vkDestroyShaderModule(g_vulkanInstance->getDevice(), shaderModule, nullptr);
}

Wolf::PipelineVulkan::~PipelineVulkan()
{
	vkDestroyPipeline(g_vulkanInstance->getDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(g_vulkanInstance->getDevice(), m_pipelineLayout, nullptr);
}

VkPipelineBindPoint Wolf::PipelineVulkan::getPipelineBindPoint() const
{
	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	switch (m_type)
	{
	case Type::RENDERING:
		bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		break;
	case Type::COMPUTE:
		bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		break;
	case Type::RAY_TRACING:
		bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
		break;
	}
	return bindPoint;
}

VkShaderModule Wolf::PipelineVulkan::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(g_vulkanInstance->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		Debug::sendError("Error : create shader module");

	return shaderModule;
}
