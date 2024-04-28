/// \file Opaque.h
/// \author Paolo Mazzon
/// \brief Defines opaque headers for source files that need them
#pragma once
#include "VK2D/Structs.h"
#include "Constants.h"
#include "VK2D/Camera.h"
#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#define VMA_VULKAN_VERSION 1002000
#include <VulkanMemoryAllocator/src/VmaUsage.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Groups up a couple things related to VkPhysicalDevice
struct VK2DPhysicalDevice_t {
	VkPhysicalDevice dev; ///< Internal vulkan pointer
	struct {
		uint32_t graphicsFamily; ///< Queue family for graphics pipeline
		uint32_t computeFamily;  ///< Queue family for compute pipeline
	} QueueFamily;               ///< Nicely groups up queue families
	VkPhysicalDeviceMemoryProperties mem; ///< Memory properties of this device
	VkPhysicalDeviceFeatures feats;       ///< Features of this device
	VkPhysicalDeviceProperties props;     ///< Device properties
};

/// \brief Logical device that is essentially a wrapper of VkDevice
struct VK2DLogicalDevice_t {
	VkDevice dev;              ///< Logical device
	VkQueue queue;             ///< Queue for command buffers
	VkQueue loadQueue;         ///< Queue for off-thread loading
	VK2DPhysicalDevice pd;     ///< Physical device this came from
	VkCommandPool pool;        ///< Command pools to cycle through
	VkCommandPool loadPool;    ///< Command pool for off-thread loading
	SDL_atomic_t loadListSize; ///< Size of the asset load list
	VK2DAssetLoad *loadList;   ///< Assets that need to be loaded
	SDL_mutex *loadListMutex;  ///< Mutex for asset load list synchronization
	SDL_Thread *workerThread;  ///< Thread that loads assets
	SDL_atomic_t quitThread;   ///< How to tell the thread to quit
	SDL_atomic_t loads;        ///< Number of loads waiting in the list
	SDL_atomic_t doneLoading;  ///< To know when loading is complete
	SDL_mutex *shaderMutex;    ///< Mutex for creating shaders
};

/// \brief An internal representation of a camera (the user deals with VK2DCameraIndex, the renderer uses this struct)
///
/// Due to a camera's nature of being closely tied to the renderer, the renderer
/// will automatically update the relevant parts of a camera whenever needed.
typedef struct VK2DCamera {
	VK2DCameraSpec spec;           ///< Info on how to create the UBO and scissor/viewport
	VK2DUniformBufferObject *ubos; ///< UBO data for each frame
	VkDescriptorSet *uboSets;      ///< List of UBO sets, 1 per swapchain image
	VK2DCameraState state;         ///< State of this camera
} VK2DCamera;

/// \brief Makes managing buffers in Vulkan simpler
struct VK2DBuffer_t {
	VkBuffer buf;          ///< Internal Vulkan buffer
	VmaAllocation mem;     ///< Memory for the buffer
	VK2DLogicalDevice dev; ///< Device the buffer belongs to
	VkDeviceSize size;     ///< Size of this buffer in bytes
	VkDeviceSize offset;   ///< Offset for this buffer in bytes
};

/// \brief To make descriptor buffers simpler internally
typedef struct _VK2DDescriptorBufferInternal {
	VK2DBuffer deviceBuffer; ///< Device-local (on vram) buffer that the shaders will access
	VK2DBuffer stageBuffer;  ///< Host-local (on ram) buffer that data will be copied into
	void *hostData;          ///< For when stageBuffer is mapped
	VkDeviceSize size;       ///< Amount of data currently in this buffer
} _VK2DDescriptorBufferInternal;

/// \brief Automates memory management for uniform buffers and the lot
///
/// The intended usage is as follows:
///
///  1. Create the descriptor buffer
///  2. Call vk2dDescriptorBufferBeginFrame before beginning to draw
///  3.
///  4. Call vk2dDescriptorBufferEndFrame before submitting the queue
///
/// It will put a event barrier into the startframe command buffer where drawing happens,
/// and then it will put a memory copy into the second command buffer at the end of the
/// frame so Vulkan copies the buffer to device memory all at once instead of tiny increments.
struct VK2DDescriptorBuffer_t {
	_VK2DDescriptorBufferInternal *buffers; ///< List of internal buffers so that we can allocate more on the fly
	int bufferCount;          ///< Amount of internal buffers in the descriptor buffer, for when it needs to be resized
	int bufferListSize;       ///< Actual number of elements in the buffer lists
	VK2DLogicalDevice dev;    ///< Device this lives on
};

