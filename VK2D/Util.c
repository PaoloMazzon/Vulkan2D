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
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[8];
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
        vertexInputAttributeDescription[7] = vk2dInitVertexInputAttributeDescription(7, 0, VK_FORMAT_R32_UINT, offsetof(VK2DDrawInstance, textureIndex));
        pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 8);
		init = true;
	}

	return pipelineVertexInputStateCreateInfo;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetShadowsVertexInputState() {
	static VkVertexInputBindingDescription vertexInputBindingDescription;
	static VkVertexInputAttributeDescription vertexInputAttributeDescription[1];
	static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	static bool init = false;

	if (!init) {
		vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(VK_VERTEX_INPUT_RATE_VERTEX, sizeof(vec3), 0);
		vertexInputAttributeDescription[0] = vk2dInitVertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
		pipelineVertexInputStateCreateInfo = vk2dInitPipelineVertexInputStateCreateInfo(&vertexInputBindingDescription, 1, vertexInputAttributeDescription, 1);
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
		} else {
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Unable to allocate buffer for file \"%s\".\n", filename);
		}
		fclose(file);
	} else {
	    vk2dRaise(VK2D_STATUS_FILE_NOT_FOUND, "File \"%s\" was unable to be opened.\n", filename);
	}

	return buffer;
}

unsigned char *_vk2dCopyBuffer(void *buffer, int size) {
	unsigned char *new = NULL;
	if (buffer != NULL && size != 0) {
		new = malloc(size);
		if (new != NULL) {
			memcpy(new, buffer, size);
		} else {
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Unable to copy buffer of size %i bytes.\n", size);
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
	VkResult result = vkCreateCommandPool(dev->dev, &commandPoolCreateInfo2, VK_NULL_HANDLE, &dev->loadPool);
    if (result != VK_SUCCESS) {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create command buffer for worker thread.");
    }
	while (SDL_AtomicGet(&dev->quitThread) == 0) {
		if (SDL_AtomicGet(&dev->loads) > 0) {
			// Find an asset to load
			VK2DAssetLoad asset = {0};
			int spot = -1;
			bool found = false;
			SDL_LockMutex(dev->loadListMutex);
			for (int i = 0; i < SDL_AtomicGet(&dev->loadListSize) && !found; i++) {
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

			SDL_AtomicAdd(&dev->loads, -1);
			SDL_UnlockMutex(dev->loadListMutex);

			// Now we load the asset based on its type
			if (asset.type == VK2D_ASSET_TYPE_TEXTURE_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.texture = _vk2dTextureFromInternal(fileData, size, false);
				if (*asset.Output.texture == NULL)
                    vk2dLog("Failed to load texture \"%s\".", asset.Load.filename);
				free(fileData);
			} else if (asset.type == VK2D_ASSET_TYPE_TEXTURE_MEMORY) {
				*asset.Output.texture = _vk2dTextureFromInternal(asset.Load.data, asset.Load.size, false);
				if (*asset.Output.texture == NULL)
                    vk2dLog("Failed to load texture from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.model = _vk2dModelFromInternal(fileData, size, *asset.Data.Model.tex, false);
				if (*asset.Output.model == NULL)
                    vk2dLog("Failed to load model \"%s\".", asset.Load.filename);
				free(fileData);
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_MEMORY) {
				*asset.Output.model = _vk2dModelFromInternal(asset.Load.data, asset.Load.size, *asset.Data.Model.tex, false);
				if (*asset.Output.model == NULL)
                    vk2dLog("Failed to load model from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_FILE) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderLoad(asset.Load.filename, asset.Load.fragmentFilename, asset.Data.Shader.uniformBufferSize);
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_MEMORY) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderFrom(asset.Load.data, asset.Load.size, asset.Load.fragmentData, asset.Load.fragmentSize, asset.Data.Shader.uniformBufferSize);
			}

			loaded++;
			gLoadStatus = (float)loaded / (float)SDL_AtomicGet(&dev->loadListSize);
		}

		// Signify the end of loading
		if (SDL_AtomicGet(&dev->loads) == 0 && loaded > 0) {
			SDL_LockMutex(dev->loadListMutex);

			SDL_AtomicSet(&dev->doneLoading, 1);

			// We now don't need the list anymore so we can delete it
			free(dev->loadList);
			dev->loadList = NULL;
			SDL_AtomicSet(&dev->loads, 0);
			SDL_AtomicSet(&dev->loadListSize, 0);
			SDL_UnlockMutex(dev->loadListMutex);
		}
	}

	return 0;
}

void vk2dAssetsLoad(VK2DAssetLoad *assets, uint32_t count) {
	VK2DLogicalDevice dev = vk2dRendererGetDevice();
	VK2DRenderer gRenderer = vk2dRendererGetPointer();

	if (gRenderer->limits.supportsMultiThreadLoading) {
		// We only accept lists when the current one is done
		if (SDL_AtomicGet(&dev->loadListSize) > 0)
			return;

		SDL_AtomicSet(&dev->doneLoading, 0);
		SDL_LockMutex(dev->loadListMutex);
		SDL_AtomicSet(&dev->loadListSize, count);
		SDL_AtomicSet(&dev->loads, count);
		dev->loadList = malloc(sizeof(VK2DAssetLoad) * count);
		if (dev->loadList == NULL) {
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create load list for worker thread.");
		}
		memcpy(dev->loadList, assets, sizeof(VK2DAssetLoad) * count);
		SDL_UnlockMutex(dev->loadListMutex);
	} else {
		for (int i = 0; i < count; i++) {
			VK2DAssetLoad asset;
			memcpy(&asset, &assets[i], sizeof(struct VK2DAssetLoad));
			if (asset.type == VK2D_ASSET_TYPE_TEXTURE_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.texture = _vk2dTextureFromInternal(fileData, size, true);
				if (*asset.Output.texture == NULL)
                    vk2dLog("Failed to load texture \"%s\".", asset.Load.filename);
				free(fileData);
			} else if (asset.type == VK2D_ASSET_TYPE_TEXTURE_MEMORY) {
				*asset.Output.texture = _vk2dTextureFromInternal(asset.Load.data, asset.Load.size, true);
				if (*asset.Output.texture == NULL)
                    vk2dLog("Failed to load texture from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_FILE) {
				uint32_t size;
				uint8_t *fileData = _vk2dLoadFile(asset.Load.filename, &size);
				*asset.Output.model = _vk2dModelFromInternal(fileData, size, *asset.Data.Model.tex, true);
				if (*asset.Output.model == NULL)
                    vk2dLog("Failed to load model \"%s\".", asset.Load.filename);
				free(fileData);
			} else if (asset.type == VK2D_ASSET_TYPE_MODEL_MEMORY) {
				*asset.Output.model = _vk2dModelFromInternal(asset.Load.data, asset.Load.size, *asset.Data.Model.tex, true);
				if (*asset.Output.model == NULL)
                    vk2dLog("Failed to load model from buffer.");
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_FILE) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderLoad(asset.Load.filename, asset.Load.fragmentFilename, asset.Data.Shader.uniformBufferSize);
			} else if (asset.type == VK2D_ASSET_TYPE_SHADER_MEMORY) {
				// Shaders are internally synchronized
				*asset.Output.shader = vk2dShaderFrom(asset.Load.data, asset.Load.size, asset.Load.fragmentData, asset.Load.fragmentSize, asset.Data.Shader.uniformBufferSize);
			}
		}
	}
}

void vk2dAssetsWait() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->limits.supportsMultiThreadLoading) {
		VK2DLogicalDevice dev = vk2dRendererGetDevice();
		while (SDL_AtomicGet(&dev->doneLoading) == 0) {
			volatile int i = 0;
		}
	}
}

bool vk2dAssetsLoadComplete() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->limits.supportsMultiThreadLoading) {
		return SDL_AtomicGet(&vk2dRendererGetDevice()->doneLoading) != 0;
	}
	return true;
}

