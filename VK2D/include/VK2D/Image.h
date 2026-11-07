/// \file Image.h
/// \author Paolo Mazzon
/// \brief Abstraction of VkImage to make managing them easier
#pragma once
#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a Vulkan image (Users want VK2DTexture, not VK2DImage, they are different)
/// \param dev Device to create the image with
/// \param width Width of the image in pixels
/// \param height Height of the image in pixels
/// \param format Format of the image
/// \param aspectMask Aspect mask of the image
/// \param usage How the image will be used
/// \param samples MSAA level of the image (can't be more than max supported, 1 for no MSAA)
/// \return Returns the new image or NULL
VK2DImage vk2dImageCreate(VK2DLogicalDevice dev, uint32_t width, uint32_t height, VkFormat format, VkImageAspectFlags aspectMask, VkImageUsageFlags usage, VkSampleCountFlagBits samples);

/// \brief Loads an image from the disk (generates mipmaps)
/// \param dev Device to get the memory from
/// \param filename Filename of the image
/// \return Returns a new image or NULL if it failed
/// \warning Requires a renderer be initialized to work
VK2DImage vk2dImageLoad(VK2DLogicalDevice dev, const char *filename);

/// \brief Creates an image from an SDL2 surface
/// \param dev Device to create the image with
/// \param pixels Pixels to create the image with, should be 32 bit RGBA
/// \param w Width in pixels of the image
/// \param h Height in pixels of the image
/// \param mainThread Whether or not the image is created on the main thread
/// \return Returns a new image or NULL if it failed
VK2DImage vk2dImageFromPixels(VK2DLogicalDevice dev, const void *pixels, int w, int h, bool mainThread);

/// \brief Replaces a rectangle of pixels in a sampled RGBA image
/// \param img Image to update; it must have been created with transfer-destination and sampled usage, as images
/// from vk2dImageFromPixels and vk2dImageLoad are, and must be in the shader-read-only layout
/// \param pixels Tightly packed 8-bit RGBA pixels for the rectangle, w * h * 4 bytes, in the image's format
/// (sRGB for images from vk2dImageFromPixels and vk2dImageLoad)
/// \param x Left edge of the rectangle in pixels
/// \param y Top edge of the rectangle in pixels
/// \param w Width of the rectangle in pixels
/// \param h Height of the rectangle in pixels
/// \return Returns true if the pixels were uploaded, false if an argument was invalid or a resource could not be
/// created (the reason is raised as a status)
/// \warning Not thread-safe: call it from the thread that owns the renderer. It submits its own command buffer and
/// waits for the device queue to go idle, so it suits occasional uploads such as video frames, not per-draw use.
/// \warning The new pixels apply to every draw of the frame being recorded, including draws recorded before the call.
bool vk2dImageUpdate(VK2DImage img, const void *pixels, int x, int y, int w, int h);

/// \brief Frees an image from memory
/// \param img Image to free
void vk2dImageFree(VK2DImage img);

#ifdef __cplusplus
};
#endif