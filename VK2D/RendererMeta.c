/// \file RendererMeta.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>

#include "VK2D/RendererMeta.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/Constants.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Image.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Blobs.h"
#include "VK2D/Buffer.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Polygon.h"
#include "VK2D/Math.h"
#include "VK2D/Util.h"
#include "VK2D/DescriptorBuffer.h"
#include "VK2D/Opaque.h"

// For debugging
PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT;

VK2DVertexColour unitSquare[] = {
		{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}},
		{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}}
};
const uint32_t unitSquareVertices = 6;

vec2 unitSquareOutline[] = {
		{0, 0},
		{1, 0},
		{1, 1},
		{0, 1}
};
uint32_t unitSquareOutlineVertices = 4;

const vec2 LINE_VERTICES[] = {
		{0, 0},
		{1, 0}
};
const uint32_t LINE_VERTEX_COUNT = 2;

// Renderer has to keep track of user shaders in case the swapchain gets recreated
VK2DResult _vk2dRendererAddShader(VK2DShader shader) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return VK2D_ERROR;
	uint32_t i;
	uint32_t found = UINT32_MAX;
	for (i = 0; i < gRenderer->shaderListSize && found == UINT32_MAX; i++)
		if (gRenderer->customShaders[i] == NULL)
			found = i;

	// Make some room in the shader list
	if (found == UINT32_MAX) {
		VK2DShader *newList = realloc(gRenderer->customShaders, sizeof(VK2DShader) * (gRenderer->shaderListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		if (newList != NULL) {
			found = gRenderer->shaderListSize;
			gRenderer->customShaders = newList;

			// Set the new slots to null
			for (i = gRenderer->shaderListSize; i < gRenderer->shaderListSize + VK2D_DEFAULT_ARRAY_EXTENSION; i++) {
				gRenderer->customShaders[i] = NULL;
			}

			gRenderer->shaderListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		} else {
		    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to extend shaders list for %i shaders.", gRenderer->shaderListSize);
		    return VK2D_ERROR;
		}
	}

	gRenderer->customShaders[found] = shader;
	return VK2D_SUCCESS;
}

void _vk2dRendererRemoveShader(VK2DShader shader) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	for (i = 0; i < gRenderer->shaderListSize; i++)
		if (gRenderer->customShaders[i] == shader)
			gRenderer->customShaders[i] = NULL;
}

uint64_t _vk2dHashSets(VkDescriptorSet *sets, uint32_t setCount) {
	uint64_t hash = 0;
	for (uint32_t i = 0; i < setCount; i++) {
		uint64_t *pointer = (void*)&sets[i];
		hash += (*pointer) * (i + 1);
	}
	return hash;
}

void _vk2dRendererResetBoundPointers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	gRenderer->prevPipe = VK_NULL_HANDLE;
	gRenderer->prevSetHash = 0;
	gRenderer->prevVBO = VK_NULL_HANDLE;
}

// This is called when a render-target texture is created to make the renderer aware of it
void _vk2dRendererAddTarget(VK2DTexture tex) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	bool foundSpot = false;
	VK2DTexture *newList;
	for (i = 0; i < gRenderer->targetListSize; i++) {
		if (gRenderer->targets[i] == NULL) {
			foundSpot = true;
			gRenderer->targets[i] = tex;
		}
	}

	// We extend the list if no spots were found
	if (!foundSpot) {
		newList = realloc(gRenderer->targets, (gRenderer->targetListSize + VK2D_DEFAULT_ARRAY_EXTENSION) * sizeof(VK2DTexture));

		if (newList != NULL) {
			gRenderer->targets = newList;
			gRenderer->targets[gRenderer->targetListSize] = tex;

			// Fill the newly allocated parts of the list with NULL
			for (i = gRenderer->targetListSize + 1; i < gRenderer->targetListSize + VK2D_DEFAULT_ARRAY_EXTENSION; i++)
				gRenderer->targets[i] = NULL;

			gRenderer->targetListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		} else {
		    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to extend targets list for %i target spots.", gRenderer->targetListSize);
		}
	}
}

// Called when a render-target texture is destroyed so the renderer can remove it from its list
void _vk2dRendererRemoveTarget(VK2DTexture tex) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	for (i = 0; i < gRenderer->targetListSize; i++)
		if (gRenderer->targets[i] == tex)
			gRenderer->targets[i] = NULL;
}

// This is used when changing the render target to make sure the texture is either ready to be drawn itself or rendered to
void _vk2dTransitionImageLayout(VkImage img, VkImageLayout old, VkImageLayout new) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (vk2dStatusFatal())
	    return;

	VkPipelineStageFlags sourceStage = 0;
	VkPipelineStageFlags destinationStage = 0;

	VkImageMemoryBarrier barrier = {0};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old;
	barrier.newLayout = new;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (old == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (new == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && old == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	} else {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Unsupported image transition.");
	}

	vkCmdPipelineBarrier(
			gRenderer->commandBuffer[gRenderer->scImageIndex],
			sourceStage, destinationStage,
			0,
			0, VK_NULL_HANDLE,
			0, VK_NULL_HANDLE,
			1, &barrier
	);
}

// Rebuilds the matrices for a given buffer and camera
void _vk2dPrintMatrix(FILE* out, mat4 m, const char* prefix);
void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera, int index) {
	if (camera->type == VK2D_CAMERA_TYPE_DEFAULT) {
		// Assemble view
		mat4 view = {0};
		vec3 eyes = {camera->x + (camera->w * 0.5), camera->y + (camera->h * 0.5), -2};
		vec3 center = {eyes[0], eyes[1], 0};
		vec3 up = {sin(-camera->rot), -cos(-camera->rot), 0};
		cameraMatrix(view, eyes, center, up);

		// Projection
		mat4 proj = {0};
		orthographicMatrix(proj, camera->h / camera->zoom, camera->w / camera->h, 0.1, 10);

		// Multiply together
		memset(ubo->viewproj[index], 0, 64);
		multiplyMatrix(view, proj, ubo->viewproj[index]);
	} else {
		// Assemble view
		mat4 view = {0};
		cameraMatrix(view, camera->Perspective.eyes, camera->Perspective.centre, camera->Perspective.up);

		// Projection
		mat4 proj = {0};
		if (camera->type == VK2D_CAMERA_TYPE_ORTHOGRAPHIC)
			orthographicMatrix(proj, camera->h / camera->zoom, camera->w / camera->h, 0.1, 10);
		else
			perspectiveMatrix(proj, camera->Perspective.fov, camera->w /camera->h, 0.1, 10);

		// Multiply together
		memset(ubo->viewproj[index], 0, 64);
		multiplyMatrix(view, proj, ubo->viewproj[index]);
	}
}

//uint32_t frame, uint32_t descriptorFrame, int camera
// Copies the camera ubos to the descriptor buffer and spits out the corresponding descriptor set
void _vk2dRendererFlushUBOBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;

	VkBuffer buffer;
	VkDeviceSize offset;
	vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], &gRenderer->workingUBO, sizeof(VK2DUniformBufferObject), &buffer, &offset);
	VkDescriptorBufferInfo bufferInfo = {buffer, offset, sizeof(VK2DUniformBufferObject)};
	VkWriteDescriptorSet write = {0};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &bufferInfo;
	write.dstSet = gRenderer->uboDescriptorSets[gRenderer->currentFrame];
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);
}

void _vk2dRendererCreateDebug() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->options.enableDebug && gRenderer != NULL) {
		fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(gRenderer->vk,
																									 "vkCreateDebugReportCallbackEXT");
		fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(gRenderer->vk,
																									   "vkDestroyDebugReportCallbackEXT");

		if (fvkCreateDebugReportCallbackEXT != NULL && fvkDestroyDebugReportCallbackEXT != NULL) {
			VkDebugReportCallbackCreateInfoEXT callbackCreateInfoEXT = vk2dInitDebugReportCallbackCreateInfoEXT(
					_vk2dDebugCallback);
			fvkCreateDebugReportCallbackEXT(gRenderer->vk, &callbackCreateInfoEXT, VK_NULL_HANDLE, &gRenderer->dr);
		} else {
			vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to initialize debug layers.");
		}
	}
}

void _vk2dRendererDestroyDebug() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->options.enableDebug && gRenderer->dr != NULL) {
		fvkDestroyDebugReportCallbackEXT(gRenderer->vk, gRenderer->dr, VK_NULL_HANDLE);
	}
}

