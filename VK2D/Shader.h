/// \file Shader.h
/// \author Paolo Mazzon
/// \brief Makes shaders possible in VK2D
#pragma once
#include "VK2D/Structs.h"
#include "VK2D/Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a shader you can use to render textures
/// \param vertexShader File containing the compiled SPIR-V vertex shader
/// \param fragmentShader File containing the compiled SPIR-V fragment shader
/// \param uniformBufferSize Size of the shader's expected uniform buffer (0 is valid)
/// \return Returns a new shader or NULL
/// \warning There are strict requirements for how the shader should be constructed
/// \warning uniformBufferSize must be less than the `maxShaderBufferSize` field of `vk2dRendererGetLimits`
/// \warning uniformBufferSize must be a multiple of 4
///
/// Check the shaders shaders/tex.vert and shaders/tex.frag for information on how
/// the shaders should be set up. You may choose to not include a uniform buffer if
/// you specify a uniform buffer size of 0.
VK2DShader vk2dShaderLoad(const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize);

/// \brief Creates a shader you can use to render textures from an in-memory buffer
/// \param vertexShaderBuffer Buffer containing compiled SPIR-V shader code
/// \param vertexShaderBufferSize Size of the vertexShaderBuffer buffer in bytes
/// \param fragmentShaderBuffer File containing the compiled SPIR-V fragment shader
/// \param fragmentShaderBufferSize Size of the fragmentShaderBuffer buffer in bytes
/// \param uniformBufferSize Size of the shader's expected uniform buffer (0 is valid)
/// \return Returns a new shader or NULL
/// \warning There are strict requirements for how the shader should be constructed
/// \warning uniformBufferSize must be less than the `maxShaderBufferSize` field of `vk2dRendererGetLimits`
/// \warning uniformBufferSize must be a multiple of 4
///
/// Check the shaders shaders/tex.vert and shaders/tex.frag for information on how
/// the shaders should be set up. You may choose to not include a uniform buffer if
/// you specify a uniform buffer size of 0.
VK2DShader vk2dShaderFrom(const uint8_t *vertexShaderBuffer, int vertexShaderBufferSize, const uint8_t *fragmentShaderBuffer, int fragmentShaderBufferSize, uint32_t uniformBufferSize);

/// \brief Frees a shader from memory
/// \param shader Shader to free
void vk2dShaderFree(VK2DShader shader);

#ifdef __cplusplus
}
#endif