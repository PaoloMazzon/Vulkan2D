#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "VK2D/Validation.h"
#include <stdio.h>
#include <time.h>

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;

const VK2DVertexColour SAMPLE_TRIANGLE[] = {
		{{0.0, 0.0, +0.0}, {1.0, 1.0, 0.5, 1}},
		{{200, 200, +0.0}, {1.0, 0.5, 1.0, 1}},
		{{200, 0.0, +0.0}, {0.5, 1.0, 1.0, 1}},
		{{0.0, 0.0, +0.0}, {0.5, 0.5, 1.0, 1}},
		{{0.0, 200, +0.0}, {0.0, 1.0, 0.5, 1}},
		{{200, 200, +0.0}, {1.0, 1.0, 1.0, 1}}
};
const uint32_t VERTICES = 6;

int main(int argc, const char *argv[]) {
	// Basic SDL setup
	SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
	SDL_Event e;
	bool quit = false;
	int keyboardSize;
	const uint8_t *keyboard = SDL_GetKeyboardState(&keyboardSize);
	if (window == NULL)
		return -1;

	// Initialize vk2d
	VK2DRendererConfig config = {msaa_32x, sm_TripleBuffer, ft_Nearest};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	vec4 line = {1, 0, 0, 1};
	vec4 white = {1, 1, 1, 1};
	int32_t error = vk2dRendererInit(window, config);

	if (error < 0)
		return -1;

	VK2DCamera cam = {0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
	vk2dRendererSetCamera(cam);

	// Load Some test assets **must be done after vk2d is initialized**
	VK2DPolygon testPoly = vk2dPolygonShapeCreateRaw(vk2dRendererGetDevice(), (void *) SAMPLE_TRIANGLE, VERTICES);
	VK2DImage testImage = vk2dImageLoad(vk2dRendererGetDevice(), "assets/caveguy.png");
	VK2DTexture testTexture = vk2dTextureLoad(testImage, 0, 0, 1, 1);
	VK2DTexture testSurface = vk2dTextureCreate(vk2dRendererGetDevice(), 100, 100);
	bool drawnToTestSurface = false;

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double secondCounter = SDL_GetPerformanceCounter();
	double frameCounter = 0;

	// Testing values for fanciness
	float rot = 0;
	float scaleRot = 0;
	float xScale = 0;
	float yScale = 0;
	float camSpeed = 200; // per second
	float camRotSpeed = VK2D_PI; // per second
	float camZoomSpeed = 0.5; // per second

	while (!quit) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		// Calculate delta
		double delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();

		// Process player input
		SDL_PumpEvents();
		cam.x += camSpeed * delta * (float)keyboard[SDL_SCANCODE_D];
		cam.x -= camSpeed * delta * (float)keyboard[SDL_SCANCODE_A];
		cam.y += camSpeed * delta * (float)keyboard[SDL_SCANCODE_S];
		cam.y -= camSpeed * delta * (float)keyboard[SDL_SCANCODE_W];
		cam.rot += camRotSpeed * delta * (float)keyboard[SDL_SCANCODE_Q];
		cam.rot -= camRotSpeed * delta * (float)keyboard[SDL_SCANCODE_E];
		cam.zoom += camZoomSpeed * delta * (float)keyboard[SDL_SCANCODE_I];
		if (keyboard[SDL_SCANCODE_O]) {
			cam.zoom -= camZoomSpeed * delta;
		}
		if (keyboard[SDL_SCANCODE_8]) {
			config.msaa = msaa_8x;
			vk2dRendererSetConfig(config);
		}
		if (keyboard[SDL_SCANCODE_4]) {
			config.msaa = msaa_4x;
			vk2dRendererSetConfig(config);
		}
		if (keyboard[SDL_SCANCODE_1]) {
			config.msaa = msaa_1x;
			vk2dRendererSetConfig(config);
		}
		vk2dRendererSetCamera(cam);


		// Move the caveguy around
		rot += VK2D_PI * 1.5 * delta;
		scaleRot += VK2D_PI * 3.25 * delta;
		xScale = cos(scaleRot) * 0.25;
		yScale = sin(scaleRot) * 0.25;

		// All rendering must happen after this
		vk2dRendererStartFrame(clear);

		// Draw to the test surface
		if (!drawnToTestSurface) {
			drawnToTestSurface = true;
			vk2dRendererSetTarget(testSurface);
			vk2dRendererClear();
			vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
		}

		// Draw some test assets
		vk2dDrawTexture(testSurface, -100, -100);
		vk2dDrawPolygon(testPoly, 0, 0);
		vk2dRendererSetColourMod(line);
		vk2dRendererSetColourMod(white);
		vk2dRendererDrawTexture(testTexture, 64, 64, 4 + 3 * xScale, 4 + 3 * yScale, rot, 8, 8, 0, 0, 16, 16);
		vk2dRendererEndFrame();

		// Window title is set to the framerate every second
		frameCounter += 1;
		if (SDL_GetPerformanceCounter() - secondCounter >= SDL_GetPerformanceFrequency()) {
			char title[50];
			sprintf(title, "Vulkan2D [%0.2fms] [%0.2ffps]", vk2dRendererGetAverageFrameTime(), 1000 / vk2dRendererGetAverageFrameTime());
			SDL_SetWindowTitle(window, title);
			secondCounter = SDL_GetPerformanceCounter();
			frameCounter = 0;
		}
	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dTextureFree(testSurface);
	vk2dTextureFree(testTexture);
	vk2dImageFree(testImage);
	vk2dPolygonFree(testPoly);
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}