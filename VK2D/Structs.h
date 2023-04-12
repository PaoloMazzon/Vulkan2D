/// \file Structs.h
/// \author Paolo Mazzon
/// \brief Forward declares struct typedefs
#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// For opaque pointer types
#define VK2D_OPAQUE_POINTER(type) typedef struct type *type;

// For user-modifiable and user-visible structures
#define VK2D_USER_STRUCT(type) typedef struct type type;

/// \brief Describes what kind of vertices are in use
typedef enum {
	vt_Texture = 0, ///< Vertex meant for the texture pipeline
	vt_Shape = 1,   ///< Vertex meant for the shapes pipelines
	vt_Model = 2,   ///< Vertex meant for models
	vt_Other = 3,   ///< Unspecified vertex type
} VK2DVertexType;

/// \brief Blend modes that can be used to render if VK2D_GENERATE_BLEND_MODES is enabled
///
/// The blend modes are kind of limited because very few people need more than these, but
/// if you require a different blend mode there is only 3 steps to adding it:
///
/// + Add a new blend mode to the end of this enumerator and increment `bm_Max`
/// + Add the new blend mode to the `VK2D_BLEND_MODES` array in BlendModes.h
/// + Make sure the indices in the array match the enumerator
typedef enum {
	bm_Blend = 0,    ///< Default blend mode, good for almost everything
	bm_None = 1,     ///< No blending, new colour is law
	bm_Add = 2,      ///< Additive blending
	bm_Subtract = 3, ///< Subtraction blending, new colour is subtracted from current colour
	bm_Max = 4       ///< Total number of blend modes (used for looping)
} VK2DBlendMode;

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
typedef enum {
	ss_Fragment = VK_SHADER_STAGE_FRAGMENT_BIT, ///< Fragment (pixel) shader
	ss_Vertex = VK_SHADER_STAGE_VERTEX_BIT      ///< Vertex shader
} VK2DShaderStage;

/// \brief The state a camera is in
typedef enum {
	cs_Normal = 0,   ///< Camera is being rendered/updated as normal
	cs_Disabled = 1, ///< Camera is not being rendered or updated
	cs_Deleted = 2,  ///< Camera is "deleted" and all data is invalid
	cs_Reset = 3,    ///< Camera is being reset by the renderer
} VK2DCameraState;

/// \brief Type of camera
typedef enum {
	ct_Default = 0,      ///< Default camera used for 2D games in VK2D
	ct_Orthographic = 1, ///< Orthographic camera for 3D rendering
	ct_Perspective = 2,  ///< Perspective camera for 3D rendering
} VK2DCameraType;

// VK2D pointers
VK2D_OPAQUE_POINTER(VK2DRenderer)
VK2D_OPAQUE_POINTER(VK2DImage)
VK2D_OPAQUE_POINTER(VK2DLogicalDevice)
VK2D_OPAQUE_POINTER(VK2DPhysicalDevice)
VK2D_OPAQUE_POINTER(VK2DBuffer)
VK2D_OPAQUE_POINTER(VK2DPipeline)
VK2D_OPAQUE_POINTER(VK2DTexture)
VK2D_OPAQUE_POINTER(VK2DDescCon)
VK2D_OPAQUE_POINTER(VK2DPolygon)
VK2D_OPAQUE_POINTER(VK2DShader)
VK2D_OPAQUE_POINTER(VK2DModel)
VK2D_OPAQUE_POINTER(VK2DDescriptorBuffer)

/// \brief 2D vector of floats
typedef float vec2[2];

/// \brief 3D vector of floats
typedef float vec3[3];

/// \brief 4D vector of floats
typedef float vec4[4];

/// \brief 4x4 matrix of floats
typedef float mat4[16];

/// \brief Type used for referencing cameras
typedef int32_t VK2DCameraIndex;

/// \brief Vertex data for rendering shapes
struct VK2DVertexColour {
	vec3 pos;    ///< Position of this vertex
	vec4 colour; ///< Colour of this vertex
};

/// \brief Vertex data for 3D models
struct VK2DVertex3D {
	vec3 pos; ///< Position of the vertex
	vec2 uv;  ///< UV coordinates for this vertex
};

/// \brief The VP buffer
struct VK2DUniformBufferObject {
	mat4 viewproj; ///< View and projection matrix multiplied together
} ;

