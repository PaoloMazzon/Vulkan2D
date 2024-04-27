/// \file Renderer.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>
#include <time.h>

#include "VK2D/RendererMeta.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/Constants.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Texture.h"
#include "VK2D/Shader.h"
#include "VK2D/Image.h"
#include "VK2D/Model.h"
#include "VK2D/DescriptorBuffer.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Opaque.h"

/******************************* Forward declarations *******************************/

bool _vk2dFileExists(const char *filename);
unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);

/******************************* Globals *******************************/

// For everything
VK2DRenderer gRenderer = NULL;
SDL_atomic_t gRNG;

static const char* DEBUG_EXTENSIONS[] = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};
static const char* DEBUG_LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
};
static const int DEBUG_LAYER_COUNT = 1;
static const int DEBUG_EXTENSION_COUNT = 1;

static const char* BASE_EXTENSIONS[] = {0};
static const char* BASE_LAYERS[] = {0};
static const int BASE_LAYER_COUNT = 0;
static const int BASE_EXTENSION_COUNT = 0;

static VK2DStartupOptions DEFAULT_STARTUP_OPTIONS = {
		false,
		true,
		true,
		"vk2derror.txt",
		false,
		256 * 1000
};

/******************************* User-visible functions *******************************/

VK2DResult vk2dRendererInit(SDL_Window *window, VK2DRendererConfig config, VK2DStartupOptions *options) {
	gRenderer = calloc(1, sizeof(struct VK2DRenderer_t));
	VK2DResult errorCode = VK2D_SUCCESS;
	uint32_t totalExtensionCount, i, sdlExtensions;
	const char** totalExtensions;

	// Validation initialization needs to happen right away
	vk2dValidationBegin();

	// Find the startup options
	VK2DStartupOptions userOptions;
	if (options == NULL) {
		userOptions = DEFAULT_STARTUP_OPTIONS;
	} else {
		userOptions = *options;
		if (userOptions.vramPageSize == 0)
			userOptions.vramPageSize = 256 * 1024;
	}

	// Print all available layers
	VkLayerProperties *systemLayers;
	uint32_t systemLayerCount;
	vkEnumerateInstanceLayerProperties(&systemLayerCount, VK_NULL_HANDLE);
	systemLayers = malloc(sizeof(VkLayerProperties) * systemLayerCount);
	vkEnumerateInstanceLayerProperties(&systemLayerCount, systemLayers);
	vk2dLogMessage("Available layers: ");
	for (i = 0; i < systemLayerCount; i++)
		vk2dLogMessage("  - %s", systemLayers[i].layerName);
	vk2dLogMessage("");
	free(systemLayers);

	// Find number of total number of extensions
	SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensions, VK_NULL_HANDLE);
	if (userOptions.enableDebug) {
		totalExtensionCount = sdlExtensions + DEBUG_EXTENSION_COUNT;
	} else {
		totalExtensionCount = sdlExtensions + BASE_EXTENSION_COUNT;
	}
	totalExtensions = malloc(totalExtensionCount * sizeof(char*));

	if (vk2dPointerCheck(gRenderer)) {
		// Copy user options
		gRenderer->options = userOptions;

		// Load extensions
		SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensions, totalExtensions);
		if (userOptions.enableDebug) {
			for (i = sdlExtensions; i < totalExtensionCount; i++) totalExtensions[i] = DEBUG_EXTENSIONS[i - sdlExtensions];
		} else {
			for (i = sdlExtensions; i < totalExtensionCount; i++) totalExtensions[i] = BASE_EXTENSIONS[i - sdlExtensions];
		}

		// Log all used extensions
		vk2dLogMessage("Vulkan Enabled Extensions: ");
		for (i = 0; i < totalExtensionCount; i++)
			vk2dLogMessage(" - %s", totalExtensions[i]);
		vk2dLogMessage(""); // Newline

		// Create instance, physical, and logical device
		VkInstanceCreateInfo instanceCreateInfo;
		if (userOptions.enableDebug) {
			instanceCreateInfo = vk2dInitInstanceCreateInfo((void *) &VK2D_DEFAULT_CONFIG, DEBUG_LAYERS,
																				 DEBUG_LAYER_COUNT, totalExtensions,
																				 totalExtensionCount);
		} else {
			instanceCreateInfo = vk2dInitInstanceCreateInfo((void *) &VK2D_DEFAULT_CONFIG, BASE_LAYERS,
																				 BASE_LAYER_COUNT, totalExtensions,
																				 totalExtensionCount);
		}
		vk2dErrorCheck(vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &gRenderer->vk))
		gRenderer->pd = vk2dPhysicalDeviceFind(gRenderer->vk, VK2D_DEVICE_BEST_FIT);
		gRenderer->ld = vk2dLogicalDeviceCreate(gRenderer->pd, false, true, userOptions.enableDebug, &gRenderer->limits);
		gRenderer->window = window;

		// Assign user settings, except for screen mode which will be handled later
		gRenderer->config = config;
		gRenderer->config.msaa = gRenderer->limits.maxMSAA >= config.msaa ? config.msaa : gRenderer->limits.maxMSAA;
		gRenderer->newConfig = gRenderer->config;

		// Calculate the limit for shader uniform buffers
		gRenderer->limits.maxShaderBufferSize = gRenderer->pd->props.limits.maxUniformBufferRange < userOptions.vramPageSize ? gRenderer->pd->props.limits.maxUniformBufferRange : userOptions.vramPageSize;
		gRenderer->limits.maxGeometryVertices = (userOptions.vramPageSize / sizeof(VK2DVertexColour)) - 1;

		// Create the VMA
		VmaAllocatorCreateInfo allocatorCreateInfo = {0};
		allocatorCreateInfo.device = gRenderer->ld->dev;
		allocatorCreateInfo.physicalDevice = gRenderer->pd->dev;
		allocatorCreateInfo.instance = gRenderer->vk;
		allocatorCreateInfo.vulkanApiVersion = VK_MAKE_VERSION(1, 1, 0);
		vmaCreateAllocator(&allocatorCreateInfo, &gRenderer->vma);

		// Initialize subsystems
		_vk2dRendererCreateDebug();
		_vk2dRendererCreateWindowSurface();
		_vk2dRendererCreateSwapchain();
		_vk2dRendererCreateColourResources();
		_vk2dRendererCreateDepthBuffer();
		_vk2dRendererCreateRenderPass();
		_vk2dRendererCreateDescriptorSetLayouts();
		_vk2dRendererCreatePipelines();
		_vk2dRendererCreateFrameBuffer();
		_vk2dRendererCreateDescriptorPool(false);
		_vk2dRendererCreateDescriptorBuffers();
		_vk2dRendererCreateUniformBuffers(true);
		_vk2dRendererCreateSampler();
		_vk2dRendererCreateUnits();
		_vk2dRendererCreateSynchronization();

		vk2dRendererSetColourMod((void*)VK2D_DEFAULT_COLOUR_MOD);
		gRenderer->viewport.x = 0;
		gRenderer->viewport.y = 0;
		gRenderer->viewport.width = gRenderer->surfaceWidth;
		gRenderer->viewport.height = gRenderer->surfaceHeight;
		gRenderer->viewport.minDepth = 0;
		gRenderer->viewport.maxDepth = 1;

		// Initialize the random seed
		SDL_AtomicSet(&gRNG, time(0));
	} else {
		errorCode = VK2D_ERROR;
	}

	return errorCode;
}

