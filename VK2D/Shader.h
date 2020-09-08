/// \file Shader.h
/// \author Paolo Mazzon
/// \brief Makes shaders possible in VK2D
#pragma once
#include "VK2D/Structs.h"
#include "VK2D/Constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Wrapper for data needed to manage a shader
///
/// There are some limitations of shaders and some things to be aware of. For one, you
/// can only have one uniform buffer that is sent to both shaders. Also, for each shader,
/// the renderer must go through and increment the current uniform buffer every frame to
/// make sure you don't modify one that is currently in an executing command buffer.
///
/// And most importantly, these shaders are all treated as texture shaders, which is to
/// say they will all receive the same data that the tex.vert and tex.frag shaders would
/// (push constants, vertex attributes, and uniforms) in addition to a user-defined uniform
/// buffer.
struct VK2DShader {
	unsigned char *spvVert;  ///< Vertex shader in SPIR-V
	uint32_t spvVertSize;    ///< Size of the vertex shader (in bytes)
	unsigned char *spvFrag;  ///< Fragment shader in SPIR-V
	uint32_t spvFragSize;    ///< Size of the fragment shader (in bytes)
	VK2DPipeline pipe;       ///< Pipeline associated with this shader
	uint32_t uniformSize;    ///< Uniform buffer size in bytes
	uint32_t currentUniform; ///< Current uniform buffer that is allowed to be modified
	VK2DBuffer uniforms[VK2D_MAX_FRAMES_IN_FLIGHT]; ///< Uniform buffers
};

// TODO: This

#ifdef __cplusplus
}
#endif