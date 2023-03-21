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
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureLoad(const char *filename);

/// \brief Same as vk2dTextureLoad but it uses a byte buffer instead of pulling from a file
/// \param data Pointer to the image data, either png, bmp, jpg, or tiff
/// \param size Size in bytes of the data buffer
/// \return Returns a new texture or NULL if it failed
/// \warning Textures created with this function are NOT valid render targets
VK2DTexture vk2dTextureFrom(void *data, int size);

/// \brief Creates a texture meant as a drawing target
/// \param w Width of the texture
/// \param h Height of the texture
/// \return Returns a new texture or NULL if it failed
VK2DTexture vk2dTextureCreate(float w, float h);

/// \brief Gets the width in pixels of a texture
/// \param tex Texture to get the width from
/// \return Returns the width in pixels
float vk2dTextureWidth(VK2DTexture tex);

/// \brief Gets the height in pixels of a texture
/// \param tex Texture to get the height from
/// \return Returns the height in pixels
float vk2dTextureHeight(VK2DTexture tex);

/// \brief Checks if a texture was created as a target or not
/// \param Texture to check
/// \return Returns true if its a valid target, false otherwise
bool vk2dTextureIsTarget(VK2DTexture tex);

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

#ifdef __cplusplus
};
#endif