// Grabs a preferred present mode if available returning FIFO if its unavailable
VkPresentModeKHR _vk2dRendererGetPresentMode(VkPresentModeKHR mode) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();

	// Put present modes in the limits
	gRenderer->limits.supportsImmediate = false;
	gRenderer->limits.supportsTripleBuffering = false;
	for (int i = 0; i < gRenderer->presentModeCount; i++) {
		if (gRenderer->presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			gRenderer->limits.supportsImmediate = true;
		if (gRenderer->presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			gRenderer->limits.supportsTripleBuffering = true;
	}

	for (int i = 0; i < gRenderer->presentModeCount; i++)
		if (gRenderer->presentModes[i] == mode)
			return mode;
	return VK_PRESENT_MODE_FIFO_KHR;
}

void _vk2dRendererGetSurfaceSize() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities);
	if (result != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to get surface size, Vulkan error %i.", result);
	} else {
        if (gRenderer->surfaceCapabilities.currentExtent.width == UINT32_MAX ||
            gRenderer->surfaceCapabilities.currentExtent.height == UINT32_MAX) {
            SDL_GetWindowSizeInPixels(gRenderer->window, (void *) &gRenderer->surfaceWidth,
                                       (void *) &gRenderer->surfaceHeight);
        } else {
            gRenderer->surfaceWidth = gRenderer->surfaceCapabilities.currentExtent.width;
            gRenderer->surfaceHeight = gRenderer->surfaceCapabilities.currentExtent.height;
        }
    }
}

void _vk2dRendererCreateWindowSurface() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer != NULL) {
        // Create the surface then load up surface relevant values
        if (SDL_Vulkan_CreateSurface(gRenderer->window, gRenderer->vk, VK_NULL_HANDLE, &gRenderer->surface)) {
            VkResult result = vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, VK_NULL_HANDLE);
            if (result != VK_SUCCESS) {
                vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to get present modes, Vulkan error %i.", result);
                return;
            }
            gRenderer->presentModes = malloc(sizeof(VkPresentModeKHR) * gRenderer->presentModeCount);

            if (gRenderer->presentModes) {
                result = vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, gRenderer->presentModes);
                VkResult result2 = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities);
                if (result != VK_SUCCESS || result2 != VK_SUCCESS) {
                    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to get present modes, Vulkan error %i/%i.", result, result2);
                    return;
                }

                // You may want to search for a different format, but according to the Vulkan hardware database, 100% of systems support VK_FORMAT_B8G8R8A8_SRGB
                gRenderer->surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
                gRenderer->surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                _vk2dRendererGetSurfaceSize();
            } else {
                vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate %i present modes.", gRenderer->presentModeCount);
            }
        } else {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR | VK2D_STATUS_SDL_ERROR, "Failed to create surface, SDL error: %s.", SDL_GetError());
        }
	}
}

void _vk2dRendererDestroyWindowSurface() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroySurfaceKHR(gRenderer->vk, gRenderer->surface, VK_NULL_HANDLE);
	free(gRenderer->presentModes);
}

void _vk2dRendererCreateSwapchain() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;

	uint32_t imageCount = gRenderer->surfaceCapabilities.minImageCount > 3 ? gRenderer->surfaceCapabilities.minImageCount : 3;
	if (gRenderer->surfaceCapabilities.maxImageCount > 0 && imageCount > gRenderer->surfaceCapabilities.maxImageCount) {
		imageCount = gRenderer->surfaceCapabilities.maxImageCount;
	}

	gRenderer->config.screenMode = (VK2DScreenMode)_vk2dRendererGetPresentMode((VkPresentModeKHR)gRenderer->config.screenMode);
	VkSwapchainCreateInfoKHR  swapchainCreateInfoKHR = vk2dInitSwapchainCreateInfoKHR(
			gRenderer->surface,
			gRenderer->surfaceCapabilities,
			gRenderer->surfaceFormat,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			(VkPresentModeKHR)gRenderer->config.screenMode,
			VK_NULL_HANDLE,
			imageCount
	);
	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(gRenderer->pd->dev, gRenderer->pd->QueueFamily.graphicsFamily, gRenderer->surface, &supported);
	if (supported) {
		vkCreateSwapchainKHR(gRenderer->ld->dev, &swapchainCreateInfoKHR, VK_NULL_HANDLE, &gRenderer->swapchain);

        VkResult result = vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, VK_NULL_HANDLE);
        if (result >= 0) {
            gRenderer->swapchainImageViews = calloc(1, gRenderer->swapchainImageCount * sizeof(VkImageView));
            gRenderer->swapchainImages = calloc(1, gRenderer->swapchainImageCount * sizeof(VkImage));
            if (gRenderer->swapchainImageViews && gRenderer->swapchainImages) {
                result = vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, gRenderer->swapchainImages);

                if (result >= 0) {
                    for (i = 0; i < gRenderer->swapchainImageCount; i++) {
                        VkImageViewCreateInfo imageViewCreateInfo = vk2dInitImageViewCreateInfo(gRenderer->swapchainImages[i], gRenderer->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
                        result = vkCreateImageView(gRenderer->ld->dev, &imageViewCreateInfo, VK_NULL_HANDLE, &gRenderer->swapchainImageViews[i]);
                        if (result < 0) {
                            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to acquire swapchain image [%i], Vulkan error %i.", i, result);
                            break;
                        }
                    }
                    vk2dLog("Swapchain (%i images) initialized...", swapchainCreateInfoKHR.minImageCount);
                } else {
                    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to acquire swapchain images, Vulkan error %i.", result);
                }
            } else {
                vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate swapchain image arrays.");
                free(gRenderer->swapchainImageViews);
                gRenderer->swapchainImageViews = NULL;
                free(gRenderer->swapchainImages);
                gRenderer->swapchainImages = NULL;
            }
        } else {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to get swapchain images, Vulkan error %i.", result);
        }
	} else {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Physical device does not support KHR swapchain.");
	}
}

void _vk2dRendererDestroySwapchain() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vkDestroyImageView(gRenderer->ld->dev, gRenderer->swapchainImageViews[i], VK_NULL_HANDLE);

	vkDestroySwapchainKHR(gRenderer->ld->dev, gRenderer->swapchain, VK_NULL_HANDLE);
	free(gRenderer->swapchainImageViews);
	free(gRenderer->swapchainImages);
}

void _vk2dRendererCreateDepthBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;

	// Find a supported depth buffer format
	VkFormat formats[] = {VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	int formatCount = 4;
	VkFormat selectedFormat = VK_FORMAT_MAX_ENUM;

	for (int i = 0; i < formatCount && selectedFormat == VK_FORMAT_MAX_ENUM; i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(gRenderer->pd->dev, formats[i], &props);
		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
			selectedFormat = formats[i];
	}

	if (selectedFormat != VK_FORMAT_MAX_ENUM) {
		gRenderer->depthBufferFormat = selectedFormat;
		gRenderer->depthBuffer = vk2dImageCreate(gRenderer->ld, gRenderer->surfaceWidth, gRenderer->surfaceHeight, gRenderer->depthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, (VkSampleCountFlagBits)gRenderer->config.msaa);
        vk2dLog("Depth buffer initialized...");
	} else {
        vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to acquire depth format.");
	}
}

void _vk2dRendererDestroyDepthBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dImageFree(gRenderer->depthBuffer);
	gRenderer->depthBuffer = NULL;
}


void _vk2dRendererCreateColourResources() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	if (gRenderer->config.msaa != VK2D_MSAA_1X) {
		gRenderer->msaaImage = vk2dImageCreate(
				gRenderer->ld,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				gRenderer->surfaceFormat.format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				(VkSampleCountFlagBits) gRenderer->config.msaa);
        vk2dLog("MSAA %ix enabled...", gRenderer->config.msaa);
	} else {
        vk2dLog("MSAA disabled...");
	}
}

void _vk2dRendererDestroyColourResources() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->msaaImage != NULL)
		vk2dImageFree(gRenderer->msaaImage);
	gRenderer->msaaImage = NULL;
}

void _vk2dRendererCreateDescriptorBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	gRenderer->descriptorBuffers = malloc(sizeof(VK2DDescriptorBuffer) * VK2D_MAX_FRAMES_IN_FLIGHT);
	if (gRenderer->descriptorBuffers == NULL) {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate descriptor buffers array.");
	    return;
	}
	for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
		gRenderer->descriptorBuffers[i] = vk2dDescriptorBufferCreate(gRenderer->options.vramPageSize);
	}

	// Calculate max instances
	const int maxDrawInstances = gRenderer->options.vramPageSize / sizeof(VK2DDrawInstance);
	const int maxDrawCommands = gRenderer->options.vramPageSize / sizeof(VK2DDrawCommand);

	gRenderer->limits.maxInstancedDraws = maxDrawCommands < maxDrawInstances ? maxDrawCommands : maxDrawInstances;
	gRenderer->limits.maxInstancedDraws--;

    vk2dLog("Descriptor buffers created...");
}

void _vk2dRendererDestroyDescriptorBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->descriptorBuffers == NULL)
	    return;
	for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++)
		vk2dDescriptorBufferFree(gRenderer->descriptorBuffers[i]);
	free(gRenderer->descriptorBuffers);
}


void _vk2dRendererCreateRenderPass() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t attachCount;
	if (gRenderer->config.msaa != 1) {
		attachCount = 3; // colour, depth, resolve
	} else {
		attachCount = 2; // colour, depth
	}
	VkAttachmentReference resolveAttachment;
	VkAttachmentDescription attachments[3];
	memset(attachments, 0, sizeof(VkAttachmentDescription) * attachCount);
	attachments[0].format = gRenderer->surfaceFormat.format;
	attachments[0].samples = (VkSampleCountFlagBits)gRenderer->config.msaa;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = gRenderer->config.msaa > 1 ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachments[1].format = gRenderer->depthBufferFormat;
	attachments[1].samples = (VkSampleCountFlagBits)gRenderer->config.msaa;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	if (gRenderer->config.msaa != 1) {
		attachments[2].format = gRenderer->surfaceFormat.format;
		attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		resolveAttachment.attachment = 2;
		resolveAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// Set up subpass color attachment
	const uint32_t colourAttachCount = 1;
	VkAttachmentReference subpassColourAttachments0[1];
	uint32_t i;
	for (i = 0; i < colourAttachCount; i++) {
		subpassColourAttachments0[i].attachment = 0;
		subpassColourAttachments0[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	// Subpass depth attachment
	VkAttachmentReference subpassDepthAttachmentReference = {0};
	subpassDepthAttachmentReference.attachment = 1;
	subpassDepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Set up subpass
	const uint32_t subpassCount = 1;
	VkSubpassDescription subpasses[1] = {0};
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount = colourAttachCount;
	subpasses[0].pColorAttachments = subpassColourAttachments0;
	subpasses[0].pDepthStencilAttachment = &subpassDepthAttachmentReference;
	subpasses[0].pResolveAttachments = gRenderer->config.msaa > 1 ? &resolveAttachment : VK_NULL_HANDLE;

	// Subpass dependency
	VkSubpassDependency subpassDependency = {0};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = vk2dInitRenderPassCreateInfo(attachments, attachCount, subpasses, subpassCount, &subpassDependency, 1);
	VkResult result = vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->renderPass);
	if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create render pass, out of memory.");
        return;
	} else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Failed to create render pass, out of video memory.");
        return;
	}

	// Two more render passes for mid-frame render pass reset to swapchain and rendering to textures
	if (gRenderer->config.msaa != 1) {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	} else {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	result = vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->midFrameSwapRenderPass);
    if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create render pass, out of memory.");
        return;
    } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Failed to create render pass, out of video memory.");
        return;
    }

	if (gRenderer->config.msaa != 1) {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	} else {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	result = vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->externalTargetRenderPass);
    if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create render pass, out of memory.");
        return;
    } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Failed to create render pass, out of video memory.");
        return;
    }

    vk2dLog("Render pass initialized...");
}

void _vk2dRendererDestroyRenderPass() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->renderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->externalTargetRenderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->midFrameSwapRenderPass, VK_NULL_HANDLE);
}

void _vk2dRendererCreateDescriptorSetLayouts() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    VkResult r1, r2, r3, r4, r5, r6, r7;

    // For texture samplers
	const uint32_t layoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[1];
	descriptorSetLayoutBinding[0] = vk2dInitDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBinding, layoutCount);
	r1 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslSampler);

	// For view projection buffers
	const uint32_t shapeLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingShapes[1];
	descriptorSetLayoutBindingShapes[0] = vk2dInitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo shapesDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingShapes, shapeLayoutCount);
	r2 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &shapesDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslBufferVP);

	// For user-created shaders
	const uint32_t userLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingUser[1];
	descriptorSetLayoutBindingUser[0] = vk2dInitDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo userDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingUser, userLayoutCount);
	r3 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &userDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslBufferUser);

	// For sampled textures
	const uint32_t texLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingTex[1];
	descriptorSetLayoutBindingTex[0] = vk2dInitDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo texDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingTex, texLayoutCount);
	r4 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &texDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslTexture);

	// For texture array
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindingTexArray = {
            .binding = 2,
            .descriptorCount = gRenderer->options.maxTextures,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImmutableSamplers = VK_NULL_HANDLE,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    const VkDescriptorBindingFlagsEXT flags =
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlags = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .bindingCount = 1,
            .pBindingFlags = &flags
    };
    VkDescriptorSetLayoutCreateInfo texArraySetLayoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
            .pNext = &bindingFlags,
            .pBindings = &descriptorSetLayoutBindingTexArray,
            .bindingCount = 1
    };
    r5 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &texArraySetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslTextureArray);

    // DSL for compute
    VkDescriptorSetLayoutBinding dslbCompute[2] = {
            {
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .binding = 0
            },
            {
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .binding = 1
            }
    };
    VkDescriptorSetLayoutCreateInfo dslComputeCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pBindings = dslbCompute,
            .bindingCount = 2
    };
    r6 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &dslComputeCreateInfo, VK_NULL_HANDLE, &gRenderer->dslSpriteBatch);

    // For instanced vertex shader sbo shaders
    const uint32_t sboLayoutCount = 1;
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindingSBO[1];
    descriptorSetLayoutBindingSBO[0] = vk2dInitDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, VK_NULL_HANDLE);
    VkDescriptorSetLayoutCreateInfo sboDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingSBO, sboLayoutCount);
    r7 = vkCreateDescriptorSetLayout(gRenderer->ld->dev, &sboDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslBufferSBO);

	if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS || r4 != VK_SUCCESS || r5 != VK_SUCCESS || r6 != VK_SUCCESS || r7 != VK_SUCCESS) {
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create descriptor set layouts %i/%i/%i/%i/%i/%i/%i.", r1, r2, r3, r4, r5, r6, r7);
	    return;
	}

    vk2dLog("Descriptor set layout initialized...");
}

void _vk2dRendererDestroyDescriptorSetLayout() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslSampler, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferVP, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferUser, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslTexture, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslTextureArray, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslSpriteBatch, VK_NULL_HANDLE);
    vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferSBO, VK_NULL_HANDLE);
}

VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState();
VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState();
void _vk2dShaderBuildPipe(VK2DShader shader);
void _vk2dRendererCreatePipelines() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	VkPipelineVertexInputStateCreateInfo textureVertexInfo = _vk2dGetTextureVertexInputState();
	VkPipelineVertexInputStateCreateInfo colourVertexInfo = _vk2dGetColourVertexInputState();
	VkPipelineVertexInputStateCreateInfo modelVertexInfo = _vk2dGetModelVertexInputState();

	VkPipelineVertexInputStateCreateInfo instanceVertexInfo = _vk2dGetInstanceVertexInputState();
    VkPipelineVertexInputStateCreateInfo shadowsVertexInfo = _vk2dGetShadowsVertexInputState();

	// Default shader files
	uint32_t shaderColourVertSize = sizeof(VK2DVertColour);
	unsigned char *shaderColourVert = (void*)VK2DVertColour;
	uint32_t shaderColourFragSize = sizeof(VK2DFragColour);
	unsigned char *shaderColourFrag = (void*)VK2DFragColour;
	uint32_t shaderModelVertSize = sizeof(VK2DVertModel);
	unsigned char *shaderModelVert = (void*)VK2DVertModel;
	uint32_t shaderModelFragSize = sizeof(VK2DFragModel);
	unsigned char *shaderModelFrag = (void*)VK2DFragModel;
    uint32_t shaderInstancedVertSize = sizeof(VK2DVertInstanced);
    unsigned char *shaderInstancedVert = (void*)VK2DVertInstanced;
    uint32_t shaderInstancedFragSize = sizeof(VK2DFragInstanced);
    unsigned char *shaderInstancedFrag = (void*)VK2DFragInstanced;
    uint32_t shaderShadowsVertSize = sizeof(VK2DVertShadows);
    unsigned char *shaderShadowsVert = (void*)VK2DVertShadows;
    uint32_t shaderShadowsFragSize = sizeof(VK2DFragShadows);
    unsigned char *shaderShadowsFrag = (void*)VK2DFragShadows;

	// In case one of the load files failed
	if (vk2dStatusFatal())
	    return;

	// Texture pipeline
    VkDescriptorSetLayout instancedLayout[] = {gRenderer->dslBufferVP, gRenderer->dslSampler, gRenderer->dslTextureArray, gRenderer->dslBufferSBO};

	// Polygon pipelines
	gRenderer->primFillPipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderColourVert,
			shaderColourVertSize,
			shaderColourFrag,
			shaderColourFragSize,
			&gRenderer->dslBufferVP,
			1,
			&colourVertexInfo,
			true,
			gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_DEFAULT);
	gRenderer->primLinePipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderColourVert,
			shaderColourVertSize,
			shaderColourFrag,
			shaderColourFragSize,
			&gRenderer->dslBufferVP,
			1,
			&colourVertexInfo,
			false,
			gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_DEFAULT);

	// 3D pipelines
	gRenderer->modelPipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderModelVert,
			shaderModelVertSize,
			shaderModelFrag,
			shaderModelFragSize,
            instancedLayout,
			3,
			&modelVertexInfo,
			true,
			gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_3D);
	gRenderer->wireframePipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderModelVert,
			shaderModelVertSize,
			shaderModelFrag,
			shaderModelFragSize,
            instancedLayout,
			3,
			&modelVertexInfo,
			false,
			gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_3D);
	gRenderer->instancedPipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderInstancedVert,
			shaderInstancedVertSize,
			shaderInstancedFrag,
			shaderInstancedFragSize,
			instancedLayout,
			4,
			&instanceVertexInfo,
			true,
			gRenderer->config.msaa,
			VK2D_PIPELINE_TYPE_INSTANCING);

	// Shadows pipeline
    gRenderer->shadowsPipe = vk2dPipelineCreate(
            gRenderer->ld,
            gRenderer->renderPass,
            gRenderer->surfaceWidth,
            gRenderer->surfaceHeight,
            shaderShadowsVert,
            shaderShadowsVertSize,
            shaderShadowsFrag,
            shaderShadowsFragSize,
            instancedLayout,
            1,
            &shadowsVertexInfo,
            true,
            gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_SHADOWS);

    // Compute
    gRenderer->spriteBatchPipe = vk2dPipelineCreateCompute(
            gRenderer->ld,
            sizeof(VK2DComputePushBuffer),
            (void*)VK2DCompSpritebatch,
            sizeof(VK2DCompSpritebatch),
            &gRenderer->dslSpriteBatch,
            1);

	// Shader pipelines
	for (i = 0; i < gRenderer->shaderListSize; i++) {
		if (gRenderer->customShaders[i] != NULL) {
			vk2dPipelineFree(gRenderer->customShaders[i]->pipe);
			_vk2dShaderBuildPipe(gRenderer->customShaders[i]);
		}
	}

    // In case something somewhere failed
    if (!vk2dStatusFatal())
        vk2dLog("Pipelines initialized...");
}

void _vk2dRendererDestroyPipelines(bool preserveCustomPipes) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dPipelineFree(gRenderer->primLinePipe);
	vk2dPipelineFree(gRenderer->primFillPipe);
	vk2dPipelineFree(gRenderer->modelPipe);
	vk2dPipelineFree(gRenderer->wireframePipe);
    vk2dPipelineFree(gRenderer->instancedPipe);
    vk2dPipelineFree(gRenderer->shadowsPipe);
    vk2dPipelineFree(gRenderer->spriteBatchPipe);

    if (!preserveCustomPipes)
		free(gRenderer->customShaders);
}

void _vk2dRendererCreateFrameBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	gRenderer->framebuffers = malloc(sizeof(VkFramebuffer) * gRenderer->swapchainImageCount);

	if (gRenderer->framebuffers) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			// There is no 3rd attachment if msaa is disabled
			const int attachCount = gRenderer->config.msaa > 1 ? 3 : 2;
			VkImageView attachments[3];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->msaaImage->view;
				attachments[1] = gRenderer->depthBuffer->view;
				attachments[2] = gRenderer->swapchainImageViews[i];
			} else {
				attachments[0] = gRenderer->swapchainImageViews[i];
				attachments[1] = gRenderer->depthBuffer->view;
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->renderPass, gRenderer->surfaceWidth, gRenderer->surfaceHeight, attachments, attachCount);
			VkResult result = vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->framebuffers[i]);
			if (result == VK_SUCCESS) {
                vk2dLog("Framebuffer[%i] ready...", i);
            } else {
                if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
                    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create framebuffer, out of memory.");
                } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
                    vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Failed to create framebuffer, out of video memory.");
                }
                free(gRenderer->framebuffers);
                gRenderer->framebuffers = NULL;
                break;
			}
		}
        vk2dLog("Framebuffers initialized...");
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate framebuffer list.");
	}

}

void _vk2dRendererDestroyFrameBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	if (gRenderer->framebuffers != NULL) {
        for (i = 0; i < gRenderer->swapchainImageCount; i++)
            vkDestroyFramebuffer(gRenderer->ld->dev, gRenderer->framebuffers[i], VK_NULL_HANDLE);
        free(gRenderer->framebuffers);
    }
}

void _vk2dRendererCreateUniformBuffers(bool newCamera) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	if (newCamera) { // If the renderer has not yet been initialized
		VK2DCameraSpec cam = {
				VK2D_CAMERA_TYPE_DEFAULT,
				0,
				0,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				1,
				0,
				0,
				0,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight
		};

		// Set all cameras to deleted and create the default one
		for (int i = 0; i < VK2D_MAX_CAMERAS; i++)
			gRenderer->cameras[i].state = VK2D_CAMERA_STATE_DELETED;
		vk2dCameraCreate(cam);
	} else {
		// Recreate camera buffers with new screen
		for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
			if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_RESET) {
				gRenderer->cameras[i].state = VK2D_CAMERA_STATE_DELETED;
				vk2dCameraCreate(gRenderer->cameras[i].spec);
			}
		}
	}

	// Set default camera new viewport/scissor
	gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.wOnScreen = gRenderer->surfaceWidth;
	gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.hOnScreen = gRenderer->surfaceHeight;
	gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.w = gRenderer->surfaceWidth;
	gRenderer->cameras[VK2D_DEFAULT_CAMERA].spec.h = gRenderer->surfaceHeight;

	if (!vk2dStatusFatal())
        vk2dLog("UBO initialized...");
}

void _vk2dRendererDestroyUniformBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	for (i = 0; i < VK2D_MAX_CAMERAS; i++)
		if (vk2dCameraGetState(i) == VK2D_CAMERA_STATE_NORMAL || vk2dCameraGetState(i) == VK2D_CAMERA_STATE_DISABLED)
			vk2dCameraSetState(i, VK2D_CAMERA_STATE_RESET);
}

void _vk2dRendererCreateSpriteBatching() {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    gRenderer->drawCommands = malloc(sizeof(struct VK2DDrawCommand) * gRenderer->limits.maxInstancedDraws);
    gRenderer->drawCommandCount = 0;

    if (gRenderer->drawCommands == NULL) {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate sprite batch of count %i.", gRenderer->limits.maxInstancedDraws);
    }
}

void _vk2dRendererDestroySpriteBatching() {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    free(gRenderer->drawCommands);
}