/// \brief Abstraction for descriptor pools and sets so you can dynamically use them
struct VK2DDescCon_t {
	VkDescriptorPool *pools;      ///< List of pools
	VkDescriptorSetLayout layout; ///< Layout for these sets
	uint32_t buffer;              ///< Whether or not pools support uniform buffers
	uint32_t sampler;             ///< Whether or not pools support texture samplers
	uint32_t storageBuffer;       ///< Whether or not pools support storage buffers
	VK2DLogicalDevice dev;        ///< Device pools are created with

	// pools will always have poolListSize elements, but only elements up to poolsInUse will be
	// valid pools (in an effort to avoid constantly reallocating memory)
	uint32_t poolsInUse;   ///< Number of actively in use pools in pools
	uint32_t poolListSize; ///< Total length of pools array
};

/// \brief A handy abstraction that groups up pipeline state and makes multiple shaders easier
struct VK2DPipeline_t {
	VK2DLogicalDevice dev;      ///< Device this pipeline belongs to
	VkRenderPass renderPass;    ///< Render pass this pipeline uses
	VkPipelineLayout layout;    ///< Internal pipeline layout
	VkRect2D rect;              ///< For setting up command buffers
	VkClearValue clearValue[2]; ///< Clear values for the two attachments: colour and depth
	VkPipeline pipes[VK2D_BLEND_MODE_MAX];   ///< Internal pipelines if `VK2D_GENERATE_BLEND_MODES` is enabled
};

/// \brief Makes shapes easier to deal with
struct VK2DPolygon_t {
	VK2DBuffer vertices;  ///< Internal memory for the vertices
	VK2DVertexType type;  ///< What kind of vertices this stores
	uint32_t vertexCount; ///< Number of vertices
};

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
struct VK2DShader_t {
	uint8_t *spvVert;        ///< Vertex shader in SPIR-V
	uint32_t spvVertSize;    ///< Size of the vertex shader (in bytes)
	uint8_t *spvFrag;        ///< Fragment shader in SPIR-V
	uint32_t spvFragSize;    ///< Size of the fragment shader (in bytes)
	VK2DPipeline pipe;       ///< Pipeline associated with this shader
	uint32_t uniformSize;    ///< Uniform buffer size in bytes
	VK2DLogicalDevice dev;   ///< Device this belongs to
	VK2DDescCon descCons[VK2D_MAX_FRAMES_IN_FLIGHT]; ///< Descriptor sets for the uniform buffers
};

/// \brief Simple wrapper that groups image things together
struct VK2DImage_t {
	VkImage img;           ///< Internal image
	VkImageView view;      ///< Image view bound to the image
	VmaAllocation mem;     ///< Memory the image is stored on
	VK2DLogicalDevice dev; ///< Device this image belongs to
	uint32_t width;        ///< Width in pixels of the image
	uint32_t height;       ///< Height in pixels of the image
	VkDescriptorSet set;   ///< Descriptor set for this image
};

/// \brief Takes the headache out of Vulkan textures
///
/// Only textures created with vk2dTextureCreate may be rendered to. Should you try to
/// set the render target to a texture not created with vk2dTextureCreate, you can expect
/// a segfault.
struct VK2DTexture_t {
	VK2DImage img;          ///< Internal image
	VK2DImage depthBuffer;  ///< For 3D rendering when its a target
	VK2DImage sampledImg;   ///< Image for MSAA
	VkFramebuffer fbo;      ///< Framebuffer of this texture so it can be drawn to
	VK2DBuffer ubo;         ///< UBO that will be used when drawing to this texture
	VkDescriptorSet uboSet; ///< Set for the UBO
	bool imgHandled;        ///< Whether or not to free the image with the texture (if it was loaded with vk2dTextureLoad)
};

