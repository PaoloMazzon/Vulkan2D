/// \file RendererMeta.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>

#include "VK2D/RendererMeta.h"
#include "VK2D/Renderer.h"
#include "VK2D/Validation.h"
#include "VK2D/Initializers.h"
#include "VK2D/Constants.h"
#include "VK2D/PhysicalDevice.h"
#include "VK2D/LogicalDevice.h"
#include "VK2D/Image.h"
#include "VK2D/Texture.h"
#include "VK2D/Pipeline.h"
#include "VK2D/Blobs.h"
#include "VK2D/Buffer.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Polygon.h"
#include "VK2D/Math.h"
#include "VK2D/Shader.h"
#include "VK2D/Util.h"
#include "VK2D/Model.h"
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
void _vk2dRendererAddShader(VK2DShader shader) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	uint32_t found = UINT32_MAX;
	for (i = 0; i < gRenderer->shaderListSize && found == UINT32_MAX; i++)
		if (gRenderer->customShaders[i] == NULL)
			found = i;

	// Make some room in the shader list
	if (found == UINT32_MAX) {
		VK2DShader *newList = realloc(gRenderer->customShaders, sizeof(VK2DShader) * (gRenderer->shaderListSize + VK2D_DEFAULT_ARRAY_EXTENSION));
		if (vk2dPointerCheck(newList)) {
			found = gRenderer->shaderListSize;
			gRenderer->customShaders = newList;

			// Set the new slots to null
			for (i = gRenderer->shaderListSize; i < gRenderer->shaderListSize + VK2D_DEFAULT_ARRAY_EXTENSION; i++) {
				gRenderer->customShaders[i] = NULL;
			}

			gRenderer->shaderListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		}
	}

	gRenderer->customShaders[found] = shader;
}

void _vk2dRendererRemoveShader(VK2DShader shader) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
	gRenderer->prevPipe = VK_NULL_HANDLE;
	gRenderer->prevSetHash = 0;
	gRenderer->prevVBO = VK_NULL_HANDLE;
}

// This is called when a render-target texture is created to make the renderer aware of it
void _vk2dRendererAddTarget(VK2DTexture tex) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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

		if (vk2dPointerCheck(newList)) {
			gRenderer->targets = newList;
			gRenderer->targets[gRenderer->targetListSize] = tex;

			// Fill the newly allocated parts of the list with NULL
			for (i = gRenderer->targetListSize + 1; i < gRenderer->targetListSize + VK2D_DEFAULT_ARRAY_EXTENSION; i++)
				gRenderer->targets[i] = NULL;

			gRenderer->targetListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		}
	}
}

// Called when a render-target texture is destroyed so the renderer can remove it from its list
void _vk2dRendererRemoveTarget(VK2DTexture tex) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	for (i = 0; i < gRenderer->targetListSize; i++)
		if (gRenderer->targets[i] == tex)
			gRenderer->targets[i] = NULL;
}

// This is used when changing the render target to make sure the texture is either ready to be drawn itself or rendered to
void _vk2dTransitionImageLayout(VkImage img, VkImageLayout old, VkImageLayout new) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
		vk2dLogMessage("!!!Unsupported image transition!!!");
		vk2dErrorCheck(-1);
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
void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera) {

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
		memset(ubo->viewproj, 0, 64);
		multiplyMatrix(view, proj, ubo->viewproj);
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
		memset(ubo->viewproj, 0, 64);
		multiplyMatrix(view, proj, ubo->viewproj);
	}
}

// Flushes the data from a ubo to its respective buffer, frame being the swapchain buffer to flush
void _vk2dRendererFlushUBOBuffer(uint32_t frame, uint32_t descriptorFrame, int camera) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VkBuffer buffer;
	VkDeviceSize offset;
	vk2dDescriptorBufferCopyData(gRenderer->descriptorBuffers[descriptorFrame], &gRenderer->cameras[camera].ubos[frame], sizeof(VK2DUniformBufferObject), &buffer, &offset);
	VkDescriptorBufferInfo bufferInfo = {buffer, offset, sizeof(VK2DUniformBufferObject)};
	VkWriteDescriptorSet write = {0};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.descriptorCount = 1;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.pBufferInfo = &bufferInfo;
	write.dstSet = gRenderer->cameras[camera].uboSets[frame];
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);
}