void _vk2dRendererCreateDescriptorPool(bool preserveDescCons) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	if (!preserveDescCons) {
		gRenderer->descConSamplers = vk2dDescConCreate(gRenderer->ld, gRenderer->dslTexture, VK2D_NO_LOCATION, 2, VK2D_NO_LOCATION);
		gRenderer->descConSamplersOff = vk2dDescConCreate(gRenderer->ld, gRenderer->dslTexture, VK2D_NO_LOCATION, 2, VK2D_NO_LOCATION);
		gRenderer->descConVP = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferVP, 0, VK2D_NO_LOCATION, VK2D_NO_LOCATION);
		gRenderer->descConUser = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferUser, 3, VK2D_NO_LOCATION, VK2D_NO_LOCATION);
		for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
            gRenderer->descConCompute[i] = vk2dDescConCreate(gRenderer->ld, gRenderer->dslSpriteBatch, VK2D_NO_LOCATION, VK2D_NO_LOCATION, 0);
            gRenderer->descConShaders[i] = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferUser, 3, VK2D_NO_LOCATION, VK2D_NO_LOCATION);
            gRenderer->descConSBO[i] = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferSBO, VK2D_NO_LOCATION, VK2D_NO_LOCATION, 3);
        }

		// And the one sampler set
		VkDescriptorPoolSize sizes = {VK_DESCRIPTOR_TYPE_SAMPLER, 4};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vk2dInitDescriptorPoolCreateInfo(&sizes, 1, 4);
		VkResult result = vkCreateDescriptorPool(gRenderer->ld->dev, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &gRenderer->samplerPool);
		if (result == VK_SUCCESS) {
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vk2dInitDescriptorSetAllocateInfo(gRenderer->samplerPool, 1, &gRenderer->dslSampler);
            result = vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->samplerSet);
            VkResult result2 = vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->modelSamplerSet);

            if (result != VK_SUCCESS || result2 != VK_SUCCESS)
                vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate descriptor set, Vulkan error %i/%i.", result, result2);
            else
                vk2dLog("Descriptor controllers initialized...");
		} else {
		    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate descriptor pool, Vulkan error %i.", result);
		}

		// Tex array set
		VkDescriptorPoolSize texArraySize = {
		        .descriptorCount = gRenderer->options.maxTextures,
		        .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
		};
		VkDescriptorPoolCreateInfo texArrayPoolCreateInfo = {
		        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		        .pPoolSizes = &texArraySize,
		        .poolSizeCount = 1,
		        .maxSets = 1,
		        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
		};
		VkResult texArrayResult = vkCreateDescriptorPool(gRenderer->ld->dev, &texArrayPoolCreateInfo, VK_NULL_HANDLE, &gRenderer->texArrayPool);
		if (texArrayResult != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate texture array descriptor pool, Vulkan error %i.", result);
		}

		// Make the tex array set
		VkDescriptorSetVariableDescriptorCountAllocateInfo setCounts = {
		        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
		        .pDescriptorCounts = &gRenderer->options.maxTextures,
		        .descriptorSetCount = 1
		};
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		        .descriptorPool = gRenderer->texArrayPool,
		        .descriptorSetCount = 1,
		        .pSetLayouts = &gRenderer->dslTextureArray,
		        .pNext = &setCounts
		};
		result = vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->texArrayDescriptorSet);
        if (result != VK_SUCCESS) {
            vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to allocate texture array descriptor set, Vulkan error %i.", result);
        }

        // Make the descriptor tracker
        gRenderer->textureArray = calloc(gRenderer->options.maxTextures, sizeof(struct VK2DTextureDescriptorInfo_t));
        if (gRenderer->textureArray == NULL) {
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate texture info for %i potential textures.", gRenderer->options.maxTextures);
        }

        // Make the viewproj descriptor sets
        for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++)
            gRenderer->uboDescriptorSets[i] = vk2dDescConGetSet(gRenderer->descConVP);
	} else {
        vk2dLog("Descriptor controllers preserved...");
	}
}

void _vk2dRendererDestroyDescriptorPool(bool preserveDescCons) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (!preserveDescCons) {
		vk2dDescConFree(gRenderer->descConSamplers);
		vk2dDescConFree(gRenderer->descConSamplersOff);
		vk2dDescConFree(gRenderer->descConVP);
		vk2dDescConFree(gRenderer->descConUser);
        for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
            vk2dDescConFree(gRenderer->descConCompute[i]);
            vk2dDescConFree(gRenderer->descConShaders[i]);
            vk2dDescConFree(gRenderer->descConSBO[i]);
        }
        vkDestroyDescriptorPool(gRenderer->ld->dev, gRenderer->samplerPool, VK_NULL_HANDLE);
        vkDestroyDescriptorPool(gRenderer->ld->dev, gRenderer->texArrayPool, VK_NULL_HANDLE);
        free(gRenderer->textureArray);
    }
}

void _vk2dRendererCreateSynchronization() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk2dInitSemaphoreCreateInfo(0);
	VkFenceCreateInfo fenceCreateInfo = vk2dInitFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	gRenderer->imageAvailableSemaphores = calloc(1, sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->renderFinishedSemaphores = calloc(1, sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->inFlightFences = calloc(1, sizeof(VkFence) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->imagesInFlight = calloc(1, sizeof(VkFence) * gRenderer->swapchainImageCount);
	gRenderer->commandBuffer = calloc(1, sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);
    gRenderer->dbCommandBuffer = calloc(1, sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);
    gRenderer->computeCommandBuffer = calloc(1, sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);

    if (gRenderer->imageAvailableSemaphores != NULL && gRenderer->renderFinishedSemaphores != NULL
		&& gRenderer->inFlightFences != NULL && gRenderer->imagesInFlight != NULL) {
		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
			VkResult r1 = vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->imageAvailableSemaphores[i]);
			VkResult r2 = vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->renderFinishedSemaphores[i]);
			VkResult r3 = vkCreateFence(gRenderer->ld->dev, &fenceCreateInfo, VK_NULL_HANDLE, &gRenderer->inFlightFences[i]);
			if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS)
			    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create synchronization objects, Vulkan error %i/%i/%i.", r1, r2, r3);
		}
	} else {
	    vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate synchronization objects.");
	}

	if (gRenderer->commandBuffer != NULL && gRenderer->dbCommandBuffer != NULL) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			gRenderer->commandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
            gRenderer->dbCommandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
            gRenderer->computeCommandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
        }
	} else {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate synchronization objects.");
	}

	if (!vk2dStatusFatal())
        vk2dLog("Synchronization initialized...");
}

void _vk2dRendererDestroySynchronization() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;

	if (gRenderer->renderFinishedSemaphores != NULL && gRenderer->imageAvailableSemaphores != NULL && gRenderer->inFlightFences != NULL) {
        for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(gRenderer->ld->dev, gRenderer->renderFinishedSemaphores[i], VK_NULL_HANDLE);
            vkDestroySemaphore(gRenderer->ld->dev, gRenderer->imageAvailableSemaphores[i], VK_NULL_HANDLE);
            vkDestroyFence(gRenderer->ld->dev, gRenderer->inFlightFences[i], VK_NULL_HANDLE);
        }
    }
	free(gRenderer->imagesInFlight);
	free(gRenderer->inFlightFences);
	free(gRenderer->imageAvailableSemaphores);
	free(gRenderer->renderFinishedSemaphores);
    free(gRenderer->commandBuffer);
    free(gRenderer->dbCommandBuffer);
    free(gRenderer->computeCommandBuffer);
}

void _vk2dRendererCreateSampler() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;

	// 2D sampler
	VkSamplerCreateInfo samplerCreateInfo = vk2dInitSamplerCreateInfo(gRenderer->config.filterMode == VK2D_FILTER_TYPE_LINEAR, gRenderer->config.filterMode == VK2D_FILTER_TYPE_LINEAR ? gRenderer->config.msaa : 1, 1);
	VkResult r1 = vkCreateSampler(gRenderer->ld->dev, &samplerCreateInfo, VK_NULL_HANDLE, &gRenderer->textureSampler);
	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.sampler = gRenderer->textureSampler;
	VkWriteDescriptorSet write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLER, 1, gRenderer->samplerSet, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);

	// 3D sampler
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	VkResult r2 = vkCreateSampler(gRenderer->ld->dev, &samplerCreateInfo, VK_NULL_HANDLE, &gRenderer->modelSampler);
	imageInfo.sampler = gRenderer->modelSampler;
	write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLER, 1, gRenderer->modelSamplerSet, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);

	if (r1 == VK_SUCCESS && r2 == VK_SUCCESS)
        vk2dLog("Created texture sampler...");
	else
	    vk2dRaise(VK2D_STATUS_VULKAN_ERROR, "Failed to create sampler descriptor sets, Vulkan error %i/%i.", r1, r2);
}

void _vk2dRendererDestroySampler() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroySampler(gRenderer->ld->dev, gRenderer->textureSampler, VK_NULL_HANDLE);
	vkDestroySampler(gRenderer->ld->dev, gRenderer->modelSampler, VK_NULL_HANDLE);
}

void _vk2dRendererCreateUnits() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	// Squares are simple
	gRenderer->unitSquare = vk2dPolygonShapeCreateRaw(unitSquare, unitSquareVertices);
	gRenderer->unitSquareOutline = vk2dPolygonCreateOutline(unitSquareOutline, unitSquareOutlineVertices);

	// Now to generate the circle
	vec2 *circleVertices = malloc(sizeof(vec2) * VK2D_CIRCLE_VERTICES);
	uint32_t i;
	for (i = 0; i < VK2D_CIRCLE_VERTICES; i++) {
		circleVertices[i][0] = cos(((double)i / VK2D_CIRCLE_VERTICES) * (VK2D_PI * 2)) * 0.5;
		circleVertices[i][1] = sin(((double)i / VK2D_CIRCLE_VERTICES) * (VK2D_PI * 2)) * 0.5;
	}
	gRenderer->unitCircle = vk2dPolygonCreate(circleVertices, VK2D_CIRCLE_VERTICES);
	gRenderer->unitCircleOutline = vk2dPolygonCreateOutline(circleVertices, VK2D_CIRCLE_VERTICES);
	gRenderer->unitLine = vk2dPolygonCreateOutline((void*)LINE_VERTICES, LINE_VERTEX_COUNT);
    if (!vk2dStatusFatal())
        vk2dLog("Created unit polygons...");
}

