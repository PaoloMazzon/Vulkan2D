/// \file Structs.h
/// \author Paolo Mazzon
/// \brief Forward declares struct typedefs
#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Abstraction to make managing pointers easier for VK2DRenderer
typedef struct VK2DRenderer *VK2DRenderer;

/// \brief Abstraction to make managing pointers easier for VK2DImage
typedef struct VK2DImage *VK2DImage;

/// \brief Abstraction to make managing pointers easier for VK2DLogicalDevice
typedef struct VK2DLogicalDevice *VK2DLogicalDevice;

/// \brief Abstraction to make managing pointers easier for VK2DPhysicalDevice
typedef struct VK2DPhysicalDevice *VK2DPhysicalDevice;

/// \brief Abstraction to make managing pointers easier for VK2DBuffer
typedef struct VK2DBuffer *VK2DBuffer;

/// \brief Abstraction to make managing pointers easier for VK2DPipeline
typedef struct VK2DPipeline *VK2DPipeline;

/// \brief Abstraction to make managing pointers easier for VK2DTexture
typedef struct VK2DTexture *VK2DTexture;

/// \brief Abstraction to make managing pointers easier for VK2DDescCon
typedef struct VK2DDescCon *VK2DDescCon;

/// \brief Abstraction to make managing pointers easier for VK2DPolygon
typedef struct VK2DPolygon *VK2DPolygon;

/// \brief Abstraction to make managing pointers easier for VK2DShader
typedef struct VK2DShader *VK2DShader;

/// \brief 2D vector of floats
typedef float vec2[2];

/// \brief 3D vector of floats
typedef float vec3[3];

/// \brief 4D vector of floats
typedef float vec4[4];

/// \brief 4x4 matrix of floats
typedef float mat4[16];

/// \brief Vertex data for rendering images
typedef struct {
	vec3 pos;    ///< Position of the vertex
	vec4 colour; ///< Colour of this vertex
	vec2 tex;    ///< Texture coordinate
} VK2DVertexTexture;

/// \brief Vertex data for rendering shapes
typedef struct {
	vec3 pos;    ///< Position of this vertex
	vec4 colour; ///< Colour of this vertex
} VK2DVertexColour;

/// \brief The VP buffer
typedef struct {
	mat4 view;  ///< View matrix
	mat4 proj;  ///< Projection matrix
} VK2DUniformBufferObject;

/// \brief Buffer passed per-model via push constants
typedef struct {
	mat4 model;     ///< Model matrix
	vec4 colourMod; ///< Current colour modifier
} VK2DPushBuffer;

/// \brief Describes what kind of vertices are in use
typedef enum {
	vt_Texture = 0, ///< Vertex meant for the texture pipeline
	vt_Shape = 1,   ///< Vertex meant for the shapes pipelines
	vt_Other = 2    ///< Unspecified vertex type
} VK2DVertexType;

/// \brief Multisampling detail
///
/// While Vulkan does technically support 64 samples per pixel,
/// there exists no device in the hardware database that supports
/// it. Higher values look smoother but have a bigger impact on
/// performance. Should you request an msaa larger than the device
/// supports the maximum supported msaa is used.
typedef enum {
	msaa_1x = VK_SAMPLE_COUNT_1_BIT,   ///< 1 sample per pixel
	msaa_2x = VK_SAMPLE_COUNT_2_BIT,   ///< 2 samples per pixel
	msaa_4x = VK_SAMPLE_COUNT_4_BIT,   ///< 4 samples per pixel
	msaa_8x = VK_SAMPLE_COUNT_8_BIT,   ///< 8 samples per pixel
	msaa_16x = VK_SAMPLE_COUNT_16_BIT, ///< 16 samples per pixel
	msaa_32x = VK_SAMPLE_COUNT_32_BIT, ///< 32 samples per pixel
} VK2DMSAA;

/// \brief How to present images
///
/// This is system dependent and its possible for a system to not support
/// sm_Immediate or sm_TripleBuffer. While it is technically possible to not
/// support VSync, the Vulkan spec states it must be supported and the hardware
/// database agrees with that so VK2D assumes VSync is always supported. Should
/// you request a mode that is not available, the option will default to sm_Vsync.
typedef enum {
	sm_Immediate = VK_PRESENT_MODE_IMMEDIATE_KHR, ///< Quickest mode, just plop to screen but may have screen tearing
	sm_VSync = VK_PRESENT_MODE_FIFO_KHR,          ///< Slower but prevents screen tearing
	sm_TripleBuffer = VK_PRESENT_MODE_MAILBOX_KHR ///< Optimal for gaming but a bit slower than immediate (machines may not support this)
} VK2DScreenMode;

/// \brief Specifies how textures will be filtered at higher and lower resolutions
typedef enum {
	ft_Linear = VK_FILTER_LINEAR,  ///< Linear interpolation, good for most things
	ft_Nearest = VK_FILTER_NEAREST ///< Nearest neighbor filter, good for pixel art
} VK2DFilterType;

/// \brief A bitwise-able enum representing different shader stages
///
/// Right now VK2D only supports vertex and fragment shaders, but it is possible that
/// it may support geometry and tesselation in the future (I don't see a use case right now).
typedef enum {
	ss_Fragment = VK_SHADER_STAGE_FRAGMENT_BIT, ///< Fragment (pixel) shader
	ss_Vertex = VK_SHADER_STAGE_VERTEX_BIT      ///< Vertex shader
} VK2DShaderStage;

/// \brief User configurable settings
typedef struct VK2DConfiguration {
	const char* applicationName; ///< Name of this program
	const char* engineName;      ///< Name of this engine
	uint32_t applicationVersion; ///< Version of the program
	uint32_t engineVersion;      ///< Version of this engine
	uint32_t apiVersion;         ///< Version of vulkan
} VK2DConfiguration;

/// \brief User configurable settings
typedef struct VK2DRendererConfig {
	VK2DMSAA msaa;             ///< Current MSAA
	VK2DScreenMode screenMode; ///< Current screen mode
	VK2DFilterType filterMode; ///< How to filter textures
} VK2DRendererConfig;

/// \brief Camera information
typedef struct VK2DCamera {
	float x;    ///< X position of the camera (top left coordinates)
	float y;    ///< Y position of the camera (top left coordinates)
	float w;    ///< Virtual width of the screen
	float h;    ///< Virtual height of the screen
	float zoom; ///< Zoom percentage (Relative to the virtual width and height, not actual)
	float rot;  ///< Rotation of the camera
} VK2DCamera;
