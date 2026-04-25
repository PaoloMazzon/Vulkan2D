/// \file Util.c
/// \author Paolo Mazzon
#include "VK2D/Util.h"
#include "VK2D/Initializers.h"
#include "VK2D/Logger.h"
#include "VK2D/Model.h"
#include "VK2D/Opaque.h"
#include "VK2D/Renderer.h"
#include "VK2D/Shader.h"
#include "VK2D/Structs.h"
#include "VK2D/Texture.h"
#include "VK2D/Validation.h"
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static float gLoadStatus = 0;

// Gets the vertex input information for VK2DVertexTexture (Uses static
// variables to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState() {
  return vk2dInitPipelineVertexInputStateCreateInfo(VK_NULL_HANDLE, 0,
                                                    VK_NULL_HANDLE, 0);
  ;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetModelVertexInputState() {
  static VkVertexInputBindingDescription vertexInputBindingDescription;
  static VkVertexInputAttributeDescription vertexInputAttributeDescription[2];
  static VkPipelineVertexInputStateCreateInfo
      pipelineVertexInputStateCreateInfo;
  static bool init = false;

  if (!init) {
    vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(
        VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVertex3D), 0);
    vertexInputAttributeDescription[0] =
        vk2dInitVertexInputAttributeDescription(
            0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertex3D, pos));
    vertexInputAttributeDescription[1] =
        vk2dInitVertexInputAttributeDescription(1, 0, VK_FORMAT_R32G32_SFLOAT,
                                                offsetof(VK2DVertex3D, uv));
    pipelineVertexInputStateCreateInfo =
        vk2dInitPipelineVertexInputStateCreateInfo(
            &vertexInputBindingDescription, 1, vertexInputAttributeDescription,
            2);
    init = true;
  }

  return pipelineVertexInputStateCreateInfo;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetInstanceVertexInputState() {
  static VkVertexInputBindingDescription vertexInputBindingDescription;
  static VkVertexInputAttributeDescription vertexInputAttributeDescription[9];
  static VkPipelineVertexInputStateCreateInfo
      pipelineVertexInputStateCreateInfo;
  static bool init = false;
  uint32_t stride = sizeof(VK2DDrawInstance);

  if (!init) {
    vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(
        VK_VERTEX_INPUT_RATE_INSTANCE, stride, 0);
    vertexInputAttributeDescription[0] =
        vk2dInitVertexInputAttributeDescription(
            0, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, texturePos));
    vertexInputAttributeDescription[1] =
        vk2dInitVertexInputAttributeDescription(
            1, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, colour));
    vertexInputAttributeDescription[2] =
        vk2dInitVertexInputAttributeDescription(
            2, 0, VK_FORMAT_R32_UINT, offsetof(VK2DDrawInstance, textureIndex));
    vertexInputAttributeDescription[3] =
        vk2dInitVertexInputAttributeDescription(
            3, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, model));
    vertexInputAttributeDescription[4] =
        vk2dInitVertexInputAttributeDescription(
            4, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, model) + (sizeof(VK2DVec4) * 1));
    vertexInputAttributeDescription[5] =
        vk2dInitVertexInputAttributeDescription(
            5, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, model) + (sizeof(VK2DVec4) * 2));
    vertexInputAttributeDescription[6] =
        vk2dInitVertexInputAttributeDescription(
            6, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DDrawInstance, model) + (sizeof(VK2DVec4) * 3));
    pipelineVertexInputStateCreateInfo =
        vk2dInitPipelineVertexInputStateCreateInfo(
            &vertexInputBindingDescription, 1, vertexInputAttributeDescription,
            7);
    init = true;
  }

  return vk2dInitPipelineVertexInputStateCreateInfo(VK_NULL_HANDLE, 0,
                                                    VK_NULL_HANDLE, 0);
  ;
  return pipelineVertexInputStateCreateInfo;
}

