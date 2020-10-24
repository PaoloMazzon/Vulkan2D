/// \file Texture.h
/// \author Paolo Mazzon
/// \brief Makes managing textures samplers and off-screen rendering simpler
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Takes the headache out of Vulkan textures
///
/// Only textures created with vk2dTextureCreate may be rendered to. Should you try to
/// set the render target to a texture not created with vk2dTextureCreate, you can expect
/// a segfault.
struct VK2DTexture {
	VK2DImage img;          ///< Internal image
	VK2DImage sampledImg;   ///< Image for MSAA
	VkFramebuffer fbo;      ///< Framebuffer of this texture so it can be drawn to
	VK2DBuffer ubo;         ///< UBO that will be used when drawing to this texture
	VkDescriptorSet uboSet; ///< Set for the UBO
	bool imgHandled;        ///< Whether or not to free the image with the texture (if it was loaded with vk2dTextureLoad)
};

/// \brief Creates a texture from an image
/// \param image Image to use
/// \return Returns a new texture or NULL if it failed
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureLoadFromImage(VK2DImage image);

/// \brief Loads a texture from a file (png, bmp, jpg, tiff)
/// \param filename File to load
/// \return Returns a new texture or NULL if it failed
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureLoad(const char *filename);

/// \brief Creates a texture meant as a drawing target
/// \param dev Device to create the texture with (this isn't required in vk2dTextureLoad because the image you pass it already knows what device to use)
/// \param w Width of the texture
/// \param h Height of the texture
/// \return Returns a new texture or NULL if it failed
/// \warning Textures are closely tied to the renderer and require the renderer be initialized to load and use them
VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h);

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

#ifdef __cplusplus
};
#endif
