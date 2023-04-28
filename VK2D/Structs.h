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
#define VK2D_OPAQUE_POINTER(type) typedef struct type##_t *type;

// For user-modifiable and user-visible structures
#define VK2D_USER_STRUCT(type) typedef struct type type;

/// \brief Describes what kind of vertices are in use
typedef enum {
	VK2D_VERTEX_TYPE_TEXTURE = 0, ///< Vertex meant for the texture pipeline
	VK2D_VERTEX_TYPE_SHAPE = 1,   ///< Vertex meant for the shapes pipelines
	VK2D_VERTEX_TYPE_MODEL = 2,   ///< Vertex meant for models
	VK2D_VERTEX_TYPE_OTHER = 3,   ///< Unspecified vertex type
	VK2D_VERTEX_TYPE_MAX = 4      ///< Maximum number of vertex types
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
	VK2D_BLEND_MODE_BLEND = 0,    ///< Default blend mode, good for almost everything
	VK2D_BLEND_MODE_NONE = 1,     ///< No blending, new colour is law
	VK2D_BLEND_MODE_ADD = 2,      ///< Additive blending
	VK2D_BLEND_MODE_SUBTRACT = 3, ///< Subtraction blending, new colour is subtracted from current colour
	VK2D_BLEND_MODE_MAX = 4       ///< Total number of blend modes (used for looping)
} VK2DBlendMode;

/// \brief Multisampling detail
///
/// While Vulkan does technically support 64 samples per pixel,
/// there exists no device in the hardware database that supports
/// it. Higher values look smoother but have a bigger impact on
/// performance. Should you request an msaa larger than the device
/// supports the maximum supported msaa is used.
typedef enum {
	VK2D_MSAA_1X = VK_SAMPLE_COUNT_1_BIT,   ///< 1 sample per pixel
	VK2D_MSAA_2X = VK_SAMPLE_COUNT_2_BIT,   ///< 2 samples per pixel
	VK2D_MSAA_4X = VK_SAMPLE_COUNT_4_BIT,   ///< 4 samples per pixel
	VK2D_MSAA_8X = VK_SAMPLE_COUNT_8_BIT,   ///< 8 samples per pixel
	VK2D_MSAA_16X = VK_SAMPLE_COUNT_16_BIT, ///< 16 samples per pixel
	VK2D_MSAA_32X = VK_SAMPLE_COUNT_32_BIT, ///< 32 samples per pixel
} VK2DMSAA;

/// \brief How to present images
///
/// This is system dependent and its possible for a system to not support
/// sm_Immediate or sm_TripleBuffer. While it is technically possible to not
/// support VSync, the Vulkan spec states it must be supported and the hardware
/// database agrees with that so VK2D assumes VSync is always supported. Should
/// you request a mode that is not available, the option will default to sm_Vsync.
typedef enum {
	VK2D_SCREEN_MODE_IMMEDIATE = VK_PRESENT_MODE_IMMEDIATE_KHR,  ///< Quickest mode, just plop to screen but may have screen tearing
	VK2D_SCREEN_MODE_VSYNC = VK_PRESENT_MODE_FIFO_KHR,           ///< Slower but prevents screen tearing
	VK2D_SCREEN_MODE_TRIPLE_BUFFER = VK_PRESENT_MODE_MAILBOX_KHR ///< Optimal for gaming but a bit slower than immediate (machines may not support this)
} VK2DScreenMode;

/// \brief Specifies how textures will be filtered at higher and lower resolutions
typedef enum {
	VK2D_FILTER_TYPE_LINEAR = VK_FILTER_LINEAR,  ///< Linear interpolation, good for most things
	VK2D_FILTER_TYPE_NEAREST = VK_FILTER_NEAREST ///< Nearest neighbor filter, good for pixel art
} VK2DFilterType;

/// \brief A bitwise-able enum representing different shader stages
typedef enum {
	VK2D_SHADER_STAGE_FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT, ///< Fragment (pixel) shader
	VK2D_SHADER_STAGE_VERTEX = VK_SHADER_STAGE_VERTEX_BIT      ///< Vertex shader
} VK2DShaderStage;

/// \brief The state a camera is in
typedef enum {
	VK2D_CAMERA_STATE_NORMAL = 0,   ///< Camera is being rendered/updated as normal
	VK2D_CAMERA_STATE_DISABLED = 1, ///< Camera is not being rendered or updated
	VK2D_CAMERA_STATE_DELETED = 2,  ///< Camera is "deleted" and all data is invalid
	VK2D_CAMERA_STATE_RESET = 3,    ///< Camera is being reset by the renderer
	VK2D_CAMERA_STATE_MAX = 4       ///< Total number of camera states
} VK2DCameraState;

/// \brief Type of camera
typedef enum {
	VK2D_CAMERA_TYPE_DEFAULT = 0,      ///< Default camera used for 2D games in VK2D
	VK2D_CAMERA_TYPE_ORTHOGRAPHIC = 1, ///< Orthographic camera for 3D rendering
	VK2D_CAMERA_TYPE_PERSPECTIVE = 2,  ///< Perspective camera for 3D rendering
	VK2D_CAMERA_TYPE_MAX = 3           ///< Maximum number of camera types
} VK2DCameraType;