float vk2dAssetsLoadStatus() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->limits.supportsMultiThreadLoading) {
		return gLoadStatus;
	}
	return 1;
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

void vk2dAssetsSetTextureFile(VK2DAssetLoad *array, int index, const char *filename, VK2DTexture *outVar) {
	array[index].type = VK2D_ASSET_TYPE_TEXTURE_FILE;
	array[index].Load.filename = filename;
	array[index].Output.texture = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dAssetsSetTextureMemory(VK2DAssetLoad *array, int index, void *buffer, int size, VK2DTexture *outVar) {
	array[index].type = VK2D_ASSET_TYPE_TEXTURE_MEMORY;
	array[index].Load.data = buffer;
	array[index].Load.size = size;
	array[index].Output.texture = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dAssetsSetModelFile(VK2DAssetLoad *array, int index, const char *filename, VK2DTexture *texture, VK2DModel *outVar) {
	array[index].type = VK2D_ASSET_TYPE_MODEL_FILE;
	array[index].Load.filename = filename;
	array[index].Data.Model.tex = texture;
	array[index].Output.model = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dAssetsSetModelMemory(VK2DAssetLoad *array, int index, void *buffer, int size, VK2DTexture *texture, VK2DModel *outVar) {
	array[index].type = VK2D_ASSET_TYPE_MODEL_MEMORY;
	array[index].Load.data = buffer;
	array[index].Load.size = size;
	array[index].Data.Model.tex = texture;
	array[index].Output.model = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dAssetsSetShaderFile(VK2DAssetLoad *array, int index, const char *vertexFilename, const char *fragmentFilename, uint32_t uniformBufferSize, VK2DShader *outVar) {
	array[index].type = VK2D_ASSET_TYPE_SHADER_FILE;
	array[index].Load.filename = vertexFilename;
	array[index].Load.fragmentFilename = fragmentFilename;
	array[index].Data.Shader.uniformBufferSize = uniformBufferSize;
	array[index].Output.shader = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dAssetsSetShaderMemory(VK2DAssetLoad *array, int index, void *vertexBuffer, int vertexBufferSize, void *fragmentBuffer, int fragmentBufferSize, uint32_t uniformBufferSize, VK2DShader *outVar) {
	array[index].type = VK2D_ASSET_TYPE_SHADER_MEMORY;
	array[index].Load.data = vertexBuffer;
	array[index].Load.size = vertexBufferSize;
	array[index].Load.fragmentData = fragmentBuffer;
	array[index].Load.fragmentSize = fragmentBufferSize;
	array[index].Data.Shader.uniformBufferSize = uniformBufferSize;
	array[index].Output.shader = outVar;
	array[index].state = VK2D_ASSET_TYPE_ASSET;
}

void vk2dSleep(double seconds) {
    if (seconds <= 0)
        return;
    double start = SDL_GetPerformanceCounter();
    double milliseconds = floor(seconds * 1000);
    SDL_Delay(milliseconds);
    while ((SDL_GetPerformanceCounter() - start) / (double)SDL_GetPerformanceFrequency() < seconds) {
        volatile int i = 0;
    }
}