void vk2dRendererQuit() {
	if (gRenderer != NULL) {
		vkQueueWaitIdle(gRenderer->ld->queue);

		// Destroy subsystems
		_vk2dRendererDestroySynchronization();
		_vk2dRendererDestroyTargetsList();
		_vk2dRendererDestroyUnits();
		_vk2dRendererDestroySampler();
		_vk2dRendererDestroyDescriptorPool(false);
		_vk2dRendererDestroyDescriptorBuffers();
		_vk2dRendererDestroyUniformBuffers();
		_vk2dRendererDestroyFrameBuffer();
		_vk2dRendererDestroyPipelines(false);
		_vk2dRendererDestroyDescriptorSetLayout();
		_vk2dRendererDestroyRenderPass();
		_vk2dRendererDestroyDepthBuffer();
		_vk2dRendererDestroyColourResources();
		_vk2dRendererDestroySwapchain();
		_vk2dRendererDestroyWindowSurface();
		_vk2dRendererDestroyDebug();

		vmaDestroyAllocator(gRenderer->vma);

		// Destroy core bits
		vk2dLogicalDeviceFree(gRenderer->ld);
		vk2dPhysicalDeviceFree(gRenderer->pd);

		vk2dLogMessage("VK2D has been uninitialized.");
		vk2dValidationEnd();
		free(gRenderer);
		gRenderer = NULL;
	}
}

