#define SDL_MAIN_HANDLED
#include <SDL3/SDL_vulkan.h>
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

typedef struct Light_t {
    vec2 pos;
    vec4 colour;
} Light;

const vec2 POLY_1[] = {
        {13, 74},
        {99, 35},
        {152, 61},
        {151, 108},
        {74, 122},
};
const int POLY_1_COUNT = sizeof(POLY_1) / sizeof(vec2);

const vec2 POLY_2[] = {
        {216, 79},
        {230, 45},
        {331, 52},
        {365, 127},
        {363, 176},
        {293, 133},
};
const int POLY_2_COUNT = sizeof(POLY_2) / sizeof(vec2);

const vec2 POLY_3[] = {
        {340, 228},
        {361, 250},
        {332, 248},
};
const int POLY_3_COUNT = sizeof(POLY_3) / sizeof(vec2);

const vec2 POLY_4[] = {
        {232, 178},
        {285, 223},
        {275, 275},
        {140, 230},
        {139, 226},
};
const int POLY_4_COUNT = sizeof(POLY_4) / sizeof(vec2);

const vec2 POLY_5[] = {
        {42, 165},
        {88, 178},
        {93, 220},
        {22, 218},
};
const int POLY_5_COUNT = sizeof(POLY_5) / sizeof(vec2);

const vec2 MOVING_POLY[] = {
        {-10, 10},
        {10, 10},
        {10,-10},
        {-10, -10},
};
const int MOVING_POLY_COUNT = sizeof(MOVING_POLY) / sizeof(vec2);

