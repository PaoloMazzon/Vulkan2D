/// \file Renderer.c
/// \author Paolo Mazzon
#include <vulkan/vulkan.h>
#include <SDL2/SDL_vulkan.h>
#include "VK2D/Renderer.h"
#include "VK2D/BuildOptions.h"
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

// To set DPI awareness
#ifdef WIN32
#include <Windows.h>
#endif // WIN32

/******************************* Forward declarations *******************************/

bool _vk2dFileExists(const char *filename);
unsigned char* _vk2dLoadFile(const char *filename, uint32_t *size);

/******************************* Globals *******************************/

// For debugging
PFN_vkCreateDebugReportCallbackEXT fvkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT fvkDestroyDebugReportCallbackEXT;

// For everything
VK2DRenderer gRenderer = NULL;

#ifdef VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};
static const char* LAYERS[] = {
		"VK_LAYER_KHRONOS_validation"
};
static const int LAYER_COUNT = 1;
static const int EXTENSION_COUNT = 1;
#else // VK2D_ENABLE_DEBUG
static const char* EXTENSIONS[] = {

};
static const char* LAYERS[] = {

};
static const int LAYER_COUNT = 0;
static const int EXTENSION_COUNT = 0;
#endif // VK2D_ENABLE_DEBUG

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

/******************************* Internal functions *******************************/

// Renderer has to keep track of user shaders in case the swapchain gets recreated
void _vk2dRendererAddShader(VK2DShader shader) {
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
			gRenderer->shaderListSize += VK2D_DEFAULT_ARRAY_EXTENSION;
		}
	}

	gRenderer->customShaders[found] = shader;
}

void _vk2dRendererRemoveShader(VK2DShader shader) {
	uint32_t i;
	for (i = 0; i < gRenderer->shaderListSize; i++)
		if (gRenderer->customShaders[i] == shader)
			gRenderer->customShaders[i] = NULL;
}

static uint64_t inline _vk2dHashSets(VkDescriptorSet *sets, uint32_t setCount) {
	uint64_t hash = 0;
	for (uint32_t i = 0; i < setCount; i++) {
		uint64_t *pointer = (void*)&sets[i];
		hash += (*pointer) * (i + 1);
	}
	return hash;
}

static void _vk2dRendererResetBoundPointers() {
	gRenderer->prevPipe = VK_NULL_HANDLE;
	gRenderer->prevSetHash = 0;
	gRenderer->prevVBO = VK_NULL_HANDLE;
}

// This is called when a render-target texture is created to make the renderer aware of it
void _vk2dRendererAddTarget(VK2DTexture tex) {
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
	uint32_t i;
	for (i = 0; i < gRenderer->targetListSize; i++)
		if (gRenderer->targets[i] == tex)
			gRenderer->targets[i] = NULL;
}

// This is used when changing the render target to make sure the texture is either ready to be drawn itself or rendered to
void _vk2dTransitionImageLayout(VkImage img, VkImageLayout old, VkImageLayout new) {
	VkPipelineStageFlags sourceStage = 0;
	VkPipelineStageFlags destinationStage = 0;

	VkImageMemoryBarrier barrier = {};
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
void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCamera *camera) {
	// Assemble view
	vec3 eyes = {-camera->x - (camera->w * 0.5), camera->y + (camera->h * 0.5), 2};
	vec3 center = {-camera->x - (camera->w * 0.5), camera->y + (camera->h * 0.5), 0};//-camera->w / 2, camera->h / 2, 0};
	vec3 up = {sin(camera->rot), -cos(camera->rot), 0};
	cameraMatrix(ubo->view, eyes, center, up);

	// Projection is quite a simple matter
	orthographicMatrix(ubo->proj, camera->h / camera->zoom, camera->w / camera->h, 0.1, 10);
}

// Flushes the data from a ubo to its respective buffer, frame being the swapchain buffer to flush
static void _vk2dRendererFlushUBOBuffer(uint32_t frame) {
	void *data;
	vmaMapMemory(gRenderer->vma, gRenderer->uboBuffers[frame]->mem,  &data);
	memcpy(data, &gRenderer->ubos[frame], sizeof(VK2DUniformBufferObject));
	vmaUnmapMemory(gRenderer->vma, gRenderer->uboBuffers[frame]->mem);
}

static void _vk2dRendererCreateDebug() {
#ifdef VK2D_ENABLE_DEBUG
	fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(gRenderer->vk, "vkCreateDebugReportCallbackEXT");
	fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(gRenderer->vk, "vkDestroyDebugReportCallbackEXT");

	if (vk2dPointerCheck(fvkCreateDebugReportCallbackEXT) && vk2dPointerCheck(fvkDestroyDebugReportCallbackEXT)) {
		VkDebugReportCallbackCreateInfoEXT callbackCreateInfoEXT = vk2dInitDebugReportCallbackCreateInfoEXT(_vk2dDebugCallback);
		fvkCreateDebugReportCallbackEXT(gRenderer->vk, &callbackCreateInfoEXT, VK_NULL_HANDLE, &gRenderer->dr);
	}
#endif // VK2D_ENABLE_DEBUG
}

