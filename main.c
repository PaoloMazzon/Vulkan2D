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
		{{400, 300, +0.0}, {1.0, 0.5, 1.0, 1}},
		{{400, 0.0, +0.0}, {0.5, 1.0, 1.0, 1}},
		{{0.0, 0.0, +0.0}, {0.5, 0.5, 1.0, 1}},
		{{0.0, 300, +0.0}, {0.0, 1.0, 0.5, 1}},
		{{400, 300, +0.0}, {1.0, 1.0, 1.0, 1}}
};
const uint32_t VERTICES = 6;

const VK2DVertex3D SAMPLE_MODEL[] = {
		{{-0.5, -0.5, 000}, {1, 0}},
		{{0.5, -0.5, 000}, {0, 0}},
		{{0.5, 0.5, 000}, {0, 1}},
		{{0.5, 0.5, 000}, {0, 1}},
		{{-0.5, 0.5, 000}, {1, 1}},
		{{-0.5, -0.5, 000}, {1, 0}},
};

const uint32_t SAMPLE_MODEL_VERTICES = 6;

// Very basic and simple font renderer for the font in this test specifically
void renderFont(float x, float y, VK2DTexture tex, const char *text) {
	float ox = x;
	for (int i = 0; i < (int)strlen(text); i++) {
		if (text[i] != '\n') {
			vk2dDrawTexturePart(tex, x, y, (text[i] * 8) % 128, floorf(text[i] / 16) * 16, 8, 16);
			x += 8;
		} else {
			x = ox;
			y += 16;
		}
	}
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
	VK2DRendererConfig config = {msaa_32x, sm_TripleBuffer, ft_Nearest};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	int32_t error = vk2dRendererInit(window, config, &options);

	if (error < 0)
		return -1;

	VK2DCameraSpec cam = {ct_Default, 0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
	vk2dRendererSetCamera(cam);

	// Load Some test assets **must be done after vk2d is initialized**
	VK2DPolygon testPoly = vk2dPolygonShapeCreateRaw((void *) SAMPLE_TRIANGLE, VERTICES);
	VK2DTexture testTexture = vk2dTextureLoad("assets/caveguy.png");
	VK2DTexture testSurface = vk2dTextureCreate(100, 100);
	VK2DTexture testFont = vk2dTextureLoad("assets/font.png");
	VK2DCameraIndex testCamera = vk2dCameraCreate(cam);
	bool drawnToTestSurface = false;

	// Setup 3D camera and model
	VK2DCameraSpec cameraSpec3D = {ct_Perspective, 0, 0, WINDOW_WIDTH / 4, WINDOW_HEIGHT / 4, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	cameraSpec3D.Perspective.eyes[0] = 2;
	cameraSpec3D.Perspective.eyes[1] = 2;
	cameraSpec3D.Perspective.eyes[2] = 2;
	cameraSpec3D.Perspective.up[2] = 1;
	cameraSpec3D.Perspective.fov = 70;
	VK2DCameraIndex camera3D = vk2dCameraCreate(cameraSpec3D);
	VK2DModel testModel = vk2dModelCreate(SAMPLE_MODEL, SAMPLE_MODEL_VERTICES, testTexture);
	VK2DShader shader = vk2dShaderLoad("assets/tex.vert.spv", "assets/tex.frag.spv", 4);

	// Delta and fps
	double lastTime = SDL_GetPerformanceCounter();
	double delta = 0;

	// Testing values for fanciness
	float rot = 0;
	float scaleRot = 0;
	float xScale;
	float yScale;
	float camSpeed = 200; // per second
	float camRotSpeed = VK2D_PI; // per second
	float camZoomSpeed = 0.5f; // per second

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}

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

		// Move the caveguy around
		rot += VK2D_PI * 1.5 * delta;
		scaleRot += VK2D_PI * 3.25 * delta;
		xScale = cos(scaleRot) * 0.25;
		yScale = sin(scaleRot) * 0.25;

		// Adjust for window size
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		cam.w = (float)windowWidth / 2;
		cam.h = (float)windowHeight / 2;
		vk2dCameraUpdate(testCamera, cam);

		// Update shader buffer
		float x = 1;
		vk2dShaderUpdate(shader, &x);

		// All rendering must happen after this
		vk2dRendererStartFrame(clear);

		// Draw to the font surface
		if (!drawnToTestSurface) {
			drawnToTestSurface = true;
			vk2dRendererSetTarget(testSurface);
			vk2dRendererClear();
			vk2dDrawTexture(testTexture, 0, 0);
			vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
		}

		vk2dRendererSetTarget(VK2D_TARGET_SCREEN);

		// Draw some test assets
		vk2dRendererLockCameras(testCamera);
		vk2dDrawTexture(testSurface, -100, -100);
		vk2dDrawPolygon(testPoly, 0, 0);
		vk2dDrawTexture(testTexture, 0, 0);
		vk2dRendererDrawShader(shader, testTexture, 64, 64, 4 + 3 * xScale, 4 + 3 * yScale, rot, 8, 8, 0, 0, 16, 16);
		vk2dRendererDrawTexture(testTexture, 250, 170, 6 + 3 * xScale, 6 + 3 * yScale, (rot * 0.9) - (VK2D_PI / 2), 8, 8, 0, 0, 16, 16);

		// Lock to 3D camera for 3D model
		vk2dRendererLockCameras(camera3D);
		vec3 axis = {0, 0, 1};
		vk2dRendererDrawModel(testModel, 0, 0, 0, 1, 1, 1, -rot, axis, 0, 0, 0);

		// Draw debug overlay
		vk2dRendererLockCameras(VK2D_DEFAULT_CAMERA);
		char title[50];
		sprintf(title, "Vulkan2D [%0.2fms] [%0.2ffps]", vk2dRendererGetAverageFrameTime(), 1000 / vk2dRendererGetAverageFrameTime());
		vk2dRendererSetColourMod(VK2D_BLACK);
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		vk2dDrawRectangle(0, 0, (float)w, 17);
		vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
		renderFont(0, 0, testFont, title);
		vk2dRendererUnlockCameras();

		// End the frame
		vk2dRendererEndFrame();


	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dShaderFree(shader);
	vk2dModelFree(testModel);
	vk2dTextureFree(testFont);
	vk2dTextureFree(testSurface);
	vk2dTextureFree(testTexture);
	vk2dPolygonFree(testPoly);
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}