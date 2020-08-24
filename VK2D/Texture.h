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
struct VK2DTexture {
	VkSampler imgSampler;          ///< Sampler to make it shader visible
	VK2DImage img;                 ///< Internal image
	VK2DPolygon bounds;            ///< Needed to render and more so to store the texture coordinates
	VkFramebuffer fbo;             ///< Framebuffer of this texture so it can be drawn to
};

/// \brief Creates a texture from an image
/// \param image Image to use
/// \param xInImage X position in the image the texture begins (in pixels)
/// \param yInImage Y position in the image the texture begins (in pixels)
/// \param wInImage Width of the image to draw (in pixels)
/// \param hInImage Height of the image to draw (in pixels)
/// \param filter Filter to use when scaling the texture
/// \param anisotropy What MSAA level to use
/// \return Returns a new texture or NULL if it failed
///
/// The *InImage parameters specify what regions of the image the texture is to draw. This is
/// essentially just so you can have sprite sheets.
VK2DTexture vk2dTextureLoad(VK2DImage image, float xInImage, float yInImage, float wInImage, float hInImage, VkBool32 filter, float anisotropy);

/// \brief Creates a texture meant as a drawing target
/// \param dev Device to create the texture with (this isn't required in vk2dTextureLoad because the image you pass it already knows what device to use)
/// \param w Width of the texture
/// \param h Height of the texture
/// \param filter Filter to use when scaling the texture
/// \param anisotropy What MSAA level to use
/// \return Returns a new texture or NULL if it failed
VK2DTexture vk2dTextureCreate(VK2DLogicalDevice dev, float w, float h, VkBool32 filter, float anisotropy);

/// \brief Frees a texture from memory
/// \param tex Texture to free
void vk2dTextureFree(VK2DTexture tex);

#ifdef __cplusplus
};
#endif