static void _vk2dRendererDestroyDebug() {
#ifdef VK2D_ENABLE_DEBUG
	fvkDestroyDebugReportCallbackEXT(gRenderer->vk, gRenderer->dr, VK_NULL_HANDLE);
#endif // VK2D_ENABLE_DEBUG
}

// Grabs a preferred present mode if available returning FIFO if its unavailable
static VkPresentModeKHR _vk2dRendererGetPresentMode(VkPresentModeKHR mode) {
	uint32_t i;
	for (i = 0; i < gRenderer->presentModeCount; i++)
		if (gRenderer->presentModes[i] == mode)
			return mode;
	return VK_PRESENT_MODE_FIFO_KHR;
}

static void _vk2dRendererGetSurfaceSize() {
	vk2dErrorCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gRenderer->pd->dev, gRenderer->surface, &gRenderer->surfaceCapabilities));
	if (gRenderer->surfaceCapabilities.currentExtent.width == UINT32_MAX || gRenderer->surfaceCapabilities.currentExtent.height == UINT32_MAX) {
		SDL_Vulkan_GetDrawableSize(gRenderer->window, (void*)&gRenderer->surfaceWidth, (void*)&gRenderer->surfaceHeight);
	} else {
		gRenderer->surfaceWidth = gRenderer->surfaceCapabilities.currentExtent.width;
		gRenderer->surfaceHeight = gRenderer->surfaceCapabilities.currentExtent.height;
	}
}

static void _vk2dRendererCreateWindowSurface() {
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

static void _vk2dRendererDestroyWindowSurface() {
	vkDestroySurfaceKHR(gRenderer->vk, gRenderer->surface, VK_NULL_HANDLE);
	free(gRenderer->presentModes);
}

static void _vk2dRendererCreateSwapchain() {
	uint32_t i;

	gRenderer->config.screenMode = (VK2DScreenMode)_vk2dRendererGetPresentMode((VkPresentModeKHR)gRenderer->config.screenMode);
	VkSwapchainCreateInfoKHR  swapchainCreateInfoKHR = vk2dInitSwapchainCreateInfoKHR(
			gRenderer->surface,
			gRenderer->surfaceCapabilities,
			gRenderer->surfaceFormat,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			(VkPresentModeKHR)gRenderer->config.screenMode,
			VK_NULL_HANDLE,
			gRenderer->surfaceCapabilities.minImageCount + (gRenderer->config.screenMode == sm_TripleBuffer ? 1 : 0)
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

static void _vk2dRendererDestroySwapchain() {
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vkDestroyImageView(gRenderer->ld->dev, gRenderer->swapchainImageViews[i], VK_NULL_HANDLE);

	vkDestroySwapchainKHR(gRenderer->ld->dev, gRenderer->swapchain, VK_NULL_HANDLE);
}

static void _vk2dRendererCreateColourResources() {
	if (gRenderer->config.msaa != msaa_1x) {
		gRenderer->msaaImage = vk2dImageCreate(
				gRenderer->ld,
				gRenderer->surfaceWidth,
				gRenderer->surfaceHeight,
				gRenderer->surfaceFormat.format,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				(VkSampleCountFlagBits) gRenderer->config.msaa);
		vk2dLogMessage("Colour resources initialized...");
	} else {
		vk2dLogMessage("Colour resources not enabled...");
	}
}

static void _vk2dRendererDestroyColourResources() {
	if (gRenderer->msaaImage != NULL)
		vk2dImageFree(gRenderer->msaaImage);
	gRenderer->msaaImage = NULL;
}

static void _vk2dRendererCreateRenderPass() {
	uint32_t attachCount;
	if (gRenderer->config.msaa != 1) {
		attachCount = 2; // colour, resolve
	} else {
		attachCount = 1; // colour
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
	if (gRenderer->config.msaa != 1) {
		attachments[1].format = gRenderer->surfaceFormat.format;
		attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		resolveAttachment.attachment = 1;
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

	// Set up subpass
	const uint32_t subpassCount = 1;
	VkSubpassDescription subpasses[1] = {};
	subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount = colourAttachCount;
	subpasses[0].pColorAttachments = subpassColourAttachments0;
	subpasses[0].pDepthStencilAttachment = VK_NULL_HANDLE;
	subpasses[0].pResolveAttachments = gRenderer->config.msaa > 1 ? &resolveAttachment : VK_NULL_HANDLE;

	// Subpass dependency
	VkSubpassDependency subpassDependency = {};
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
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	} else {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	vk2dErrorCheck(vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->midFrameSwapRenderPass));

	if (gRenderer->config.msaa != 1) {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	} else {
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	}
	vk2dErrorCheck(vkCreateRenderPass(gRenderer->ld->dev, &renderPassCreateInfo, VK_NULL_HANDLE, &gRenderer->externalTargetRenderPass));

	vk2dLogMessage("Render pass initialized...");
}

static void _vk2dRendererDestroyRenderPass() {
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->renderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->externalTargetRenderPass, VK_NULL_HANDLE);
	vkDestroyRenderPass(gRenderer->ld->dev, gRenderer->midFrameSwapRenderPass, VK_NULL_HANDLE);
}

static void _vk2dRendererCreateDescriptorSetLayouts() {
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

static void _vk2dRendererDestroyDescriptorSetLayout() {
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslSampler, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferVP, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslBufferUser, VK_NULL_HANDLE);
	vkDestroyDescriptorSetLayout(gRenderer->ld->dev, gRenderer->dslTexture, VK_NULL_HANDLE);
}

VkPipelineVertexInputStateCreateInfo _vk2dGetTextureVertexInputState();
VkPipelineVertexInputStateCreateInfo _vk2dGetColourVertexInputState();
void _vk2dShaderBuildPipe(VK2DShader shader);
static void _vk2dRendererCreatePipelines() {
	uint32_t i;
	VkPipelineVertexInputStateCreateInfo textureVertexInfo = _vk2dGetTextureVertexInputState();
	VkPipelineVertexInputStateCreateInfo colourVertexInfo = _vk2dGetColourVertexInputState();

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

	// Potentially load some different ones
#ifdef VK2D_LOAD_CUSTOM_SHADERS
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
#endif // VK2D_LOAD_CUSTOM_SHADERS

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
			gRenderer->config.msaa);

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
			gRenderer->config.msaa);
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
			gRenderer->config.msaa);

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

	vk2dLogMessage("Pipelines initialized...");
}