void vk2dRendererWait() {
	if (gRenderer != NULL)
		vkQueueWaitIdle(gRenderer->ld->queue);
	else
		vk2dLogMessage("Renderer is not initialized");
}

VK2DRenderer vk2dRendererGetPointer() {
	return gRenderer;
}

void vk2dRendererResetSwapchain() {
	if (gRenderer != NULL)
		gRenderer->resetSwapchain = true;
	else
		vk2dLogMessage("Renderer is not initialized");
}

VK2DRendererConfig vk2dRendererGetConfig() {
	if (gRenderer != NULL)
		return gRenderer->config;
	else
		vk2dLogMessage("Renderer is not initialized");
	VK2DRendererConfig c = {0};
	return c;
}

void vk2dRendererSetConfig(VK2DRendererConfig config) {
	if (gRenderer != NULL) {
		gRenderer->newConfig = config;
		gRenderer->newConfig.msaa = gRenderer->limits.maxMSAA >= config.msaa ? config.msaa : gRenderer->limits.maxMSAA;
		vk2dRendererResetSwapchain();
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererStartFrame(const vec4 clearColour) {
	if (gRenderer != NULL) {
		if (!gRenderer->procedStartFrame) {
			gRenderer->procedStartFrame = true;

			/*********** Get image and synchronization ***********/

			gRenderer->previousTime = SDL_GetPerformanceCounter();

			// Wait for previous rendering to be finished
			vkWaitForFences(gRenderer->ld->dev, 1, &gRenderer->inFlightFences[gRenderer->currentFrame], VK_TRUE,
							UINT64_MAX);

			// Acquire image
			vkAcquireNextImageKHR(gRenderer->ld->dev, gRenderer->swapchain, UINT64_MAX,
								  gRenderer->imageAvailableSemaphores[gRenderer->currentFrame], VK_NULL_HANDLE,
								  &gRenderer->scImageIndex);

			if (gRenderer->imagesInFlight[gRenderer->scImageIndex] != VK_NULL_HANDLE) {
				vkWaitForFences(gRenderer->ld->dev, 1, &gRenderer->imagesInFlight[gRenderer->scImageIndex], VK_TRUE,
								UINT64_MAX);
			}
			gRenderer->imagesInFlight[gRenderer->scImageIndex] = gRenderer->inFlightFences[gRenderer->currentFrame];

			/*********** Start-of-frame tasks ***********/

			// Reset currently bound items
			_vk2dRendererResetBoundPointers();

			// Reset current render targets
			gRenderer->targetFrameBuffer = gRenderer->framebuffers[gRenderer->scImageIndex];
			gRenderer->targetRenderPass = gRenderer->renderPass;
			gRenderer->targetSubPass = 0;
			gRenderer->targetImage = gRenderer->swapchainImages[gRenderer->scImageIndex];
			gRenderer->targetUBOSet = gRenderer->cameras[0].uboSets[gRenderer->scImageIndex]; // TODO: Should prob be reworked
			gRenderer->target = VK2D_TARGET_SCREEN;

			// Start the render pass
			VkCommandBufferBeginInfo beginInfo = vk2dInitCommandBufferBeginInfo(
					VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
					VK_NULL_HANDLE);
			vkResetCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex], 0);
			vkResetCommandBuffer(gRenderer->dbCommandBuffer[gRenderer->scImageIndex], 0);
			vk2dErrorCheck(vkBeginCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex], &beginInfo));
			vk2dErrorCheck(vkBeginCommandBuffer(gRenderer->dbCommandBuffer[gRenderer->scImageIndex], &beginInfo));

			// Begin descriptor buffer
			vk2dDescriptorBufferBeginFrame(gRenderer->descriptorBuffers[gRenderer->currentFrame], gRenderer->commandBuffer[gRenderer->scImageIndex]);

			// Flush the current ubo into its buffer for the frame
			for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
				if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL) {
					_vk2dCameraUpdateUBO(&gRenderer->cameras[i].ubos[gRenderer->scImageIndex],
										 &gRenderer->cameras[i].spec);
					_vk2dRendererFlushUBOBuffer(gRenderer->scImageIndex, gRenderer->currentFrame, i);
				}
			}

			// Reset shader desc cons
			for (int i = 0; i < gRenderer->shaderListSize; i++)
				if (gRenderer->customShaders[i] != NULL)
					vk2dDescConReset(gRenderer->customShaders[i]->descCons[gRenderer->scImageIndex]);

			// Setup render pass
			VkRect2D rect = {0};
			rect.extent.width = gRenderer->surfaceWidth;
			rect.extent.height = gRenderer->surfaceHeight;
			const uint32_t clearCount = 2;
			VkClearValue clearValues[2] = {0};
			clearValues[0].color.float32[0] = clearColour[0];
			clearValues[0].color.float32[1] = clearColour[1];
			clearValues[0].color.float32[2] = clearColour[2];
			clearValues[0].color.float32[3] = clearColour[3];
			clearValues[1].depthStencil.depth = 1;
			VkRenderPassBeginInfo renderPassBeginInfo = vk2dInitRenderPassBeginInfo(
					gRenderer->renderPass,
					gRenderer->framebuffers[gRenderer->scImageIndex],
					rect,
					clearValues,
					clearCount);

			vkCmdBeginRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex], &renderPassBeginInfo,
								 VK_SUBPASS_CONTENTS_INLINE);
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

