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

/// Tells drawing functions to use the full width of the texture instead of specifying width/height yourself
extern const float VK2D_FULL_TEXTURE;

/// Default configuration of this renderer
extern const VkApplicationInfo VK2D_DEFAULT_CONFIG;

/// How many sets to allocate in a pool at a time (10 should be a good amount)
extern const uint32_t VK2D_DEFAULT_DESCRIPTOR_POOL_ALLOCATION;

/// ID representing an invalid pipeline or one not existing
extern const int32_t VK2D_PIPELINE_ID_NONE;

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

/// Maximum number of frames to be processed at once - You generally want this and VK2D_DEVICE_COMMAND_POOLS to be the same
#define VK2D_MAX_FRAMES_IN_FLIGHT 3

/// First 33 digits of pi
#define VK2D_PI 3.14159265358979323846264338327950

/// \brief Converts degrees to radians
#define VK2D_DEGREES(degrees) ((degrees) * (VK2D_PI/180.0))

/// \brief Converts radians to degrees
#define VK2D_RADIANS(radians) ((radians) * (180.0/VK2D_PI))

/// Number representing an invalid camera
extern VK2DCameraIndex VK2D_INVALID_CAMERA;

/// Maximum number of frames to be processed at once
#define VK2D_MAX_FRAMES_IN_FLIGHT 3

/// The default camera created and updated by the renderer itself
extern const VK2DCameraIndex VK2D_DEFAULT_CAMERA;

extern VK2DShadowObject VK2D_INVALID_SHADOW_OBJECT;

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