/// \file Pipeline.c
/// \author Paolo Mazzon
#include "VK2D/Pipeline.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/LogicalDevice.h"
#include <malloc.h>

VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, uint32_t width, uint32_t height, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkDescriptorSetLayout setLayout, VkPipelineVertexInputStateCreateInfo *vertexInfo, bool fill, VK2DMSAA msaa) {
	VK2DPipeline pipe = malloc(sizeof(struct VK2DPipeline));
	uint32_t i;

	if (vk2dPointerCheck(pipe)) {
		// Load pipeline base values
		pipe->dev = dev;
		pipe->renderPass = renderPass;
		pipe->rect.offset.x = 0;
		pipe->rect.offset.y = 0;
		pipe->rect.extent.width = width;
		pipe->rect.extent.height = height;
		pipe->clearValue[0].depthStencil.depth = 1;
		pipe->clearValue[0].depthStencil.stencil = 0;
		pipe->clearValue[1].color.int32[0] = 0;
		pipe->clearValue[1].color.int32[1] = 0;
		pipe->clearValue[1].color.int32[2] = 0;
		pipe->clearValue[1].color.int32[3] = 0;

		// Create the shader modules
		VkShaderModuleCreateInfo vertCreateInfo = vk2dInitShaderModuleCreateInfo((void*)vertBuffer, vertSize);
		VkShaderModuleCreateInfo fragCreateInfo = vk2dInitShaderModuleCreateInfo((void*)fragBuffer, fragSize);
		VkShaderModule vertShader, fragShader;
		vkCreateShaderModule(dev->dev, &vertCreateInfo, VK_NULL_HANDLE, &vertShader);
		vkCreateShaderModule(dev->dev, &fragCreateInfo, VK_NULL_HANDLE, &fragShader);
		const uint32_t shaderStageCount = 2;
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[] = {
				vk2dInitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
				vk2dInitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader),
		};

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = width;
		scissor.extent.height = height;
		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = vk2dInitPipelineViewportStateCreateInfo(VK_NULL_HANDLE, &scissor);
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = vk2dInitPipelineRasterizationStateCreateInfo(fill);
		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = vk2dInitPipelineMultisampleStateCreateInfo((VkSampleCountFlagBits)msaa);
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = vk2dInitPipelineDepthStencilStateCreateInfo();

		const uint32_t colourAttachCount = 1;
		VkPipelineColorBlendAttachmentState colourBlendAttachments[colourAttachCount];
		for (i = 0; i < colourAttachCount; i++) {
			colourBlendAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colourBlendAttachments[i].blendEnable = VK_TRUE;
			colourBlendAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colourBlendAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colourBlendAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
			colourBlendAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colourBlendAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colourBlendAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;
		}
		VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = vk2dInitPipelineColorBlendStateCreateInfo(colourBlendAttachments, colourAttachCount);

		const uint32_t stateCount = 3;
		VkDynamicState states[] = {
				VK_DYNAMIC_STATE_BLEND_CONSTANTS,
				VK_DYNAMIC_STATE_LINE_WIDTH,
				VK_DYNAMIC_STATE_VIEWPORT
		};
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = vk2dInitPipelineDynamicStateCreateInfo(states, stateCount);
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vk2dInitPipelineLayoutCreateInfo(&setLayout, 1);
		vkCreatePipelineLayout(dev->dev, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipe->layout);
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = vk2dInitPipelineInputAssemblyStateCreateInfo();


		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = vk2dInitGraphicsPipelineCreateInfo(
				shaderStageCreateInfo,
				shaderStageCount,
				vertexInfo,
				&pipelineInputAssemblyStateCreateInfo,
				&pipelineViewportStateCreateInfo,
				&pipelineRasterizationStateCreateInfo,
				&pipelineMultisampleStateCreateInfo,
				&pipelineDepthStencilStateCreateInfo,
				&pipelineColorBlendStateCreateInfo,
				&pipelineDynamicStateCreateInfo,
				pipe->layout,
				renderPass);
		vk2dErrorCheck(vkCreateGraphicsPipelines(dev->dev, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &pipe->pipe));

		vkDestroyShaderModule(dev->dev, vertShader, VK_NULL_HANDLE);
		vkDestroyShaderModule(dev->dev, fragShader, VK_NULL_HANDLE);
	}

	return pipe;
}

void vk2dPipelineFree(VK2DPipeline pipe) {
	if (pipe != NULL) {
		vkDestroyPipelineLayout(pipe->dev->dev, pipe->layout, VK_NULL_HANDLE);
		vkDestroyPipeline(pipe->dev->dev, pipe->pipe, VK_NULL_HANDLE);
		free(pipe);
	}
}