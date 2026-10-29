/// \file Shader.c
/// \author Paolo Mazzon
#include <stdlib.h>

#include "VK2D/Opaque.h"
#include "VK2D/Shader.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"
#include "VK2D/Util.h"
#include "VK2D/ShaderCompiler.h"
#include "VK2D/RendererMeta.h"

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

VK2DShader vk2dShaderFrom(const uint8_t *vertexShaderBuffer, int vertexShaderBufferSize, const uint8_t *fragmentShaderBuffer, int fragmentShaderBufferSize, uint32_t uniformBufferSize) {
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

	VK2DRenderer renderer = vk2dRendererGetPointer();

	if (renderer == NULL)
	    return NULL;

	uint8_t *vertFile = _vk2dCopyBuffer(vertexShaderBuffer, vertexShaderBufferSize);
	if (vertFile == NULL)
	    return NULL;

	uint8_t *fragFile = _vk2dCopyBuffer(fragmentShaderBuffer, fragmentShaderBufferSize);
    if (fragFile == NULL) {
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

        _vk2dRendererAddShader(out);
        _vk2dShaderBuildPipe(out);
    } else {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate shader.");
    }

	return out;
}

VK2DShader vk2dSlangLoad(const char *slangFile) {
    uint32_t size;
    const char *file = (void*)_vk2dLoadFile("assets/shader.slang", &size);

    if (!file) {
        return NULL;
    }

    VK2DCompiledShaders compiledShaders;
    if (!_vk2dShaderCompile(file, size, &compiledShaders)) {
        free((void*)file);
        return NULL;
    }
    free((void*)file);

    uint32_t vertShaderSize;
    const uint8_t *vertShader = _vk2dRendererGetUserShader(&vertShaderSize);
    VK2DShader shader = vk2dShaderFrom(
            vertShader, vertShaderSize,
            (void*)compiledShaders.fragmentSpirv, compiledShaders.fragmentSpirvSize,
            compiledShaders.userDataSize);
    free(compiledShaders.fragmentSpirv);
    return shader;
}

VK2DShader vk2dSlangFrom(const char *slangFile, int slangFileSize) {
    VK2DCompiledShaders compiledShaders;
    if (!_vk2dShaderCompile(slangFile, slangFileSize, &compiledShaders)) {
        return NULL;
    }

    uint32_t vertShaderSize;
    const uint8_t *vertShader = _vk2dRendererGetUserShader(&vertShaderSize);
    VK2DShader shader = vk2dShaderFrom(
            vertShader, vertShaderSize,
            (void*)compiledShaders.fragmentSpirv, compiledShaders.fragmentSpirvSize,
            compiledShaders.userDataSize);
    free(compiledShaders.fragmentSpirv);
    return shader;
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