static void _vk2dRendererDestroyPipelines(bool preserveCustomPipes) {
	vk2dPipelineFree(gRenderer->primLinePipe);
	vk2dPipelineFree(gRenderer->primFillPipe);
	vk2dPipelineFree(gRenderer->texPipe);

	if (!preserveCustomPipes)
		free(gRenderer->customShaders);
}

static void _vk2dRendererCreateFrameBuffer() {
	uint32_t i;
	gRenderer->framebuffers = malloc(sizeof(VkFramebuffer) * gRenderer->swapchainImageCount);

	if (vk2dPointerCheck(gRenderer->framebuffers)) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			// There is no 3rd attachment if msaa is disabled
			const int attachCount = gRenderer->config.msaa > 1 ? 2 : 1;
			VkImageView attachments[attachCount];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->msaaImage->view;
				attachments[1] = gRenderer->swapchainImageViews[i];
			} else {
				attachments[0] = gRenderer->swapchainImageViews[i];
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->renderPass, gRenderer->surfaceWidth, gRenderer->surfaceHeight, attachments, attachCount);
			vk2dErrorCheck(vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->framebuffers[i]));
			vk2dLogMessage("Framebuffer[%i] ready...", i);
		}
	}

	vk2dLogMessage("Framebuffers initialized...");
}

static void _vk2dRendererDestroyFrameBuffer() {
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vkDestroyFramebuffer(gRenderer->ld->dev, gRenderer->framebuffers[i], VK_NULL_HANDLE);
	free(gRenderer->framebuffers);
}

static void _vk2dRendererCreateUniformBuffers(bool newCamera) {
	gRenderer->ubos = calloc(1, sizeof(VK2DUniformBufferObject) * gRenderer->swapchainImageCount);
	gRenderer->uboBuffers = malloc(sizeof(VK2DBuffer) * gRenderer->swapchainImageCount);
	gRenderer->uboSets = malloc(sizeof(VkDescriptorSet) * gRenderer->swapchainImageCount);
	uint32_t i;

	VK2DCamera cam = {
			0,
			0,
			gRenderer->surfaceWidth,
			gRenderer->surfaceHeight,
			1,
			0
	};
	if (newCamera)
		gRenderer->camera = cam;

	if (vk2dPointerCheck(gRenderer->ubos) && vk2dPointerCheck(gRenderer->uboBuffers)) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++) {
			gRenderer->uboBuffers[i] = vk2dBufferCreate(gRenderer->ld, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			_vk2dCameraUpdateUBO(&gRenderer->ubos[i], &gRenderer->camera);
			_vk2dRendererFlushUBOBuffer(i);
			gRenderer->uboSets[i] = vk2dDescConGetBufferSet(gRenderer->descConVP, gRenderer->uboBuffers[i]);
		}
	}

	VK2DCamera unitCam = {
			0,
			0,
			1,
			1,
			1,
			0
	};
	VK2DUniformBufferObject unitUBO = {};
	_vk2dCameraUpdateUBO(&unitUBO, &unitCam);
	gRenderer->unitUBO = vk2dBufferLoad(gRenderer->ld, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &unitUBO);
	gRenderer->unitUBOSet = vk2dDescConGetBufferSet(gRenderer->descConVP, gRenderer->unitUBO);

	vk2dLogMessage("UBO initialized...");
}

static void _vk2dRendererDestroyUniformBuffers() {
	uint32_t i;
	for (i = 0; i < gRenderer->swapchainImageCount; i++)
		vk2dBufferFree(gRenderer->uboBuffers[i]);
	vk2dBufferFree(gRenderer->unitUBO);
	free(gRenderer->ubos);
	free(gRenderer->uboBuffers);
	free(gRenderer->uboSets);
}

