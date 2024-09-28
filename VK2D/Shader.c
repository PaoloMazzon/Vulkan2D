/// \file Shader.c
/// \author Paolo Mazzon
#include "VK2D/Shader.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Util.h"
#include "VK2D/Opaque.h"
#include <malloc.h>

void _vk2dRendererAddShader(VK2DShader shader);
void _vk2dRendererRemoveShader(VK2DShader shader);
unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);
VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState();

void _vk2dShaderBuildPipe(VK2DShader shader) {
    if (vk2dStatusFatal())
        return;
	VK2DRenderer renderer = vk2dRendererGetPointer();
	VkPipelineVertexInputStateCreateInfo textureVertexInfo = _vk2dGetTextureVertexInputState();

	VkDescriptorSetLayout layout[] = {renderer->dslBufferVP, renderer->dslSampler, renderer->dslTextureArray, renderer->dslBufferUser};
	uint32_t layoutCount;
	if (shader->uniformSize != 0) {
		layoutCount = 4;
	} else {
		layoutCount = 3;
	}
	shader->pipe = vk2dPipelineCreate(
			renderer->ld,
			renderer->renderPass,
			renderer->surfaceWidth,
			renderer->surfaceHeight,
			shader->spvVert,
			shader->spvVertSize,
			shader->spvFrag,
			shader->spvFragSize,
			layout,
			layoutCount,
			&textureVertexInfo,
			true,
			renderer->config.msaa,
            VK2D_PIPELINE_TYPE_USER_SHADER);
}

VK2DShader vk2dShaderFrom(uint8_t *vertexShaderBuffer, int vertexShaderBufferSize, uint8_t *fragmentShaderBuffer, int fragmentShaderBufferSize, uint32_t uniformBufferSize) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (vk2dStatusFatal() || gRenderer == NULL)
        return NULL;
	if (uniformBufferSize % 4 != 0) {
        vk2dRaise(VK2D_STATUS_BAD_FORMAT, "Uniform buffer size for shader is invalid, must be multiple of 4");
		return NULL;
	} else if (uniformBufferSize > gRenderer->limits.maxShaderBufferSize) {
        vk2dRaise(VK2D_STATUS_BEYOND_LIMIT, "Uniform buffer of size %i is greater than the maximum allowed uniform buffer size of %i from vk2dRendererGetLimits",
                uniformBufferSize, gRenderer->limits.maxShaderBufferSize);
		return NULL;
	}

	int i;
	VK2DRenderer renderer = vk2dRendererGetPointer();

	if (renderer == NULL)
	    return NULL;

	uint8_t *vertFile = _vk2dCopyBuffer(vertexShaderBuffer, vertexShaderBufferSize);
	if (vertFile == NULL)
	    return NULL;

	uint8_t *fragFile = _vk2dCopyBuffer(fragmentShaderBuffer, fragmentShaderBufferSize);
    if (vertFile == NULL) {
        free(vertFile);
        return NULL;
    }
	VK2DLogicalDevice dev = vk2dRendererGetDevice();
    VK2DShader out = calloc(1, sizeof(struct VK2DShader_t));

    if (out != NULL) {
        out->spvFrag = fragFile;
        out->spvVert = vertFile;
        out->spvVertSize = vertexShaderBufferSize;
        out->spvFragSize = fragmentShaderBufferSize;
        out->uniformSize = uniformBufferSize;
        out->dev = dev;

        if (!gRenderer->limits.supportsMultiThreadLoading || SDL_LockMutex(dev->shaderMutex) == 0) {
            _vk2dRendererAddShader(out);
            _vk2dShaderBuildPipe(out);
            if (gRenderer->limits.supportsMultiThreadLoading)
                SDL_UnlockMutex(dev->shaderMutex);
        } else {
            vk2dRaise(VK2D_STATUS_SDL_ERROR, "Failed to lock mutex, SDL error: %s.", SDL_GetError());
            free(out);
            out = NULL;
        }
    } else {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate shader.");
    }

	return out;
}

VK2DShader vk2dShaderLoad(const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (vk2dStatusFatal() || gRenderer == NULL)
        return NULL;
    if (uniformBufferSize % 4 != 0) {
        vk2dRaise(VK2D_STATUS_BAD_FORMAT, "Uniform buffer size for shader is invalid, must be multiple of 4");
        return NULL;
    } else if (uniformBufferSize > gRenderer->limits.maxShaderBufferSize) {
        vk2dRaise(VK2D_STATUS_BEYOND_LIMIT, "Uniform buffer of size %i is greater than the maximum allowed uniform buffer size of %i from vk2dRendererGetLimits",
                  uniformBufferSize, gRenderer->limits.maxShaderBufferSize);
        return NULL;
    }

	uint32_t vertFileSize, fragFileSize, i;
	VK2DRenderer renderer = vk2dRendererGetPointer();

    uint8_t *vertFile = _vk2dLoadFile(vertexShader, &vertFileSize);
    if (vertFile == NULL)
        return NULL;

    uint8_t *fragFile = _vk2dLoadFile(fragmentShader, &fragFileSize);
    if (vertFile == NULL) {
        free(vertFile);
        return NULL;
    }
    VK2DShader out = malloc(sizeof(struct VK2DShader_t));
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (vk2dRendererGetPointer() != NULL) {
		if (out != NULL) {
			out->spvFrag = fragFile;
			out->spvVert = vertFile;
			out->spvVertSize = vertFileSize;
			out->spvFragSize = fragFileSize;
			out->uniformSize = uniformBufferSize;
			out->dev = dev;

            if (!gRenderer->limits.supportsMultiThreadLoading || SDL_LockMutex(dev->shaderMutex) == 0) {
                _vk2dRendererAddShader(out);
                _vk2dShaderBuildPipe(out);
                if (gRenderer->limits.supportsMultiThreadLoading)
                    SDL_UnlockMutex(dev->shaderMutex);
            } else {
                vk2dRaise(VK2D_STATUS_SDL_ERROR, "Failed to lock mutex, SDL error: %s.", SDL_GetError());
                free(out);
                out = NULL;
            }
		}
	} else {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate shader.");
	}

	return out;
}

void vk2dShaderFree(VK2DShader shader) {
	uint32_t i;
	if (vk2dRendererGetPointer() != NULL)
		_vk2dRendererRemoveShader(shader);
	if (shader != NULL) {
		vk2dPipelineFree(shader->pipe);
		free(shader->spvVert);
		free(shader->spvFrag);
	}
}