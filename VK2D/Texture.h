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
	VkDescriptorImageInfo imgInfo; ///< Info on how to send it through uniforms
	uint32_t w;                    ///< Width of the texture
	uint32_t h;                    ///< Height of the texture
	VkFramebuffer fbo;             ///< Framebuffer of this texture so it can be drawn to
};

#ifdef __cplusplus
};
#endif
