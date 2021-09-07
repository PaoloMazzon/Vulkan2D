/// \file Camera.c
/// \author Paolo Mazzon
#include "VK2D/Camera.h"

VK2DCameraIndex vk2dCameraCreate(VK2DCameraSpec spec) {
	// TODO: Check renderer's list of cameras for an available spot and place the camera there, creating necessary UBOs as well
	return 0;
}

void vk2dCameraUpdate(VK2DCameraIndex index, VK2DCameraSpec spec) {
	// TODO: Access renderer camera list, copy spec into the camera's internal spec
}

void vk2dCameraSetState(VK2DCameraIndex index, VK2DCameraState state) {
	// TODO: Either just change state or delete necessary data as well
}

VK2DCameraState vk2dCameraGetState(VK2DCameraIndex index) {
	// TODO: Access renderer camera list, return camera state
	return 0;
}