VK2DResult vk2dRendererEndFrame() {
	VK2DResult res = VK2D_SUCCESS;
	if (gRenderer != NULL) {
		if (gRenderer->procedStartFrame) {
			gRenderer->procedStartFrame = false;

			// Make sure we're not in the wrong pipeline
			if (gRenderer->target != VK2D_TARGET_SCREEN) {
				vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
			}

			// Finish the primary command buffer, its time to PRESENT things
			vkCmdEndRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex]);
			vk2dDescriptorBufferEndFrame(gRenderer->descriptorBuffers[gRenderer->currentFrame], gRenderer->dbCommandBuffer[gRenderer->scImageIndex]);
			vk2dErrorCheck(vkEndCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex]));
			vk2dErrorCheck(vkEndCommandBuffer(gRenderer->dbCommandBuffer[gRenderer->scImageIndex]));

			// Wait for image before doing things
			VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
			VkCommandBuffer bufs[] = {gRenderer->commandBuffer[gRenderer->scImageIndex], gRenderer->dbCommandBuffer[gRenderer->scImageIndex]};
			VkSubmitInfo submitInfo = vk2dInitSubmitInfo(
					bufs,
					2,
					&gRenderer->renderFinishedSemaphores[gRenderer->currentFrame],
					1,
					&gRenderer->imageAvailableSemaphores[gRenderer->currentFrame],
					1,
					waitStage);

			// Submit
			vkResetFences(gRenderer->ld->dev, 1, &gRenderer->inFlightFences[gRenderer->currentFrame]);
			vk2dErrorCheck(vkQueueSubmit(gRenderer->ld->queue, 1, &submitInfo,
										 gRenderer->inFlightFences[gRenderer->currentFrame]));

			// Final present info bit
			VkResult result;
			VkPresentInfoKHR presentInfo = vk2dInitPresentInfoKHR(&gRenderer->swapchain, 1, &gRenderer->scImageIndex,
																  &result,
																  &gRenderer->renderFinishedSemaphores[gRenderer->currentFrame],
																  1);
			VkResult queueRes = vkQueuePresentKHR(gRenderer->ld->queue, &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || gRenderer->resetSwapchain ||
				queueRes == VK_ERROR_OUT_OF_DATE_KHR) {
				_vk2dRendererResetSwapchain();
				gRenderer->resetSwapchain = false;
				res = VK2D_RESET_SWAPCHAIN;
			} else {
				vk2dErrorCheck(result);
				vk2dErrorCheck(queueRes);
			}

			gRenderer->currentFrame = (gRenderer->currentFrame + 1) % VK2D_MAX_FRAMES_IN_FLIGHT;

			// Calculate time
			gRenderer->accumulatedTime += (((double) SDL_GetPerformanceCounter() - gRenderer->previousTime) /
										   (double) SDL_GetPerformanceFrequency()) * 1000;
			gRenderer->amountOfFrames++;
			if (gRenderer->accumulatedTime >= 1000) {
				gRenderer->frameTimeAverage = gRenderer->accumulatedTime / gRenderer->amountOfFrames;
				gRenderer->accumulatedTime = 0;
				gRenderer->amountOfFrames = 0;
			}
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}

	return res;
}