void _vk2dRendererCreateDebug() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->options.enableDebug) {
		fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr(gRenderer->vk,
																									 "vkCreateDebugReportCallbackEXT");
		fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(gRenderer->vk,
																									   "vkDestroyDebugReportCallbackEXT");

		if (fvkCreateDebugReportCallbackEXT != NULL && fvkDestroyDebugReportCallbackEXT != NULL) {
			VkDebugReportCallbackCreateInfoEXT callbackCreateInfoEXT = vk2dInitDebugReportCallbackCreateInfoEXT(
					_vk2dDebugCallback);
			fvkCreateDebugReportCallbackEXT(gRenderer->vk, &callbackCreateInfoEXT, VK_NULL_HANDLE, &gRenderer->dr);
		} else {
			vk2dErrorCheck(-1)
		}
	}
}

void _vk2dRendererDestroyDebug() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->options.enableDebug) {
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
	vk2dErrorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities));
	if (gRenderer->surfaceCapabilities.currentExtent.width == UINT32_MAX || gRenderer->surfaceCapabilities.currentExtent.height == UINT32_MAX) {
		SDL_Vulkan_GetDrawableSize(gRenderer->window, (void*)&gRenderer->surfaceWidth, (void*)&gRenderer->surfaceHeight);
	} else {
		gRenderer->surfaceWidth = gRenderer->surfaceCapabilities.currentExtent.width;
		gRenderer->surfaceHeight = gRenderer->surfaceCapabilities.currentExtent.height;
	}
}

void _vk2dRendererCreateWindowSurface() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// Create the surface then load up surface relevant values
	vk2dErrorCheck(SDL_Vulkan_CreateSurface(gRenderer->window, gRenderer->vk, &gRenderer->surface) == SDL_TRUE ? VK_SUCCESS : -1);
	vk2dErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, VK_NULL_HANDLE));
	gRenderer->presentModes = malloc(sizeof(VkPresentModeKHR) * gRenderer->presentModeCount);

	if (vk2dPointerCheck(gRenderer->presentModes)) {
		vk2dErrorCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->presentModeCount, gRenderer->presentModes));
		vk2dErrorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities));
		// You may want to search for a different format, but according to the Vulkan hardware database, 100% of systems support VK_FORMAT_B8G8R8A8_SRGB
		gRenderer->surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
		gRenderer->surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		_vk2dRendererGetSurfaceSize();
	}
}

void _vk2dRendererDestroyWindowSurface() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroySurfaceKHR(gRenderer->vk, gRenderer->surface, VK_NULL_HANDLE);
	free(gRenderer->presentModes);
}

void _vk2dRendererCreateSwapchain() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
	if (vk2dErrorInline(supported != VK_TRUE ? -1 : VK_SUCCESS))
		vkCreateSwapchainKHR(gRenderer->ld->dev, &swapchainCreateInfoKHR, VK_NULL_HANDLE, &gRenderer->swapchain);

	vk2dErrorCheck(vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, VK_NULL_HANDLE));
	gRenderer->swapchainImageViews = malloc(gRenderer->swapchainImageCount * sizeof(VkImageView));
	gRenderer->swapchainImages = malloc(gRenderer->swapchainImageCount * sizeof(VkImage));
	if (vk2dPointerCheck(gRenderer->swapchainImageViews) && vk2dPointerCheck(gRenderer->swapchainImages)) {
		vk2dErrorCheck(vkGetSwapchainImagesKHR(gRenderer->ld->dev, gRenderer->swapchain, &gRenderer->swapchainImageCount, gRenderer->swapchainImages));

		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			VkImageViewCreateInfo imageViewCreateInfo = vk2dInitImageViewCreateInfo(gRenderer->swapchainImages[i], gRenderer->surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			vk2dErrorCheck(vkCreateImageView(gRenderer->ld->dev, &imageViewCreateInfo, VK_NULL_HANDLE, &gRenderer->swapchainImageViews[i]));
		}
	}

	vk2dLogMessage("Swapchain (%i images) initialized...", swapchainCreateInfoKHR.minImageCount);
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
		vk2dLogMessage("Depth buffer initialized...");
	} else {
		vk2dLogMessage("Failed to create depth buffer.");
	}
}

void _vk2dRendererDestroyDepthBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dImageFree(gRenderer->depthBuffer);
	gRenderer->depthBuffer = NULL;
}


