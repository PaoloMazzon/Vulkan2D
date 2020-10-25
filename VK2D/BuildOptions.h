/// \file BuildOptions.h
/// \author Paolo Mazzon
/// \brief Declares all the types (and build options) the renderer needs
#pragma once
#include <vulkan/vulkan.h>
#include <VK2D/Structs.h>

#ifdef __cplusplus
extern "C" {
#endif

// Comment any of these out to disable the option

/// Maximum number of frames to be processed at once
#define VK2D_MAX_FRAMES_IN_FLIGHT 3

/// Whether or not to enable validation layers/debugging callbacks
#define VK2D_ENABLE_DEBUG

/// Whether or not to enable printing logs to stdout
#define VK2D_STDOUT_LOGGING

/// Whether or not errors are allowed to quit the program
#define VK2D_QUIT_ON_ERROR

/// Enables printing errors to a file
#define VK2D_ERROR_FILE "vk2derror.log"

/// Creates "unit polygons" that make drawing primitives without polygons possible (takes a little bit of ram and vram)
#define VK2D_UNIT_GENERATION

/// Makes pipelines generate a bunch of blend modes (otherwise you can only use standard alpha blending).
/// Check VK2DBlendMode for more information.
#define VK2D_GENERATE_BLEND_MODES

/// This one is a bit trickier. Basically, if the renderer finds a file named `colourfrag.spv`, `colourvert.spv`,
/// `texfrag.spv`, or `texvert.spv` in the directory `shaders` (relative to its working directory), it will use
/// those shaders in place of its default ones (as found in Blobs.h). It expects them to be compiled SPIR-V
/// shader code.
#define VK2D_LOAD_CUSTOM_SHADERS

#ifdef __cplusplus
};
#endif