static void _vk2dRendererCreateDescriptorPool(bool preserveDescCons) {
	if (!preserveDescCons) {
		gRenderer->descConSamplers = vk2dDescConCreate(gRenderer->ld, gRenderer->dslTexture, VK2D_NO_LOCATION, 2);
		gRenderer->descConVP = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferVP, 0, VK2D_NO_LOCATION);
		gRenderer->descConUser = vk2dDescConCreate(gRenderer->ld, gRenderer->dslBufferUser, 2, VK2D_NO_LOCATION);

		// And the one sampler set
		VkDescriptorPoolSize sizes = {VK_DESCRIPTOR_TYPE_SAMPLER, 1};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vk2dInitDescriptorPoolCreateInfo(&sizes, 1, 1);
		vk2dErrorCheck(vkCreateDescriptorPool(gRenderer->ld->dev, &descriptorPoolCreateInfo, VK_NULL_HANDLE, &gRenderer->samplerPool));
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = vk2dInitDescriptorSetAllocateInfo(gRenderer->samplerPool, 1, &gRenderer->dslSampler);
		vk2dErrorCheck(vkAllocateDescriptorSets(gRenderer->ld->dev, &descriptorSetAllocateInfo, &gRenderer->samplerSet));
		vk2dLogMessage("Descriptor controllers initialized...");
	} else {
		vk2dLogMessage("Descriptor controllers preserved...");
	}
}

static void _vk2dRendererDestroyDescriptorPool(bool preserveDescCons) {
	if (!preserveDescCons) {
		vk2dDescConFree(gRenderer->descConSamplers);
		vk2dDescConFree(gRenderer->descConVP);
		vk2dDescConFree(gRenderer->descConUser);
		vkDestroyDescriptorPool(gRenderer->ld->dev, gRenderer->samplerPool, VK_NULL_HANDLE);
	}
}

static void _vk2dRendererCreateSynchronization() {
	uint32_t i;
	VkSemaphoreCreateInfo semaphoreCreateInfo = vk2dInitSemaphoreCreateInfo(0);
	VkFenceCreateInfo fenceCreateInfo = vk2dInitFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	gRenderer->imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->inFlightFences = malloc(sizeof(VkFence) * VK2D_MAX_FRAMES_IN_FLIGHT);
	gRenderer->imagesInFlight = calloc(1, sizeof(VkFence) * gRenderer->swapchainImageCount);
	gRenderer->commandBuffer = malloc(sizeof(VkCommandBuffer) * gRenderer->swapchainImageCount);

	if (vk2dPointerCheck(gRenderer->imageAvailableSemaphores) && vk2dPointerCheck(gRenderer->renderFinishedSemaphores)
		&& vk2dPointerCheck(gRenderer->inFlightFences) && vk2dPointerCheck(gRenderer->imagesInFlight)) {
		for (i = 0; i < VK2D_MAX_FRAMES_IN_FLIGHT; i++) {
			vk2dErrorCheck(vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->imageAvailableSemaphores[i]));
			vk2dErrorCheck(vkCreateSemaphore(gRenderer->ld->dev, &semaphoreCreateInfo, VK_NULL_HANDLE, &gRenderer->renderFinishedSemaphores[i]));
			vk2dErrorCheck(vkCreateFence(gRenderer->ld->dev, &fenceCreateInfo, VK_NULL_HANDLE, &gRenderer->inFlightFences[i]));
		}
	}

	if (vk2dPointerCheck(gRenderer->commandBuffer)) {
		for (i = 0; i < gRenderer->swapchainImageCount; i++)
			gRenderer->commandBuffer[i] = vk2dLogicalDeviceGetCommandBuffer(gRenderer->ld, true);
	}

	vk2dLogMessage("Synchronization initialized...");
}