void _vk2dRendererCreateColourResources() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->config.msaa != VK2D_MSAA_1X) {
		gRenderer->msaaImage = vk2dImageCreate(
				gRenderer->ld,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				gRenderer->surfaceFormat.format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				(VkSampleCountFlagBits) gRenderer->config.msaa);
		vk2dLogMessage("Colour resources initialized for MSAA of %i...", gRenderer->config.msaa);
	} else {
		vk2dLogMessage("Colour resources not enabled...");
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
	gRenderer->descriptorBuffers = malloc(sizeof(VK2DDescriptorBuffer) * VK2D_MAX_FRAMES_IN_FLIGHT);

	vk2dPointerCheck(gRenderer->descriptorBuffers);
	for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
		gRenderer->descriptorBuffers[i] = vk2dDescriptorBufferCreate();
	}

	// Calculate max instances
	gRenderer->limits.maxInstancedDraws = gRenderer->options.vramPageSize / sizeof(VK2DDrawInstance);
	gRenderer->limits.maxInstancedDraws--;

	vk2dLogMessage("Descriptor buffers created...");
}

void _vk2dRendererDestroyDescriptorBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	for (int i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++)
		vk2dDescriptorBufferFree(gRenderer->descriptorBuffers[i]);
	free(gRenderer->descriptorBuffers);
}


void _vk2dRendererCreateRenderPass() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t attachCount;
	if (gRenderer->config.msaa != 1) {
		attachCount = 3; // colour, depth, resolve
	} else {
		attachCount = 2; // colour, depth
	}
	VkAttachmentReference resolveAttachment;
	VkAttachmentDescription attachments[attachCount];
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
	VkAttachmentReference subpassColourAttachments0[colourAttachCount];
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
	vk2dErrorCheck(vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->renderPass));

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
	vk2dErrorCheck(vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->midFrameSwapRenderPass));

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
	vk2dErrorCheck(vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->externalTargetRenderPass));

	vk2dLogMessage("Render pass initialized...");
}

void _vk2dRendererDestroyRenderPass() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->renderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->externalTargetRenderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->midFrameSwapRenderPass, VK_NULL_HANDLE);
}

void _vk2dRendererCreateDescriptorSetLayouts() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// For texture samplers
	const uint32_t layoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[layoutCount];
	descriptorSetLayoutBinding[0] = vk2dInitDescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBinding, layoutCount);
	vk2dErrorCheck(vkCreateDescriptorSetLayout(gRenderer->ld->dev, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslSampler));

	// For view projection buffers
	const uint32_t shapeLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingShapes[shapeLayoutCount];
	descriptorSetLayoutBindingShapes[0] = vk2dInitDescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo shapesDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingShapes, shapeLayoutCount);
	vk2dErrorCheck(vkCreateDescriptorSetLayout(gRenderer->ld->dev, &shapesDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslBufferVP));

	// For user-created shaders
	const uint32_t userLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingUser[userLayoutCount];
	descriptorSetLayoutBindingUser[0] = vk2dInitDescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo userDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingUser, userLayoutCount);
	vk2dErrorCheck(vkCreateDescriptorSetLayout(gRenderer->ld->dev, &userDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslBufferUser));

	// For sampled textures
	const uint32_t texLayoutCount = 1;
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindingTex[texLayoutCount];
	descriptorSetLayoutBindingTex[0] = vk2dInitDescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE);
	VkDescriptorSetLayoutCreateInfo texDescriptorSetLayoutCreateInfo = vk2dInitDescriptorSetLayoutCreateInfo(descriptorSetLayoutBindingTex, texLayoutCount);
	vk2dErrorCheck(vkCreateDescriptorSetLayout(gRenderer->ld->dev, &texDescriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &gRenderer->dslTexture));

	vk2dLogMessage("Descriptor set layout initialized...");
}

void _vk2dRendererDestroyDescriptorSetLayout() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslSampler, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferVP, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferUser, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslTexture, VK_NULL_HANDLE);
}

VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState();
VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState();
void _vk2dShaderBuildPipe(VK2DShader shader);
void _vk2dRendererCreatePipelines() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	VkPipelineVertexInputStateCreateInfo textureVertexInfo = _vk2dGetTextureVertexInputState();
	VkPipelineVertexInputStateCreateInfo colourVertexInfo = _vk2dGetColourVertexInputState();
	VkPipelineVertexInputStateCreateInfo modelVertexInfo = _vk2dGetModelVertexInputState();
	VkPipelineVertexInputStateCreateInfo instanceVertexInfo = _vk2dGetInstanceVertexInputState();
    VkPipelineVertexInputStateCreateInfo shadowsVertexInfo = _vk2dGetShadowsVertexInputState();

	// Default shader files
	uint32_t shaderTexVertSize = sizeof(VK2DVertTex);
	bool CustomTexVertShader = false;
	unsigned char *shaderTexVert = (void*)VK2DVertTex;
	uint32_t shaderTexFragSize = sizeof(VK2DFragTex);
	bool CustomTexFragShader = false;
	unsigned char *shaderTexFrag = (void*)VK2DFragTex;
	uint32_t shaderColourVertSize = sizeof(VK2DVertColour);
	bool CustomColourVertShader = false;
	unsigned char *shaderColourVert = (void*)VK2DVertColour;
	uint32_t shaderColourFragSize = sizeof(VK2DFragColour);
	bool CustomColourFragShader = false;
	unsigned char *shaderColourFrag = (void*)VK2DFragColour;
	uint32_t shaderModelVertSize = sizeof(VK2DVertModel);
	bool CustomModelVertShader = false;
	unsigned char *shaderModelVert = (void*)VK2DVertModel;
	uint32_t shaderModelFragSize = sizeof(VK2DFragModel);
	bool CustomModelFragShader = false;
	unsigned char *shaderModelFrag = (void*)VK2DFragModel;
    uint32_t shaderInstancedVertSize = sizeof(VK2DVertInstanced);
    bool CustomInstancedVertShader = false;
    unsigned char *shaderInstancedVert = (void*)VK2DVertInstanced;
    uint32_t shaderInstancedFragSize = sizeof(VK2DFragInstanced);
    bool CustomInstancedFragShader = false;
    unsigned char *shaderInstancedFrag = (void*)VK2DFragInstanced;
    uint32_t shaderShadowsVertSize = sizeof(VK2DVertShadows);
    bool CustomShadowsVertShader = false;
    unsigned char *shaderShadowsVert = (void*)VK2DVertShadows;
    uint32_t shaderShadowsFragSize = sizeof(VK2DFragShadows);
    bool CustomShadowsFragShader = false;
    unsigned char *shaderShadowsFrag = (void*)VK2DFragShadows;

    // Potentially load some different ones
	if (gRenderer->options.loadCustomShaders) {
		if (_vk2dFileExists("shaders/texvert.spv")) {
			CustomTexVertShader = true;
			shaderTexVert = _vk2dLoadFile("shaders/texvert.spv", &shaderTexVertSize);
		}
		if (_vk2dFileExists("shaders/texfrag.spv")) {
			CustomTexFragShader = true;
			shaderTexFrag = _vk2dLoadFile("shaders/texfrag.spv", &shaderTexFragSize);
		}
		if (_vk2dFileExists("shaders/colourvert.spv")) {
			CustomColourVertShader = true;
			shaderColourVert = _vk2dLoadFile("shaders/colourvert.spv", &shaderColourVertSize);
		}
		if (_vk2dFileExists("shaders/colourfrag.spv")) {
			CustomColourFragShader = true;
			shaderColourFrag = _vk2dLoadFile("shaders/colourfrag.spv", &shaderColourFragSize);
		}
		if (_vk2dFileExists("shaders/modelvert.spv")) {
			CustomModelVertShader = true;
			shaderModelVert = _vk2dLoadFile("shaders/modelvert.spv", &shaderModelVertSize);
		}
		if (_vk2dFileExists("shaders/modelfrag.spv")) {
			CustomModelFragShader = true;
			shaderModelFrag = _vk2dLoadFile("shaders/modelfrag.spv", &shaderModelFragSize);
		}
        if (_vk2dFileExists("shaders/instancedvert.spv")) {
            CustomInstancedVertShader = true;
            shaderInstancedVert = _vk2dLoadFile("shaders/instancedvert.spv", &shaderInstancedVertSize);
        }
        if (_vk2dFileExists("shaders/instancedfrag.spv")) {
            CustomInstancedFragShader = true;
            shaderInstancedFrag = _vk2dLoadFile("shaders/instancedfrag.spv", &shaderInstancedFragSize);
        }
        if (_vk2dFileExists("shaders/shadowsvert.spv")) {
            CustomShadowsVertShader = true;
            shaderShadowsVert = _vk2dLoadFile("shaders/shadowsvert.spv", &shaderInstancedVertSize);
        }
        if (_vk2dFileExists("shaders/shadowsfrag.spv")) {
            CustomShadowsFragShader = true;
            shaderShadowsFrag = _vk2dLoadFile("shaders/shadowsfrag.spv", &shaderInstancedFragSize);
        }
	}

	// Texture pipeline
	VkDescriptorSetLayout layout[] = {gRenderer->dslBufferVP, gRenderer->dslSampler, gRenderer->dslTexture};
	gRenderer->texPipe = vk2dPipelineCreate(
			gRenderer->ld,
			gRenderer->renderPass,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			shaderTexVert,
			shaderTexVertSize,
			shaderTexFrag,
			shaderTexFragSize,
			layout,
			3,
			&textureVertexInfo,
			true,
			gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_DEFAULT);

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
			layout,
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
			layout,
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
			layout,
			3,
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
            layout,
            1,
            &instanceVertexInfo,
            true,
            gRenderer->config.msaa,
            VK2D_PIPELINE_TYPE_SHADOWS);

	// Shader pipelines
	for (i = 0; i < gRenderer->shaderListSize; i++) {
		if (gRenderer->customShaders[i] != NULL) {
			vk2dPipelineFree(gRenderer->customShaders[i]->pipe);
			_vk2dShaderBuildPipe(gRenderer->customShaders[i]);
		}
	}

	// Free custom shaders
	if (CustomTexVertShader)
		free(shaderTexVert);
	if (CustomTexFragShader)
		free(shaderTexFrag);
	if (CustomColourVertShader)
		free(shaderColourVert);
	if (CustomColourFragShader)
		free(shaderColourFrag);
	if (CustomModelVertShader)
		free(shaderModelVert);
	if (CustomModelFragShader)
		free(shaderModelFrag);
    if (CustomInstancedVertShader)
        free(shaderInstancedVert);
    if (CustomInstancedFragShader)
        free(shaderInstancedFrag);
    if (CustomShadowsVertShader)
        free(shaderShadowsVert);
    if (CustomShadowsFragShader)
        free(shaderShadowsFrag);

    vk2dLogMessage("Pipelines initialized...");
}

