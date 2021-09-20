/// \file Camera.c
/// \author Paolo Mazzon
#include "VK2D/Camera.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/DescriptorControl.h"

void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera);
void _vk2dRendererFlushUBOBuffer(uint32_t frame, int camera);
VK2DCameraIndex vk2dCameraCreate(VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	VK2DCameraIndex position = VK2D_INVALID_CAMERA;

	// Find a spot for a new camera
	for (int i = 0; i < VK2D_MAX_CAMERAS && position == VK2D_INVALID_CAMERA; i++)
		if (gRenderer->cameras[i].state == cs_Deleted)
			position = i;

	// Create the new camera
	if (position != VK2D_INVALID_CAMERA) {
		// Setup pointer and basic info
		VK2DCamera *cam = &gRenderer->cameras[position];
		vk2dCameraUpdate(position, spec);
		cam->state = cs_Normal;

		// Create the lists first
		cam->ubos = calloc(1, sizeof(VK2DUniformBufferObject) * gRenderer->swapchainImageCount);
		cam->buffers = malloc(sizeof(VK2DBuffer) * gRenderer->swapchainImageCount);
		cam->uboSets = malloc(sizeof(VkDescriptorSet) * gRenderer->swapchainImageCount);
		vk2dPointerCheck(cam->ubos);
		vk2dPointerCheck(cam->buffers);
		vk2dPointerCheck(cam->uboSets);

		// Populate the lists
		for (int i = 0; i < gRenderer->swapchainImageCount; i++) {
			cam->buffers[i] = vk2dBufferCreate(gRenderer->ld, sizeof(VK2DUniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			_vk2dCameraUpdateUBO(&cam->ubos[i], &spec);
			_vk2dRendererFlushUBOBuffer(i, position);
			cam->uboSets[i] = vk2dDescConGetBufferSet(gRenderer->descConVP, cam->buffers[i]);
		}
	}

	return position;
}

void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	gRenderer->cameras[index].spec = spec;

	// Make sure the w/h on screen is not zero
	if (gRenderer->cameras[index].spec.wOnScreen == 0)
		gRenderer->cameras[index].spec.wOnScreen = gRenderer->surfaceWidth;
	if (gRenderer->cameras[index].spec.hOnScreen == 0)
		gRenderer->cameras[index].spec.hOnScreen = gRenderer->surfaceHeight;
}

VK2DCameraSpec vk2dCameraGetSpec(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->cameras[index].spec;
}

void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	// Free internal resources
	if ((state == cs_Deleted || state == cs_Reset) && (gRenderer->cameras[index].state == cs_Disabled || gRenderer->cameras[index].state == cs_Normal)) {
		for (int i = 0; i < gRenderer->swapchainImageCount; i++)
			vk2dBufferFree(gRenderer->cameras[index].buffers[i]);
		free(gRenderer->cameras[index].ubos);
		free(gRenderer->cameras[index].buffers);
		free(gRenderer->cameras[index].uboSets);
	}
	gRenderer->cameras[index].state = state;
}

VK2DCameraState vk2dCameraGetState(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return gRenderer->cameras[index].state;
}
