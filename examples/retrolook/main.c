#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include "../debug.c"

/************************ Constants ************************/
const float LOGICAL_WIDTH = 256;
const float LOGICAL_HEIGHT = 224;
const int WINDOW_WIDTH  = (int)LOGICAL_WIDTH * 3;
const int WINDOW_HEIGHT = (int)LOGICAL_HEIGHT * 3;

static inline float min(float x, float y) {
	if (x < y)
		return x;
	return y;
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const uint8_t *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.1, 0.0, 0.2, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;
	double time = 0;

	// Load assets
	debugInit(window);
	VK2DTexture texTarget = vk2dTextureCreate(LOGICAL_WIDTH, LOGICAL_HEIGHT);
	vk2dRendererSetTextureCamera(true);
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguyuv.png");
	VK2DModel modCaveguy = vk2dModelLoad("assets/caveguydie.obj", texCaveguy);

	// Create cameras
	VK2DCameraSpec camera3D = {VK2D_CAMERA_TYPE_PERSPECTIVE, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT, 1, 0, 0, 0, LOGICAL_WIDTH, LOGICAL_WIDTH};
	camera3D.Perspective.eyes[0] = -4;
	camera3D.Perspective.up[1] = 1;
	camera3D.Perspective.fov = VK2D_PI * 0.2;
	VK2DCameraIndex modelCamera = vk2dCameraCreate(camera3D);

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();
		time += delta;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

		// All rendering must happen after this
		vk2dRendererStartFrame(VK2D_BLACK);

		// Clear the virtual target and start rendering to it
		vk2dRendererSetTarget(texTarget);
		vk2dRendererSetColourMod(clear);
		vk2dRendererClear();
		vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);

		// Here is where we draw things to the virtual screen
		vk2dRendererLockCameras(modelCamera);
		vec3 axis = {0, 1, 0};
		vk2dRendererDrawModel(modCaveguy, 0, 0, 0, 1, 1, 1, time, axis, 0, 0, 0);

		vk2dRendererSetTarget(VK2D_TARGET_SCREEN);

		// Draw the virtual target integer-scaled to the center of the window
		int ww, wh;
		SDL_GetWindowSize(window, &ww, &wh);
		float w = (float)ww;
		float h = (float)wh;
		float scale = min(floorf(w / LOGICAL_WIDTH), floorf(h / LOGICAL_WIDTH)) + 1;
		float xOnWindow = (w - (LOGICAL_WIDTH * scale)) / 2;
		float yOnWindow = (h - (LOGICAL_HEIGHT * scale)) / 2;
		vk2dRendererLockCameras(VK2D_DEFAULT_CAMERA);
		vk2dDrawTextureExt(texTarget, xOnWindow, yOnWindow, scale, scale, 0, 0, 0);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(texTarget);
	vk2dTextureFree(texCaveguy);
	vk2dModelFree(modCaveguy);
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}