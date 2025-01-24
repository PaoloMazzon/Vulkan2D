#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>
#include "../debug.c"

/************************ Constants ************************/

const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

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
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {
	        .quitOnError = true,
	        .enableDebug = false,
	        .stdoutLogging = true,
	};
	vk2dRendererInit(window, config, &options);
    debugInit(window);

	// Load Some test assets
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");

	// Delta and fps
	const double startTime = SDL_GetPerformanceCounter();

	while (!quit && !vk2dStatusFatal()) {
		const double time = (double)(SDL_GetPerformanceCounter() - startTime) / (double)SDL_GetPerformanceFrequency();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		vk2dRendererStartFrame(clear);

        for (int i = 0; i < 100000; i++) {
            const float caveguyX = vk2dRandom(-16, WINDOW_WIDTH);
            const float caveguyY = vk2dRandom(-16, WINDOW_HEIGHT);
            const float caveguyScaleX = vk2dRandom(0.1, 2);
            const float caveguyScaleY = vk2dRandom(0.1, 2);
            const float caveguyRotation = vk2dRandom(0, VK2D_PI * 2);
            const float caveguyOriginX = 8;
            const float caveguyOriginY = 8;

            vk2dDrawTextureExt(
                    texCaveguy,
                    caveguyX, caveguyY,
                    caveguyScaleX, caveguyScaleY,
                    caveguyRotation, caveguyOriginX, caveguyOriginY);
        }

		debugRenderOverlay();
		vk2dRendererFlushSpriteBatch();

		vk2dRendererEndFrame();
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(texCaveguy);
	debugCleanup();
    vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}