void _vk2dRendererDestroyPipelines(bool preserveCustomPipes) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vk2dPipelineFree(gRenderer->primLinePipe);
	vk2dPipelineFree(gRenderer->primFillPipe);
	vk2dPipelineFree(gRenderer->texPipe);
	vk2dPipelineFree(gRenderer->modelPipe);
	vk2dPipelineFree(gRenderer->wireframePipe);
    vk2dPipelineFree(gRenderer->instancedPipe);
    vk2dPipelineFree(gRenderer->shadowsPipe);

    if (!preserveCustomPipes)
		free(gRenderer->customShaders);
}

void _vk2dRendererCreateFrameBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	gRenderer->framebuffers = malloc(sizeof(VkFramebuffer) * gRenderer->swapchainImageCount);

	if (vk2dPointerCheck(gRenderer->framebuffers)) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			// There is no 3rd attachment if msaa is disabled
			const int attachCount = gRenderer->config.msaa > 1 ? 3 : 2;
			VkImageView attachments[attachCount];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->msaaImage->view;
				attachments[1] = gRenderer->depthBuffer->view;
				attachments[2] = gRenderer->swapchainImageViews[i];
			} else {
				attachments[0] = gRenderer->swapchainImageViews[i];
				attachments[1] = gRenderer->depthBuffer->view;
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->renderPass, gRenderer->surfaceWidth, gRenderer->surfaceHeight, attachments, attachCount);
			vk2dErrorCheck(vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->framebuffers[i]));
			vk2dLogMessage("Framebuffer[%i] ready...", i);
		}
	}

	vk2dLogMessage("Framebuffers initialized...");
}

void _vk2dRendererDestroyFrameBuffer() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vkDestroyFramebuffer(gRenderer->ld->dev, gRenderer->framebuffers[i], VK_NULL_HANDLE);
	free(gRenderer->framebuffers);
}

void _vk2dRendererCreateUniformBuffers(bool newCamera) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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

	VK2DCameraSpec unitCam = {
			VK2D_CAMERA_TYPE_DEFAULT,
			0,
			0,
			1,
			1,
			1,
	};
	VK2DUniformBufferObject unitUBO = {0};
	_vk2dCameraUpdateUBO(&unitUBO, &unitCam);
	gRenderer->unitUBO = vk2dBufferLoad(gRenderer->ld, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &unitUBO, true);
	gRenderer->unitUBOSet = vk2dDescConGetBufferSet(gRenderer->descConVP, gRenderer->unitUBO);

	vk2dLogMessage("UBO initialized...");
}

