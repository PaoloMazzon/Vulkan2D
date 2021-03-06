/// \file Pipeline.h
/// \author Paolo Mazzon
/// \brief Simple abstraction over VkPipeline objects
#pragma once
#include "VK2D/Structs.h"
#include "VK2D/BuildOptions.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief A handy abstraction that groups up pipeline state and makes multiple shaders easier
struct VK2DPipeline {
	VK2DLogicalDevice dev;      ///< Device this pipeline belongs to
	VkRenderPass renderPass;    ///< Render pass this pipeline uses
	VkPipelineLayout layout;    ///< Internal pipeline layout
	VkRect2D rect;              ///< For setting up command buffers
	VkClearValue clearValue[2]; ///< Clear values for the two attachments: colour and depth

#ifdef VK2D_GENERATE_BLEND_MODES
	VkPipeline pipes[bm_Max]; ///< Internal pipelines if `VK2D_GENERATE_BLEND_MODES` is enabled
#else // VK2D_GENERATE_BLEND_MODES
	VkPipeline pipe; ///< Internal pipeline (blend only)
#endif // VK2D_GENERATE_BLEND_MODES
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
///  + Rasterization line width (if wireframe is enabled)
///  + Scissor
///
/// (This may change in the future) .
VK2DPipeline vk2dPipelineCreate(VK2DLogicalDevice dev, VkRenderPass renderPass, uint32_t width, uint32_t height, unsigned char *vertBuffer, uint32_t vertSize, unsigned char *fragBuffer, uint32_t fragSize, VkDescriptorSetLayout *setLayout, uint32_t layoutCount, VkPipelineVertexInputStateCreateInfo *vertexInfo, bool fill, VK2DMSAA msaa);

/// \brief Grabs a pipeline given a blend mode, returning the default one if blend mode generation is disabled
/// \param pipe Pipeline to grab the proper blended pipeline from
/// \param blendMode Blend mode you want to draw with
/// \return Returns a pipeline with the desired blend mode
VkPipeline vk2dPipelineGetPipe(VK2DPipeline pipe, VK2DBlendMode blendMode);

/// \brief Frees a pipeline from memory
/// \param pipe Pipeline to free
void vk2dPipelineFree(VK2DPipeline pipe);

#ifdef __cplusplus
};
#endif