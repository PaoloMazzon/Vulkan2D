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
	VK2DLogicalDevice dev;   ///< Device this belongs to
	VK2DBuffer uniforms[VK2D_MAX_FRAMES_IN_FLIGHT];  ///< Uniform buffers
	VkDescriptorSet sets[VK2D_MAX_FRAMES_IN_FLIGHT]; ///< Descriptor sets for the uniform buffers
};

/// \brief Creates a shader you can use to render textures
/// \param dev Device to create the shader with
/// \param vertexShader File containing the compiled SPIR-V vertex shader
/// \param fragmentShader File containing the compiled SPIR-V fragment shader
/// \param uniformBufferSize Size of the shader's expected uniform buffer (0 is valid)
/// \return Returns a new shader or NULL
/// \warning There are strict requirements for how the shader should be constructed
///
/// The following layout variables are a requirement for shaders uploaded to this
/// function.
///
/// Vertex shader layouts:
///
///     layout(set = 0, binding = 0) uniform UniformBufferObject {
///         mat4 view;
///         mat4 proj;
///     } ubo;
///     layout(push_constant) uniform PushBuffer {
///         mat4 model;
///         vec4 colourMod;
///     } pushBuffer;
///     layout(location = 0) in vec3 inPosition;
///     layout(location = 1) in vec4 inColor;
///     layout(location = 2) in vec2 inTexCoord;
///     layout(location = 0) out vec4 fragColor;
///     layout(location = 1) out vec2 fragTexCoord;
///
/// Fragment shader layouts:
///
///     layout(set = 1, binding = 1) uniform sampler texSampler;
///     layout(set = 2, binding = 2) uniform texture2D tex;
///     layout(push_constant) uniform PushBuffer {
///         mat4 model;
///         vec4 colourMod;
///     } pushBuffer;
///     layout(location = 0) in vec4 fragColor;
///     layout(location = 1) in vec2 fragTexCoord;
///     layout(location = 0) out vec4 outColor;
///
/// Optionally if you specify a size for uniform data (if you need
/// to pass data to your shaders) you must also specify the following
/// layout in both the fragment and vertex shader:
///
///     layout(set = 3, binding = 3) uniform UserData {
///         // your data here...
///     } userData;
///
/// The specified uniform buffer size must match the size of the data you
/// use in `UserData` exactly and it must also be a multiple of 4.
VK2DShader vk2dShaderCreate(VK2DLogicalDevice dev, const char *vertexShader, const char *fragmentShader, uint32_t uniformBufferSize);

/// \brief Updates a uniform in a shader
/// \param shader Shader to update the uniform data for
/// \param data Data to upload to the uniform
/// \param size Size of data in bytes
/// \warning Do not call this more than once a frame
/// \warning If you specified 0 for `size` in vk2dShaderCreate you cannot call this
void vk2dShaderUpdate(VK2DShader shader, void *data, uint32_t size);

/// \brief Frees a shader from memory
/// \param shader Shader to free
void vk2dShaderFree(VK2DShader shader);

#ifdef __cplusplus
}
#endif