void _vk2dRendererDestroyUniformBuffers() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	for (i = 0; i < VK2D_MAX_CAMERAS; i++)
		if (vk2dCameraGetState(i) == VK2D_CAMERA_STATE_NORMAL || vk2dCameraGetState(i) == VK2D_CAMERA_STATE_DISABLED)
			vk2dCameraSetState(i, VK2D_CAMERA_STATE_RESET);
	vk2dBufferFree(gRenderer->unitUBO);
}

void _vk2dRendererCreateDescriptorPool(bool preserveDescCons) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (!preserveDescCons) {
		gRenderer->descConSamplers = vk2dDescConCreate(gRenderer->ld, gRenderer->dslTexture, VK2D_NO_LOCATION, 2, VK2D_NO_LOCATION);
		gRenderer->descConSamplersOff = vk2dDescConCreate(gRenderer->ld, gRenderer->dslTexture, VK2D_NO_LOCATION, 2, VK2D_NO_LOCATION);
		gRenderer->descConVP = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferVP, 0, VK2D_NO_LOCATION, VK2D_NO_LOCATION);
		gRenderer->descConUser = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferUser, 3, VK2D_NO_LOCATION, VK2D_NO_LOCATION);

		// And the one sampler set
		VkDescriptorPoolSize sizes = {VK_DESCRIPTOR_TYPE_SAMPLER, 4};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vk2dInitDescriptorPoolCreateInfo(&sizes, 1, 4);
		vk2dErrorCheck(vkCreateDescriptorPool(gRenderer->ld->dev, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &gRenderer->samplerPool));
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vk2dInitDescriptorSetAllocateInfo(gRenderer->samplerPool, 1, &gRenderer->dslSampler);
		vk2dErrorCheck(vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->samplerSet));
		vk2dErrorCheck(vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->modelSamplerSet));
		vk2dLogMessage("Descriptor controllers initialized...");
	} else {
		vk2dLogMessage("Descriptor controllers preserved...");
	}
}

void _vk2dRendererDestroyDescriptorPool(bool preserveDescCons) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (!preserveDescCons) {
		vk2dDescConFree(gRenderer->descConSamplers);
		vk2dDescConFree(gRenderer->descConSamplersOff);
		vk2dDescConFree(gRenderer->descConVP);
		vk2dDescConFree(gRenderer->descConUser);
		vkDestroyDescriptorPool(gRenderer->ld->dev, gRenderer->samplerPool, VK_NULL_HANDLE);
	}
}

void _vk2dRendererCreateSynchronization() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk2dInitSemaphoreCreateInfo(0);
	VkFenceCreateInfo fenceCreateInfo = vk2dInitFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	gRenderer->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->inFlightFences = malloc(sizeof(VkFence) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->imagesInFlight = calloc(1, sizeof(VkFence) * gRenderer->swapchainImageCount);
	gRenderer->commandBuffer = malloc(sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);
	gRenderer->dbCommandBuffer = malloc(sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);

	if (vk2dPointerCheck(gRenderer->imageAvailableSemaphores) && vk2dPointerCheck(gRenderer->renderFinishedSemaphores)
		&& vk2dPointerCheck(gRenderer->inFlightFences) && vk2dPointerCheck(gRenderer->imagesInFlight)) {
		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
			vk2dErrorCheck(vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->imageAvailableSemaphores[i]));
			vk2dErrorCheck(vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->renderFinishedSemaphores[i]));
			vk2dErrorCheck(vkCreateFence(gRenderer->ld->dev, &fenceCreateInfo, VK_NULL_HANDLE, &gRenderer->inFlightFences[i]));
		}
	}

	if (vk2dPointerCheck(gRenderer->commandBuffer) && vk2dPointerCheck(gRenderer->dbCommandBuffer)) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			gRenderer->commandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
			gRenderer->dbCommandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
		}
	}

	vk2dLogMessage("Synchronization initialized...");
}