void _vk2dRendererDestroyUnits() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dPolygonFree(gRenderer->unitSquare);
	vk2dPolygonFree(gRenderer->unitSquareOutline);
	vk2dPolygonFree(gRenderer->unitCircle);
	vk2dPolygonFree(gRenderer->unitCircleOutline);
	vk2dPolygonFree(gRenderer->unitLine);
}

void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void _vk2dRendererRefreshTargets() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	uint32_t i;
	uint32_t targetsRefreshed = 0;
	for (i = 0; i < gRenderer->targetListSize; i++) {
		if (gRenderer->targets[i] != NULL) {
			targetsRefreshed++;

			// Sampled image
			vk2dImageFree(gRenderer->targets[i]->sampledImg);
			vk2dImageFree(gRenderer->targets[i]->depthBuffer);
			gRenderer->targets[i]->sampledImg = vk2dImageCreate(
					gRenderer->ld,
					gRenderer->targets[i]->img->width,
					gRenderer->targets[i]->img->height,
					VK_FORMAT_B8G8R8A8_SRGB,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
					(VkSampleCountFlagBits)gRenderer->config.msaa);
			_vk2dImageTransitionImageLayout(gRenderer->ld, gRenderer->targets[i]->sampledImg->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			//_vk2dImageTransitionImageLayout(gRenderer->ld, gRenderer->targets[i]->depthBuffer->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			gRenderer->targets[i]->depthBuffer = vk2dImageCreate(gRenderer->ld, gRenderer->targets[i]->img->width, gRenderer->targets[i]->img->height, gRenderer->depthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, (VkSampleCountFlagBits)gRenderer->config.msaa);

			// Framebuffer
			vkDestroyFramebuffer(gRenderer->ld->dev, gRenderer->targets[i]->fbo, VK_NULL_HANDLE);
			const int attachCount = gRenderer->config.msaa > 1 ? 3 : 2;
			VkImageView attachments[3];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->targets[i]->sampledImg->view;
				attachments[1] = gRenderer->targets[i]->depthBuffer->view;
				attachments[2] = gRenderer->targets[i]->img->view;
			} else {
				attachments[0] = gRenderer->targets[i]->img->view;
				attachments[1] = gRenderer->targets[i]->depthBuffer->view;
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->externalTargetRenderPass, gRenderer->targets[i]->img->width, gRenderer->targets[i]->img->height, attachments, attachCount);
			VkResult result = vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->targets[i]->fbo);
            if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
                vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to create framebuffer, out of memory.");
            } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
                vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Failed to create framebuffer, out of video memory.");
            }
		}
	}
	if (!vk2dStatusFatal())
        vk2dLog("Refreshed %i render targets...", targetsRefreshed);
}

void _vk2dRendererDestroyTargetsList() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	free(gRenderer->targets);
}

// If the window is resized or minimized or whatever
void _vk2dRendererResetSwapchain() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;

	// Hang while minimized
	SDL_WindowFlags flags;
	flags = SDL_GetWindowFlags(gRenderer->window);
	while (flags & SDL_WINDOW_MINIMIZED) {
		flags = SDL_GetWindowFlags(gRenderer->window);
		SDL_PumpEvents();
	}
	VkResult result = vkDeviceWaitIdle(gRenderer->ld->dev);
    if (result == VK_ERROR_OUT_OF_HOST_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_RAM,"Out of memory.");
        return;
    } else if (result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
        vk2dRaise(VK2D_STATUS_OUT_OF_VRAM, "Out of video memory.");
        return;
    } else if (result == VK_ERROR_DEVICE_LOST) {
        vk2dRaise(VK2D_STATUS_DEVICE_LOST, "Device lost on wait.");
        return;
    }

	// Free swapchain
	_vk2dRendererDestroySynchronization();
	_vk2dRendererDestroySampler();
	_vk2dRendererDestroyDescriptorPool(true);
	_vk2dRendererDestroyUniformBuffers();
	_vk2dRendererDestroyFrameBuffer();
	_vk2dRendererDestroyPipelines(true);
	_vk2dRendererDestroyRenderPass();
	_vk2dRendererDestroyDepthBuffer();
	_vk2dRendererDestroyColourResources();
	_vk2dRendererDestroySwapchain();

    vk2dLog("Destroyed swapchain assets...");

	// Swap out configs in case they were changed
	gRenderer->config = gRenderer->newConfig;

	// Restart swapchain
	_vk2dRendererGetSurfaceSize();
	_vk2dRendererCreateSwapchain();
	_vk2dRendererCreateColourResources();
	_vk2dRendererCreateDepthBuffer();
	_vk2dRendererCreateRenderPass();
	_vk2dRendererCreatePipelines();
	_vk2dRendererCreateFrameBuffer();
	_vk2dRendererCreateDescriptorPool(true);
	_vk2dRendererCreateUniformBuffers(false);
	_vk2dRendererCreateSampler();
	_vk2dRendererRefreshTargets();
	_vk2dRendererCreateSynchronization();

	if (!vk2dStatusFatal())
        vk2dLog("Recreated swapchain assets...");
}


void _vk2dRendererDrawRaw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight, VK2DCameraIndex cam) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];

    // Account for various coordinate-based qualms
    originX *= -xscale;
    originY *= yscale;
    //originX -= xInTex;
    //originY -= yInTex;

    // Push constants
    VK2DPushBuffer push = {0};
    identityMatrix(push.model);
    // Only do rotation matrices if a rotation is specified for optimization purposes
    if (rot != 0) {
        vec3 axis = {0, 0, 1};
        vec3 origin = {-originX + x, originY + y, 0};
        vec3 originTranslation = {originX, -originY, 0};
        translateMatrix(push.model, origin);
        rotateMatrix(push.model, axis, rot);
        translateMatrix(push.model, originTranslation);
    } else {
        vec3 origin = {x, y, 0};
        translateMatrix(push.model, origin);
    }
    // Only scale matrix if specified for optimization purposes
    if (xscale != 1 || yscale != 1) {
        vec3 scale = {xscale, yscale, 1};
        scaleMatrix(push.model, scale);
    }
    push.colourMod[0] = gRenderer->colourBlend[0];
    push.colourMod[1] = gRenderer->colourBlend[1];
    push.colourMod[2] = gRenderer->colourBlend[2];
    push.colourMod[3] = gRenderer->colourBlend[3];
    push.cameraIndex = cam;

    // Check if we actually need to bind things
    uint64_t hash = _vk2dHashSets(sets, setCount);
    if (gRenderer->prevPipe != vk2dPipelineGetPipe(pipe, gRenderer->blendMode)) {
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk2dPipelineGetPipe(pipe, gRenderer->blendMode));
        gRenderer->prevPipe = vk2dPipelineGetPipe(pipe, gRenderer->blendMode);
    }
    if (gRenderer->prevSetHash != hash) {
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, setCount, sets, 0, VK_NULL_HANDLE);
        gRenderer->prevSetHash = hash;
    }
    if (poly != NULL && gRenderer->prevVBO != poly->vertices->buf) {
        VkDeviceSize offsets[] = {poly->vertices->offset};
        vkCmdBindVertexBuffers(buf, 0, 1, &poly->vertices->buf, offsets);
        gRenderer->prevVBO = poly->vertices->buf;
    }

    // Dynamic state that can't be optimized further and the draw call
    cam = cam == VK2D_INVALID_CAMERA ? VK2D_DEFAULT_CAMERA : cam; // Account for invalid camera
    VkRect2D scissor;
    VkViewport viewport;
    if (gRenderer->target == NULL) {
        viewport.x = gRenderer->cameras[cam].spec.xOnScreen;
        viewport.y = gRenderer->cameras[cam].spec.yOnScreen;
        viewport.width = gRenderer->cameras[cam].spec.wOnScreen;
        viewport.height = gRenderer->cameras[cam].spec.hOnScreen;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->cameras[cam].spec.wOnScreen;
        scissor.extent.height = gRenderer->cameras[cam].spec.hOnScreen;
        scissor.offset.x = gRenderer->cameras[cam].spec.xOnScreen;
        scissor.offset.y = gRenderer->cameras[cam].spec.yOnScreen;
    } else {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = gRenderer->target->img->width;
        viewport.height = gRenderer->target->img->height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->target->img->width;
        scissor.extent.height = gRenderer->target->img->height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
    }
    vkCmdSetViewport(buf, 0, 1, &viewport);
    vkCmdSetScissor(buf, 0, 1, &scissor);
    if (gRenderer->limits.maxLineWidth != 1)
        vkCmdSetLineWidth(buf, lineWidth);
    else
        vkCmdSetLineWidth(buf, 1);
    vkCmdPushConstants(buf, pipe->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK2DPushBuffer), &push);
    if (poly != NULL)
        vkCmdDraw(buf, poly->vertexCount, 1, 0, 0);
    else // The only time this would be the case is for textures, where the shader provides the vertices
        vkCmdDraw(buf, 6, 1, 0, 0);
}

