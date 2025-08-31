/// \file Structs.h
/// \author Paolo Mazzon
/// \brief Forward declares struct typedefs
#pragma once
#include <vulkan/vulkan.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Maximum number of cameras that can exist, enabled or disabled, at once - this is here instead of constants to avoid circular dependencies
#define VK2D_MAX_CAMERAS 10

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
	VK2D_PIPELINE_TYPE_DEFAULT = 0,     ///< Default 2D pipelines
	VK2D_PIPELINE_TYPE_3D = 1,          ///< 3D pipelines
	VK2D_PIPELINE_TYPE_INSTANCING = 2,  ///< Pipelines for instancing
	VK2D_PIPELINE_TYPE_SHADOWS = 3,     ///< Pipeline for shadows
	VK2D_PIPELINE_TYPE_USER_SHADER = 4, ///< Pipeline for user shaders
	VK2D_PIPELINE_TYPE_MAX = 5,         ///< Max number of pipeline types
} VK2DPipelineType;

/// \brief Return codes through the renderer
typedef enum {
	VK2D_SUCCESS = 0,         ///< Everything worked
	VK2D_RESET_SWAPCHAIN = 1, ///< The swapchain (renderer) was just reset (likely due to window resize or something similar)
	VK2D_ERROR = -1           ///< Error occurred
} VK2DResult;

/// \brief Status codes for logging/error reporting
typedef enum {
    VK2D_STATUS_NONE                     = 0,     ///< Nothing important to report
    VK2D_STATUS_FILE_NOT_FOUND           = 1<<0,  ///< File was not found for something like an image load, not fatal
    VK2D_STATUS_BAD_FORMAT               = 1<<1,  ///< Bad file format
    VK2D_STATUS_TOO_MANY_CAMERAS         = 1<<2,  ///< No camera slots left to use, not fatal
    VK2D_STATUS_DEVICE_LOST              = 1<<3,  ///< General Vulkan catch-all for when something goes wrong
    VK2D_STATUS_VULKAN_ERROR             = 1<<4,  ///< Some sort of specific vulkan error
    VK2D_STATUS_OUT_OF_RAM               = 1<<5,  ///< Out of host memory
    VK2D_STATUS_OUT_OF_VRAM              = 1<<6,  ///< Out of gpu memory
    VK2D_STATUS_RENDERER_NOT_INITIALIZED = 1<<7,  ///< Renderer has not been initialized
    VK2D_STATUS_SDL_ERROR                = 1<<8,  ///< General SDL-catch all, not fatal
    VK2D_STATUS_BEYOND_LIMIT             = 1<<9,  ///< User requested a setting that was beyond host limits, not fatal
    VK2D_STATUS_BAD_ASSET                = 1<<10, ///< User tried to pass a NULL asset to a VK2D method, not fatal
} VK2DStatus;

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

typedef enum {
	/// \brief Debug message
	VK2D_LOG_SEVERITY_DEBUG = 0,
	/// \brief Standard log message
	VK2D_LOG_SEVERITY_INFO = 1,
	/// \brief Warning
	VK2D_LOG_SEVERITY_WARN = 2,
	/// \brief Error, recoverable
	VK2D_LOG_SEVERITY_ERROR = 3,
	/// \brief Fatal error, system is in an invalid, unrecoverable state
	/// \note The supplied logger is expected to abort here, abort() will
	///       be called if the supplied logger does not abort.
	VK2D_LOG_SEVERITY_FATAL = 4,
	/// \brief Unknown severity level.
	/// \note This will get printed on all error outputs, regardless of
	///       severity.
	VK2D_LOG_SEVERITY_UNKNOWN = 5,
} VK2DLogSeverity;

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
VK2D_OPAQUE_POINTER(VK2DShadowEnvironment)
VK2D_OPAQUE_POINTER(VK2DGui)

/// \brief 2D vector of floats
typedef float vec2[2];

/// \brief 3D vector of floats
typedef float vec3[3];

