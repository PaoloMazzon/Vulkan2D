/// \file Util.c
/// \author Paolo Mazzon
#include "Util.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "VK2D/Initializers.h"
#include "VK2D/Structs.h"

// Gets the vertex input information for VK2DVertexTexture (Uses static variables to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState() {
	return vk2dInitPipelineVertexInputStateCreateInfo(VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 0);;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetModelVertexInputState() {
	static VkVertexInputBindingDescription vertexInputBindingDescription;
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[2];
	static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	static bool init = false;

	if (!init) {
		vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVertex3D), 0);
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertex3D, pos));
		vertexInputAttributeDescription[1] = vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VK2DVertex3D, uv));
		pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 2);
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
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertexColour, pos));
		vertexInputAttributeDescription[1] = vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DVertexColour, colour));
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
	fprintf(out, "View-Proj:\n");
	_vk2dPrintMatrix(out, ubo.viewproj, "    ");
	fflush(out);
}

bool _vk2dFileExists(const char *filename) {
	FILE* file = fopen(filename, "r");
	bool out = file == NULL ? false : true;
	if (file != NULL) {fclose(file);}
	return out;
}

unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size) {
	FILE* file = fopen(filename, "rb");
	unsigned char *buffer = NULL;
	*size = 0;

	if (file != NULL) {
		// Find file size
		fseek(file, 0, SEEK_END);
		*size = ftell(file);
		rewind(file);

		buffer = malloc(*size);

		if (buffer != NULL) {
			// Fill the buffer
			fread(buffer, 1, *size, file);
		}
		fclose(file);
	}

	return buffer;
}

unsigned char *_vk2dCopyBuffer(void *buffer, int size) {
	char *new = NULL;
	if (buffer != NULL && size != 0) {
		new = malloc(size);
		if (new != NULL) {
			memcpy(new, buffer, size);
		}
	}
	return new;
}