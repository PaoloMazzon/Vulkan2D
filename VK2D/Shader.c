/// \file Shader.c
/// \author Paolo Mazzon
#include "VK2D/Shader.h"

void _vk2dRendererAddShader(VK2DShader shader);
void _vk2dRendererRemoveShader(VK2DShader shader);
unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);

void _vk2dShaderBuildPipe(VK2DShader shader) {
	// TODO: This
}

VK2DShader vk2dShaderCreate(const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize) {
	return NULL; // TODO: This
}

void vk2dShaderUpdate(VK2DShader shader, void *data, uint32_t size) {
	// TODO: This
}

void vk2dShaderFree(VK2DShader shader) {
	// TODO: This
}