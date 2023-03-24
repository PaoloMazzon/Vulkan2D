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
/// buffer if one is specified.
struct VK2DShader {
	uint8_t *spvVert;        ///< Vertex shader in SPIR-V
	uint32_t spvVertSize;    ///< Size of the vertex shader (in bytes)
	uint8_t *spvFrag;        ///< Fragment shader in SPIR-V
	uint32_t spvFragSize;    ///< Size of the fragment shader (in bytes)
	VK2DPipeline pipe;       ///< Pipeline associated with this shader
	uint32_t uniformSize;    ///< Uniform buffer size in bytes
	uint32_t currentUniform; ///< Current uniform buffer that is allowed to be modified
	VK2DLogicalDevice dev;   ///< Device this belongs to
	VK2DBuffer uniforms[VK2D_MAX_FRAMES_IN_FLIGHT];  ///< Uniform buffers
	VkDescriptorSet sets[VK2D_MAX_FRAMES_IN_FLIGHT]; ///< Descriptor sets for the uniform buffers
};

/// \brief Creates a shader you can use to render textures
/// \param vertexShader File containing the compiled SPIR-V vertex shader
/// \param fragmentShader File containing the compiled SPIR-V fragment shader
/// \param uniformBufferSize Size of the shader's expected uniform buffer (0 is valid, must be a multiple of 4)
/// \return Returns a new shader or NULL
/// \warning There are strict requirements for how the shader should be constructed
///
/// Check the shaders shaders/tex.vert and shaders/tex.frag for information on how
/// the shaders should be laid out. Additionally, if you provide a uniformBufferSize other
/// than 0, you must specify
///
///     layout(set = 3, binding = 3) uniform UserData {
///         float myval; // examples, you may pass whatever here.
///         vec3 mycolour;
///         // your data here...
///     } userData;
///
/// At the top of both shaders.
VK2DShader vk2dShaderLoad(const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize);

/// \brief Creates a shader you can use to render textures from an in-memory buffer
/// \param vertexShaderBuffer Buffer containing compiled SPIR-V shader code
/// \param vertexShaderBufferSize Size of the vertexShaderBuffer buffer in bytes
/// \param fragmentShaderBuffer File containing the compiled SPIR-V fragment shader
/// \param fragmentShaderBufferSize Size of the fragmentShaderBuffer buffer in bytes
/// \param uniformBufferSize Size of the shader's expected uniform buffer (0 is valid, must be a multiple of 4)
/// \return Returns a new shader or NULL
/// \warning There are strict requirements for how the shader should be constructed
///
/// Check the shaders shaders/tex.vert and shaders/tex.frag for information on how
/// the shaders should be laid out. Additionally, if you provide a uniformBufferSize other
/// than 0, you must specify
///
///     layout(set = 3, binding = 3) uniform UserData {
///         float myval; // examples, you may pass whatever here.
///         vec3 mycolour;
///         // your data here...
///     } userData;
///
/// At the top of both shaders.
VK2DShader vk2dShaderFrom(uint8_t *vertexShaderBuffer, int vertexShaderBufferSize, uint8_t *fragmentShaderBuffer, int fragmentShaderBufferSize, uint32_t uniformBufferSize);

/// \brief Updates a uniform in a shader
/// \param shader Shader to update the uniform data for
/// \param data Data to upload to the uniform
/// \warning Do not call this more than once a frame
/// \warning If you specified 0 for `size` in vk2dShaderLoad you cannot call this
///
/// Because of how VK2D works you cannot just call this once and depend on the same
/// data being present in following frames. VK2D updates (by default) 3 frames at a
/// time so internally there are 3 shaders buffers meaning you must call this 3 times
/// before the same data is present on each buffer, but much more practically just call
/// this once a frame.
void vk2dShaderUpdate(VK2DShader shader, void *data);

/// \brief Frees a shader from memory
/// \param shader Shader to free
void vk2dShaderFree(VK2DShader shader);

#ifdef __cplusplus
}
#endif