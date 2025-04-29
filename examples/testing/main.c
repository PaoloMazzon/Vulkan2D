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

const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

int main(int argc, const char *argv[]) {
	// Basic SDL setup
    SDL_Init(SDL_INIT_EVENTS);
	SDL_Window *window = SDL_CreateWindow("VK2D", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const bool *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {
	        .quitOnError = true,
	        .enableDebug = false,
	        .stdoutLogging = true,
	        .vramPageSize = sizeof(VK2DDrawInstance) * 2000010,
	};
	vk2dRendererInit(window, config, &options);
    debugInit(window);

	// Load Some test assets
	VK2DTexture texCaveguy = vk2dTextureLoad("assets/caveguy.png");

	// Delta and fps
	const double startTime = SDL_GetPerformanceCounter();
	VK2DDrawCommand *commands = calloc(100000, sizeof(VK2DDrawCommand));

    for (int i = 0; i < 100000; i++) {
        commands[i].pos[0] = 400 + sinf(i) * i * 0.5;//vk2dRandom(-16, WINDOW_WIDTH);
        commands[i].pos[1] = 300 + cosf(i) * i * 0.5;//vk2dRandom(-16, WINDOW_HEIGHT);
        commands[i].scale[0] = 1;//vk2dRandom(0.1, 2);
        commands[i].scale[1] = 1;//vk2dRandom(0.1, 2);
        commands[i].rotation = 0;//vk2dRandom(0, VK2D_PI * 2);
        commands[i].origin[0] = 8;
        commands[i].origin[1] = 8;
        commands[i].colour[0] = 1;
        commands[i].colour[1] = 1;
        commands[i].colour[2] = 1;
        commands[i].colour[3] = 1;
        commands[i].textureIndex = vk2dTextureGetID(texCaveguy);
        commands[i].texturePos[2] = 16;
        commands[i].texturePos[3] = 16;
    }

	while (!quit && !vk2dStatusFatal()) {
		const double time = (double)(SDL_GetPerformanceCounter() - startTime) / (double)SDL_GetPerformanceFrequency();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);

		vk2dRendererStartFrame(clear);

        vk2dRendererAddBatch(commands, 8192);
        //vk2dRendererFlushSpriteBatch();

		debugRenderOverlay();

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
