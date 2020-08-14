#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include <stdio.h>

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

int main(int argc, const char *argv[]) {
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_Event e;
	bool quit = false;

	if (window == NULL)
		return -1;

	int32_t error = vk2dRendererInit(window, td_Max, sm_TripleBuffer, msaa_32x);

	if (error < 0)
		return -1;

	while (!quit) {
		while (SDL_PollEvent(&e))
			if (e.type == SDL_QUIT)
				quit = true;
		// TODO: Test VK2D
	}

	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}