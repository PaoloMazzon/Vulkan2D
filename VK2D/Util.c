/// \file Util.c
/// \author Paolo Mazzon
/// \brief These are "hidden" functions to make certain things simpler
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <math.h>
#include "VK2D/Initializers.h"
#include "VK2D/Structs.h"

// Gets the vertex input information for VK2DVertexTexture (Uses static variables to persist attached descriptions)
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

// Gets the vertex input information for VK2DVertexColour (Uses static variables to persist attached descriptions)
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

// Prints a matrix
void _vk2dPrintMatrix(FILE* out, mat4 m, const char* prefix) {
	fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[0], m[4], m[8],  m[12]);
	fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[1], m[5], m[9],  m[13]);
	fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[2], m[6], m[10], m[14]);
	fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[3], m[7], m[11], m[15]);
}

// Prints a UBO all fancy like
void _vk2dPrintUBO(FILE* out, VK2DUniformBufferObject ubo) {
	fprintf(out, "View:\n");
	_vk2dPrintMatrix(out, ubo.view, "    ");
	fprintf(out, "Projection:\n");
	_vk2dPrintMatrix(out, ubo.proj, "    ");
	fflush(out);
}