static void _vk2dRendererDestroySynchronization() {
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

static void _vk2dRendererCreateSampler() {
	VkSamplerCreateInfo samplerCreateInfo = vk2dInitSamplerCreateInfo(gRenderer->config.filterMode == ft_Linear, gRenderer->config.filterMode == ft_Linear ? gRenderer->config.msaa : 1, 1);
	vk2dErrorCheck(vkCreateSampler(gRenderer->ld->dev, &samplerCreateInfo, VK_NULL_HANDLE, &gRenderer->textureSampler));
	VkDescriptorImageInfo imageInfo = {}; // TODO: fix the bass-ackwards sampler/tex descriptor sets
	imageInfo.sampler = gRenderer->textureSampler;
	VkWriteDescriptorSet write = vk2dInitWriteDescriptorSet(VK_DESCRIPTOR_TYPE_SAMPLER, 1, gRenderer->samplerSet, VK_NULL_HANDLE, 1, &imageInfo);
	vkUpdateDescriptorSets(gRenderer->ld->dev, 1, &write, 0, VK_NULL_HANDLE);

	vk2dLogMessage("Created texture sampler...");
}

static void _vk2dRendererDestroySampler() {
	vkDestroySampler(gRenderer->ld->dev, gRenderer->textureSampler, VK_NULL_HANDLE);
}

static void _vk2dRendererCreateUnits() {
#ifdef VK2D_UNIT_GENERATION
	// Squares are simple
	gRenderer->unitSquare = vk2dPolygonShapeCreateRaw(gRenderer->ld, unitSquare, unitSquareVertices);
	gRenderer->unitSquareOutline = vk2dPolygonCreateOutline(gRenderer->ld, unitSquareOutline, unitSquareOutlineVertices);

	// Now to generate the circle
	vec2 *circleVertices = malloc(sizeof(vec2) * VK2D_CIRCLE_VERTICES);
	uint32_t i;
	for (i = 0; i < VK2D_CIRCLE_VERTICES; i++) {
		circleVertices[i][0] = cos(((double)i / VK2D_CIRCLE_VERTICES) * (VK2D_PI * 2)) * 0.5;
		circleVertices[i][1] = sin(((double)i / VK2D_CIRCLE_VERTICES) * (VK2D_PI * 2)) * 0.5;
	}
	gRenderer->unitCircle = vk2dPolygonCreate(gRenderer->ld, circleVertices, VK2D_CIRCLE_VERTICES);
	gRenderer->unitCircleOutline = vk2dPolygonCreateOutline(gRenderer->ld, circleVertices, VK2D_CIRCLE_VERTICES);
	gRenderer->unitLine = vk2dPolygonCreateOutline(gRenderer->ld, (void*)LINE_VERTICES, LINE_VERTEX_COUNT);
	vk2dLogMessage("Created unit polygons...");
#else // VK2D_UNIT_GENERATION
	vk2dLogMessage("Unit polygons disabled...");
#endif // VK2D_UNIT_GENERATION
}

static void _vk2dRendererDestroyUnits() {
#ifdef VK2D_UNIT_GENERATION
	vk2dPolygonFree(gRenderer->unitSquare);
	vk2dPolygonFree(gRenderer->unitSquareOutline);
	vk2dPolygonFree(gRenderer->unitCircle);
	vk2dPolygonFree(gRenderer->unitCircleOutline);
	vk2dPolygonFree(gRenderer->unitLine);
#endif // VK2D_UNIT_GENERATION
}

void _vk2dImageTransitionImageLayout(VK2DLogicalDevice dev, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
static void _vk2dRendererRefreshTargets() {
	uint32_t i;
	uint32_t targetsRefreshed = 0;
	for (i = 0; i < gRenderer->targetListSize; i++) {
		if (gRenderer->targets[i] != NULL) {
			targetsRefreshed++;

			// Sampled image
			vk2dImageFree(gRenderer->targets[i]->sampledImg);
			gRenderer->targets[i]->sampledImg = vk2dImageCreate(
					gRenderer->ld,
					gRenderer->targets[i]->img->width,
					gRenderer->targets[i]->img->height,
					VK_FORMAT_B8G8R8A8_SRGB,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					(VkSampleCountFlagBits)gRenderer->config.msaa);
			_vk2dImageTransitionImageLayout(gRenderer->ld, gRenderer->targets[i]->sampledImg->img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			// Framebuffer
			vkDestroyFramebuffer(gRenderer->ld->dev, gRenderer->targets[i]->fbo, VK_NULL_HANDLE);
			const int attachCount = gRenderer->config.msaa > 1 ? 2 : 1;
			VkImageView attachments[attachCount];
			if (gRenderer->config.msaa > 1) {
				attachments[0] = gRenderer->targets[i]->sampledImg->view;
				attachments[1] = gRenderer->targets[i]->img->view;
			} else {
				attachments[0] = gRenderer->targets[i]->img->view;
			}

			VkFramebufferCreateInfo framebufferCreateInfo = vk2dInitFramebufferCreateInfo(gRenderer->externalTargetRenderPass, gRenderer->targets[i]->img->width, gRenderer->targets[i]->img->height, attachments, attachCount);
			vk2dErrorCheck(vkCreateFramebuffer(gRenderer->ld->dev, &framebufferCreateInfo, VK_NULL_HANDLE, &gRenderer->targets[i]->fbo));
		}
	}
	vk2dLogMessage("Refreshed %i render targets...", targetsRefreshed);
}

static void _vk2dRendererDestroyTargetsList() {
	free(gRenderer->targets);
}

// If the window is resized or minimized or whatever
static void _vk2dRendererResetSwapchain() {
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
	_vk2dRendererDestroyColourResources();
	_vk2dRendererDestroySwapchain();

	vk2dLogMessage("Destroyed swapchain assets...");

	// Swap out configs in case they were changed
	gRenderer->config = gRenderer->newConfig;

	// Restart swapchain
	_vk2dRendererGetSurfaceSize();
	_vk2dRendererCreateSwapchain();
	_vk2dRendererCreateColourResources();
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

/******************************* User-visible functions *******************************/

int32_t vk2dRendererInit(SDL_Window *window, VK2DRendererConfig config) {
	gRenderer = calloc(1, sizeof(struct VK2DRenderer));
	int32_t errorCode = 0;
	uint32_t totalExtensionCount, i, sdlExtensions;
	const char** totalExtensions;

	// Windows 10 dpi settings don't play nice
#ifdef WIN32
	SetProcessDPIAware();
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
#endif // WIN32

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
	totalExtensionCount = sdlExtensions + EXTENSION_COUNT;
	totalExtensions = malloc(totalExtensionCount * sizeof(char*));

	if (vk2dPointerCheck(gRenderer)) {
		// Load extensions
		SDL_Vulkan_GetInstanceExtensions(window, &sdlExtensions, totalExtensions);
		for (i = sdlExtensions; i < totalExtensionCount; i++) totalExtensions[i] = EXTENSIONS[i - sdlExtensions];

		// Log all used extensions
		vk2dLogMessage("Vulkan Enabled Extensions: ");
		for (i = 0; i < totalExtensionCount; i++)
			vk2dLogMessage(" - %s", totalExtensions[i]);
		vk2dLogMessage(""); // Newline

		// Create instance, physical, and logical device
		VkInstanceCreateInfo instanceCreateInfo = vk2dInitInstanceCreateInfo((void*)&VK2D_DEFAULT_CONFIG, LAYERS, LAYER_COUNT, totalExtensions, totalExtensionCount);
		vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &gRenderer->vk);
		gRenderer->pd = vk2dPhysicalDeviceFind(gRenderer->vk, VK2D_DEVICE_BEST_FIT);
		gRenderer->ld = vk2dLogicalDeviceCreate(gRenderer->pd, false, true);
		gRenderer->window = window;

		// Assign user settings, except for screen mode which will be handled later
		VK2DMSAA maxMSAA = vk2dPhysicalDeviceGetMSAA(gRenderer->pd);
		gRenderer->config = config;
		gRenderer->config.msaa = maxMSAA >= config.msaa ? config.msaa : maxMSAA;
		gRenderer->newConfig = gRenderer->config;

		// Create the VMA
		VmaAllocatorCreateInfo allocatorCreateInfo = {};
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
		_vk2dRendererCreateRenderPass();
		_vk2dRendererCreateDescriptorSetLayouts();
		_vk2dRendererCreatePipelines();
		_vk2dRendererCreateFrameBuffer();
		_vk2dRendererCreateDescriptorPool(false);
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
	} else {
		errorCode = -1;
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
		_vk2dRendererDestroyUniformBuffers();
		_vk2dRendererDestroyFrameBuffer();
		_vk2dRendererDestroyPipelines(false);
		_vk2dRendererDestroyDescriptorSetLayout();
		_vk2dRendererDestroyRenderPass();
		_vk2dRendererDestroyColourResources();
		_vk2dRendererDestroySwapchain();
		_vk2dRendererDestroyWindowSurface();
		_vk2dRendererDestroyDebug();

		vmaDestroyAllocator(gRenderer->vma);

		// Destroy core bits
		vk2dLogicalDeviceFree(gRenderer->ld);
		vk2dPhysicalDeviceFree(gRenderer->pd);

		free(gRenderer);
		gRenderer = NULL;

		vk2dLogMessage("VK2D has been uninitialized.");
	}
}

void vk2dRendererWait() {
	vkQueueWaitIdle(gRenderer->ld->queue);
}

VK2DRenderer vk2dRendererGetPointer() {
	return gRenderer;
}

void vk2dRendererResetSwapchain() {
	gRenderer->resetSwapchain = true;
}

VK2DRendererConfig vk2dRendererGetConfig() {
	return gRenderer->config;
}

void vk2dRendererSetConfig(VK2DRendererConfig config) {
	gRenderer->newConfig = config;
	VK2DMSAA maxMSAA = vk2dPhysicalDeviceGetMSAA(gRenderer->pd);
	gRenderer->newConfig.msaa = maxMSAA >= config.msaa ? config.msaa : maxMSAA;
	vk2dRendererResetSwapchain();
}

void vk2dRendererStartFrame(vec4 clearColour) {
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
		gRenderer->targetUBOSet = gRenderer->uboSets[gRenderer->scImageIndex];
		gRenderer->target = VK2D_TARGET_SCREEN;

		// Flush the current ubo into its buffer for the frame
		_vk2dCameraUpdateUBO(&gRenderer->ubos[gRenderer->scImageIndex], &gRenderer->camera);
		_vk2dRendererFlushUBOBuffer(gRenderer->scImageIndex);

		// Start the render pass
		VkCommandBufferBeginInfo beginInfo = vk2dInitCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
																			VK_NULL_HANDLE);
		vkResetCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex], 0);
		vk2dErrorCheck(vkBeginCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex], &beginInfo));

		// Setup render pass
		VkRect2D rect = {};
		rect.extent.width = gRenderer->surfaceWidth;
		rect.extent.height = gRenderer->surfaceHeight;
		const uint32_t clearCount = 1;
		VkClearValue clearValues[1] = {};
		clearValues[0].color.float32[0] = clearColour[0];
		clearValues[0].color.float32[1] = clearColour[1];
		clearValues[0].color.float32[2] = clearColour[2];
		clearValues[0].color.float32[3] = clearColour[3];
		VkRenderPassBeginInfo renderPassBeginInfo = vk2dInitRenderPassBeginInfo(
				gRenderer->renderPass,
				gRenderer->framebuffers[gRenderer->scImageIndex],
				rect,
				clearValues,
				clearCount);

		vkCmdBeginRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex], &renderPassBeginInfo,
							 VK_SUBPASS_CONTENTS_INLINE);
	}
}

