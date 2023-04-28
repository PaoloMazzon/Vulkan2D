/// \file LogicalDevice.h
/// \author Paolo Mazzon
/// \brief Declares functionality as it pertains to VkDevice
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Constants.h"
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Creates a logical device for rendering
/// \param dev Physical device to create this from
/// \param enableAllFeatures If enabled, all features of the physical device will be enabled, otherwise just the features the renderer requires
/// \param graphicsDevice Decides what queue family to create the device with (true for graphics, false for compute)
/// \return Returns a VK2DLogicalDevice or NULL if it failed
///
/// The features that will be enabled if enableAllFeatures is false are as follows:
///
///  - wideLines
///  - fillModeNonSolid
///  - samplerAnisotropy
///  - shaderStorageImageMultisample
///
/// samplerAnisotropy is for nice looking edges when it is so desired, and the others
/// are for drawing shapes without being filled in (Something games often want to do).
///
/// Because most things use and store a pointer to a logical device in this renderer, it is vital
/// that you always free this last (assuming you're not just letting the renderer take care of all
/// this). It could be seen as spaghetti to have something's pointer all over the place, but since
/// A) the vulkan spec says all things allocated on the device must be freed before the device and
/// B) the VK2DLogicalDevice will likely never change beyond the command pools getting reset, its
/// not at all unreasonable to just store the logical device on anything that needs it constantly.
/// TL;DR, always free VK2DLogicalDevice last (or ignore this if you're using VK2DRenderer).
VK2DLogicalDevice vk2dLogicalDeviceCreate(VK2DPhysicalDevice dev, bool enableAllFeatures, bool graphicsDevice, bool debug, VK2DRendererLimits *limits);

/// \brief Frees a logical device from memory
void vk2dLogicalDeviceFree(VK2DLogicalDevice dev);

/// \brief Resets a command pool
/// \param dev Device containing the pool
/// \param pool Pool index to reset
/// \warning Resetting a pool does not free its command buffers, it just puts them in the initial state
void vk2dLogicalDeviceResetPool(VK2DLogicalDevice dev);

/// \brief Gets a command buffer from a device
/// \param dev Logical device to get the buffer from
/// \param pool Pool in the device to get the buffer from
/// \return Returns a VkCommandBuffer in the initial state (see Vulkan spec for more info on command buffer states)
VkCommandBuffer vk2dLogicalDeviceGetCommandBuffer(VK2DLogicalDevice dev, bool primary);

/// \brief Fills a list with new command buffers
/// \param dev Device to get the buffers from
/// \param pool Pool to get the buffers from
/// \param primary Are they primary command buffers or not
/// \param n Amount of command buffers to allocate
/// \param list List to put them in (should be at least n * sizeof(VkCommandBuffer) in size)
void vk2dLogicalDeviceGetCommandBuffers(VK2DLogicalDevice dev, bool primary, uint32_t n, VkCommandBuffer *list);

/// \brief Frees a command buffer
/// \param dev Logical device the buffer belongs to
/// \param buffer Buffer to free
/// \param pool Pool the buffer belongs to
void vk2dLogicalDeviceFreeCommandBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer);

/// \brief Gets a command buffer created with the single use flag
/// \param dev Logical device to use
/// \param mainThread Whether or not the command buffer should be in the main thread or not
/// \return Returns a new command buffer in the recording state
VkCommandBuffer vk2dLogicalDeviceGetSingleUseBuffer(VK2DLogicalDevice dev, bool mainThread);

/// \brief Submits a single use command buffer
/// \param dev Device it belongs to
/// \param buffer Command buffer to submit
/// \param mainThread Whether or not the command buffer should be in the main thread or not
///
/// To be more specific, this will submit the command buffer then wait for the queue
/// to idle. After that it will free the buffer.
void vk2dLogicalDeviceSubmitSingleBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer, bool mainThread);

/// \brief Grabs a fence from a logical device
/// \param dev Logical device to get the fence from
/// \param flags Flags to use when creating the fence (Refer to Vulkan spec)
/// \return Returns a new fence
VkFence vk2dLogicalDeviceGetFence(VK2DLogicalDevice dev, VkFenceCreateFlagBits flags);

/// \brief Frees a fence
/// \param dev Device that the fence came from
/// \param fence Fence to free
void vk2dLogicalDeviceFreeFence(VK2DLogicalDevice dev, VkFence fence);

/// \brief Grabs a semaphore from a logical device
/// \param dev Device to get the semaphore from
/// \return Returns the new semaphore
VkSemaphore vk2dLogicalDeviceGetSemaphore(VK2DLogicalDevice dev);

/// \brief Frees a semaphore
/// \param dev Device the semaphore came from
/// \param semaphore Semaphore to free
void vk2dLogicalDeviceFreeSemaphore(VK2DLogicalDevice dev, VkSemaphore semaphore);

#ifdef __cplusplus
};
#endif