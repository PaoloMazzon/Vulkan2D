/// \file RendererMeta.h
/// \author Paolo Mazzon
/// \brief Declares functions only the internal renderer needs
#pragma once
#include <VK2D/Renderer.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************************** Helper Functions ******************************/

// This is called when a render-target texture is created to make the renderer aware of it
void _vk2dRendererAddTarget(VK2DTexture tex);

// Called when a render-target texture is destroyed so the renderer can remove it from its list
void _vk2dRendererRemoveTarget(VK2DTexture tex);

// This is used when changing the render target to make sure the texture is either ready to be drawn itself or rendered to
void _vk2dTransitionImageLayout(VkImage img, VkImageLayout old, VkImageLayout new);

// Rebuilds the matrices for a given buffer and camera
void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera);

// Flushes the data from a ubo to its respective buffer, frame being the swapchain buffer to flush
void _vk2dRendererFlushUBOBuffer(uint32_t frame, uint32_t descriptorFrame, int camera);

// Grabs a preferred present mode if available returning FIFO if its unavailable
VkPresentModeKHR _vk2dRendererGetPresentMode(VkPresentModeKHR mode);

// Keeps track of a user shader in case the swapchain is reset, returns VK2D_ERROR if error
VK2DResult _vk2dRendererAddShader(VK2DShader shader);

// Stops tracking a user-created shader
void _vk2dRendererRemoveShader(VK2DShader shader);

// Hashes a list of descriptor set
uint64_t _vk2dHashSets(VkDescriptorSet *sets, uint32_t setCount);

// Resets the bound pipeline information
void _vk2dRendererResetBoundPointers();

// Gets the size of the rendered surface
void _vk2dRendererGetSurfaceSize();

/****************************** Renderer Initialization/Destruction ******************************/

void _vk2dRendererCreateDebug();
void _vk2dRendererDestroyDebug();
void _vk2dRendererCreateWindowSurface();
void _vk2dRendererDestroyWindowSurface();
void _vk2dRendererCreateSwapchain();
void _vk2dRendererDestroySwapchain();
void _vk2dRendererCreateDepthBuffer();
void _vk2dRendererDestroyDepthBuffer();
void _vk2dRendererCreateDescriptorBuffers();
void _vk2dRendererDestroyDescriptorBuffers();
void _vk2dRendererCreateColourResources();
void _vk2dRendererDestroyColourResources();
void _vk2dRendererCreateRenderPass();
void _vk2dRendererDestroyRenderPass();
void _vk2dRendererCreateDescriptorSetLayouts();
void _vk2dRendererDestroyDescriptorSetLayout();
void _vk2dRendererCreatePipelines();
void _vk2dRendererDestroyPipelines(bool preserveCustomPipes);
void _vk2dRendererCreateFrameBuffer();
void _vk2dRendererDestroyFrameBuffer();
void _vk2dRendererCreateUniformBuffers(bool newCamera);
void _vk2dRendererDestroyUniformBuffers();
void _vk2dRendererCreateSpriteBatching();
void _vk2dRendererDestroySpriteBatching();
void _vk2dRendererCreateDescriptorPool(bool preserveDescCons);
void _vk2dRendererDestroyDescriptorPool(bool preserveDescCons);
void _vk2dRendererCreateSynchronization();
void _vk2dRendererDestroySynchronization();
void _vk2dRendererCreateSampler();
void _vk2dRendererDestroySampler();
void _vk2dRendererCreateUnits();
void _vk2dRendererDestroyUnits();
void _vk2dRendererRefreshTargets();
void _vk2dRendererDestroyTargetsList();
void _vk2dRendererResetSwapchain();

/****************************** Back-end Drawing ******************************/

void _vk2dRendererDrawRaw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight, VK2DCameraIndex cam);
void _vk2dRendererDrawRawShadows(VkDescriptorSet set, VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object, vec4 colour, vec2 lightSource, VK2DCameraIndex cam);
void _vk2dRendererDrawRawInstanced(VkDescriptorSet *sets, uint32_t setCount, VK2DDrawInstance *instances, int count, VK2DCameraIndex cam);
void _vk2dRendererDrawInstanced(VkDescriptorSet *sets, uint32_t setCount, VK2DDrawInstance *instances, int count);
void _vk2dRendererDraw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight);
void _vk2dRendererDrawShadows(VK2DShadowEnvironment shadowEnvironment, vec4 colour, vec2 lightSource);
void _vk2dRendererDrawRaw3D(VkDescriptorSet *sets, uint32_t setCount, VK2DModel model, VK2DPipeline pipe, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ, VK2DCameraIndex cam, float lineWidth);
void _vk2dRendererDraw3D(VkDescriptorSet *sets, uint32_t setCount, VK2DModel model, VK2DPipeline pipe, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ, float lineWidth);

#ifdef __cplusplus
}
#endif