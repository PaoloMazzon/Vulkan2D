/// \file Pipeline.c
/// \author Paolo Mazzon
#include "VK2D/Pipeline.h"
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/BlendModes.h"
#include <malloc.h>
#include "VK2D/Renderer.h"
#include "VK2D/Opaque.h"

VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, uint32_t width, uint32_t height, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkDescriptorSetLayout *setLayouts, uint32_t layoutCount, VkPipelineVertexInputStateCreateInfo *vertexInfo, bool fill, VK2DMSAA msaa, VK2DPipelineType type) {
    if (vk2dStatusFatal())
        return NULL;

	VK2DPipeline pipe = calloc(1, sizeof(struct VK2DPipeline_t));
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;

	if (pipe != NULL) {
		// Figure out if wireframe is allowed
		bool polygonFill = fill;
		if (!polygonFill && !gRenderer->limits.supportsWireframe)
			polygonFill = true;

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
		VkResult result = vkCreateShaderModule(dev->dev, &vertCreateInfo, VK_NULL_HANDLE, &vertShader);
		VkResult result2 = vkCreateShaderModule(dev->dev, &fragCreateInfo, VK_NULL_HANDLE, &fragShader);
		const uint32_t shaderStageCount = 2;
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo[] = {
				vk2dInitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShader),
				vk2dInitPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader),
		};

        if (result != VK_SUCCESS || result2 != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create shader modules, Vulkan error %i/%i.", result, result2);
            free(pipe);
            pipe = NULL;
            return NULL;
        }

		VkRect2D scissor = {0};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = width;
		scissor.extent.height = height;
		VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = vk2dInitPipelineViewportStateCreateInfo(VK_NULL_HANDLE, &scissor);
		VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = vk2dInitPipelineRasterizationStateCreateInfo(polygonFill);

		VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = vk2dInitPipelineMultisampleStateCreateInfo((VkSampleCountFlagBits)msaa);
		VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = vk2dInitPipelineDepthStencilStateCreateInfo();

		const uint32_t stateCount = 3;
		VkDynamicState states[] = {
				VK_DYNAMIC_STATE_LINE_WIDTH,
				VK_DYNAMIC_STATE_SCISSOR,
				VK_DYNAMIC_STATE_VIEWPORT,
		};
		VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = vk2dInitPipelineDynamicStateCreateInfo(states, stateCount);
		VkPushConstantRange range = {0};
        if (type == VK2D_PIPELINE_TYPE_3D) {
            range.size = sizeof(VK2D3DPushBuffer);
        } else if (type == VK2D_PIPELINE_TYPE_DEFAULT) {
            range.size = sizeof(VK2DPushBuffer);
        } else if (type == VK2D_PIPELINE_TYPE_SHADOWS) {
            range.size = sizeof(VK2DShadowsPushBuffer);
        }
        range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		if (type != VK2D_PIPELINE_TYPE_INSTANCING)
			pipelineLayoutCreateInfo = vk2dInitPipelineLayoutCreateInfo(setLayouts, layoutCount, 1, &range);
		else
			pipelineLayoutCreateInfo = vk2dInitPipelineLayoutCreateInfo(setLayouts, layoutCount, 0, VK_NULL_HANDLE);
		result = vkCreatePipelineLayout(dev->dev, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipe->layout);
		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = vk2dInitPipelineInputAssemblyStateCreateInfo(fill);

        if (result != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create pipeline layout, Vulkan error %i.", result);
            free(pipe);
            pipe = NULL;
            vkDestroyShaderModule(dev->dev, vertShader, VK_NULL_HANDLE);
            vkDestroyShaderModule(dev->dev, fragShader, VK_NULL_HANDLE);
            return NULL;
        }

		// 3D/shadow settings
		if (type == VK2D_PIPELINE_TYPE_3D) {
			pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
			pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
		} else if (type == VK2D_PIPELINE_TYPE_SHADOWS) {
            pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

		for (i = 0; i < VK2D_BLEND_MODE_MAX; i++) {
			VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = vk2dInitPipelineColorBlendStateCreateInfo(&VK2D_BLEND_MODES[i], 1);
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
			result = vkCreateGraphicsPipelines(dev->dev, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, &pipe->pipes[i]);
			if (result != VK_SUCCESS) {
			    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create pipeline, Vulkan error %i.", result);
			    break;
			}
		}

		vkDestroyShaderModule(dev->dev, vertShader, VK_NULL_HANDLE);
		vkDestroyShaderModule(dev->dev, fragShader, VK_NULL_HANDLE);
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate pipeline struct.");
	}

	return pipe;
}

VkPipeline vk2dPipelineGetPipe(VK2DPipeline pipe, VK2DBlendMode blendMode) {
    if (vk2dStatusFatal())
        return NULL;
	return pipe->pipes[blendMode];
}

void vk2dPipelineFree(VK2DPipeline pipe) {
	if (pipe != NULL) {
		vkDestroyPipelineLayout(pipe->dev->dev, pipe->layout, VK_NULL_HANDLE);
		uint32_t i;
		for (i = 0; i < VK2D_BLEND_MODE_MAX; i++)
			vkDestroyPipeline(pipe->dev->dev, pipe->pipes[i], VK_NULL_HANDLE);
		free(pipe);
	}
}