/// \brief Types of graphics pipelines
typedef enum {
	VK2D_PIPELINE_TYPE_DEFAULT = 0,    ///< Default 2D pipelines
	VK2D_PIPELINE_TYPE_3D = 1,         ///< 3D pipelines
	VK2D_PIPELINE_TYPE_INSTANCING = 2, ///< Pipelines for instancing
	VK2D_PIPELINE_TYPE_MAX = 3,        ///< Max number of pipeline types
} VK2DPipelineType;

/// \brief Return codes through the renderer
typedef enum {
	VK2D_SUCCESS = 0,         ///< Everything worked
	VK2D_RESET_SWAPCHAIN = 1, ///< The swapchain (renderer) was just reset (likely due to window resize or something similar)
	VK2D_ERROR = -1           ///< Error occurred
} VK2DResult;

/// \brief Types of assets
typedef enum {
	VK2D_ASSET_TYPE_TEXTURE_FILE = 1,   ///< Load a texture from a filename
	VK2D_ASSET_TYPE_TEXTURE_MEMORY = 2, ///< Load a texture from a binary blob
	VK2D_ASSET_TYPE_MODEL_FILE = 3,     ///< Load a model from a filename
	VK2D_ASSET_TYPE_MODEL_MEMORY = 4,   ///< Load a model from a binary blob
	VK2D_ASSET_TYPE_SHADER_FILE = 5,    ///< Load a shader from a filename
	VK2D_ASSET_TYPE_SHADER_MEMORY = 6,  ///< Load a shader from a binary blob
} VK2DAssetType;

/// \brief State an asset may be in
typedef enum {
	VK2D_ASSET_TYPE_ASSET = 0,   ///< Normal asset awaiting load
	VK2D_ASSET_TYPE_PENDING = 1, ///< Asset is pending a queue family transfer
	VK2D_ASSET_TYPE_NONE = 2,    ///< This slot is empty
} VK2DAssetState;

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

	/// Determines the size of a video-memory page in bytes. This can cap the max uniform
	/// buffer size for shaders and does cap the maximum amount of instances you
	/// may draw in one call. By default this is ~250kb and you may leave it as 0
	/// to default it to that value.
	uint64_t vramPageSize;
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
/// Each field specifies the renderer's behaviour in the event you attempt to
/// do something the host machine is not capable of.
struct VK2DRendererLimits {
	VK2DMSAA maxMSAA;             ///< Maximum MSAA the host supports, if you request an msaa higher than this value, this value will be used instead
	bool supportsTripleBuffering; ///< Whether or not the host supports triple buffering, if you request triple buffering and this is false, vsync will be used instead
	bool supportsImmediate;       ///< Whether or not the host supports immediate mode, if you request immediate mode and this is false, vsync will be used instead
	bool supportsWireframe;       ///< Doesn't really mean anything :skull:
	float maxLineWidth;           ///< Maximum line width supported on the platform, if you specify a line width greater than this value, your requested line width will be clamped to this number
	uint64_t maxInstancedDraws;   ///< Maximum amount of instances you may draw at once, if you request to draw more instances than this it will simply be capped to this number
	uint64_t maxShaderBufferSize; ///< Maximum size of a shader's uniform buffer in bytes, if you attempt to create a shader with a uniform buffer size greater than this value NULL will be returned
};

/// \brief Represents the data you need for each element in an instanced draw
struct VK2DDrawInstance {
	vec4 texturePos; ///< x in tex, y in tex, w in tex, and h in tex
	vec4 colour;     ///< Colour mod of this draw
	vec2 pos;        ///< X/Y in game world for this instance
	mat4 model;      ///< Model for this instance, generally shouldn't contain translations
};

/// \brief Information needed to queue an asset loading off-thread
struct VK2DAssetLoad {
	VK2DAssetType type;   ///< Type of asset this is
	VK2DAssetState state; ///< State this asset is in
	union {
		struct {
			const char *filename;         ///< Filename to pull from or filename of the vertex shader
			const char *fragmentFilename; ///< Fragment shader filename
		};
		struct {
			int size;           ///< Data size or data size of the vertex shader
			void *data;         ///< Data to pull from or data for the vertex shader
			int fragmentSize;   ///< Fragment shader's size
			void *fragmentData; ///< Fragment shader's data
		};
	} Load; ///< Information needed to create the asset

	union {
		struct {
			int uniformBufferSize; ///< Uniform buffer size of this shader
		} Shader; ///< Information needed if this is a texture
		struct {
			VK2DTexture *tex; ///< Texture to use for this model (pointer so the model's texture may be in the same list)
		} Model; ///< Information needed if this is a model
	} Data; ///< Asset-specific information

	union {
		VK2DShader *shader;   ///< Pointer to where the output object will be placed
		VK2DModel *model;     ///< Pointer to where the output object will be placed
		VK2DTexture *texture; ///< Pointer to where the output object will be placed
	} Output; ///< How the user will receive the loaded asset
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
VK2D_USER_STRUCT(VK2DDrawInstance)
VK2D_USER_STRUCT(VK2DAssetLoad)

#ifdef __cplusplus
}
#endif