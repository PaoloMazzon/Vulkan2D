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
	VK2DRendererConfig config = {VK2D_MSAA_32X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
	vec4 clear = {0.0, 0.5, 1.0, 1.0};
	VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
	int32_t error = vk2dRendererInit(window, config, &options);

	if (error < 0)
		return -1;

	VK2DCameraSpec defcam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0};
	vk2dRendererSetCamera(defcam);

	// Load Some test assets
	VK2DPolygon testPoly = vk2dPolygonShapeCreateRaw((void *) SAMPLE_TRIANGLE, VERTICES);
	VK2DTexture testTexture = vk2dTextureLoad("assets/caveguy.png");
	VK2DTexture testSurface = vk2dTextureCreate(100, 100);
	VK2DCameraSpec cam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
	VK2DCameraIndex testCamera = vk2dCameraCreate(cam);
	debugInit(window);
	bool drawnToTestSurface = false;

	// Setup 3D camera and models
	VK2DCameraSpec cam3D = {VK2D_CAMERA_TYPE_PERSPECTIVE, 0, 0, WINDOW_WIDTH / 4, WINDOW_HEIGHT / 4, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
	cam3D.Perspective.eyes[0] = 2;
	cam3D.Perspective.eyes[1] = 2;
	cam3D.Perspective.eyes[2] = 2;
	cam3D.Perspective.up[2] = 1;
	cam3D.Perspective.fov = 70;
	float xyrot = (-VK2D_PI * 3) / 4;
	float zrot = -VK2D_PI / 5;
	VK2DCameraIndex camera3D = vk2dCameraCreate(cam3D);
	VK2DTexture texVikingRoom = vk2dTextureLoad("assets/viking_room.png");
	VK2DModel modelVikingRoom = vk2dModelLoad("assets/viking_room.obj", texVikingRoom);
	VK2DTexture texCaveguyUV = vk2dTextureLoad("assets/caveguyuv.png");
	VK2DModel modelCaveguy = vk2dModelLoad("assets/caveguy.obj", texCaveguyUV);
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
	float prevMX = 0;
	float prevMY = 0;
	float shaderFloat = 0;

	while (!quit) {
		delta = ((double)SDL_GetPerformanceCounter() - lastTime) / (double)SDL_GetPerformanceFrequency();
		lastTime = SDL_GetPerformanceCounter();

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				quit = true;
			}
		}
		SDL_PumpEvents();

		// 2D camera movement
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
		rot += VK2D_PI * 1.5 * delta;
		scaleRot += VK2D_PI * 3.25 * delta;
		xScale = cos(scaleRot) * 0.25;
		yScale = sin(scaleRot) * 0.25;

		// 3D camera movements
		int mmx, mmy;
		int button = SDL_GetMouseState(&mmx, &mmy);
		float mx = mmx;
		float my = mmy;
		if (button & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			xyrot -= (mx - prevMX) * delta;
			zrot += (my - prevMY) * delta;
		}
		prevMX = mx;
		prevMY = my;

		// MSAA controls
		if (keyboard[SDL_SCANCODE_8] || keyboard[SDL_SCANCODE_4] || keyboard[SDL_SCANCODE_2] || keyboard[SDL_SCANCODE_1]) {
			if (keyboard[SDL_SCANCODE_8])
				config.msaa = VK2D_MSAA_8X;
			if (keyboard[SDL_SCANCODE_4])
				config.msaa = VK2D_MSAA_4X;
			if (keyboard[SDL_SCANCODE_2])
				config.msaa = VK2D_MSAA_2X;
			if (keyboard[SDL_SCANCODE_1])
				config.msaa = VK2D_MSAA_1X;
			vk2dRendererSetConfig(config);
		}

		// Adjust for window size
		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		cam.w = (float)windowWidth / 2;
		cam.h = (float)windowHeight / 2;
		vk2dCameraUpdate(testCamera, cam);

		// Update 3D camera
		cam3D.Perspective.centre[0] = cam3D.Perspective.eyes[0] + sin(xyrot);
		cam3D.Perspective.centre[1] = cam3D.Perspective.eyes[1] + cos(xyrot);
		cam3D.Perspective.centre[2] = cam3D.Perspective.eyes[2] + tan(zrot);
		cam3D.Perspective.fov = (button & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? VK2D_PI * 0.1 : VK2D_PI * 0.3;
		cam3D.wOnScreen = windowWidth;
		cam3D.hOnScreen = windowHeight;
		cam3D.w = windowWidth / 4;
		cam3D.h = windowHeight / 4;
		vk2dCameraUpdate(camera3D, cam3D);

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

		// Draw 2D portions
		vk2dRendererLockCameras(testCamera);
		vk2dDrawTexture(testSurface, -100, -100);
		vk2dDrawPolygon(testPoly, 0, 0);
		vk2dDrawTexture(testTexture, 0, 0);
		shaderFloat += delta * 5;
		vk2dRendererDrawShader(shader, &shaderFloat, testTexture, 64, 64, 4 + 3 * xScale, 4 + 3 * yScale, rot, 8, 8, 0, 0, 16, 16);
		shaderFloat += delta * 5;
		vk2dRendererDrawShader(shader, &shaderFloat, testTexture, 250, 170, 6 + 3 * xScale, 6 + 3 * yScale, (rot * 0.9) - (VK2D_PI / 2), 8, 8, 0, 0, 16, 16);

		// Draw 3D portions
		vk2dRendererLockCameras(camera3D);
		vec3 axis = {0, 0, 1};
		vk2dRendererDrawModel(modelVikingRoom, 0, 0, -0.25, 0.75, 0.75, 0.75, sin(-rot / 3) * 0.5, axis, 0, 0, 0);
		vk2dRendererDrawModel(modelCaveguy, 3, 0, -2, 1, 1, 1, -rot / 3, axis, 0, 0, 0);

		debugRenderOverlay();

		// End the frame
		vk2dRendererEndFrame();


	}

	// vk2dRendererWait must be called before freeing things
	vk2dRendererWait();
	vk2dShaderFree(shader);
	vk2dModelFree(modelVikingRoom);
	vk2dTextureFree(texVikingRoom);
	vk2dModelFree(modelCaveguy);
	vk2dTextureFree(texCaveguyUV);
	debugCleanup();
	vk2dTextureFree(testSurface);
	vk2dTextureFree(testTexture);
	vk2dPolygonFree(testPoly);
	vk2dRendererQuit();
	SDL_DestroyWindow(window);
	return 0;
}