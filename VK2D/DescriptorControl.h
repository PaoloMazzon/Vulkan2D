/// \file DescriptorControl.h
/// \author Paolo Mazzon
/// \brief Makes dynamic descriptor control simpler
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Abstraction for descriptor pools and sets so you can dynamically use them
struct VK2DDescCon {
	VkDescriptorPool *pools;      ///< List of pools
	VkDescriptorSetLayout layout; ///< Layout for these sets
	uint32_t buffer;              ///< Whether or not pools support uniform buffers
	uint32_t sampler;             ///< Whether or not pools support texture samplers
	VK2DLogicalDevice dev;        ///< Device pools are created with

	// pools will always have poolListSize elements, but only elements up to poolsInUse will be
	// valid pools (in an effort to avoid constantly reallocating memory)
	uint32_t poolsInUse;   ///< Number of actively in use pools in pools
	uint32_t poolListSize; ///< Total length of pools array
};

/// \brief Creates an empty descriptor controller
/// \param layout Descriptor set layout to use
/// \param buffer Location of the buffer or VK2D_NO_LOCATION if unused (binding)
/// \param sampler Location of the sampler or VK2D_NO_LOCATION if unused (binding)
/// \return New descriptor controller or NULL if it failed
VK2DDescCon vk2dDescConCreate(VK2DLogicalDevice dev, VkDescriptorSetLayout layout, uint32_t buffer, uint32_t sampler);

/// \brief Frees a descriptor controller from memory
/// \param descCon Descriptor controller to free
void vk2dDescConFree(VK2DDescCon descCon);

/// \brief Creates, updates, and returns a descriptor set ready to be bound to a command buffer
/// \param descCon Descriptor controller to pull the set from
/// \param buffer Buffer to bind to the descriptor set
/// \return Returns a new descriptor set ready to be bound to a command buffer (valid until vk2dDescConReset is called)
VkDescriptorSet vk2dDescConGetBufferSet(VK2DDescCon descCon, VK2DBuffer buffer);

/// \brief Creates, updates, and returns a blank descriptor set
/// \param descCon Descriptor controller to pull the set from
/// \return Returns a new descriptor set
VkDescriptorSet vk2dDescConGetSet(VK2DDescCon descCon);

/// \brief Creates, updates, and returns a descriptor set ready to be bound to a command buffer
/// \param descCon Descriptor controller to pull the set from
/// \param tex Texture to bind to the descriptor set (namely the sampler and image view)
/// \return Returns a new descriptor set ready to be bound to a command buffer (valid until vk2dDescConReset is called)
VkDescriptorSet vk2dDescConGetSamplerSet(VK2DDescCon descCon, VK2DTexture tex);

/// \brief Creates, updates, and returns a descriptor set ready to be bound to a command buffer
/// \param descCon Descriptor controller to pull the set from
/// \param tex Texture to bind to the descriptor set
/// \param buffer Buffer to bind to the descriptor set
/// \return Returns a new descriptor set ready to be bound to a command buffer (valid until vk2dDescConReset is called)
VkDescriptorSet vk2dDescConGetSamplerBufferSet(VK2DDescCon descCon, VK2DTexture tex, VK2DBuffer buffer);

/// \brief Resets all pools in a descriptor controller (basically deletes all active sets so new ones can be allocated)
/// \param descCon Descriptor controller to reset
void vk2dDescConReset(VK2DDescCon descCon);

#ifdef __cplusplus
}
#endif