void vk2dRendererEndFrame() {
	if (gRenderer->procedStartFrame) {
		gRenderer->procedStartFrame = false;
		// Finish the primary command buffer, its time to PRESENT things
		vkCmdEndRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex]);
		vk2dErrorCheck(vkEndCommandBuffer(gRenderer->commandBuffer[gRenderer->scImageIndex]));

		// Wait for image before doing things
		VkPipelineStageFlags waitStage[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		VkSubmitInfo submitInfo = vk2dInitSubmitInfo(
				&gRenderer->commandBuffer[gRenderer->scImageIndex],
				1,
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
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || gRenderer->resetSwapchain || queueRes == VK_ERROR_OUT_OF_DATE_KHR) {
			_vk2dRendererResetSwapchain();
			gRenderer->resetSwapchain = false;
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
}

VK2DLogicalDevice vk2dRendererGetDevice() {
	return gRenderer->ld;
}

void vk2dRendererSetTarget(VK2DTexture target) {
	if (target != gRenderer->target) {
		gRenderer->target = target;
		// Figure out which render pass to use
		VkRenderPass pass = target == VK2D_TARGET_SCREEN ? gRenderer->midFrameSwapRenderPass : gRenderer->externalTargetRenderPass;
		VkFramebuffer framebuffer = target == VK2D_TARGET_SCREEN ? gRenderer->framebuffers[gRenderer->scImageIndex] : target->fbo;
		VkImage image = target == VK2D_TARGET_SCREEN ? gRenderer->swapchainImages[gRenderer->scImageIndex] : target->img->img;
		VkDescriptorSet buffer = target == VK2D_TARGET_SCREEN ? gRenderer->uboSets[gRenderer->scImageIndex] : target->uboSet;

		vkCmdEndRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex]);

		// Now we either have to transition the image layout depending on whats going in and whats poppin out
		if (target == VK2D_TARGET_SCREEN)
			_vk2dTransitionImageLayout(gRenderer->targetImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		else
			_vk2dTransitionImageLayout(target->img->img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Assign new render targets
		gRenderer->targetRenderPass = pass;
		gRenderer->targetFrameBuffer = framebuffer;
		gRenderer->targetImage = image;
		gRenderer->targetUBOSet = buffer;

		// Setup new render pass
		VkRect2D rect = {};
		rect.extent.width = target == VK2D_TARGET_SCREEN ? gRenderer->surfaceWidth : target->img->width;
		rect.extent.height = target == VK2D_TARGET_SCREEN ? gRenderer->surfaceHeight : target->img->height;
		VkRenderPassBeginInfo renderPassBeginInfo = vk2dInitRenderPassBeginInfo(
				pass,
				framebuffer,
				rect,
				VK_NULL_HANDLE,
				0);

		vkCmdBeginRenderPass(gRenderer->commandBuffer[gRenderer->scImageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		_vk2dRendererResetBoundPointers();
	}
}

void vk2dRendererSetColourMod(vec4 mod) {
	gRenderer->colourBlend[0] = mod[0];
	gRenderer->colourBlend[1] = mod[1];
	gRenderer->colourBlend[2] = mod[2];
	gRenderer->colourBlend[3] = mod[3];
}

void vk2dRendererGetColourMod(vec4 dst) {
	dst[0] = gRenderer->colourBlend[0];
	dst[1] = gRenderer->colourBlend[1];
	dst[2] = gRenderer->colourBlend[2];
	dst[3] = gRenderer->colourBlend[3];
}

void vk2dRendererSetBlendMode(VK2DBlendMode blendMode) {
	gRenderer->blendMode = blendMode;
}

VK2DBlendMode vk2dRendererGetBlendMode() {
	return gRenderer->blendMode;
}

void vk2dRendererSetCamera(VK2DCamera camera) {
	gRenderer->camera = camera;
}

VK2DCamera vk2dRendererGetCamera() {
	return gRenderer->camera;
}

void vk2dRendererSetViewport(float x, float y, float w, float h) {
	gRenderer->viewport.x = x;
	gRenderer->viewport.y = y;
	gRenderer->viewport.width = w;
	gRenderer->viewport.height = h;
}

void vk2dRendererGetViewport(float *x, float *y, float *w, float *h) {
	*x = gRenderer->viewport.x;
	*y = gRenderer->viewport.y;
	*w = gRenderer->viewport.width;
	*h = gRenderer->viewport.height;
}

void vk2dRendererSetTextureCamera(bool useCameraOnTextures) {
	gRenderer->enableTextureCameraUBO = useCameraOnTextures;
}

double vk2dRendererGetAverageFrameTime() {
	return gRenderer->frameTimeAverage;
}

static inline void _vk2dRendererDraw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight);

void vk2dRendererClear() {
	VkDescriptorSet set = gRenderer->unitUBOSet;
	_vk2dRendererDraw(&set, 1, gRenderer->unitSquare, gRenderer->primFillPipe, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0);
}

void vk2dRendererDrawRectangle(float x, float y, float w, float h, float r, float ox, float oy) {
#ifdef VK2D_UNIT_GENERATION
	vk2dRendererDrawPolygon(gRenderer->unitSquare, x, y, true, 1, w, h, r, ox, oy);
#endif // VK2D_UNIT_GENERATION
}

void vk2dRendererDrawRectangleOutline(float x, float y, float w, float h, float r, float ox, float oy, float lineWidth) {
#ifdef VK2D_UNIT_GENERATION
	vk2dRendererDrawPolygon(gRenderer->unitSquareOutline, x, y, false, lineWidth, w, h, r, ox, oy);
#endif //  VK2D_UNIT_GENERATION
}

void vk2dRendererDrawCircle(float x, float y, float r) {
#ifdef VK2D_UNIT_GENERATION
	vk2dRendererDrawPolygon(gRenderer->unitCircle, x, y, true, 1, r * 2, r * 2, 0, 0, 0);
#endif //  VK2D_UNIT_GENERATION
}

void vk2dRendererDrawCircleOutline(float x, float y, float r, float lineWidth) {
#ifdef VK2D_UNIT_GENERATION
	vk2dRendererDrawPolygon(gRenderer->unitCircleOutline, x, y, false, lineWidth, r * 2, r * 2, 0, 0, 0);
#endif //  VK2D_UNIT_GENERATION
}

void vk2dRendererDrawLine(float x1, float y1, float x2, float y2) {
#ifdef VK2D_UNIT_GENERATION
	float x = sqrtf(powf(y2 - y1, 2) + powf(x2 - x1, 2));
	float r = atan2f(y2 - y1, x2 - x1);
	vk2dRendererDrawPolygon(gRenderer->unitLine, x1, y1, false, 1, x, 1, r, 0, 0);
#endif //  VK2D_UNIT_GENERATION
}

static inline void _vk2dRendererDraw(VkDescriptorSet *sets, uint32_t setCount, VK2DPolygon poly, VK2DPipeline pipe, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float lineWidth, float xInTex, float yInTex, float texWidth, float texHeight) {
	VkCommandBuffer buf = gRenderer->commandBuffer[gRenderer->scImageIndex];

	// Account for various coordinate-based qualms
	originX *= xscale;
	originY *= yscale;
	originX -= xInTex;
	originY -= yInTex;
	rot *= -1;

	// Push constants
	VK2DPushBuffer push = {};
	identityMatrix(push.model);
	vec3 axis = {0, 0, 1};
	vec3 originTranslation = {originX, -originY, 0};
	vec3 origin2 = {-originX - x, originY + y, 0};
	vec3 scale = {-xscale, yscale, 1};
	translateMatrix(push.model, origin2);
	rotateMatrix(push.model, axis, rot);
	translateMatrix(push.model, originTranslation);
	scaleMatrix(push.model, scale);
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
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(buf, 0, 1, &poly->vertices->buf, offsets);
		gRenderer->prevVBO = poly->vertices->buf;
	}
	VkRect2D scissor = {};
	scissor.extent.width = gRenderer->surfaceWidth;
	scissor.extent.height = gRenderer->surfaceHeight;

	// Dynamic state that can't be optimized further and the draw call
	vkCmdSetViewport(buf, 0, 1, &gRenderer->viewport);
	vkCmdSetScissor(buf, 0, 1, &scissor);
	vkCmdSetLineWidth(buf, lineWidth);
	vkCmdPushConstants(buf, pipe->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(VK2DPushBuffer), &push);
	if (poly != NULL)
		vkCmdDraw(buf, poly->vertexCount, 1, 0, 0);
	else // The only time this would be the case is for textures, where the shader provides the vertices
		vkCmdDraw(buf, 6, 1, 0, 0);
}

void vk2dRendererDrawShader(VK2DShader shader, VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight) {
	VkDescriptorSet sets[4];
	if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO)
		sets[0] = gRenderer->targetUBOSet;
	else
		sets[0] = gRenderer->uboSets[gRenderer->scImageIndex];
	sets[1] = gRenderer->samplerSet;
	sets[2] = tex->img->set;
	sets[3] = shader->sets[shader->currentUniform];

	uint32_t setCount = shader->uniformSize == 0 ? 3 : 4;
	_vk2dRendererDraw(sets, setCount, NULL, shader->pipe, x, y, xscale, yscale, rot, originX, originY, 1, xInTex, yInTex, texWidth, texHeight);
}

