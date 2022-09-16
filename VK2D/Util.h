/// \file Util.h
/// \author Paolo Mazzon
/// \brief Utilities for in-engine use only
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdio.h>
#include "VK2D/Structs.h"

/// \brief Gets the vertex input information for VK2DVertexTexture (Uses static variables to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState();

/// \brief Gets the vertex input information for VK2DVertex3D (Uses static variables to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetModelVertexInputState();

/// \brief Gets the vertex input information for VK2DVertexColour (Uses static variables to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState();

/// \brief Prints a matrix
void _vk2dPrintMatrix(FILE* out, mat4 m, const char* prefix);

/// \brief Prints a UBO all fancy like
void _vk2dPrintUBO(FILE* out, VK2DUniformBufferObject ubo);

/// \brief Checks if a file exists
bool _vk2dFileExists(const char *filename);

/// \brief Loads a file into a buffer and returns it (as well as its size)
unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);