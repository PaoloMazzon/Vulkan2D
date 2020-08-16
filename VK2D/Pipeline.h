/// \file Pipeline.h
/// \author Paolo Mazzon
/// \brief Simple abstraction over VkPipeline objects
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief A handy abstraction that groups up pipeline state and makes multiple shaders easier
struct VK2DPipeline {
	VK2DLogicalDevice dev;     ///< Device this pipeline belongs to
	VkRenderPass renderPass;   ///< Render pass this pipeline uses
	VkPipeline pipe;           ///< Internal pipeline
	VkPipelineLayout layout;   ///< Internal pipeline layout
};

/// \brief Creates a graphics pipeline
/// \param dev Device to create the pipeline with
/// \param renderPass Render pass that will be used with the pipeline
/// \param vertBuffer Buffer containing the compiled SPIR-V vertex buffer
/// \param vertSize Size of the aforementioned buffer in bytes
/// \param fragBuffer Buffer containing the compiled SPIR-V fragment buffer
/// \param fragSize Size of the aforementioned buffer in bytes
/// \param vertexInfo Tells the pipeline how to read input vertices
/// \param rasterizationInfo Tells the pipeline how to draw pixels
/// \param msaaInfo Tells the pipeline what kind of MSAA to use
/// \return Returns the new pipeline or NULL if it failed
///
/// Several things are set to dynamic state and will be ignored, hence this asking for only 3
/// create infos when in reality there are many more. For a complete list of dynamic state,
///
///  + Viewports
///  + Colour blending
///  + Rasterization line width (if wireframe is enabled)
///
/// (This may change in the future) .
VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkPipelineVertexInputStateCreateInfo vertexInfo, VkPipelineRasterizationStateCreateInfo rasterizationInfo, VkPipelineMultisampleStateCreateInfo msaaInfo);

/// \brief Begins a command buffer for a given pipeline
/// \param pipe Pipeline to begin the buffer with
/// \param buffer Buffer to begin (should be in initial state)
/// \param blendConstants How to blend colours for RGBA, hence the 4 (See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#framebuffer-blendfactors)
/// \param viewports Vector of viewports to draw with
/// \param viewportCount Amount of viewports to draw with
/// \param lineWidth Width of wireframe lines (if wireframe is not enabled this value is not used)
void vk2dPipelineBeginBuffer(VK2DPipeline pipe, VkCommandBuffer buffer, float blendConstants[4], VkViewport *viewports, uint32_t viewportCount, float lineWidth);

/// \brief Frees a pipeline from memory
/// \param pipe Pipeline to free
void vk2dPipelineFree(VK2DPipeline pipe);

#ifdef __cplusplus
};
#endif