void _vk2dRendererDestroySynchronization() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	uint32_t i;

	for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(gRenderer->ld->dev, gRenderer->renderFinishedSemaphores[i], VK_NULL_HANDLE);
		vkDestroySemaphore(gRenderer->ld->dev, gRenderer->imageAvailableSemaphores[i], VK_NULL_HANDLE);
		vkDestroyFence(gRenderer->ld->dev, gRenderer->inFlightFences[i], VK_NULL_HANDLE);
	}
	free(gRenderer->imagesInFlight);
	free(gRenderer->inFlightFences);
	free(gRenderer->imageAvailableSemaphores);
	free(gRenderer->renderFinishedSemaphores);
	free(gRenderer->commandBuffer);
}

void _vk2dRendererCreateSampler() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();

	// 2D sampler
	VkSamplerCreateInfo samplerCreateInfo = vk2dInitSamplerCreateInfo(gRenderer->config.filterMode == VK2D_FILTER_TYPE_LINEAR, gRenderer->config.filterMode == VK2D_FILTER_TYPE_LINEAR ? gRenderer->config.msaa : 1, 1);
	vk2dErrorCheck(vkCreateSampler(gRenderer->ld->dev, &samplerCreateInfo, VK_NULL_HANDLE, &gRenderer->textureSampler));
	VkDescriptorImageInfo imageInfo = {0};
	imageInfo.sampler = gRenderer->textureSampler;
	VkWriteDescriptorSet write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLER, 1, gRenderer->samplerSet, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);

	// 3D sampler
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vk2dErrorCheck(vkCreateSampler(gRenderer->ld->dev, &samplerCreateInfo, VK_NULL_HANDLE, &gRenderer->modelSampler));
	imageInfo.sampler = gRenderer->modelSampler;
	write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLER, 1, gRenderer->modelSamplerSet, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);

	vk2dLogMessage("Created texture sampler...");
}

void _vk2dRendererDestroySampler() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	vkDestroySampler(gRenderer->ld->dev, gRenderer->textureSampler, VK_NULL_HANDLE);
	vkDestroySampler(gRenderer->ld->dev, gRenderer->modelSampler, VK_NULL_HANDLE);
}

void _vk2dRendererCreateUnits() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
	vk2dLogMessage("Created unit polygons...");
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
			VkImageView attachments[attachCount];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->targets[i]->sampledImg->view;
				attachments[1] = gRenderer->targets[i]->depthBuffer->view;
				attachments[2] = gRenderer->targets[i]->img->view;
			} else {
				attachments[0] = gRenderer->targets[i]->img->view;
				attachments[1] = gRenderer->targets[i]->depthBuffer->view;
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->externalTargetRenderPass, gRenderer->targets[i]->img->width, gRenderer->targets[i]->img->height, attachments, attachCount);
			vk2dErrorCheck(vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->targets[i]->fbo));
		}
	}
	vk2dLogMessage("Refreshed %i render targets...", targetsRefreshed);
}

void _vk2dRendererDestroyTargetsList() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	free(gRenderer->targets);
}

// If the window is resized or minimized or whatever
void _vk2dRendererResetSwapchain() {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// Hang while minimized
	SDL_WindowFlags flags;
	flags = SDL_GetWindowFlags(gRenderer->window);
	while (flags & SDL_WINDOW_MINIMIZED) {
		flags = SDL_GetWindowFlags(gRenderer->window);
		SDL_PumpEvents();
	}
	vkDeviceWaitIdle(gRenderer->ld->dev);

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

	vk2dLogMessage("Destroyed swapchain assets...");

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

	vk2dLogMessage("Recreated swapchain assets...");
}

void _vk2dRendererDrawRaw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight, VK2DCameraIndex cam) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
	push.texCoords[0] = xInTex;
	push.texCoords[1] = yInTex;
	push.texCoords[2] = texWidth;
	push.texCoords[3] = texHeight;

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

void _vk2dRendererDrawRawInstanced(VkDescriptorSet *sets, uint32_t setCount, VK2DDrawInstance *instances, int count, VK2DCameraIndex cam) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
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
	// Only render to 3D cameras
	for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
		if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type != VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
			sets[0] = gRenderer->cameras[i].uboSets[gRenderer->scImageIndex];
			_vk2dRendererDrawRaw3D(sets, setCount, model, pipe, x, y, z, xscale, yscale, zscale, rot, axis, originX, originY, originZ, i, lineWidth);
		}
	}
}

