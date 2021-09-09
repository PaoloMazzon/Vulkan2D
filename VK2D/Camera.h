/// \file Camera.h
/// \author Paolo Mazzon
/// \brief User-level camera things
#pragma once
#include "VK2D/Structs.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief An internal representation of a camera (the user deals with VK2DCameraIndex, the renderer uses this struct)
///
/// Due to a camera's nature of being closely tied to the renderer, the renderer
/// will automatically update the relevant parts of a camera whenever needed.
struct VK2DCamera {
	VK2DCameraSpec spec;           ///< Info on how to create the UBO and scissor/viewport
	VK2DUniformBufferObject *ubos; ///< UBO data for each frame
	VK2DBuffer *buffers;           ///< Used for updating the UBOs
	VkDescriptorSet *uboSets;      ///< List of UBO sets, 1 per swapchain image
	VK2DCameraState state;         ///< State of this camera
};

/// \brief Creates a new camera and returns the index, or returns VK2D_INVALID_CAMERA if no more cameras can be created
/// \param spec Initial state for the camera
/// \return Either a new camera index or VK2D_INVALID_CAMERA if the camera limit has been reached
///
/// Cameras are created in the state `cs_Normal`.
VK2DCameraIndex vk2dCameraCreate(VK2DCameraSpec spec);

/// \brief Updates a camera with new positional data
/// \param index Index of the camera to update (camera must not be in the state `cs_Deleted`)
/// \param spec New camera spec to apply
///
/// This can be called at any time, but the actual camera is only updated whenever `vk2dRendererStartFrame` is
/// called. Any calls to this function will only be visible the next time `vk2dRendererStartFrame` is called.
void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec);

/// \brief Sets the state of a camera
/// \param index Index of the camera to update
/// \param state The new state of the camera, if its `cs_Deleted` the camera is completely invalidated
///
/// Cameras are automatically deleted with the renderer, there is no need to free each one yourself
void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state);

/// \brief Gets the state of a camera
/// \param index Index of the camera to get the state of
/// \return Returns the state of the camera
VK2DCameraState vk2dCameraGetState(VK2DCameraIndex index);

#ifdef __cplusplus
}
#endif