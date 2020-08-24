/// \file Util.c
/// \author Paolo Mazzon
/// \brief These are "hidden" functions to make certain things simpler
#include <vulkan/vulkan.h>
#include "VK2D/Initializers.h"
#include "VK2D/Structs.h"

// Gets the vertex input information for VK2DVertexTexture
VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState() {
	static VkVertexInputBindingDescription vertexInputBindingDescription;
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[3];
	static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	static bool init = false;

	if (!init) {
		vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVertexTexture), 0);
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertexTexture, pos));
		vertexInputAttributeDescription[1] = vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DVertexTexture, colour));
		vertexInputAttributeDescription[2] = vk2dInitVertexInputAttributeDescription(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VK2DVertexTexture, tex));
		pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 3);
		init = true;
	}

	return pipelineVertexInputStateCreateInfo;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState() {
	static VkVertexInputBindingDescription vertexInputBindingDescription;
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[2];
	static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	static bool init = false;

	if (!init) {
		vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVertexColour), 0);
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertexTexture, pos));
		vertexInputAttributeDescription[1] = vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DVertexTexture, colour));
		pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 2);
		init = true;
	}

	return pipelineVertexInputStateCreateInfo;
}

