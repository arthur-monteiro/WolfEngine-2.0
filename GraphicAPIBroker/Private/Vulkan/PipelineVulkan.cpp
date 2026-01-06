#include "PipelineVulkan.h"

#include <Debug.h>

#include "DescriptorSetLayoutVulkan.h"
#include "FormatsVulkan.h"
#include "RenderPassVulkan.h"
#include "ShaderStagesVulkan.h"
#include "Vulkan.h"

VkCompareOp compareOpToVkType(Wolf::CompareOp in)
{
	switch (in)
	{
		case Wolf::CompareOp::NEVER:
			return VK_COMPARE_OP_NEVER;
		case Wolf::CompareOp::LESS:
			return VK_COMPARE_OP_LESS;
		case Wolf::CompareOp::EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case Wolf::CompareOp::LESS_OR_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case Wolf::CompareOp::GREATER:
			return VK_COMPARE_OP_GREATER;
		case Wolf::CompareOp::NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case Wolf::CompareOp::GREATER_OR_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case Wolf::CompareOp::ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
	}

	Wolf::Debug::sendCriticalError("Unsupported compare operation");
	return VK_COMPARE_OP_LESS_OR_EQUAL;
}

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
		Debug::sendCriticalError("Failed to create pipeline layout");

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

		shaderStageInfo.stage = wolfStageBitToVkStageBit(shaderCreateInfo.stage);
		shaderStageInfo.module = shaderModules.back();
		shaderStageInfo.pName = shaderCreateInfo.entryPointName.data();

		shaderStages.push_back(shaderStageInfo);
	}

	/* Input */
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescriptions(renderingPipelineCreateInfo.vertexInputBindingDescriptions.size());
	for (uint32_t i = 0; i < renderingPipelineCreateInfo.vertexInputBindingDescriptions.size(); ++i)
	{
		VertexInputBindingDescription wolfBindingDescription = renderingPipelineCreateInfo.vertexInputBindingDescriptions[i];
		VkVertexInputBindingDescription& vkBindingDescription = vkVertexInputBindingDescriptions[i];

		vkBindingDescription.binding = wolfBindingDescription.binding;
		vkBindingDescription.stride = wolfBindingDescription.stride;
		vkBindingDescription.inputRate = wolfVertexInputRateToVkVertexInputRate(wolfBindingDescription.inputRate);
	}
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vkVertexInputBindingDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = vkVertexInputBindingDescriptions.data();

	std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions(renderingPipelineCreateInfo.vertexInputAttributeDescriptions.size());
	for (uint32_t i = 0; i < renderingPipelineCreateInfo.vertexInputAttributeDescriptions.size(); ++i)
	{
		VertexInputAttributeDescription wolfAttributeDescription = renderingPipelineCreateInfo.vertexInputAttributeDescriptions[i];
		VkVertexInputAttributeDescription& vkAttributeDescription = vkVertexInputAttributeDescriptions[i];

		vkAttributeDescription.binding = wolfAttributeDescription.binding;
		vkAttributeDescription.format = wolfFormatToVkFormat(wolfAttributeDescription.format);
		vkAttributeDescription.offset = wolfAttributeDescription.offset;
		vkAttributeDescription.location = wolfAttributeDescription.location;
	}
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkVertexInputAttributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = vkVertexInputAttributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = wolfPrimitiveTopologyToVkPrimitiveTopology(renderingPipelineCreateInfo.topology);
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
	scissor.extent = { renderingPipelineCreateInfo.extent.width, renderingPipelineCreateInfo.extent.height };

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
	rasterizer.polygonMode = wolfPolygonModeToVkPolygonMode(renderingPipelineCreateInfo.polygonMode);
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = wolfCullModeFlagsToVkCullModeFlags(renderingPipelineCreateInfo.cullModeFlags);
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = renderingPipelineCreateInfo.depthBiasConstantFactor > 0.0f || renderingPipelineCreateInfo.depthBiasSlopeFactor > 0.0f;
	rasterizer.depthBiasConstantFactor = renderingPipelineCreateInfo.depthBiasConstantFactor;
	rasterizer.depthBiasSlopeFactor = renderingPipelineCreateInfo.depthBiasSlopeFactor;

	/* Multisampling */
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = wolfSampleCountFlagBitsToVkSampleCountFlagBits(renderingPipelineCreateInfo.msaaSamplesFlagBit);

	/* Color blend */
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(renderingPipelineCreateInfo.blendModes.size());
	for (uint32_t i = 0; i < renderingPipelineCreateInfo.blendModes.size(); ++i)
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
	depthStencil.depthCompareOp = compareOpToVkType(renderingPipelineCreateInfo.depthCompareOp);
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
	std::vector<VkDynamicState> vkDynamicStates;
	if (!renderingPipelineCreateInfo.dynamicStates.empty())
	{
		vkDynamicStates.reserve(renderingPipelineCreateInfo.dynamicStates.size());
		for (const DynamicState& dynamicState : renderingPipelineCreateInfo.dynamicStates)
		{
			vkDynamicStates.push_back(wolfDynamicStateToVkDynamicState(dynamicState));
		}

		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.pDynamicStates = vkDynamicStates.data();
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(vkDynamicStates.size());

		pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
	}

	if (vkCreateGraphicsPipelines(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendCriticalError("Failed to create graphic pipeline");

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
		Debug::sendCriticalError("Failed to create pipeline layout");

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
		Debug::sendCriticalError("Failed to create compute pipeline");

	vkDestroyShaderModule(g_vulkanInstance->getDevice(), computeShaderModule, nullptr);
}

Wolf::PipelineVulkan::PipelineVulkan(const RayTracingPipelineCreateInfo& rayTracingPipelineCreateInfo, std::span<ResourceReference<const DescriptorSetLayout>> descriptorSetLayouts)
{
	m_type = Type::RAY_TRACING;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts(pipelineLayoutCreateInfo.setLayoutCount);
	for (uint32_t i = 0; i < pipelineLayoutCreateInfo.setLayoutCount; ++i)
		vkDescriptorSetLayouts[i] = descriptorSetLayouts[i].operator-><const DescriptorSetLayoutVulkan>()->getDescriptorSetLayout();
	pipelineLayoutCreateInfo.pSetLayouts = vkDescriptorSetLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(g_vulkanInstance->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		Debug::sendCriticalError("Failed to create pipeline layout");

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	std::vector<VkShaderModule> shaderModules;
	for (auto& shaderCreateInfo : rayTracingPipelineCreateInfo.shaderCreateInfos)
	{
		// Create shader module
		shaderModules.push_back(createShaderModule(shaderCreateInfo.shaderCode));

		// Add stage
		VkPipelineShaderStageCreateInfo shaderStageInfo = {};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = wolfStageBitToVkStageBit(shaderCreateInfo.stage);
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

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> vkShaderGroups(rayTracingPipelineCreateInfo.shaderGroupsCreateInfos.size());
	for (uint32_t i = 0; i < rayTracingPipelineCreateInfo.shaderGroupsCreateInfos.size(); ++i)
	{
		VkRayTracingShaderGroupCreateInfoKHR& vkShaderGroup = vkShaderGroups[i];
		RayTracingShaderGroupCreateInfo wolfShaderGroup = rayTracingPipelineCreateInfo.shaderGroupsCreateInfos[i];

		vkShaderGroup = {};
		vkShaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		vkShaderGroup.type = wolfRayTracingShaderGroupTypeToVkRayTracingShaderGroupType(wolfShaderGroup.type);
		vkShaderGroup.anyHitShader = wolfShaderGroup.anyHitShader;
		vkShaderGroup.closestHitShader = wolfShaderGroup.closestHitShader;
		vkShaderGroup.generalShader = wolfShaderGroup.generalShader;
		vkShaderGroup.intersectionShader = wolfShaderGroup.intersectionShader;
	}

	rayPipelineInfo.groupCount = static_cast<uint32_t>(vkShaderGroups.size());
	rayPipelineInfo.pGroups = vkShaderGroups.data();
	rayPipelineInfo.maxPipelineRayRecursionDepth = 2;
	rayPipelineInfo.layout = m_pipelineLayout;
	rayPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	rayPipelineInfo.basePipelineIndex = 0;

	if (vkCreateRayTracingPipelinesKHR(g_vulkanInstance->getDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		Debug::sendCriticalError("Failed to create ray tracing pipeline");

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

VkPolygonMode Wolf::PipelineVulkan::wolfPolygonModeToVkPolygonMode(PolygonMode polygonMode)
{
	switch (polygonMode)
	{
		case PolygonMode::FILL:
			return VK_POLYGON_MODE_FILL;
		case PolygonMode::LINE:
			return VK_POLYGON_MODE_LINE;
		case PolygonMode::POINT:
			return VK_POLYGON_MODE_POINT;
	}

	Debug::sendError("Unhandled polygon mode");
	return VK_POLYGON_MODE_MAX_ENUM;
}

VkPrimitiveTopology Wolf::PipelineVulkan::wolfPrimitiveTopologyToVkPrimitiveTopology(PrimitiveTopology primitiveTopology)
{
	switch (primitiveTopology)
	{
		case PrimitiveTopology::TRIANGLE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case PrimitiveTopology::PATCH_LIST:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		case PrimitiveTopology::LINE_LIST:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	}

	Debug::sendError("Unhandled primitive topology");
	return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

VkCullModeFlagBits Wolf::PipelineVulkan::wolfCullModeFlagBitsToVkCullModeFlagBits(CullModeFlagBits cullModeFlagBits)
{
	switch (cullModeFlagBits)
	{
		case NONE:
			return VK_CULL_MODE_NONE;
		case BACK:
			return VK_CULL_MODE_BACK_BIT;
		case FRONT:
			return VK_CULL_MODE_FRONT_BIT;
		case CULL_MODE_FLAG_BITS_MAX:
			return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	}

	Debug::sendError("Unhandled cull mode");
	return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
}

VkCullModeFlags Wolf::PipelineVulkan::wolfCullModeFlagsToVkCullModeFlags(CullModeFlags cullModeFlags)
{
	VkCullModeFlags vkCullModeFlags = 0;

#define ADD_FLAG_IF_PRESENT(flag) if (cullModeFlags & (flag)) vkCullModeFlags |= wolfCullModeFlagBitsToVkCullModeFlagBits(flag)

	for (uint32_t flag = 1; flag < CULL_MODE_FLAG_BITS_MAX; flag <<= 1)
		ADD_FLAG_IF_PRESENT(static_cast<CullModeFlagBits>(flag));

#undef ADD_FLAG_IF_PRESENT

	return vkCullModeFlags;
}

VkDynamicState Wolf::PipelineVulkan::wolfDynamicStateToVkDynamicState(DynamicState dynamicState)
{
	switch (dynamicState)
	{
		case DynamicState::VIEWPORT:
			return VK_DYNAMIC_STATE_VIEWPORT;
	}

	Debug::sendError("Unhandled dynamic state");
	return VK_DYNAMIC_STATE_MAX_ENUM;
}

VkVertexInputRate Wolf::PipelineVulkan::wolfVertexInputRateToVkVertexInputRate(VertexInputRate vertexInputRate)
{
	switch (vertexInputRate)
	{
		case VertexInputRate::VERTEX:
			return VK_VERTEX_INPUT_RATE_VERTEX;
		case VertexInputRate::INSTANCE:
			return VK_VERTEX_INPUT_RATE_INSTANCE;
	}

	Debug::sendError("Unhandled vertex input rate");
	return VK_VERTEX_INPUT_RATE_MAX_ENUM;
}

VkRayTracingShaderGroupTypeKHR Wolf::PipelineVulkan::wolfRayTracingShaderGroupTypeToVkRayTracingShaderGroupType(RayTracingShaderGroupType rayTracingShaderGroup)
{
	switch (rayTracingShaderGroup)
	{
		case RayTracingShaderGroupType::GENERAL:
			return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		case RayTracingShaderGroupType::TRIANGLES_HIT_GROUP:
			return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
	}

	Debug::sendError("Unhandled ray tracing group type");
	return VK_RAY_TRACING_SHADER_GROUP_TYPE_MAX_ENUM_KHR;
}
