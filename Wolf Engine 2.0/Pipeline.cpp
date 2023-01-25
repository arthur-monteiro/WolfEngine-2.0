#include "Pipeline.h"

Wolf::Pipeline::Pipeline(const RenderingPipelineCreateInfo& renderingPipelineCreateInfo)
{
	/* Pipeline layout */
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(renderingPipelineCreateInfo.descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = renderingPipelineCreateInfo.descriptorSetLayouts.data();
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
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

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
			colorBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_MAX;
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
	pipelineInfo.renderPass = renderingPipelineCreateInfo.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = renderingPipelineCreateInfo.enableDepthTesting;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	pipelineInfo.pDepthStencilState = &depthStencil;

	// Conservative rasterization
	if (renderingPipelineCreateInfo.enableConservativeRasterization)
	{
		VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterStateCI{};
		conservativeRasterStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
		conservativeRasterStateCI.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
		conservativeRasterStateCI.extraPrimitiveOverestimationSize = renderingPipelineCreateInfo.maxExtraPrimitiveOverestimationSize;

		rasterizer.pNext = &conservativeRasterStateCI;
	}

	// Tessellation
	if (renderingPipelineCreateInfo.patchControlPoint > 0)
	{
		VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};
		tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationStateCreateInfo.patchControlPoints = renderingPipelineCreateInfo.patchControlPoint;

		pipelineInfo.pTessellationState = &tessellationStateCreateInfo;
	}

	if (vkCreateGraphicsPipelines(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendError("Error : graphic pipeline creation");

	for (auto& shaderModule : shaderModules)
		vkDestroyShaderModule(g_vulkanInstance->getDevice(), shaderModule, nullptr);
}

Wolf::Pipeline::Pipeline(const ShaderCreateInfo& computeShaderInfo, std::span<VkDescriptorSetLayout> descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(g_vulkanInstance->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendError("Error : create pipeline layout");

	/* Shader */
	VkShaderModule computeShaderModule = createShaderModule(computeShaderInfo.shaderCode);

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
}

Wolf::Pipeline::~Pipeline()
{
	vkDestroyPipeline(g_vulkanInstance->getDevice(), m_pipeline, nullptr);
	vkDestroyPipelineLayout(g_vulkanInstance->getDevice(), m_pipelineLayout, nullptr);
}

VkShaderModule Wolf::Pipeline::createShaderModule(const std::span<char>& code)
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