// This is the upper level internal draw function that draws to each camera and not just with a scissor/viewport
void _vk2dRendererDraw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO) {
		sets[0] = gRenderer->targetUBOSet;
		_vk2dRendererDrawRaw(sets, setCount, poly, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, VK2D_INVALID_CAMERA);
	} else {
		// Only render to 2D cameras
		for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
			if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type == VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
				sets[0] = gRenderer->cameras[i].uboSets[gRenderer->scImageIndex];
				_vk2dRendererDrawRaw(sets, setCount, poly, pipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, xInTex, yInTex, texWidth, texHeight, i);
			}
		}
	}
}

void _vk2dRendererDrawInstanced(VkDescriptorSet *sets, uint32_t setCount, VK2DDrawInstance *instances, int count) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO) {
		sets[0] = gRenderer->targetUBOSet;
		_vk2dRendererDrawRawInstanced(sets, setCount, instances, count, VK2D_INVALID_CAMERA);
	} else {
		// Only render to 2D cameras
		for (int i = 0; i < VK2D_MAX_CAMERAS; i++) {
			if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_NORMAL && gRenderer->cameras[i].spec.type == VK2D_CAMERA_TYPE_DEFAULT && (i == gRenderer->cameraLocked || gRenderer->cameraLocked == VK2D_INVALID_CAMERA)) {
				sets[0] = gRenderer->cameras[i].uboSets[gRenderer->scImageIndex];
				_vk2dRendererDrawRawInstanced(sets, setCount, instances, count, i);
			}
		}
	}
}

void vk2dInstanceSet(VK2DDrawInstance *instance, float x, float y, float xScale, float yScale, float rot, float xOrigin, float yOrigin, float xInTex, float yInTex, float wInTex, float hInTex, vec4 colour) {
	memset(instance->model, 0, sizeof(mat4));
	identityMatrix(instance->model);
	if (rot != 0) {
		xOrigin *= -xScale;
		xOrigin *= yScale;
		instance->pos[0] = 0;
		instance->pos[1] = 0;
		vec3 axis = {0, 0, 1};
		vec3 origin = {-xOrigin + x, yOrigin + y, 0};
		vec3 originTranslation = {xOrigin, -yOrigin, 0};
		translateMatrix(instance->model, origin);
		rotateMatrix(instance->model, axis, rot);
		translateMatrix(instance->model, originTranslation);
	} else {
		instance->pos[0] = x;
		instance->pos[1] = y;
	}
	// Only scale matrix if specified for optimization purposes
	if (xScale != 1 || yScale != 1) {
		vec3 scale = {xScale, yScale, 1};
		scaleMatrix(instance->model, scale);
	}
	instance->texturePos[0] = xInTex;
	instance->texturePos[1] = yInTex;
	instance->texturePos[2] = wInTex;
	instance->texturePos[3] = hInTex;
	instance->colour[0] = colour[0];
	instance->colour[1] = colour[1];
	instance->colour[2] = colour[2];
	instance->colour[3] = colour[3];
}

void vk2dInstanceSetFast(VK2DDrawInstance *instance, float x, float y, float xInTex, float yInTex, float wInTex, float hInTex, vec4 colour) {
	memset(instance->model, 0, sizeof(mat4));
	identityMatrix(instance->model);
	instance->pos[0] = x;
	instance->pos[1] = y;
	instance->texturePos[0] = xInTex;
	instance->texturePos[1] = yInTex;
	instance->texturePos[2] = wInTex;
	instance->texturePos[3] = hInTex;
	instance->colour[0] = colour[0];
	instance->colour[1] = colour[1];
	instance->colour[2] = colour[2];
	instance->colour[3] = colour[3];
}

void vk2dInstanceUpdate(VK2DDrawInstance *instance, float x, float y, float xScale, float yScale, float rot, float xOrigin, float yOrigin) {
	memset(instance->model, 0, sizeof(mat4));
	identityMatrix(instance->model);
	if (rot != 0) {
		xOrigin *= -xScale;
		yOrigin *= yScale;
		instance->pos[0] = 0;
		instance->pos[1] = 0;
		vec3 axis = {0, 0, 1};
		vec3 origin = {-xOrigin + x, yOrigin + y, 0};
		vec3 originTranslation = {xOrigin, -yOrigin, 0};
		translateMatrix(instance->model, origin);
		rotateMatrix(instance->model, axis, rot);
		translateMatrix(instance->model, originTranslation);
	} else {
		instance->pos[0] = x;
		instance->pos[1] = y;
	}
	// Only scale matrix if specified for optimization purposes
	if (xScale != 1 || yScale != 1) {
		vec3 scale = {xScale, yScale, 1};
		scaleMatrix(instance->model, scale);
	}
}