/// \file Camera.c
/// \author Paolo Mazzon
#include "VK2D/Camera.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"
#include "VK2D/Validation.h"
#include "VK2D/DescriptorControl.h"
#include "VK2D/Opaque.h"

void _vk2dCameraUpdateUBO(VK2DUniformBufferObject *ubo, VK2DCameraSpec *camera);
void _vk2dRendererFlushUBOBuffer(uint32_t frame, int camera);
VK2DCameraIndex vk2dCameraCreate(VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL) {
	    return VK2D_INVALID_CAMERA;
	}

	VK2DCameraIndex position = VK2D_INVALID_CAMERA;
    // Find a spot for a new camera
    for (int i = 0; i < VK2D_MAX_CAMERAS && position == VK2D_INVALID_CAMERA; i++)
        if (gRenderer->cameras[i].state == VK2D_CAMERA_STATE_DELETED)
            position = i;

    // Create the new camera
    if (position != VK2D_INVALID_CAMERA) {
        // Setup pointer and basic info
        VK2DCamera *cam = &gRenderer->cameras[position];
        vk2dCameraUpdate(position, spec);

        // Create the lists first
        cam->ubos = calloc(1, sizeof(VK2DUniformBufferObject) * gRenderer->swapchainImageCount);
        cam->uboSets = malloc(sizeof(VkDescriptorSet) * gRenderer->swapchainImageCount);

        if (cam->uboSets != NULL && cam->ubos != NULL) {
            cam->state = VK2D_CAMERA_STATE_NORMAL;

            // Populate the lists
            for (int i = 0; i < gRenderer->swapchainImageCount; i++) {
                _vk2dCameraUpdateUBO(&cam->ubos[i], &spec);
                cam->uboSets[i] = vk2dDescConGetSet(gRenderer->descConVP);
            }
        } else {
            free(cam->uboSets);
            free(cam->ubos);
            vk2dRaise(VK2D_STATUS_OUT_OF_RAM, "Failed to allocate camera.");
        }
    } else {
        vk2dRaise(VK2D_STATUS_TOO_MANY_CAMERAS, "No more cameras available.");
    }

	return position;
}

void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL) {
	    return;
	}

    // Make sure the w/h on screen is not zero
    gRenderer->cameras[index].spec = spec;
    if (gRenderer->cameras[index].spec.wOnScreen == 0)
        gRenderer->cameras[index].spec.wOnScreen = gRenderer->surfaceWidth;
    if (gRenderer->cameras[index].spec.hOnScreen == 0)
        gRenderer->cameras[index].spec.hOnScreen = gRenderer->surfaceHeight;
}

VK2DCameraSpec vk2dCameraGetSpec(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL) {
	    VK2DCameraSpec s = {0};
	    return s;
	}

	return gRenderer->cameras[index].spec;
}

void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL) {
	    return;
	}


    // Free internal resources
    if ((state == VK2D_CAMERA_STATE_DELETED || state == VK2D_CAMERA_STATE_RESET) &&
        (gRenderer->cameras[index].state == VK2D_CAMERA_STATE_DISABLED || gRenderer->cameras[index].state == VK2D_CAMERA_STATE_NORMAL)) {
        free(gRenderer->cameras[index].ubos);
        free(gRenderer->cameras[index].uboSets);
    }
    gRenderer->cameras[index].state = state;
}

VK2DCameraState vk2dCameraGetState(VK2DCameraIndex index) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (gRenderer == NULL) {
	    return VK2D_CAMERA_STATE_DELETED;
	}

	return gRenderer->cameras[index].state;
}
