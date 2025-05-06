#define SDL_MAIN_HANDLED
#include <SDL3/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "../debug.c"

/************************ Constants ************************/
const float WINDOW_WIDTH  = 800;
const float WINDOW_HEIGHT = 600;

static inline float min(float x, float y) {
	if (x < y)
		return x;
	return y;
}

int main(int argc, const char *argv[]) {
	// Basic SDL setup
    SDL_Init(SDL_INIT_EVENTS);
	SDL_Window *window = SDL_CreateWindow("VK2D", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const bool *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_32X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear;
	vk2dColourHex(clear, "#59d964");
	VK2DStartupOptions options = {
            .quitOnError = true,
            .errorFile = "vk2derror.txt",
            .enableDebug = true,
            .stdoutLogging = true,
    };
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;
	double time = 0;

	// Load assets
	debugInit(window);
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");
	VK2DPolygon testPoly = vk2dPolygonShapeCreateRaw((void*)SAMPLE_TRIANGLE, SAMPLE_TRIANGLE_VERTICES);

	// Create cameras
	VK2DCameraSpec cameraSpec = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 1, 0, 0, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	VK2DCameraIndex topLeft = vk2dCameraCreate(cameraSpec);
	VK2DCameraSpec cameraSpec2 = {VK2D_CAMERA_TYPE_DEFAULT, 100, 50, WINDOW_WIDTH / 3, WINDOW_HEIGHT / 3, 1, 0, WINDOW_WIDTH / 2, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	VK2DCameraIndex topRight = vk2dCameraCreate(cameraSpec2);
	VK2DCameraSpec cameraSpec3 = {VK2D_CAMERA_TYPE_DEFAULT, 50, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 1, VK2D_PI * 0.1, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	VK2DCameraIndex bottomRight = vk2dCameraCreate(cameraSpec3);
	VK2DCameraSpec cameraSpec4 = {VK2D_CAMERA_TYPE_DEFAULT, 0, 100, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0, 0, WINDOW_HEIGHT / 2, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2};
	VK2DCameraIndex bottomLeft = vk2dCameraCreate(cameraSpec4);

	// Controls
	float prevMX = 0;
	float prevMY = 0;

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();
		time += delta;

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

		// Move cameras
		float mx, my;
		unsigned int button = SDL_GetMouseState(&mx, &my);
		float x = mx;
		float y = my;
		if (button & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
			// Find what quadrant we're in
			VK2DCameraIndex cameraIndex = VK2D_INVALID_CAMERA;
			if (x > WINDOW_WIDTH / 2)
				cameraIndex = (y > WINDOW_HEIGHT / 2) ? bottomRight : topRight;
			else
				cameraIndex = (y > WINDOW_HEIGHT / 2) ? bottomLeft : topLeft;

			// Move selected camera
			VK2DCameraSpec temp = vk2dCameraGetSpec(cameraIndex);
			temp.x -= (x - prevMX);
			temp.y -= (y - prevMY);
			vk2dCameraUpdate(cameraIndex, temp);
		}
		prevMX = x;
		prevMY = y;

		// All rendering must happen after this
		vk2dRendererStartFrame(clear);

		// We disable the default camera first since we don't want to draw to that (its used for ui here)
		vk2dCameraSetState(VK2D_DEFAULT_CAMERA, VK2D_CAMERA_STATE_DISABLED);
		vk2dDrawPolygon(testPoly, 0, 0);
		vk2dDrawTextureExt(texCaveguy, 100 + (sin(time * 2) * 100), 100, 10, 10, time, 8, 8);
		vk2dCameraSetState(VK2D_DEFAULT_CAMERA, VK2D_CAMERA_STATE_NORMAL);

		// Draw an outline around the 4 cameras
		vk2dRendererLockCameras(VK2D_DEFAULT_CAMERA);
		vk2dRendererSetColourMod(VK2D_BLACK);
		vk2dRendererDrawRectangleOutline(0, 0, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 0, 0, 0, 1);
		vk2dRendererDrawRectangleOutline(WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, 0, 0, 0, 1);
		vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);

		debugRenderOverlay();

		vk2dRendererEndFrame();


	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dPolygonFree(testPoly);
	vk2dTextureFree(texCaveguy);
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}