void _vk2dRendererDrawRawShader(VkDescriptorSet *sets, uint32_t setCount, VK2DTexture tex, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight, VK2DCameraIndex cam) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];

    // Account for various coordinate-based qualms
    originX *= -xscale;
    originY *= yscale;
    //originX -= xInTex;
    //originY -= yInTex;

    // Push constants
    VK2DShaderPushBuffer push = {0};
    identityMatrix(push.model);
    // Only do rotation matrices if a rotation is specified for optimization purposes
    if (rot != 0) {
        vec3 axis = {0, 0, 1};
        vec3 origin = {-originX + x, originY + y, 0};
        vec3 originTranslation = {originX, -originY, 0};
        translateMatrix(push.model, origin);
        rotateMatrix(push.model, axis, rot);
        translateMatrix(push.model, originTranslation);
    } else {
        vec3 origin = {x, y, 0};
        translateMatrix(push.model, origin);
    }
    // Only scale matrix if specified for optimization purposes
    if (xscale != 1 || yscale != 1) {
        vec3 scale = {xscale, yscale, 1};
        scaleMatrix(push.model, scale);
    }
    push.colour[0] = gRenderer->colourBlend[0];
    push.colour[1] = gRenderer->colourBlend[1];
    push.colour[2] = gRenderer->colourBlend[2];
    push.colour[3] = gRenderer->colourBlend[3];
    push.cameraIndex = cam == VK2D_INVALID_CAMERA ? 0 : cam;
    push.textureIndex = SDL_GetAtomicInt(&tex->descriptorIndex);
    push.texturePos[0] = xInTex;
    push.texturePos[1] = yInTex;
    push.texturePos[2] = texWidth;
    push.texturePos[3] = texHeight;

    // Check if we actually need to bind things
    uint64_t hash = _vk2dHashSets(sets, setCount);
    if (gRenderer->prevPipe != vk2dPipelineGetPipe(pipe, gRenderer->blendMode)) {
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk2dPipelineGetPipe(pipe, gRenderer->blendMode));
        gRenderer->prevPipe = vk2dPipelineGetPipe(pipe, gRenderer->blendMode);
    }
    if (gRenderer->prevSetHash != hash) {
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, setCount, sets, 0, VK_NULL_HANDLE);
        gRenderer->prevSetHash = hash;
    }

    // Dynamic state that can't be optimized further and the draw call
    cam = cam == VK2D_INVALID_CAMERA ? VK2D_DEFAULT_CAMERA : cam; // Account for invalid camera
    VkRect2D scissor;
    VkViewport viewport;
    if (gRenderer->target == NULL) {
        viewport.x = gRenderer->cameras[cam].spec.xOnScreen;
        viewport.y = gRenderer->cameras[cam].spec.yOnScreen;
        viewport.width = gRenderer->cameras[cam].spec.wOnScreen;
        viewport.height = gRenderer->cameras[cam].spec.hOnScreen;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->cameras[cam].spec.wOnScreen;
        scissor.extent.height = gRenderer->cameras[cam].spec.hOnScreen;
        scissor.offset.x = gRenderer->cameras[cam].spec.xOnScreen;
        scissor.offset.y = gRenderer->cameras[cam].spec.yOnScreen;
    } else {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = gRenderer->target->img->width;
        viewport.height = gRenderer->target->img->height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->target->img->width;
        scissor.extent.height = gRenderer->target->img->height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
    }
    vkCmdSetViewport(buf, 0, 1, &viewport);
    vkCmdSetScissor(buf, 0, 1, &scissor);
    if (gRenderer->limits.maxLineWidth != 1)
        vkCmdSetLineWidth(buf, lineWidth);
    else
        vkCmdSetLineWidth(buf, 1);
    vkCmdPushConstants(buf, pipe->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK2DShaderPushBuffer), &push);
    vkCmdDraw(buf, 6, 1, 0, 0);
}

void _vk2dRendererDrawRawShadows(VkDescriptorSet set, VK2DShadowEnvironment shadowEnvironment, VK2DShadowObject object, vec4 colour, vec2 lightSource, VK2DCameraIndex cam) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];
    VK2DPipeline pipe = gRenderer->shadowsPipe;
    VK2DShadowObjectInfo *objInfo = &shadowEnvironment->objectInfos[object];

    // Push constants
    VK2DShadowsPushBuffer push = {0};
    push.lightSource[0] = lightSource[0];
    push.lightSource[1] = lightSource[1];
    push.colour[0] = colour[0];
    push.colour[1] = colour[1];
    push.colour[2] = colour[2];
    push.colour[3] = colour[3];
    push.cameraIndex = cam == VK2D_INVALID_CAMERA ? 0 : cam;
    memcpy(push.model, objInfo->model, sizeof(mat4));
    // Check if we actually need to bind things
    if (gRenderer->prevPipe != vk2dPipelineGetPipe(pipe, gRenderer->blendMode)) {
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk2dPipelineGetPipe(pipe, gRenderer->blendMode));
        gRenderer->prevPipe = vk2dPipelineGetPipe(pipe, gRenderer->blendMode);
    }
    gRenderer->prevSetHash = 0;
    VkDeviceSize offsets = shadowEnvironment->vbo->offset;
    vkCmdBindVertexBuffers(buf, 0, 1, &shadowEnvironment->vbo->buf, &offsets);
    gRenderer->prevVBO = NULL;
    vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gRenderer->shadowsPipe->layout, 0, 1, &set, 0, VK_NULL_HANDLE);

    // Dynamic state that can't be optimized further and the draw call
    cam = cam == VK2D_INVALID_CAMERA ? VK2D_DEFAULT_CAMERA : cam; // Account for invalid camera
    VkRect2D scissor;
    VkViewport viewport;
    if (gRenderer->target == NULL) {
        viewport.x = gRenderer->cameras[cam].spec.xOnScreen;
        viewport.y = gRenderer->cameras[cam].spec.yOnScreen;
        viewport.width = gRenderer->cameras[cam].spec.wOnScreen;
        viewport.height = gRenderer->cameras[cam].spec.hOnScreen;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->cameras[cam].spec.wOnScreen;
        scissor.extent.height = gRenderer->cameras[cam].spec.hOnScreen;
        scissor.offset.x = gRenderer->cameras[cam].spec.xOnScreen;
        scissor.offset.y = gRenderer->cameras[cam].spec.yOnScreen;
    } else {
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = gRenderer->target->img->width;
        viewport.height = gRenderer->target->img->height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        scissor.extent.width = gRenderer->target->img->width;
        scissor.extent.height = gRenderer->target->img->height;
        scissor.offset.x = 0;
        scissor.offset.y = 0;
    }
    vkCmdSetViewport(buf, 0, 1, &viewport);
    vkCmdSetScissor(buf, 0, 1, &scissor);
    vkCmdPushConstants(buf, pipe->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK2DShadowsPushBuffer), &push);
    vkCmdDraw(buf, objInfo->vertexCount, 1, objInfo->startingVertex, 0);
}

void _vk2dRendererDrawRawInstanced(VkDescriptorSet *sets, uint32_t setCount, VK2DDrawInstance *instances, int count, VK2DCameraIndex cam) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];

	// We don't do any binding saving for instanced drawing
	_vk2dRendererResetBoundPointers();
	vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk2dPipelineGetPipe(gRenderer->instancedPipe, gRenderer->blendMode));
	vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, gRenderer->instancedPipe->layout, 0, setCount, sets, 0, VK_NULL_HANDLE);

	// Dynamic state that can't be optimized further and the draw call
	cam = cam == VK2D_INVALID_CAMERA ? VK2D_DEFAULT_CAMERA : cam; // Account for invalid camera
	VkRect2D scissor;
	VkViewport viewport;
	if (gRenderer->target == NULL) {
		viewport.x = gRenderer->cameras[cam].spec.xOnScreen;
		viewport.y = gRenderer->cameras[cam].spec.yOnScreen;
		viewport.width = gRenderer->cameras[cam].spec.wOnScreen;
		viewport.height = gRenderer->cameras[cam].spec.hOnScreen;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		scissor.extent.width = gRenderer->cameras[cam].spec.wOnScreen;
		scissor.extent.height = gRenderer->cameras[cam].spec.hOnScreen;
		scissor.offset.x = gRenderer->cameras[cam].spec.xOnScreen;
		scissor.offset.y = gRenderer->cameras[cam].spec.yOnScreen;
	} else {
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = gRenderer->target->img->width;
		viewport.height = gRenderer->target->img->height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		scissor.extent.width = gRenderer->target->img->width;
		scissor.extent.height = gRenderer->target->img->height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
	}
	// Make vertex buffer
	VkBuffer buffer;
	VkDeviceSize offset;
	vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[gRenderer->currentFrame], instances, count * sizeof(VK2DDrawInstance), &buffer, &offset);

	vkCmdBindVertexBuffers(buf, 0, 1, &buffer, &offset);
	vkCmdSetViewport(buf, 0, 1, &viewport);
	vkCmdSetScissor(buf, 0, 1, &scissor);
	vkCmdSetLineWidth(buf, 1);
	vkCmdDraw(buf, 6, count, 0, 0);
}