/// \brief A 3D model
struct VK2DModel_t {
	VK2DBuffer vertices;       ///< Internal memory for the vertices & indices
	VkDeviceSize indexOffset;  ///< Offset of the indices in the buffer
	VkDeviceSize vertexOffset; ///< Offset of the vertices in the buffer
	VK2DVertexType type;       ///< What kind of vertices this stores
	uint32_t vertexCount;      ///< Number of vertices
	uint32_t indexCount;       ///< Number of indices
	VK2DTexture tex;           ///< Texture for this model
};

/// \brief Information for hardware accelerated shadows
struct VK2DShadowEnvironment_t {
    int vboVertexSize; ///< Number of vertices in the VBO
    VK2DBuffer vbo;    ///< Vertices corresponding to the shadows that will be cast
    vec3 *vertices;    ///< Raw vertices before they get shipped off to the gpu
    int verticesSize;  ///< Size of the vertex list in elements
    int verticesCount; ///< Number of vertices in the vertices list
};

/// \brief Core rendering data, don't modify values unless you know what you're doing
struct VK2DRenderer_t {
	// Devices/core functionality (these have short names because they're constantly referenced)
	VK2DPhysicalDevice pd;       ///< Physical device (gpu)
	VK2DLogicalDevice ld;        ///< Logical device
	VkInstance vk;               ///< Core vulkan instance
	VkDebugReportCallbackEXT dr; ///< Debug information
	VmaAllocator vma;

	// User-end things
	VK2DRendererConfig config;            ///< User config
	VK2DRendererConfig newConfig;         ///< In the event that its updated, we only swap out when we're ready to reset the swapchain
	bool resetSwapchain;                  ///< If true, the swapchain (effectively the whole thing) will reset on the next rendered frame
	VK2DImage msaaImage;                  ///< In case MSAA is enabled
	vec4 colourBlend;                     ///< Used to modify colours (and transparency) of anything drawn. Passed via push constants.
	VkSampler textureSampler;             ///< Needed for textures
	VkSampler modelSampler;               ///< Same as textureSampler but for 3D (normalized coordinates)
	VkViewport viewport;                  ///< Viewport to draw with
	bool enableTextureCameraUBO;          ///< If true, when drawing to a texture the UBO for the internal camera is used instead of the texture's UBO
	VK2DBlendMode blendMode;              ///< Current blend mode to draw with
	VK2DCameraSpec defaultCameraSpec;     ///< Default camera spec (spec for camera 0)
	VK2DCamera cameras[VK2D_MAX_CAMERAS]; ///< All cameras to be drawn to
	VK2DCameraIndex cameraLocked;         ///< If true, only the default camera will be drawn to
	VK2DStartupOptions options;           ///< Root options for the renderer
	VK2DRendererLimits limits;            ///< For user safety

	// KHR Surface
	SDL_Window *window;                           ///< Window this renderer belongs to
	VkSurfaceKHR surface;                         ///< Window surface
	VkSurfaceCapabilitiesKHR surfaceCapabilities; ///< Capabilities of the surface
	VkPresentModeKHR *presentModes;               ///< All available present modes
	uint32_t presentModeCount;                    ///< Number of present modes
	VkSurfaceFormatKHR surfaceFormat;             ///< Window surface format
	uint32_t surfaceWidth;                        ///< Width of the surface
	uint32_t surfaceHeight;                       ///< Height of the surface

	// Swapchain
	VkSwapchainKHR swapchain;              ///< Swapchain (manages images and presenting to screen)
	VkImage *swapchainImages;              ///< Images of the swapchain
	VkImageView *swapchainImageViews;      ///< Image views for the swapchain images
	uint32_t swapchainImageCount;          ///< Number of images in the swapchain
	VkRenderPass renderPass;               ///< The render pass
	VkRenderPass midFrameSwapRenderPass;   ///< Render pass for mid-frame switching back to the swapchain as a target
	VkRenderPass externalTargetRenderPass; ///< Render pass for rendering to textures
	VkFramebuffer *framebuffers;           ///< Framebuffers for the swapchain images
	VK2DImage depthBuffer;                 ///< Depth buffer for 3D rendering
	VkFormat depthBufferFormat;            ///< Depth buffer format
	bool procedStartFrame;                 ///< End frame things are only done if this is true and start frame things are only done if this is false

