/// \file Pipeline.h
/// \author Paolo Mazzon
/// \brief Simple abstraction over VkPipeline objects
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a graphics pipeline
/// \param dev Device to create the pipeline with
/// \param renderPass Render pass that will be used with the pipeline
/// \param width Width of the screen (used for scissor)
/// \param height Height of the screen (used for scissor)
/// \param vertBuffer Buffer containing the compiled SPIR-V vertex buffer
/// \param vertSize Size of the aforementioned buffer in bytes
/// \param fragBuffer Buffer containing the compiled SPIR-V fragment buffer
/// \param fragSize Size of the aforementioned buffer in bytes
/// \param setLayout Describes the shader's input information
/// \param vertexInfo Tells the pipeline how to read input vertices
/// \param fill If true, polygons are drawn as wireframe and not filled
/// \param msaa What level of MSAA to use
/// \return Returns the new pipeline or NULL if it failed
///
/// Several things are set to dynamic state and will be ignored, hence this asking for only 3
/// create infos when in reality there are many more. For a complete list of dynamic state,
///
///  + Viewports
///  + Rasterization line width (if wireframe is enabled)
///  + Scissor
///
/// (This may change in the future) .
VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, uint32_t width, uint32_t height, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkDescriptorSetLayout *setLayout, uint32_t layoutCount, VkPipelineVertexInputStateCreateInfo *vertexInfo, bool fill, VK2DMSAA msaa, VK2DPipelineType type);

/// \brief Creates and returns a VK2DPipeline for a compute shader2
VK2DPipeline vk2dPipelineCreateCompute(VK2DLogicalDevice dev, uint32_t pushBufferSize, unsigned char *shaderBuffer, uint32_t shaderSize, VkDescriptorSetLayout *setLayout, uint32_t layoutCount);

/// \brief Returns the internal compute pipeline for a given vk2d pipeline
VkPipeline vk2dPipelineGetCompute(VK2DPipeline pipe);

/// \brief Grabs a pipeline given a blend mode, returning the default one if blend mode generation is disabled
/// \param pipe Pipeline to grab the proper blended pipeline from
/// \param blendMode Blend mode you want to draw with
/// \return Returns a pipeline with the desired blend mode
VkPipeline vk2dPipelineGetPipe(VK2DPipeline pipe, VK2DBlendMode blendMode);

/// \brief Gets a unique id for this specific pipeline and blend mode
/// \param pipe Pipeline to get the id from
/// \param blendMode Blend mode
/// \return Returns a unique id from this pipe and blend mode, guaranteed to be unique from any other
int32_t vk2dPipelineGetID(VK2DPipeline pipe, VK2DBlendMode blendMode);

/// \brief Frees a pipeline from memory
/// \param pipe Pipeline to free
void vk2dPipelineFree(VK2DPipeline pipe);

#ifdef __cplusplus
};
#endif