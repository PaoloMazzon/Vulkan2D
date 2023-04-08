/// \file Constants.h
/// \author Paolo Mazzon
/// \brief Defines some constants
#pragma once
#include "VK2D/Structs.h"
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Tells vk2dPhysicalDeviceFind to use the best device it finds (basically just the first non-integrated it finds that meets criteria)
extern const int32_t VK2D_DEVICE_BEST_FIT;

/// Default configuration of this renderer
extern const VkApplicationInfo VK2D_DEFAULT_CONFIG;

/// How many sets to allocate in a pool at a time (10 should be a good amount)
extern const uint32_t VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;

/// How many array slots to allocate at a time with realloc (to avoid constantly reallocating memory)
extern const uint32_t VK2D_DEFAULT_ARRAY_EXTENSION;

/// Used to specify that a variable is not present in a shader
extern const uint32_t VK2D_NO_LOCATION;

/// Draw to the screen and not a texture
extern const VK2DTexture VK2D_TARGET_SCREEN;

/// Colour mod when the renderer first initializes, and is likely the most common colour mod (white)
extern const vec4 VK2D_DEFAULT_COLOUR_MOD;

/// Number of vertices to build the unit circle with if VK2D_UNIT_GENERATION is enabled. Higher values will result in a smoother circle but will be longer to initially create, slower to render, and consume more VRAM
/// At 36, you're looking at something more than good enough for most pixel art games. At 360 you're looking at something silky-smooth for most things.
extern const float VK2D_CIRCLE_VERTICES;

/// How big the default descriptor buffer should be, as well as how much it will
/// grow by whenever it needs to grow.
extern const VkDeviceSize VK2D_DESCRIPTOR_BUFFER_EXTEND_SIZE;

/// Maximum number of frames to be processed at once - You generally want this and VK2D_DEVICE_COMMAND_POOLS to be the same
#define VK2D_MAX_FRAMES_IN_FLIGHT 3

/// Not terribly difficult to figure out the usages of this
#define VK2D_PI 3.14159265358979323846264338327950

/// Number representing an invalid camera
extern VK2DCameraIndex VK2D_INVALID_CAMERA;

/// Maximum number of cameras that can exist, enabled or disabled, at once
#define VK2D_MAX_CAMERAS 10

/// Maximum number of frames to be processed at once
#define VK2D_MAX_FRAMES_IN_FLIGHT 3

/// The default camera created and updated by the renderer itself
extern const VK2DCameraIndex VK2D_DEFAULT_CAMERA;

/************************ Colours ************************/
/// The colour black
extern const vec4 VK2D_BLACK;

/// The colour white
extern const vec4 VK2D_WHITE;

/// The colour blue
extern const vec4 VK2D_BLUE;

/// The colour red
extern const vec4 VK2D_RED;

/// The colour green
extern const vec4 VK2D_GREEN;


#ifdef __cplusplus
};
#endif