const vec2 *POLYGONS[] = {POLY_1, POLY_2, POLY_3, POLY_4, POLY_5, MOVING_POLY};
const int POLYGON_COUNTS[] = {POLY_1_COUNT, POLY_2_COUNT, POLY_3_COUNT, POLY_4_COUNT, POLY_5_COUNT, MOVING_POLY_COUNT};
const int POLYGON_COUNT = 6;

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
    VK2DRendererConfig config = {VK2D_MSAA_1X, VK2D_SCREEN_MODE_IMMEDIATE, VK2D_FILTER_TYPE_NEAREST};
    vec4 clear = {0.0, 0.5, 1.0, 1.0};
    VK2DStartupOptions options = {
            .quitOnError = true,
            .errorFile = "vk2derror.txt",
            .enableDebug = true,
            .stdoutLogging = true,
    };
    int32_t error = vk2dRendererInit(window, config, &options);

    if (error < 0)
        return -1;

    VK2DCameraSpec defcam = {VK2D_CAMERA_TYPE_DEFAULT, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 1, 0};
    vk2dRendererSetCamera(defcam);

    // Setup lights
    const int lightCount = 8;
    const int playerLight = 0;
    Light *lights = malloc(sizeof(struct Light_t) * lightCount);
    for (int i = 1; i < lightCount; i++) {
        lights[i].colour[3] = 0.95;
        lights[i].colour[0] = vk2dRandom(0, 1);
        lights[i].colour[1] = vk2dRandom(0, 1);
        lights[i].colour[2] = vk2dRandom(0, 1);
        lights[i].pos[0] = vk2dRandom(0, 400);
        lights[i].pos[1] = vk2dRandom(0, 300);
    }
    lights[playerLight].colour[3] = 0.8;
    lights[playerLight].colour[0] = 1;
    lights[playerLight].colour[1] = 1;
    lights[playerLight].colour[2] = 1;

    // Load Some assets
    VK2DTexture playerTex = vk2dTextureLoad("assets/caveguy.png");
    VK2DTexture lightOrbTex = vk2dTextureLoad("assets/whitelight.png");
    VK2DTexture lightTex = vk2dTextureCreate(400, 300);
    VK2DPolygon polygons[POLYGON_COUNT];
    VK2DPolygon polygonOutlines[POLYGON_COUNT];
    const int MOUSE_POLYGON_INDEX = 5;
    for (int i = 0; i < POLYGON_COUNT; i++) {
        polygons[i] = vk2dPolygonCreate(POLYGONS[i], POLYGON_COUNTS[i]);
        polygonOutlines[i] = vk2dPolygonCreateOutline(POLYGONS[i], POLYGON_COUNTS[i]);
    }
    VK2dShadowEnvironment shadows = vk2dShadowEnvironmentCreate();

    // Add light edges
    VK2dShadowObject mouseShadowObject;
    for (int i = 0; i < POLYGON_COUNT; i++) {
        if (i == MOUSE_POLYGON_INDEX)
            mouseShadowObject = vk2dShadowEnvironmentAddObject(shadows);
        for (int vertex = 0; vertex < POLYGON_COUNTS[i]; vertex++) {
            int prevIndex = vertex == 0 ? POLYGON_COUNTS[i] - 1 : vertex - 1;
            vk2dShadowEnvironmentAddEdge(
                    shadows,
                    POLYGONS[i][prevIndex][0],
                    POLYGONS[i][prevIndex][1],
                    POLYGONS[i][vertex][0],
                    POLYGONS[i][vertex][1]
            );
        }
    }
    vk2dShadowEnvironmentFlushVBO(shadows);


    // Camera
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
    double mousePolyRot = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
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
        float mx, my;
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
        mousePolyRot += VK2D_PI * 0.01;

        // All rendering must happen after this
        vk2dRendererStartFrame(clear);

        // Lock framerate to 60
        vk2dSleep((((double)SDL_GetPerformanceFrequency() / 60) - ((double)SDL_GetPerformanceCounter() - lastTime)) / SDL_GetPerformanceFrequency());
        lastTime = SDL_GetPerformanceCounter();

        // Draw the lights
        vk2dRendererLockCameras(testCamera);
        vk2dShadowEnvironmentObjectUpdate(shadows, mouseShadowObject, mouseX, mouseY, 1, 1, mousePolyRot, 0, 0);
        lights[playerLight].pos[0] = playerX;
        lights[playerLight].pos[1] = playerY;
        for (int i = 0; i < lightCount; i++) {
            // Prepare light texture
            vk2dRendererSetTarget(lightTex);
            vk2dRendererEmpty();

            // Draw light orb
            vk2dRendererSetColourMod(lights[i].colour);
            vk2dDrawTexture(lightOrbTex, lights[i].pos[0] - (vk2dTextureWidth(lightOrbTex) / 2), lights[i].pos[1] - (vk2dTextureHeight(lightOrbTex) / 2));
            vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);

            // Subtract shadows from it
            vk2dRendererSetBlendMode(VK2D_BLEND_MODE_SUBTRACT);
            vk2dRendererDrawShadows(shadows, VK2D_BLACK, lights[i].pos);
            vk2dRendererSetBlendMode(VK2D_BLEND_MODE_BLEND);

            // Draw it to the screen
            vk2dRendererSetTarget(VK2D_TARGET_SCREEN);
            vk2dDrawTexture(lightTex, 0, 0);
        }

        // Draw player and walls on top of lights
        for (int i = 0; i < POLYGON_COUNT; i++) {
            if (i != MOUSE_POLYGON_INDEX) {
                vk2dRendererDrawPolygon(polygons[i], 0, 0, true, 0, 1, 1, 0, 0, 0);
                vk2dRendererSetColourMod(VK2D_BLACK);
                vk2dRendererDrawPolygon(polygonOutlines[i], 0, 0, false, 3, 1, 1, 0, 0, 0);
                vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
            } else {
                vk2dRendererDrawPolygon(polygons[i], mouseX, mouseY, true, 0, 1, 1, mousePolyRot, 0, 0);
                vk2dRendererSetColourMod(VK2D_BLACK);
                vk2dRendererDrawPolygon(polygonOutlines[i], mouseX, mouseY, false, 3, 1, 1, mousePolyRot, 0, 0);
                vk2dRendererSetColourMod(VK2D_DEFAULT_COLOUR_MOD);
            }
        }
        vk2dDrawTexture(playerTex, playerX - (vk2dTextureWidth(playerTex) / 2), playerY - (vk2dTextureHeight(playerTex) / 2));

        debugRenderOverlay();

        // End the frame
        vk2dRendererEndFrame();
    }

    // vk2dRendererWait must be called before freeing things
    vk2dRendererWait();
    debugCleanup();
    free(lights);
    for (int i = 0; i < POLYGON_COUNT; i++) {
        vk2dPolygonFree(polygons[i]);
        vk2dPolygonFree(polygonOutlines[i]);
    }
    vk2dTextureFree(lightTex);
    vk2dTextureFree(lightOrbTex);
    vk2dShadowEnvironmentFree(shadows);
    vk2dTextureFree(playerTex);
    vk2dRendererQuit();
    SDL_DestroyWindow(window);
    return 0;
}