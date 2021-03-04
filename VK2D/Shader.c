/// \file Shader.c
/// \author Paolo Mazzon
#include "VK2D/Shader.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Validation.h"
#include "VK2D/DescriptorControl.h"
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
			renderer->config.msaa);
}

VK2DShader vk2dShaderCreate(VK2DLogicalDevice dev, const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize) {
	VK2DShader out = malloc(sizeof(struct VK2DShader));
	uint32_t vertFileSize, fragFileSize, i;
	VK2DRenderer renderer = vk2dRendererGetPointer();
	unsigned char *vertFile = _vk2dLoadFile(vertexShader, &vertFileSize);
	unsigned char *fragFile = _vk2dLoadFile(fragmentShader, &fragFileSize);

	if (vk2dPointerCheck(out) && vk2dPointerCheck(vertFile) && vk2dPointerCheck(fragFile)) {
		out->spvFrag = fragFile;
		out->spvVert = vertFile;
		out->spvVertSize = vertFileSize;
		out->spvFragSize = fragFileSize;
		out->uniformSize = uniformBufferSize;
		out->dev = dev;
		out->currentUniform = 0;

		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT && uniformBufferSize > 0; i++) {
			out->uniforms[i] = vk2dBufferCreate(dev, uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
												VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
												VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			out->sets[i] = vk2dDescConGetBufferSet(renderer->descConUser, out->uniforms[i]);
		}

		_vk2dShaderBuildPipe(out);
	}

	return out;
}

void vk2dShaderUpdate(VK2DShader shader, void *data, uint32_t size) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	void *mem;
	vk2dErrorCheck(vmaMapMemory(gRenderer->vma, shader->uniforms[shader->currentUniform]->mem, &mem));
	memcpy(mem, data, size);
	vmaUnmapMemory(gRenderer->vma, shader->uniforms[shader->currentUniform]->mem);
	shader->currentUniform = (shader->currentUniform + 1) % VK2D_MAX_FRAMES_IN_FLIGHT;
}

void vk2dShaderFree(VK2DShader shader) {
	uint32_t i;
	_vk2dRendererRemoveShader(shader);
	if (shader != NULL) {
		vk2dPipelineFree(shader->pipe);

		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT && shader->uniformSize != 0; i++)
			vk2dBufferFree(shader->uniforms[i]);
		free(shader->spvVert);
		free(shader->spvFrag);
	}
}