/// \brief 4D vector of floats
typedef float vec4[4];

/// \brief 4x4 matrix of floats
typedef float mat4[16];

/// \brief Type used for referencing shadow objects
typedef int32_t VK2DShadowObject;

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
	mat4 viewproj[VK2D_MAX_CAMERAS]; ///< View and projection matrix multiplied together
};

/// \brief Push buffer for user shaders
struct VK2DShaderPushBuffer {
    int32_t cameraIndex;   ///< Index of the camera in use
    uint32_t textureIndex; ///< Index of the texture that was passed to the shader
    vec2 _padding;         ///< For vec4 alignment
    vec4 texturePos;       ///< UV coordinates (unnormalized)
    vec4 colour;           ///< Colour mod of the renderer when called
    mat4 model;            ///< Model matrix
};

/// \brief Buffer passed per-model via push constants
struct VK2DPushBuffer {
	mat4 model;           ///< Model matrix
	vec4 colourMod;       ///< Current colour modifier
	int32_t cameraIndex; ///< Index of the camera
};

/// \brief Push buffer used for 3D models
struct VK2D3DPushBuffer {
	mat4 model;            ///< Model matrix
	vec4 colourMod;        ///< Color modifier
	uint32_t textureIndex; ///< Index of the texture
	uint32_t cameraIndex;  ///< Index of the camera
};

/// \brief Push buffer used for hardware-accelerated shadows
struct VK2DShadowsPushBuffer {
    mat4 model;           ///< Model matrix for this shadow object
    vec2 lightSource;     ///< Light source position
    vec2 _alignment;      ///< Simply for memory alignment
    vec4 colour;          ///< Colour of this shadow render
    uint32_t cameraIndex; ///< Index of the camera
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
	uint32_t maxTextures;   ///< Max number of textures active at once
	bool enableNuklear;     ///< Set to true for VK2D to initialize and render Nuklear

	/// Determines the size of a video-memory page in bytes. This can cap the max uniform
	/// buffer size for shaders, max instances in one instanced call, and max vertices in
	/// a single geometry render. You may leave this as 0, in which case the renderer will
	/// make it 256kb.
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
	VK2DMSAA maxMSAA;                ///< Maximum MSAA the host supports, if you request an msaa higher than this value, this value will be used instead
	bool supportsTripleBuffering;    ///< Whether or not the host supports triple buffering, if you request triple buffering and this is false, vsync will be used instead
	bool supportsImmediate;          ///< Whether or not the host supports immediate mode, if you request immediate mode and this is false, vsync will be used instead
	bool supportsWireframe;          ///< Doesn't really mean anything 💀
	float maxLineWidth;              ///< Maximum line width supported on the platform, if you specify a line width greater than this value, your requested line width will be clamped to this number
	uint64_t maxInstancedDraws;      ///< Maximum amount of instances you may draw at once, if you request to draw more instances than this it will simply be capped to this number
	uint64_t maxShaderBufferSize;    ///< Maximum size of a shader's uniform buffer in bytes, if you attempt to create a shader with a uniform buffer size greater than this value NULL will be returned
	uint64_t maxGeometryVertices;    ///< Maximum vertices that can be used in one vk2dRendererDrawGeometryCall, if you use more vertices than this nothing will happen.
	bool supportsMultiThreadLoading; ///< Whether or not the host supports loading assets in another thread, if attempt to load assets in another thread and this is false, assets will be loaded on the main thread instead
	bool supportsVRAMUsage;          ///< Whether or not the host supports accurate VRAM usage, if this is false VMA will provide a less accurate estimate
};

/// \brief Represents the data you need for each element in an instanced draw
struct VK2DDrawInstance {
	vec4 texturePos;       ///< x in tex, y in tex, w in tex, and h in tex
	vec4 colour;           ///< Colour mod of this draw
	uint32_t textureIndex; ///< Which texture this instance is using
	vec3 padding;          ///< Padding
	mat4 model;            ///< Model for this instance, generally shouldn't contain translations
};

