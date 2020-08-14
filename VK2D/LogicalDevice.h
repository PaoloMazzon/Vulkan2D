/// \file LogicalDevice.h
/// \author Paolo Mazzon
/// \brief Declares functionality as it pertains to VkDevice
#pragma once
#include <vulkan/vulkan.h>
#include "VK2D/Constants.h"
#include "VK2D/Structs.h"

/// \brief Logical device that is essentially a wrapper of VkDevice
struct VK2DLogicalDevice {
	VkDevice dev;  ///< Logical device
	VkQueue queue; ///< Queue for command buffers
	VkCommandPool pool[VK2D_DEVICE_COMMAND_POOLS]; ///< Command pools to cycle through
};

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
///
/// samplerAnisotropy is for nice looking edges when it is so desired, and the others
/// are for drawing shapes without being filled in (Something games often want to do).
VK2DLogicalDevice vk2dLogicalDeviceCreate(VK2DPhysicalDevice dev, bool enableAllFeatures, bool graphicsDevice);

/// \brief Frees a logical device from memory
void vk2dLogicalDeviceFree(VK2DLogicalDevice dev);

/// \brief Gets a command buffer from a device
/// \param dev Logical device to get the buffer from
/// \param pool Pool in the device to get the buffer from
/// \return Returns a VkCommandBuffer in the initial state (see Vulkan spec for more info on command buffer states)
VkCommandBuffer vk2dLogicalDeviceGetCommandBuffer(VK2DLogicalDevice dev, uint32_t pool);

/// \brief Frees a command buffer
/// \param dev Logical device the buffer belongs to
/// \param buffer Buffer to free
/// \param pool Pool the buffer belongs to
void vk2dLogicalDeviceFreeCommandBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer, uint32_t pool);

/// \brief Gets a command buffer created with the single use flag
/// \param dev Logical device to use
/// \param pool Which command pool to make the buffer from
/// \return Returns a new command buffer ready to have commands recorded to it
VkCommandBuffer vk2dLogicalDeviceGetSingleUseBuffer(VK2DLogicalDevice dev, uint32_t pool);

/// \brief Submits a single use command buffer
void vk2dLogicalDeviceSubmitSingleBuffer(VK2DLogicalDevice dev, VkCommandBuffer buffer, uint32_t pool);