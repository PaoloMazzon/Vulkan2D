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
	if (gRenderer != NULL) {

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
			cam->uboSets = malloc(sizeof(VkDescriptorSet) * gRenderer->swapchainImageCount);
			vk2dPointerCheck(cam->ubos);
			vk2dPointerCheck(cam->uboSets);

			// Populate the lists
			for (int i = 0; i < gRenderer->swapchainImageCount; i++) {
				_vk2dCameraUpdateUBO(&cam->ubos[i], &spec);
				cam->uboSets[i] = vk2dDescConGetSet(gRenderer->descConVP);
			}
		} else {
			vk2dLogMessage("Cannot create more cameras");
		}
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}

	return position;
}

void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer != NULL) {
		gRenderer->cameras[index].spec = spec;

		// Make sure the w/h on screen is not zero
		if (gRenderer->cameras[index].spec.wOnScreen == 0)
			gRenderer->cameras[index].spec.wOnScreen = gRenderer->surfaceWidth;
		if (gRenderer->cameras[index].spec.hOnScreen == 0)
			gRenderer->cameras[index].spec.hOnScreen = gRenderer->surfaceHeight;
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

VK2DCameraSpec vk2dCameraGetSpec(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer != NULL)
		return gRenderer->cameras[index].spec;
	else
		vk2dLogMessage("Renderer is not initialized");
}

void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer != NULL) {
		// Free internal resources
		if ((state == cs_Deleted || state == cs_Reset) &&
			(gRenderer->cameras[index].state == cs_Disabled || gRenderer->cameras[index].state == cs_Normal)) {
			free(gRenderer->cameras[index].ubos);
			free(gRenderer->cameras[index].uboSets);
		}
		gRenderer->cameras[index].state = state;
	} else {
		vk2dLogMessage("Renderer is not initialized");
	}
}

VK2DCameraState vk2dCameraGetState(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer != NULL)
		return gRenderer->cameras[index].state;
	else
		vk2dLogMessage("Renderer is not initialized");
}