/// \brief Represents a user's draw command which will later be processed into an instance
struct VK2DDrawCommand {
    vec4 texturePos;       ///< x in tex, y in tex, w in tex, and h in tex
    vec4 colour;           ///< Colour mod of this draw
    vec2 pos;              ///< X/Y in game world for this instance
    vec2 origin;           ///< X/Y Origin of this draw
    vec2 scale;            ///< X/Y Scale of this draw
    float rotation;        ///< Rotation of the draw centered around the origin
    uint32_t textureIndex; ///< Texture index for this draw (use vk2dTextureGetID)
};

/// \brief A push buffer for an instanced draw
struct VK2DInstancedPushBuffer {
    uint32_t cameraIndex; ///< Index of the camera for this draw
};

/// \brief Push buffer for the sprite batch compute shader
struct VK2DComputePushBuffer {
    uint32_t drawCount; ///< Number of draws being processed in this compute pass
};

/// \brief Info for the shadow environment to keep track of
struct VK2DShadowObjectInfo {
    bool enabled;       ///< Whether or not this object is enabled
    int startingVertex; ///< Vertex in the master VBO to start from for this object
    int vertexCount;    ///< Number of vertices for this model
    mat4 model;         ///< Model for this shadow object
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


/// \brief Callback function for logging
/// \param context User supplied context
/// \param severity Severity of the log message. If value supplied is not within
///                 the range of the enum, it will be coerced to
///                 VK2D_LOG_SEVERITY_UNKNOWN
/// \param message Message to log
typedef void (*VK2DLoggerLogFn)(void *context, VK2DLogSeverity severity, const char *message);

/// \brief Callback function for destroying the logger
/// \param context User supplied context
typedef void (*VK2DLoggerDestroyFn)(void *context);

/// \brief Retrieves the log severity from the user context.
/// \param context User supplied context.
typedef VK2DLogSeverity (*VK2DLoggerSeverityFn)(void *context);

/// \brief Contains logging callbacks and context
/// \note
struct VK2DLogger {
	/// \brief Callback to log the message.
	/// \note MUST NOT BE NULL! This will kill the program.
	VK2DLoggerLogFn log;
	/// \brief Callback called on destruction of logger.
	/// \note May be NULL, in which case no destructor is called.
	VK2DLoggerDestroyFn destroy;
	/// \brief Callback to get minimum severity of logger.
	/// \note May be NULL, in which case all messages will be logged,
	///       regardless of severity.
	VK2DLoggerSeverityFn severityFn;
	/// \brief User supplied context, passed to log() when called.
	/// \note May be NULL, this is a convenience pointer for the user.
	void *context;
};

VK2D_USER_STRUCT(VK2DVertexColour)
VK2D_USER_STRUCT(VK2DVertex3D)
VK2D_USER_STRUCT(VK2DUniformBufferObject)
VK2D_USER_STRUCT(VK2DPushBuffer)
VK2D_USER_STRUCT(VK2D3DPushBuffer)
VK2D_USER_STRUCT(VK2DShadowsPushBuffer)
VK2D_USER_STRUCT(VK2DShaderPushBuffer)
VK2D_USER_STRUCT(VK2DConfiguration)
VK2D_USER_STRUCT(VK2DStartupOptions)
VK2D_USER_STRUCT(VK2DRendererConfig)
VK2D_USER_STRUCT(VK2DCameraSpec)
VK2D_USER_STRUCT(VK2DRendererLimits)
VK2D_USER_STRUCT(VK2DDrawInstance)
VK2D_USER_STRUCT(VK2DDrawCommand)
VK2D_USER_STRUCT(VK2DAssetLoad)
VK2D_USER_STRUCT(VK2DShadowObjectInfo)
VK2D_USER_STRUCT(VK2DInstancedPushBuffer)
VK2D_USER_STRUCT(VK2DComputePushBuffer)
VK2D_USER_STRUCT(VK2DLogger)

#ifdef __cplusplus
}
#endif