/// \file Util.c
/// \author Paolo Mazzon
#include "Util.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "VK2D/Initializers.h"
#include "VK2D/Validation.h"
#include "VK2D/Structs.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/Texture.h"
#include "VK2D/Shader.h"
#include "VK2D/Model.h"

static float gLoadStatus = 0;

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

extern VK2DLogicalDevice gDeviceFromMainThread;
int _vk2dWorkerThread(void *data) {
	// Data is the logical device
	VK2DLogicalDevice dev = gDeviceFromMainThread;
	int loaded = 0;

	// Setup the command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo2 = vk2dInitCommandPoolCreateInfo(dev->pd->QueueFamily.graphicsFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	vk2dErrorCheck(vkCreateCommandPool(dev->dev, &commandPoolCreateInfo2, VK_NULL_HANDLE, &dev->loadPool));

	while (!dev->quitThread) {
		if (dev->loads > 0) {
			// Find an asset to load
			VK2DAssetLoad asset = {0};
			int spot = -1;
			bool found = false;
			SDL_LockMutex(dev->loadListMutex);
			for (int i = 0; i < dev->loadListSize && !found; i++) {
				if (dev->loadList[i].state == VK2D_ASSET_TYPE_ASSET) {
					spot = i;
					// This ensures that models will always be prioritized last
					if (dev->loadList[i].type != VK2D_ASSET_TYPE_MODEL_FILE && dev->loadList[i].type != VK2D_ASSET_TYPE_MODEL_MEMORY)
						found = true;
				}
			}

			// Remove this asset from the list kinda
			dev->loadList[spot].state = VK2D_ASSET_TYPE_PENDING;
			memcpy(&asset, &dev->loadList[spot], sizeof(VK2DAssetLoad));

			dev->loads -= 1;
			SDL_UnlockMutex(dev->loadListMutex);

			// Now we load the asset based on its type
			if (asset.type == VK2D_ASSET_TYPE_TEXTURE_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.texture = _vk2dTextureFromInternal(fileData, size, false);
				if (*asset.Output.texture == NULL)
					vk2dLogMessage("Failed to load texture \"%s\".", asset.Load.filename);
				free(data);
			} else if (asset.type == VK2D_ASSET_TYPE_TEXTURE_MEMORY) {
				*asset.Output.texture = _vk2dTextureFromInternal(asset.Load.data, asset.Load.size, false);
				if (*asset.Output.texture == NULL)
					vk2dLogMessage("Failed to load texture from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.model = _vk2dModelFromInternal(fileData, size, *asset.Data.Model.tex, false);
				if (*asset.Output.model == NULL)
					vk2dLogMessage("Failed to load model \"%s\".", asset.Load.filename);
				free(data);
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_MEMORY) {
				*asset.Output.model = _vk2dModelFromInternal(asset.Load.data, asset.Load.size, *asset.Data.Model.tex, false);
				if (*asset.Output.model == NULL)
					vk2dLogMessage("Failed to load model from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_FILE) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderLoad(asset.Load.filename, asset.Load.fragmentFilename, asset.Data.Shader.uniformBufferSize);
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_MEMORY) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderFrom(asset.Load.data, asset.Load.size, asset.Load.fragmentData, asset.Load.fragmentSize, asset.Data.Shader.uniformBufferSize);
			}

			loaded++;
			gLoadStatus = (float)loaded / (float)dev->loadListSize;
		}

		// Signify the end of loading
		if (dev->loads == 0 && loaded > 0) {
			SDL_LockMutex(dev->loadListMutex);

			dev->doneLoading = true;

			// We now don't need the list anymore so we can delete it
			free(dev->loadList);
			dev->loadList = NULL;
			dev->loads = 0;
			dev->loadListSize = 0;
			SDL_UnlockMutex(dev->loadListMutex);
		}
	}

	return 0;
}

void vk2dAssetsLoad(VK2DAssetLoad *assets, uint32_t count) {
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	// We only accept lists when the current one is done
	if (dev->loadListSize > 0)
		return;

	dev->doneLoading = false;
	SDL_LockMutex(dev->loadListMutex);
	dev->loadListSize = count;
	dev->loads = count;
	dev->loadList = malloc(sizeof(VK2DAssetLoad) * count);
	vk2dPointerCheck(dev->loadList);
	memcpy(dev->loadList, assets, sizeof(VK2DAssetLoad) * count);
	SDL_UnlockMutex(dev->loadListMutex);
}

void vk2dAssetsWait() {
	VK2DLogicalDevice dev = vk2dRendererGetDevice();
	while (!dev->doneLoading) {
		volatile int i = 0;
	}
}

bool vk2dAssetsLoadComplete() {
	return vk2dRendererGetDevice()->doneLoading;
}

float vk2dAssetsLoadStatus() {
	return gLoadStatus;
}

void vk2dAssetsFree(VK2DAssetLoad *assets, uint32_t count) {
	vk2dRendererWait();
	for (int i = 0; i < count; i++) {
		if (assets[i].type == VK2D_ASSET_TYPE_TEXTURE_FILE || assets[i].type == VK2D_ASSET_TYPE_TEXTURE_MEMORY)
			vk2dTextureFree(*assets[i].Output.texture);
		else if (assets[i].type == VK2D_ASSET_TYPE_SHADER_FILE || assets[i].type == VK2D_ASSET_TYPE_SHADER_MEMORY)
			vk2dShaderFree(*assets[i].Output.shader);
		else if (assets[i].type == VK2D_ASSET_TYPE_MODEL_FILE || assets[i].type == VK2D_ASSET_TYPE_MODEL_MEMORY)
			vk2dModelFree(*assets[i].Output.model);
	}
}