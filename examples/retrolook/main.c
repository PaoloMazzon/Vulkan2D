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
	VK2DRendererConfig config = {msaa_32x, sm_TripleBuffer, ft_Nearest};
	vec4 clear = {0.0, 0.0, 0.0, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	if (vk2dRendererInit(window, config, &options) < 0)
		return -1;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;

	// Load assets
	debugInit(window);

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

		// All rendering must happen after this
		vk2dRendererStartFrame(clear);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();


	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	debugCleanup();
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}