/// \brief Buffer passed per-model via push constants
struct VK2DPushBuffer {
	mat4 model;     ///< Model matrix
	vec4 colourMod; ///< Current colour modifier
	vec4 texCoords; ///< Where in the texture to draw from and to (x, y, w, h)
};

/// \brief Push buffer used for 3D models
struct VK2D3DPushBuffer {
	mat4 model;     ///< Model matrix
	vec4 colourMod; ///< Color modifier
};

/// \brief User configurable settings
struct VK2DConfiguration {
	const char *applicationName; ///< Name of this program
	const char *engineName;      ///< Name of this engine
	uint32_t applicationVersion; ///< Version of the program
	uint32_t engineVersion;      ///< Version of this engine
	uint32_t apiVersion;         ///< Version of vulkan
};

/// \brief Startup options that dictate some basic VK2D stuff
struct VK2DStartupOptions {
	bool enableDebug;       ///< Enables Vulkan compatibility layers
	bool stdoutLogging;     ///< Print VK2D information to stdout
	bool quitOnError;       ///< Crash the program when an error occurs
	const char *errorFile;  ///< The file to output errors to, or NULL to disable file output
	bool loadCustomShaders; ///< Whether or not to load shaders from a file instead of the built-in ones
};

/// \brief User configurable settings
/// \warning Currently filterMode cannot be changed after the renderer is created but this is likely going to be fixed later
struct VK2DRendererConfig {
	VK2DMSAA msaa;             ///< Current MSAA
	VK2DScreenMode screenMode; ///< Current screen mode
	VK2DFilterType filterMode; ///< How to filter textures -- Not change-able after renderer creation
};

/// \brief Camera information
struct VK2DCameraSpec {
	VK2DCameraType  type; ///< What type of camera this is
	float x;              ///< X position of the camera (top left coordinates) (only used in default camera type)
	float y;              ///< Y position of the camera (top left coordinates) (only used in default camera type)
	float w;              ///< Virtual width of the screen
	float h;              ///< Virtual height of the screen
	float zoom;           ///< Zoom percentage (Relative to the virtual width and height, not actual)
	float rot;            ///< Rotation of the camera
	float xOnScreen;      ///< x position in the window
	float yOnScreen;      ///< y position in the window
	float wOnScreen;      ///< Width of the camera in the window
	float hOnScreen;      ///< Height of the camera in the window

	///< Things used for perspective and orthographic cameras in place of the x/y variables
	struct {
		vec3 eyes;   ///< Where the 3D camera is
		vec3 centre; ///< Where the 3D camera is looking
		vec3 up;     ///< Which direction is up for the 3D camera
		float fov;   ///< Field of view of the camera
	} Perspective;
};

/// \brief Renderer limitations for the host
///
/// Even though the host may not support something you request of the renderer
/// (for example, if you request triple buffering but the host doesn't support
/// it), VK2D will simply use the next best option. You don't "need" to worry
/// about any of the host limits, but they are available to the user in case of
/// something like an options screen where you would only want to show the user
/// the available MSAA modes, for example.
struct VK2DRendererLimits {
	VK2DMSAA maxMSAA;             ///< Maximum MSAA the host supports (may be msaa_1x)
	bool supportsTripleBuffering; ///< Whether or not the host supports triple buffering
	bool supportsImmediate;       ///< Whether or not the host supports immediate mode
	bool supportsWireframe;       ///< Whether or not the host supports wireframe rendering
	float maxLineWidth;           ///< Maximum line width supported on the platform (may be 1, VK2D will automatically clamp line width values above this limit)
};

VK2D_USER_STRUCT(VK2DVertexColour)
VK2D_USER_STRUCT(VK2DVertex3D)
VK2D_USER_STRUCT(VK2DUniformBufferObject)
VK2D_USER_STRUCT(VK2DPushBuffer)
VK2D_USER_STRUCT(VK2D3DPushBuffer)
VK2D_USER_STRUCT(VK2DConfiguration)
VK2D_USER_STRUCT(VK2DStartupOptions)
VK2D_USER_STRUCT(VK2DRendererConfig)
VK2D_USER_STRUCT(VK2DCameraSpec)
VK2D_USER_STRUCT(VK2DRendererLimits)

#ifdef __cplusplus
}
#endif