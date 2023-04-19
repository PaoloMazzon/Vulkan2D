#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include "../debug.c"

/************************ Constants ************************/
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

static inline float random(float min, float max) {
	const int resolution = 1000;
	float n = (float)(rand() % resolution);
	return min + ((max - min) * (n / (float)resolution));
}

static inline float clamp(float val, float min, float max) {
	if (val > max)
		return max;
	if (val < min)
		return min;
	return val;
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
	int pageSizeInMegabytes = 4; // This controls the number of instances in the demo
	VK2DStartupOptions options = {false, true, true, "vk2derror.txt", false, 1024 * 1024 * pageSizeInMegabytes};
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;
	double time = 0;

	// Load assets
	debugInit(window);
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");

	// Create cameras
	VK2DCameraSpec cameraSpec = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	VK2DCameraIndex cameraIndex = vk2dCameraCreate(cameraSpec);

	// Make a crazy number of instances
	VK2DRendererLimits limits = vk2dRendererGetLimits();
	// if we make it the limit it will take a whole other page to store all the
	// instance data because the two cameras already consume some of the vram
	// pages so we subtract a few instances. This doesn't really matter when we
	// only lose out on 12ish megabytes of vram/ram but if we want millions of
	// instances we can start losing out on gigabytes.
	const int instanceCount = limits.maxInstancedDraws - 100;
	printf("Caveguys: %i\n", instanceCount);
	fflush(stdout);
	VK2DDrawInstance *instances = malloc(sizeof(VK2DDrawInstance) * instanceCount);
	for (int i = 0; i < instanceCount; i++) {
		vec4 c;
		float scale = random(0.5, 2);
		c[0] = random(0.1, 1);
		c[1] = random(0.1, 1);
		c[2] = random(0.1, 1);
		c[3] = 1;
		vk2dInstanceSet(
				&instances[i],
				random(0, WINDOW_WIDTH),
				random(0, WINDOW_HEIGHT),
				scale,
				scale,
				random(0, VK2D_PI * 2),
				8,
				8,
				0,
				0,
				16,
				16,
				c);
	}

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

		// Scale camera
		int ww, wh;
		SDL_GetWindowSize(window, &ww, &wh);
		float w = (float)ww;
		float h = (float)wh;
		cameraSpec.wOnScreen = w;
		cameraSpec.hOnScreen = h;
		cameraSpec.h = (h / w) * (float)WINDOW_WIDTH;
		vk2dCameraUpdate(cameraIndex, cameraSpec);

		// All rendering must happen after this
		vk2dRendererStartFrame(VK2D_BLACK);

		// Move the caveguys around
		const float moveSpeed = 1;
		for (int i = 0; i < instanceCount; i++) {
			instances[i].pos[0] = clamp(instances[i].pos[0] + random(-moveSpeed, moveSpeed), 0, WINDOW_WIDTH);
			instances[i].pos[1] = clamp(instances[i].pos[1] + random(-moveSpeed, moveSpeed), 0, WINDOW_HEIGHT);
		}

		// Draw game
		vk2dRendererLockCameras(cameraIndex);
		vk2dRendererDrawInstanced(texCaveguy, instances, instanceCount);
		vk2dRendererUnlockCameras();

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(texCaveguy);
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	free(instances);
	return 0;
}