VkPipelineVertexInputStateCreateInfo _vk2dGetShadowsVertexInputState() {
  static VkVertexInputBindingDescription vertexInputBindingDescription;
  static VkVertexInputAttributeDescription vertexInputAttributeDescription[1];
  static VkPipelineVertexInputStateCreateInfo
      pipelineVertexInputStateCreateInfo;
  static bool init = false;

  if (!init) {
    vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(
        VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVec3), 0);
    vertexInputAttributeDescription[0] =
        vk2dInitVertexInputAttributeDescription(0, 0,
                                                VK_FORMAT_R32G32B32_SFLOAT, 0);
    pipelineVertexInputStateCreateInfo =
        vk2dInitPipelineVertexInputStateCreateInfo(
            &vertexInputBindingDescription, 1, vertexInputAttributeDescription,
            1);
    init = true;
  }

  return pipelineVertexInputStateCreateInfo;
}

// Gets the vertex input information for VK2DVertexColour (Uses static variables
// to persist attached descriptions)
VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState() {
  static VkVertexInputBindingDescription vertexInputBindingDescription;
  static VkVertexInputAttributeDescription vertexInputAttributeDescription[2];
  static VkPipelineVertexInputStateCreateInfo
      pipelineVertexInputStateCreateInfo;
  static bool init = false;

  if (!init) {
    vertexInputBindingDescription = vk2dInitVertexInputBindingDescription(
        VK_VERTEX_INPUT_RATE_VERTEX, sizeof(VK2DVertexColour), 0);
    vertexInputAttributeDescription[0] =
        vk2dInitVertexInputAttributeDescription(
            0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VK2DVertexColour, pos));
    vertexInputAttributeDescription[1] =
        vk2dInitVertexInputAttributeDescription(
            1, 0, VK_FORMAT_R32G32B32A32_SFLOAT,
            offsetof(VK2DVertexColour, colour));
    pipelineVertexInputStateCreateInfo =
        vk2dInitPipelineVertexInputStateCreateInfo(
            &vertexInputBindingDescription, 1, vertexInputAttributeDescription,
            2);
    init = true;
  }

  return pipelineVertexInputStateCreateInfo;
}

// Prints a matrix
void _vk2dPrintMatrix(FILE *out, VK2DMat4 m, const char *prefix) {
  fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[0], m[4], m[8], m[12]);
  fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[1], m[5], m[9], m[13]);
  fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[2], m[6], m[10], m[14]);
  fprintf(out, "%s[ %f %f %f %f ]\n", prefix, m[3], m[7], m[11], m[15]);
}

// Prints a UBO all fancy like
void _vk2dPrintUBO(FILE *out, VK2DUniformBufferObject ubo) {
  fprintf(out, "View-Proj:\n[\n");
  for (int i = 0; i < 4; i++) {
    _vk2dPrintMatrix(out, ubo.viewproj[i], "    ");
  }
  fprintf(out, "]\n");
  fflush(out);
}

bool _vk2dFileExists(const char *filename) {
  FILE *file = fopen(filename, "r");
  bool out = file == NULL ? false : true;
  if (file != NULL) {
    fclose(file);
  }
  return out;
}

unsigned char *_vk2dLoadFile(const char *filename, uint32_t *size) {
  FILE *file = fopen(filename, "rb");
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
      vk2dRaise(VK2D_STATUS_OUT_OF_RAM,
                "Unable to allocate buffer for file \"%s\".\n", filename);
    }
    fclose(file);
  } else {
    vk2dRaise(VK2D_STATUS_FILE_NOT_FOUND,
              "File \"%s\" was unable to be opened.\n", filename);
  }

  return buffer;
}

unsigned char *_vk2dCopyBuffer(const void *buffer, int size) {
  unsigned char *new = NULL;
  if (buffer != NULL && size != 0) {
    new = malloc(size);
    if (new != NULL) {
      memcpy(new, buffer, size);
    } else {
      vk2dRaise(VK2D_STATUS_OUT_OF_RAM,
                "Unable to copy buffer of size %i bytes.\n", size);
    }
  }
  return new;
}

void vk2dSleep(double seconds) {
  if (seconds <= 0)
    return;
  double start = SDL_GetPerformanceCounter();
  double milliseconds = floor(seconds * 1000);
  SDL_Delay(milliseconds);
  while ((SDL_GetPerformanceCounter() - start) /
             (double)SDL_GetPerformanceFrequency() <
         seconds) {
    volatile int i = 0;
  }
}

double vk2dTime() {
  VK2DRenderer gRenderer = vk2dRendererGetPointer();
  const double now = SDL_GetPerformanceCounter();
  const double then = gRenderer->startTicks;
  return (now - then) / (double)SDL_GetPerformanceFrequency();
}
