#define SDL_MAIN_HANDLED
#include <SDL2/SDL_vulkan.h>
#include <stdbool.h>
#include "VK2D/VK2D.h"
#include "../debug.c"

/************************ Constants ************************/
const int WINDOW_WIDTH  = 800;
const int WINDOW_HEIGHT = 600;
const double MAX_VELOCITY = 2;
const double ACCELERATION = 0.2;
const double FRICTION = 0.5;

static inline double sign(double x) {
    return (x > 0 ? 1 : (x < 0 ? -1 : 0));
}

static inline double towardsZero(double num, double val) {
    double change = num > 0 ? -val : (num < 0 ? val : 0);
    if (sign(num) != sign(num + change)) {
        return -num;
    }
    return change;
}

static inline double clamp(double x, double min, double max) {
    return x > max ? max : (x < min ? min : x);
}


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
    VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
    vec4 clear = {0.0, 0.5, 1.0, 1.0};
    VK2DStartupOptions options = {true, true, true, "vk2derror.txt", false};
    int32_t error = vk2dRendererInit(window, config, &options);

    if (error < 0)
        return -1;

    VK2DCameraSpec defcam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0};
    vk2dRendererSetCamera(defcam);

    // Load Some assets
    VK2DTexture playerTex = vk2dTextureLoad("assets/caveguy.png");
    VK2DTexture lightOrbTex = vk2dTextureLoad("assets/light.png");
    VK2DTexture shadowsTex = vk2dTextureCreate(400, 300);
    VK2DShadowEnvironment shadows = vk2DShadowEnvironmentCreate();
    vk2DShadowEnvironmentAddEdge(shadows, 200, 200, 200, 250);
    vk2DShadowEnvironmentAddEdge(shadows, 200, 200, 250, 200);
    vk2DShadowEnvironmentAddEdge(shadows, 200, 250, 250, 250);
    vk2DShadowEnvironmentAddEdge(shadows, 250, 200, 250, 250);
    vk2DShadowEnvironmentAddEdge(shadows, 100, 200, 100, 250);
    vk2DShadowEnvironmentAddEdge(shadows, 100, 200, 150, 250);
    vk2DShadowEnvironmentAddEdge(shadows, 100, 250, 150, 250);
    vk2DShadowEnvironmentFlushVBO(shadows);

    VK2DCameraSpec cam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH * 0.5f, WINDOW_HEIGHT * 0.5f, 1, 0};
    VK2DCameraIndex testCamera = vk2dCameraCreate(cam);
    debugInit(window);

    // Framerate control
    double lastTime = SDL_GetPerformanceCounter();

    // Player
    double playerX = 140;
    double playerY = 140;
    double velocityX = 0;
    double velocityY = 0;
    double mouseX;
    double mouseY;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_PumpEvents();

        // Adjust for window size
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);
        cam.w = (float)windowWidth / 2;
        cam.h = (float)windowHeight / 2;
        vk2dCameraUpdate(testCamera, cam);
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        mouseX = (double)mx * 0.5;
        mouseY = (double)my * 0.5;

        // Move player
        double moveX = (double)keyboard[SDL_SCANCODE_RIGHT] - (double)keyboard[SDL_SCANCODE_LEFT];
        double moveY = (double)keyboard[SDL_SCANCODE_DOWN] - (double)keyboard[SDL_SCANCODE_UP];
        velocityX += moveX != 0 ? moveX * ACCELERATION : towardsZero(velocityX, FRICTION);
        velocityY += moveY != 0 ? moveY * ACCELERATION : towardsZero(velocityY, FRICTION);
        velocityX = clamp(velocityX, -MAX_VELOCITY, MAX_VELOCITY);
        velocityY = clamp(velocityY, -MAX_VELOCITY, MAX_VELOCITY);
        playerX += velocityX;
        playerY += velocityY;
        playerX = playerX > windowWidth * 0.5 ? -vk2dTextureWidth(playerTex) : (playerX < -vk2dTextureWidth(playerTex) ? windowWidth * 0.5 : playerX);
        playerY = playerY > windowHeight * 0.5 ? -vk2dTextureHeight(playerTex) : (playerY < -vk2dTextureHeight(playerTex) ? windowHeight * 0.5 : playerY);

        // All rendering must happen after this
        vk2dRendererStartFrame(clear);

        // Lock framerate to 60
        vk2dSleep((((double)SDL_GetPerformanceFrequency() / 60) - ((double)SDL_GetPerformanceCounter() - lastTime)) / SDL_GetPerformanceFrequency());
        lastTime = SDL_GetPerformanceCounter();

        // Draw 2D portions
        vk2dRendererLockCameras(testCamera);
        vk2dDrawTexture(playerTex, playerX - (vk2dTextureWidth(playerTex) / 2), playerY - (vk2dTextureHeight(playerTex) / 2));
        vk2dDrawCircle(mouseX, mouseY, 2);

        // Shadows
        vec4 shadowColour = {0, 0, 0, 1};
        vec2 lightSource = {playerX, playerY};
        vk2dRendererDrawShadows(shadows, shadowColour, lightSource);

        // Draw light orb on top of screen
        vk2dRendererSetTarget(shadowsTex);
        vk2dRendererSetColourMod(VK2D_BLACK);
        vk2dRendererClear();
        vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
        vk2dRendererSetBlendMode(VK2D_BLEND_MODE_SUBTRACT);
        vk2dDrawTexture(lightOrbTex, playerX - (vk2dTextureWidth(lightOrbTex) / 2), playerY - (vk2dTextureHeight(lightOrbTex) / 2));
        vk2dRendererSetBlendMode(VK2D_BLEND_MODE_BLEND);
        vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
        vk2dDrawTexture(shadowsTex, 0, 0);

        debugRenderOverlay();

        // End the frame
        vk2dRendererEndFrame();
    }

    // vk2dRendererWait must be called before freeing things
    vk2dRendererWait();
    debugCleanup();
    vk2dTextureFree(shadowsTex);
    vk2dTextureFree(lightOrbTex);
    vk2DShadowEnvironmentFree(shadows);
    vk2dTextureFree(playerTex);
    vk2dRendererQuit();
    SDL_DestroyWindow(window);
    return 0;
}