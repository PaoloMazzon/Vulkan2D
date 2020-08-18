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
	VK2DLogicalDevice dev;      ///< Device this pipeline belongs to
	VkRenderPass renderPass;    ///< Render pass this pipeline uses
	VkPipeline pipe;            ///< Internal pipeline
	VkPipelineLayout layout;    ///< Internal pipeline layout
	VkRect2D rect;              ///< For setting up command buffers
	VkClearValue clearValue[2]; ///< Clear values for the two attachments: colour and depth

};

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
///  + Colour blending
///  + Rasterization line width (if wireframe is enabled)
///
/// (This may change in the future) .
VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, uint32_t width, uint32_t height, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkDescriptorSetLayout setLayout, VkPipelineVertexInputStateCreateInfo *vertexInfo, bool fill, VK2DMSAA msaa);

/// \brief Begins a command buffer for a given pipeline
/// \param pipe Pipeline to begin the buffer with
/// \param framebuffer Framebuffer to be the render target
/// \param buffer Buffer to begin (should be in initial state)
/// \param blendConstants How to blend colours for RGBA, hence the 4 (See https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#framebuffer-blendfactors)
/// \param viewports Vector of viewports to draw with
/// \param viewportCount Amount of viewports to draw with
/// \param lineWidth Width of wireframe lines (if wireframe is not enabled this value is not used)
///
/// This function is to begin recording a command buffer with all preliminary information already
/// set up. Typically, you would create a command buffer, then call this function on it and once
/// that is done, you are ready to record whatever draw commands you want without having to bind
/// the pipeline/framebuffer/dynamic state/render pass.
void vk2dPipelineBeginBuffer(VK2DPipeline pipe, VkFramebuffer framebuffer, VkCommandBuffer buffer, float blendConstants[4], VkViewport *viewports, uint32_t viewportCount, float lineWidth);

/// \brief Frees a pipeline from memory
/// \param pipe Pipeline to free
void vk2dPipelineFree(VK2DPipeline pipe);

#ifdef __cplusplus
};
#endif