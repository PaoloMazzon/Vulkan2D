/// \file Camera.c
/// \author Paolo Mazzon
#include "VK2D/Camera.h"
#include "VK2D/Renderer.h"
#include "VK2D/Buffer.h"

VK2DCameraIndex vk2dCameraCreate(VK2DCameraSpec spec) {
	// TODO: Check renderer's list of cameras for an available spot and place the camera there, creating necessary UBOs as well
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	return 0;
}

void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	memcpy(&gRenderer->cameras[index].spec, &spec, sizeof(VK2DCameraSpec));
}

void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state) {
	VK2DRenderer gRenderer = vk2dRendererGetPointer();
	if (state == cs_Deleted || state == cs_Reset) { // Free internal resources
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
