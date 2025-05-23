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

/// \brief Gets the vertex input information for the instancing pipeline
VkPipelineVertexInputStateCreateInfo _vk2dGetInstanceVertexInputState();

/// \brief Gets the vertex input information for the shadows pipeline
VkPipelineVertexInputStateCreateInfo _vk2dGetShadowsVertexInputState();

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

/// \brief Copies a string
unsigned char *_vk2dCopyBuffer(const void *buffer, int size);

/// \brief Worker thread for off-thread loading
int _vk2dWorkerThread(void *data);

/// \brief The internal texture creation function
VK2DTexture _vk2dTextureFromInternal(const void *data, int size, bool mainThread);

/// \brief The internal model creation function
VK2DModel _vk2dModelFromInternal(const void *objFile, uint32_t objFileSize, VK2DTexture texture, bool mainThread);