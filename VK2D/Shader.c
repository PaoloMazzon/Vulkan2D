/// \file Shader.c
/// \author Paolo Mazzon
#include "VK2D/Shader.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
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
	VK2DRenderer renderer = vk2dRendererGetPointer();
	VkPipelineVertexInputStateCreateInfo textureVertexInfo = _vk2dGetTextureVertexInputState();

	VkDescriptorSetLayout layout[] = {renderer->dslBufferVP, renderer->dslSampler, renderer->dslTexture, renderer->dslBufferUser};
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
			false);
}

VK2DShader vk2dShaderFrom(uint8_t *vertexShaderBuffer, int vertexShaderBufferSize, uint8_t *fragmentShaderBuffer, int fragmentShaderBufferSize, uint32_t uniformBufferSize) {
	if (uniformBufferSize % 4 != 0) {
		vk2dLogMessage("Uniform buffer size for shader is invalid, must be multiple of 4");
		return NULL;
	}

	VK2DShader out = malloc(sizeof(struct VK2DShader));
	int i;
	VK2DRenderer renderer = vk2dRendererGetPointer();
	uint8_t *vertFile = _vk2dCopyBuffer(vertexShaderBuffer, vertexShaderBufferSize);
	uint8_t *fragFile = _vk2dCopyBuffer(fragmentShaderBuffer, fragmentShaderBufferSize);
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (vk2dRendererGetPointer() != NULL) {
		if (vk2dPointerCheck(out) && vk2dPointerCheck((void*)vertFile) && vk2dPointerCheck((void*)fragFile)) {
			out->spvFrag = fragFile;
			out->spvVert = vertFile;
			out->spvVertSize = vertexShaderBufferSize;
			out->spvFragSize = fragmentShaderBufferSize;
			out->uniformSize = uniformBufferSize;
			out->dev = dev;

			if (uniformBufferSize != 0) {
				for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT && uniformBufferSize > 0; i++) {
					out->descCons[i] = vk2dDescConCreate(dev, renderer->dslBufferUser, 3, VK2D_NO_LOCATION);
				}
			}

			_vk2dRendererAddShader(out);
			_vk2dShaderBuildPipe(out);
		}
	} else {
		free(out);
		out = NULL;
		vk2dLogMessage("Renderer is not initialized");
	}

	return out;
}

VK2DShader vk2dShaderLoad(const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize) {
	if (uniformBufferSize % 4 != 0) {
		vk2dLogMessage("Uniform buffer size for shader \"%s\"/\"%s\" is invalid, must be multiple of 4", vertexShader, fragmentShader);
		return NULL;
	}

	VK2DShader out = malloc(sizeof(struct VK2DShader));
	uint32_t vertFileSize, fragFileSize, i;
	VK2DRenderer renderer = vk2dRendererGetPointer();
	uint8_t *vertFile = _vk2dLoadFile(vertexShader, &vertFileSize);
	uint8_t *fragFile = _vk2dLoadFile(fragmentShader, &fragFileSize);
	VK2DLogicalDevice dev = vk2dRendererGetDevice();

	if (vk2dRendererGetPointer() != NULL) {
		if (vk2dPointerCheck(out) && vk2dPointerCheck((void*)vertFile) && vk2dPointerCheck((void*)fragFile)) {
			out->spvFrag = fragFile;
			out->spvVert = vertFile;
			out->spvVertSize = vertFileSize;
			out->spvFragSize = fragFileSize;
			out->uniformSize = uniformBufferSize;
			out->dev = dev;

			if (uniformBufferSize != 0) {
				for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT && uniformBufferSize > 0; i++) {
					out->descCons[i] = vk2dDescConCreate(dev, renderer->dslBufferUser, 3, VK2D_NO_LOCATION);
				}
			}

			_vk2dRendererAddShader(out);
			_vk2dShaderBuildPipe(out);
		}
	} else {
		free(out);
		out = NULL;
		vk2dLogMessage("Renderer is not initialized");
	}

	return out;
}

void vk2dShaderFree(VK2DShader shader) {
	uint32_t i;
	if (vk2dRendererGetPointer() != NULL)
		_vk2dRendererRemoveShader(shader);
	if (shader != NULL) {
		vk2dPipelineFree(shader->pipe);

		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT && shader->uniformSize != 0; i++)
			vk2dDescConFree(shader->descCons[i]);
		free(shader->spvVert);
		free(shader->spvFrag);
	}
}