	// Pipelines
	VK2DPipeline texPipe;       ///< Pipeline for rendering textures
	VK2DPipeline modelPipe;     ///< Pipeline for 3D models
	VK2DPipeline wireframePipe; ///< Pipeline for 3D wireframes
	VK2DPipeline primFillPipe;  ///< Pipeline for rendering filled shapes
	VK2DPipeline primLinePipe;  ///< Pipeline for rendering shape outlines
	VK2DPipeline instancedPipe; ///< Pipeline for instancing textures
	VK2DPipeline shadowsPipe;   ///< Pipeline for hardware-accelerated shadows
	uint32_t shaderListSize;    ///< Size of the list of customShaders
	VK2DShader *customShaders;  ///< Custom shaders the user creates

	// Uniform things
	VkDescriptorSetLayout dslSampler;         ///< Descriptor set layout for texture samplers
	VkDescriptorSetLayout dslBufferVP;        ///< Descriptor set layout for the view-projection buffer
	VkDescriptorSetLayout dslBufferUser;      ///< Descriptor set layout for user data buffers (custom shaders uniforms)
	VkDescriptorSetLayout dslTexture;         ///< Descriptor set layout for the textures
	VK2DDescCon descConSamplers;              ///< Descriptor controller for samplers
	VK2DDescCon descConSamplersOff;           ///< Descriptor controller for samplers off thread
	VK2DDescCon descConVP;                    ///< Descriptor controller for view projection buffers
	VK2DDescCon descConUser;                  ///< Descriptor controller for user buffers
	VkDescriptorPool samplerPool;             ///< Sampler pool for 1 sampler
	VkDescriptorSet samplerSet;               ///< Sampler for all textures
	VkDescriptorSet modelSamplerSet;          ///< Sampler for all 3D models
	VK2DDescriptorBuffer *descriptorBuffers;  ///< Descriptor buffer, one per swapchain image

	// Frame synchronization
	uint32_t currentFrame;                 ///< Current frame being looped through
	uint32_t scImageIndex;                 ///< Swapchain image index to be rendered to this frame
	VkSemaphore *imageAvailableSemaphores; ///< Semaphores to signal when the image is ready
	VkSemaphore *renderFinishedSemaphores; ///< Semaphores to signal when rendering is done
	VkFence *inFlightFences;               ///< Fences for each frame
	VkFence *imagesInFlight;               ///< Individual images in flight
	VkCommandBuffer *commandBuffer;        ///< Command buffers, recreated each frame
	VkCommandBuffer *dbCommandBuffer;      ///< Command buffers for descriptor buffers

	// Render targeting info
	uint32_t targetSubPass;          ///< Current sub pass being rendered to
	VkRenderPass targetRenderPass;   ///< Current render pass being rendered to
	VkFramebuffer targetFrameBuffer; ///< Current framebuffer being rendered to
	VkImage targetImage;             ///< Current image being rendered to
	VkDescriptorSet targetUBOSet;    ///< UBO being used for rendering
	VK2DTexture target;              ///< Just for simplicity sake
	VK2DTexture *targets;            ///< List of all currently loaded textures targets (in case the MSAA is changed and the sample image needs to be reloaded)
	uint32_t targetListSize;         ///< Amount of elements in the list (only non-null elements count)

	// Optimization tools - if the renderer knows the proper set/pipeline/vbo is already bound it doesn't need to rebind it
	uint64_t prevSetHash; ///< Currently bound descriptor set
	VkBuffer prevVBO;     ///< Currently bound vertex buffer
	VkPipeline prevPipe;  ///< Currently bound pipeline

	// Makes drawing things simpler
	VK2DPolygon unitSquare;        ///< Used to draw rectangles
	VK2DPolygon unitSquareOutline; ///< Used to draw rectangle outlines
	VK2DPolygon unitCircle;        ///< Used to draw circles
	VK2DPolygon unitCircleOutline; ///< Used to draw circle outlines
	VK2DPolygon unitLine;          ///< Used to draw lines
	VK2DBuffer unitUBO;            ///< Used to draw to the whole screen
	VkDescriptorSet unitUBOSet;    ///< Descriptor Set for the unit ubo

	// Debugging tools
	double previousTime;     ///< Time that the current frame started
	double amountOfFrames;   ///< Number of frames needed to calculate frameTimeAverage
	double accumulatedTime;  ///< Total time of frames for average in ms
	double frameTimeAverage; ///< Average amount of time frames are taking over a second (in ms)
};

#ifdef __cplusplus
}
#endif