void vk2dRendererDrawTexture(VK2DTexture tex, float x, float y, float xscale, float yscale, float rot, float originX, float originY, float xInTex, float yInTex, float texWidth, float texHeight) {
	VkDescriptorSet sets[3];
	if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO)
		sets[0] = gRenderer->targetUBOSet;
	else
		sets[0] = gRenderer->uboSets[gRenderer->scImageIndex];
	sets[1] = gRenderer->samplerSet;
	sets[2] = tex->img->set;
	_vk2dRendererDraw(sets, 3, NULL, gRenderer->texPipe, x, y, xscale, yscale, rot, originX, originY, 1, xInTex, yInTex, texWidth, texHeight);
}

void vk2dRendererDrawPolygon(VK2DPolygon polygon, float x, float y, bool filled, float lineWidth, float xscale, float yscale, float rot, float originX, float originY) {
	VkDescriptorSet set;
	if (gRenderer->target != VK2D_TARGET_SCREEN && !gRenderer->enableTextureCameraUBO)
		set = gRenderer->targetUBOSet;
	else
		set = gRenderer->uboSets[gRenderer->scImageIndex];
	_vk2dRendererDraw(&set, 1, polygon, filled ? gRenderer->primFillPipe : gRenderer->primLinePipe, x, y, xscale, yscale, rot, originX, originY, lineWidth, 0, 0, 0, 0);
}