// Same as above but for 3D rendering
void _vk2dRendererDrawRaw3D(VkDescriptorSet *sets, uint32_t setCount, VK2DModel model, VK2DPipeline pipe, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ, VK2DCameraIndex cam, float lineWidth) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];

	// Account for various coordinate-based qualms
	originX *= xscale;
	originY *= yscale;
	originZ *= zscale;
	//originX -= xInTex;
	//originY -= yInTex;
	rot *= -1;

	// Push constants
	VK2D3DPushBuffer push = {0};
	identityMatrix(push.model);
	vec3 originTranslation = {-originX, -originY, -originZ};
	vec3 origin2 = {originX + x, originY + y, originZ + z};
	vec3 scale = {xscale, yscale, zscale};
	translateMatrix(push.model, origin2);
	rotateMatrix(push.model, axis, rot);
	translateMatrix(push.model, originTranslation);
	scaleMatrix(push.model, scale);
	push.colourMod[0] = gRenderer->colourBlend[0];
	push.colourMod[1] = gRenderer->colourBlend[1];
	push.colourMod[2] = gRenderer->colourBlend[2];
	push.colourMod[3] = gRenderer->colourBlend[3];
	push.cameraIndex = cam;
	push.textureIndex = SDL_GetAtomicInt(&model->tex->descriptorIndex);

	// Check if we actually need to bind things
	uint64_t hash = _vk2dHashSets(sets, setCount);
	if (gRenderer->prevPipe != vk2dPipelineGetPipe(pipe, gRenderer->blendMode)) {
		vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk2dPipelineGetPipe(pipe, gRenderer->blendMode));
		gRenderer->prevPipe = vk2dPipelineGetPipe(pipe, gRenderer->blendMode);
	}
	if (gRenderer->prevSetHash != hash) {
		vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->layout, 0, setCount, sets, 0, VK_NULL_HANDLE);
		gRenderer->prevSetHash = hash;
	}
	VkDeviceSize offsets[] = {model->vertexOffset};
	vkCmdBindVertexBuffers(buf, 0, 1, &model->vertices->buf, offsets);
	gRenderer->prevVBO = model->vertices->buf;
	vkCmdBindIndexBuffer(buf, model->vertices->buf, model->indexOffset, VK_INDEX_TYPE_UINT16);

	// Dynamic state that can't be optimized further and the draw call
	cam = cam == VK2D_INVALID_CAMERA ? VK2D_DEFAULT_CAMERA : cam; // Account for invalid camera
	VkRect2D scissor;
	VkViewport viewport;
	if (gRenderer->target == NULL) {
		viewport.x = gRenderer->cameras[cam].spec.xOnScreen;
		viewport.y = gRenderer->cameras[cam].spec.yOnScreen;
		viewport.width = gRenderer->cameras[cam].spec.wOnScreen;
		viewport.height = gRenderer->cameras[cam].spec.hOnScreen;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		scissor.extent.width = gRenderer->cameras[cam].spec.wOnScreen;
		scissor.extent.height = gRenderer->cameras[cam].spec.hOnScreen;
		scissor.offset.x = gRenderer->cameras[cam].spec.xOnScreen;
		scissor.offset.y = gRenderer->cameras[cam].spec.yOnScreen;
	} else {
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = gRenderer->target->img->width;
		viewport.height = gRenderer->target->img->height;
		viewport.minDepth = 0;
		viewport.maxDepth = 1;
		scissor.extent.width = gRenderer->target->img->width;
		scissor.extent.height = gRenderer->target->img->height;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
	}
	vkCmdSetViewport(buf, 0, 1, &viewport);
	vkCmdSetScissor(buf, 0, 1, &scissor);
	if (gRenderer->limits.maxLineWidth != 1)
		vkCmdSetLineWidth(buf, lineWidth);
	else
		vkCmdSetLineWidth(buf, 1);
	vkCmdPushConstants(buf, pipe->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK2D3DPushBuffer), &push);
	vkCmdDrawIndexed(buf, model->indexCount, 1, 0, 0, 0);
}

// Same as _vk2dRendererDraw below but specifically for 3D rendering
void _vk2dRendererDraw3D(VkDescriptorSet *sets, uint32_t setCount, VK2DModel model, VK2DPipeline pipe, float x, float y, float z, float xscale, float yscale, float zscale, float rot, vec3 axis, float originX, float originY, float originZ, float lineWidth) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
	// Only render to 3D cameras
	for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
		if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type != VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
			sets[0] = gRenderer->uboDescriptorSets[gRenderer->currentFrame];
			_vk2dRendererDrawRaw3D(sets, setCount, model, pipe, x, y, z, xscale, yscale, zscale, rot, axis, originX, originY, originZ, i, lineWidth);
		}
	}
}

// This is the upper level internal draw function that draws to each camera and not just with a scissor/viewport
void _vk2dRendererDraw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO) {
        sets[0] = gRenderer->targetUBOSet;
        _vk2dRendererDrawRaw(sets, setCount, poly, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, 0);
    } else {
        // Only render to 2D cameras
        for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
            if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type == VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
                sets[0] = gRenderer->uboDescriptorSets[gRenderer->currentFrame];
                _vk2dRendererDrawRaw(sets, setCount, poly, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, i);
            }
        }
    }
}

void _vk2dRendererDrawShader(VkDescriptorSet *sets, uint32_t setCount, VK2DTexture tex, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO) {
        sets[0] = gRenderer->targetUBOSet;
        _vk2dRendererDrawRawShader(sets, setCount, tex, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, VK2D_INVALID_CAMERA);
    } else {
        // Only render to 2D cameras
        for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
            if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type == VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
                sets[0] = gRenderer->uboDescriptorSets[gRenderer->currentFrame];
                _vk2dRendererDrawRawShader(sets, setCount, tex, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, i);
            }
        }
    }
}

// This is the upper level internal draw function for shadows that draws to each camera and not just with a scissor/viewport
void _vk2dRendererDrawShadows(VK2DShadowEnvironment shadowEnvironment, vec4 colour, vec2 lightSource) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    VkDescriptorSet set;
    if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO) {
        set = gRenderer->targetUBOSet;
        for (int so = 0; so < shadowEnvironment->objectCount; so++) {
            if (shadowEnvironment->objectInfos[so].enabled)
                _vk2dRendererDrawRawShadows(set, shadowEnvironment, so, colour, lightSource, VK2D_INVALID_CAMERA);
        }
    } else {
        // Only render to 2D cameras
        for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
            if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type == VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
                set = gRenderer->uboDescriptorSets[gRenderer->currentFrame];
                // Iterate through each shadow object
                for (int so = 0; so < shadowEnvironment->objectCount; so++) {
                    if (shadowEnvironment->objectInfos[so].enabled)
                        _vk2dRendererDrawRawShadows(set, shadowEnvironment, so, colour, lightSource, i);
                }
            }
        }
    }
}

static void _vk2dRendererAddDrawCommandInternal(VK2DDrawCommand *command) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    memcpy(&gRenderer->drawCommands[gRenderer->drawCommandCount++], command, sizeof(struct VK2DDrawCommand));
}

void _vk2dRendererAddDrawCommand(VK2DDrawCommand *command) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dStatusFatal())
        return;
    _vk2dRendererAddDrawCommandInternal(command);
}

void _vk2dRendererResetBatch() {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    gRenderer->currentBatchPipeline = NULL;
    gRenderer->currentBatchPipelineID = VK2D_PIPELINE_ID_NONE;
    gRenderer->drawCommandCount = 0;
}

void _vk2dRendererFlushBatchIfNeeded(VK2DPipeline pipe) {
    VK2DRenderer gRenderer = vk2dRendererGetPointer();
    if (vk2dPipelineGetID(pipe, gRenderer->blendMode) != gRenderer->currentBatchPipelineID || gRenderer->drawCommandCount >= gRenderer->limits.maxInstancedDraws) {
        vk2dRendererFlushSpriteBatch();
        gRenderer->currentBatchPipelineID = vk2dPipelineGetID(pipe, 0);
        gRenderer->currentBatchPipeline = pipe;
    }
}
