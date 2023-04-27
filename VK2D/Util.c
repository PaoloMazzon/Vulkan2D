/// \file Util.c
/// \author Paolo Mazzon
#include "Util.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "VK2D/Initializers.h"
#include "VK2D/Structs.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"

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

VkPipelineVertexInputStateCreateInfo _vk2dGetInstanceVertexInputState() {
	static VkVertexInputBindingDescription vertexInputBindingDescription;
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[7];
	static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	static bool init = false;

	if (!init) {
		vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(VK_VERTEX_INPUT_RATE_INSTANCE, sizeof(VK2DDrawInstance), 0);
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, texturePos));
		vertexInputAttributeDescription[1] = vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, colour));
		vertexInputAttributeDescription[2] = vk2dInitVertexInputAttributeDescription(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, pos));
		vertexInputAttributeDescription[3] = vk2dInitVertexInputAttributeDescription(3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, model));
		vertexInputAttributeDescription[4] = vk2dInitVertexInputAttributeDescription(4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, model) + (sizeof(vec4) * 1));
		vertexInputAttributeDescription[5] = vk2dInitVertexInputAttributeDescription(5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, model) + (sizeof(vec4) * 2));
		vertexInputAttributeDescription[6] = vk2dInitVertexInputAttributeDescription(6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VK2DDrawInstance, model) + (sizeof(vec4) * 3));
		pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 7);
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
	unsigned char *new = NULL;
	if (buffer != NULL && size != 0) {
		new = malloc(size);
		if (new != NULL) {
			memcpy(new, buffer, size);
		}
	}
	return new;
}

int _vk2dWorkerThread(void *data) {
	// Data is the logical device
	VK2DLogicalDevice dev = data;
	int loaded = 0;

	while (!dev->quitThread) {
		if (dev->loads > 0) {
			// First we must find the asset to load
			VK2DAssetLoad asset = {0};
			SDL_LockMutex(dev->loadListMutex);
			for (int i = 0; i < dev->loadListSize; i++) {
				if (dev->loadList[i].type == VK2D_ASSET_TYPE_ASSET) {
					asset = dev->loadList[i];
					dev->loadList[i].state = VK2D_ASSET_TYPE_PENDING;
				}
			}
			dev->loads -= 1;
			SDL_UnlockMutex(dev->loadListMutex);

			// Now we load the asset based on its type
			// TODO: This
			loaded++;
		}

		// If we're done loading all the assets we need to setup a pipeline barrier
		// for every asset in the list in the pending state then trip the fence at
		// the end of it
		if (dev->loads == 0 && loaded > 0) {
			// TODO: This
		}
	}

	return 0;
}

void vk2dQueueAssetLoad(VK2DAssetLoad *assets, uint32_t count) {
	vkResetFences(vk2dRendererGetDevice()->dev, 1, &vk2dRendererGetDevice()->loadFence);
	// TODO: This
}

void vk2dWaitAssets() {
	vkWaitForFences(vk2dRendererGetDevice()->dev, 1, &vk2dRendererGetDevice()->loadFence, true, UINT64_MAX);
}