/// \file Pipeline.c
/// \author Paolo Mazzon
#include "VK2D/Pipeline.h"

VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkPipelineVertexInputStateCreateInfo vertexInfo, VkPipelineRasterizationStateCreateInfo rasterizationInfo, VkPipelineMultisampleStateCreateInfo msaaInfo) {
	return NULL; // TODO: This
}

void vk2dPipelineBeginBuffer(VK2DPipeline pipe, VkCommandBuffer buffer, float blendConstants[4], VkViewport *viewports, uint32_t viewportCount, float lineWidth) {

}

void vk2dPipelineFree(VK2DPipeline pipe) {

}