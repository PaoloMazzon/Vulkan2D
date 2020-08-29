#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include <stdio.h>

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

const VK2DVertexColour SAMPLE_TRIANGLE[] = {
		{{+0.0, -0.5, +0.0}, {1.0, 1.0, 0.5, 1}},
		{{+0.5, +0.5, +0.0}, {1.0, 0.5, 1.0, 1}},
		{{-0.5, +0.5, +0.0}, {0.5, 1.0, 1.0, 1}}
};
const uint32_t VERTICES = 3;

int main(int argc, const char *argv[]) {
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_Event e;
	bool quit = false;

	if (window == NULL)
		return -1;

	// For testing purposes, we just try to get the renderer to use the best possible settings
	VK2DRendererConfig config = {msaa_32x, sm_TripleBuffer, ft_Nearest};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	int32_t error = vk2dRendererInit(window, config);

	if (error < 0)
		return -1;

	// Load test assets
	VK2DPolygon testPoly = vk2dPolygonShapeCreate(vk2dRendererGetDevice(), (void*)SAMPLE_TRIANGLE, VERTICES);
	VK2DImage testImage = vk2dImageLoad(vk2dRendererGetDevice(), "assets/caveguy.png");
	VK2DTexture testTexture = vk2dTextureLoad(testImage, 0, 0, 16, 16);

	while (!quit) {
		while (SDL_PollEvent(&e))
			if (e.type == SDL_QUIT)
				quit = true;

		vk2dRendererStartFrame();
		vk2dRendererClear(clear);
		//vk2dRendererDrawPolygon(testPoly, true, 0, 0, 1, 1, 0);
		vk2dRendererDrawTexture(testTexture, 0, 0, 1, 1, 0);
		vk2dRendererEndFrame();
	}

	vk2dRendererWait();
	vk2dTextureFree(testTexture);
	vk2dImageFree(testImage);
	vk2dPolygonFree(testPoly);
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}