VK2DLogicalDevice vk2dRendererGetDevice() {
	if (gRenderer != NULL)
		return gRenderer->ld;
	else
		vk2dLogMessage("Renderer is not initialized");
	return NULL;
}

void vk2dRendererSetTarget(VK2DTexture target) {
	if (gRenderer != NULL) {
		if (target != gRenderer->target) {
			// In case the user attempts to switch targets from one texture to another
			if (target != VK2D_TARGET_SCREEN && gRenderer->target != VK2D_TARGET_SCREEN) {
				vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
			}

			// Dont let the user bind textures that are not targets
			if (target != VK2D_TARGET_SCREEN && !vk2dTextureIsTarget(target)) {
				vk2dLogMessage("Texture cannot be used as a target.");
				return;
			}

			gRenderer->target = target;

			// Figure out which render pass to use
			VkRenderPass pass = target == VK2D_TARGET_SCREEN ? gRenderer->midFrameSwapRenderPass
															 : gRenderer->externalTargetRenderPass;
			VkFramebuffer framebuffer =
					target == VK2D_TARGET_SCREEN ? gRenderer->framebuffers[gRenderer->scImageIndex] : target->fbo;
			VkImage image = target == VK2D_TARGET_SCREEN ? gRenderer->swapchainImages[gRenderer->scImageIndex]
														 : target->img->img;
			VkDescriptorSet buffer =
					target == VK2D_TARGET_SCREEN ? gRenderer->cameras[0].uboSets[gRenderer->scImageIndex]
												 : target->uboSet;

			vkCmdEndRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex]);

			// Now we either have to transition the image layout depending on whats going in and whats poppin out
			if (target == VK2D_TARGET_SCREEN)
				_vk2dTransitionImageLayout(gRenderer->targetImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
										   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			else
				_vk2dTransitionImageLayout(target->img->img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
										   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// Assign new render targets
			gRenderer->targetRenderPass = pass;
			gRenderer->targetFrameBuffer = framebuffer;
			gRenderer->targetImage = image;
			gRenderer->targetUBOSet = buffer;

			// Setup new render pass
			VkRect2D rect = {0};
			rect.extent.width = target == VK2D_TARGET_SCREEN ? gRenderer->surfaceWidth : target->img->width;
			rect.extent.height = target == VK2D_TARGET_SCREEN ? gRenderer->surfaceHeight : target->img->height;
			VkClearValue clear[2] = {0};
			clear[1].depthStencil.depth = 1;
			VkRenderPassBeginInfo renderPassBeginInfo = vk2dInitRenderPassBeginInfo(
					pass,
					framebuffer,
					rect,
					clear,
					2);

			vkCmdBeginRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex], &renderPassBeginInfo,
								 VK_SUBPASS_CONTENTS_INLINE);

			_vk2dRendererResetBoundPointers();
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererSetColourMod(const vec4 mod) {
	if (gRenderer != NULL) {
		gRenderer->colourBlend[0] = mod[0];
		gRenderer->colourBlend[1] = mod[1];
		gRenderer->colourBlend[2] = mod[2];
		gRenderer->colourBlend[3] = mod[3];
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererGetColourMod(vec4 dst) {
	if (gRenderer != NULL) {
		dst[0] = gRenderer->colourBlend[0];
		dst[1] = gRenderer->colourBlend[1];
		dst[2] = gRenderer->colourBlend[2];
		dst[3] = gRenderer->colourBlend[3];
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererSetBlendMode(VK2DBlendMode blendMode) {
	if (gRenderer != NULL)
		gRenderer->blendMode = blendMode;
	else
		vk2dLogMessage("Renderer is not initialized");
}

VK2DBlendMode vk2dRendererGetBlendMode() {
	if (gRenderer != NULL)
		return gRenderer->blendMode;
	else
		vk2dLogMessage("Renderer is not initialized");
	return VK2D_BLEND_MODE_NONE;
}

void vk2dRendererSetCamera(VK2DCameraSpec camera) {
	if (gRenderer != NULL) {
		gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec = camera;
		gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.wOnScreen = gRenderer->surfaceWidth;
		gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.hOnScreen = gRenderer->surfaceHeight;
		gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.xOnScreen = 0;
		gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.yOnScreen = 0;
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

VK2DCameraSpec vk2dRendererGetCamera() {
	if (gRenderer != NULL)
		return gRenderer->defaultCameraSpec;
	else
		vk2dLogMessage("Renderer is not initialized");
	VK2DCameraSpec c = {0};
	return c;
}

void vk2dRendererSetTextureCamera(bool useCameraOnTextures) {
	if (gRenderer != NULL)
		gRenderer->enableTextureCameraUBO = useCameraOnTextures;
	else
		vk2dLogMessage("Renderer is not initialized");
}

void vk2dRendererLockCameras(VK2DCameraIndex cam) {
	if (gRenderer != NULL)
		gRenderer->cameraLocked = cam;
	else
		vk2dLogMessage("Renderer is not initialized");
}

void vk2dRendererUnlockCameras() {
	if (gRenderer != NULL)
		gRenderer->cameraLocked = VK2D_INVALID_CAMERA;
	else
		vk2dLogMessage("Renderer is not initialized");
}

double vk2dRendererGetAverageFrameTime() {
	if (gRenderer != NULL)
		return gRenderer->frameTimeAverage;
	else
		vk2dLogMessage("Renderer is not initialized");
	return 0;
}

void vk2dRendererClear() {
	if (gRenderer != NULL) {
		VkDescriptorSet set = gRenderer->unitUBOSet;
		_vk2dRendererDrawRaw(&set, 1, gRenderer->unitSquare, gRenderer->primFillPipe, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0,
							 0, VK2D_INVALID_CAMERA);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererEmpty() {
	if (gRenderer != NULL) {
		// Save old renderer state to revert back
		VK2DBlendMode bm = vk2dRendererGetBlendMode();
		vec4 c;
		vk2dRendererGetColourMod(c);

		// Set the render mode to be blend mode none, and the colour to be a flat 0
		const vec4 clearColour = {0, 0, 0, 0};
		vk2dRendererSetColourMod(clearColour);
		vk2dRendererSetBlendMode(VK2D_BLEND_MODE_NONE);
		vk2dRendererClear();

		vk2dRendererSetColourMod(c);
		vk2dRendererSetBlendMode(bm);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

VK2DRendererLimits vk2dRendererGetLimits() {
	if (gRenderer != NULL) {
		return gRenderer->limits;
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
	VK2DRendererLimits l = {0};
	return l;
}

void vk2dRendererDrawRectangle(float x, float y, float w, float h, float r, float ox, float oy) {
	if (gRenderer != NULL) {
		vk2dRendererDrawPolygon(gRenderer->unitSquare, x, y, true, 1, w, h, r, ox / w, oy / h);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawRectangleOutline(float x, float y, float w, float h, float r, float ox, float oy, float lineWidth) {
	if (gRenderer != NULL) {
		vk2dRendererDrawPolygon(gRenderer->unitSquareOutline, x, y, false, lineWidth, w, h, r, ox / w, oy / h);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawCircle(float x, float y, float r) {
	if (gRenderer != NULL) {
		vk2dRendererDrawPolygon(gRenderer->unitCircle, x, y, true, 1, r * 2, r * 2, 0, 0, 0);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawCircleOutline(float x, float y, float r, float lineWidth) {
	if (gRenderer != NULL) {
		vk2dRendererDrawPolygon(gRenderer->unitCircleOutline, x, y, false, lineWidth, r * 2, r * 2, 0, 0, 0);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawLine(float x1, float y1, float x2, float y2) {
	if (gRenderer != NULL) {
		float x = sqrtf(powf(y2 - y1, 2) + powf(x2 - x1, 2));
		float r = atan2f(y2 - y1, x2 - x1);
		vk2dRendererDrawPolygon(gRenderer->unitLine, x1, y1, false, 1, x, 1, r, 0, 0);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawShader(VK2DShader shader, void *data, VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight) {
	if (gRenderer != NULL) {
		if (shader != NULL) {
			VkDescriptorSet sets[4];
			sets[1] = gRenderer->samplerSet;
			sets[2] = tex->img->set;

			// Create the data uniform
			uint32_t setCount = 3;
			if (shader->uniformSize != 0) {
				sets[3] = vk2dDescConGetSet(shader->descCons[gRenderer->scImageIndex]);
				VkBuffer buffer;
				VkDeviceSize offset;
				vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], data, shader->uniformSize, &buffer, &offset);
				VkDescriptorBufferInfo bufferInfo = {buffer,offset,shader->uniformSize};
				VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
				write.pBufferInfo = &bufferInfo;
				write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				write.dstBinding = 3;
				write.dstSet = sets[3];
				write.descriptorCount = 1;
				vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);
				setCount = 4;
			}

			_vk2dRendererDraw(sets, setCount, NULL, shader->pipe, x, y, xscale, yscale, rot, originX, originY, 1,
							  xInTex,
							  yInTex, texWidth, texHeight);
		} else {
			vk2dLogMessage("Shader does not exist");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawInstanced(VK2DTexture tex, VK2DDrawInstance *instances, uint32_t count) {
	if (gRenderer != NULL) {
		VkDescriptorSet sets[3];
		sets[1] = gRenderer->samplerSet;
		sets[2] = tex->img->set;
		if (count > gRenderer->limits.maxInstancedDraws)
			count = gRenderer->limits.maxInstancedDraws;
		_vk2dRendererDrawInstanced(sets, 3, instances, count);
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawTexture(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight) {
	if (gRenderer != NULL) {
		if (tex != NULL) {
			VkDescriptorSet sets[3];
			sets[1] = gRenderer->samplerSet;
			sets[2] = tex->img->set;
			_vk2dRendererDraw(sets, 3, NULL, gRenderer->texPipe, x, y, xscale, yscale, rot, originX, originY, 1, xInTex,
							  yInTex, texWidth, texHeight);
		} else {
			vk2dLogMessage("Texture does not exist");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawPolygon(VK2DPolygon polygon, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY) {
	if (gRenderer != NULL) {
		if (polygon != NULL) {
			VkDescriptorSet set;
			_vk2dRendererDraw(&set, 1, polygon, filled ? gRenderer->primFillPipe : gRenderer->primLinePipe, x, y,
							  xscale,
							  yscale, rot, originX, originY, lineWidth, 0, 0, 0, 0);
		} else {
			vk2dLogMessage("Polygon does not exist");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawGeometry(VK2DVertexColour *vertices, int count, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY) {
    if (gRenderer != NULL) {
        if (vertices != NULL && count > 0) {
            if (count <= gRenderer->limits.maxGeometryVertices) {
                // Copy vertex data to the current descriptor buffer
                VkBuffer buffer;
                VkDeviceSize offset;
                vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], vertices,
                                             count * sizeof(VK2DVertexColour), &buffer, &offset);
                struct VK2DBuffer_t buf;
                buf.buf = buffer;
                buf.offset = offset;
                struct VK2DPolygon_t poly;
                poly.vertexCount = count;
                poly.vertices = &buf;
                poly.type = VK2D_VERTEX_TYPE_SHAPE;
                VkDescriptorSet set;
                _vk2dRendererDraw(&set, 1, &poly, filled ? gRenderer->primFillPipe : gRenderer->primLinePipe, x, y,
                                  xscale,
                                  yscale, rot, originX, originY, lineWidth, 0, 0, 0, 0);
                _vk2dRendererResetBoundPointers();
            }
        } else {
            vk2dLogMessage("Vertices do not exist");
        }
    } else {
        vk2dLogMessage("Renderer is not initialized");
    }
}

// TODO: Embed light sources into the renderer
void vk2dRendererDrawShadows(vec3 *vertices, int count, vec2 lightSource) {
    if (gRenderer != NULL) {
        if (vertices != NULL && count > 0) {
            if (count <= gRenderer->limits.maxGeometryVertices) {
                // Copy vertex data to the current descriptor buffer
                VkBuffer buffer;
                VkDeviceSize offset;
                vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], vertices,
                                             count * sizeof(vec3), &buffer, &offset);
                _vk2dRendererDrawShadows(lightSource, buffer, offset, count);
                _vk2dRendererResetBoundPointers();
            }
        } else {
            vk2dLogMessage("Vertices do not exist");
        }
    } else {
        vk2dLogMessage("Renderer is not initialized");
    }
}

void vk2dRendererDrawModel(VK2DModel model, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ) {
	if (gRenderer != NULL) {
		if (model != NULL) {
			VkDescriptorSet sets[3];
			sets[1] = gRenderer->modelSamplerSet;
			sets[2] = model->tex->img->set;
			_vk2dRendererDraw3D(sets, 3, model, gRenderer->modelPipe, x, y, z, xscale, yscale, zscale, rot, axis, originX,
								originY, originZ, 1);
		} else {
			vk2dLogMessage("Model does not exist");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

void vk2dRendererDrawWireframe(VK2DModel model, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ, float lineWidth) {
	if (gRenderer != NULL) {
		if (model != NULL) {
			VkDescriptorSet sets[3];
			sets[1] = gRenderer->modelSamplerSet;
			sets[2] = model->tex->img->set;
			_vk2dRendererDraw3D(sets, 3, model, gRenderer->wireframePipe, x, y, z, xscale, yscale, zscale, rot, axis, originX,
								originY, originZ, lineWidth);
		} else {
			vk2dLogMessage("Model does not exist");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

static inline float _getHexValue(char c) {
	switch (c) {
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'A': return 10;
		case 'B': return 11;
		case 'C': return 12;
		case 'D': return 13;
		case 'E': return 14;
		case 'F': return 15;
		case 'a': return 10;
		case 'b': return 11;
		case 'c': return 12;
		case 'd': return 13;
		case 'e': return 14;
		case 'f': return 15;
		default: return 0;
	}
}

void vk2dColourHex(vec4 dst, const char *hex) {
	if (strlen(hex) == 7 && hex[0] == '#') {
		dst[0] = ((_getHexValue(hex[1]) * 16) + _getHexValue(hex[2])) / 255;
		dst[1] = ((_getHexValue(hex[3]) * 16) + _getHexValue(hex[4])) / 255;
		dst[2] = ((_getHexValue(hex[5]) * 16) + _getHexValue(hex[6])) / 255;
		dst[3] = 1;
	} else {
		dst[0] = dst[1] = dst[2] = dst[3] = 0;
	}
}

void vk2dColourInt(vec4 dst, uint32_t colour) {
	dst[0] = (float)((colour>>24) & 0xFF) / 255.0f;
	dst[1] = (float)((colour>>16) & 0xFF) / 255.0f;
	dst[2] = (float)((colour>>8) & 0xFF) / 255.0f;
	dst[3] = (float)((colour) & 0xFF) / 255.0f;
}

void vk2dColourRGBA(vec4 dst, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
	dst[0] = (float)r / 255.0f;
	dst[1] = (float)g / 255.0f;
	dst[2] = (float)b / 255.0f;
	dst[3] = (float)a / 255.0f;
}

float vk2dRandom(float min, float max) {
	uint32_t r = SDL_AtomicGet(&gRNG);
	const int64_t a = 1103515245;
	const int64_t c = 12345;
	const int64_t m = 2147483648;
	r = (a * r + c) % m;
	SDL_AtomicSet(&gRNG, r);
	const int64_t resolution = 5000;
	float n = (float)(r % (resolution + 1));
	return min + ((max - min) * (n / (float)resolution));
}
