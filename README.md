![basic demo](https://i.imgur.com/InP0Sou.gif)

![gif](https://github.com/user-attachments/assets/ae0f05fd-7679-4061-ac89-dce66e163232)

![example](https://github.com/user-attachments/assets/186de874-6f2f-47c4-8a58-52c14148bb46)

![example](https://github.com/user-attachments/assets/9a43483b-0bdd-4511-98a3-9ca839b070d5)

# Vulkan2D
[Vulkan2D](https://github.com/PaoloMazzon/Vulkan2D) is a 2D renderer using Vulkan and SDL3 primarily for C games. VK2D 
aims for an extremely simple API, requiring no Vulkan experience to use. This project
initially started out aiming to be a more feature complete drop-in replacement for
the SDL renderer, but since then has grown in scope. 

## Features

 + Intuitive API built on top of SDL3
 + Very fine-grained camera control
 + Direct access to Vulkan (at your own risk)
 + Hardware-accelerated 2D light and shadows
 + High-performance sprite batching
 + Only requires Vulkan 1.2

## Documentation
Check out the [documentation website](https://paolomazzon.github.io/Vulkan2D/index.html).

Vulkan2D headers are all Doxygen commented and the documentation website is automatically
generated from them (this also means the documentation will likely show up in your editor
of choice!).

## Usage
The only officially supported way to use VK2D is via `add_subdirectory`. For the least painful
and most out-of-the-box way to use VK2D, include as a Git submodule and use `add_subdirectory`
in your CMakeLists.txt file.

```bash
git submodule add --recursive https://github.com/PaoloMazzon/Vulkan2D.git
```

```cmake
add_subdirectory(Vulkan2D/)
add_executable(my-game main.c)
target_link_libraries(my-game PRIVATE Vulkan2D)
```

If you want to use your own SDL version instead of whatever's latest (which VK2D defaults to)
then you can do so with

```cmake
set(VK2D_BUILD_SDL OFF)
add_subdirectory(Vulkan2D/)
```

## Example

```c
SDL_Init(SDL_INIT_EVENTS);
SDL_Window *window = SDL_CreateWindow("VK2D", 800, 600, SDL_WINDOW_VULKAN);
SDL_Event e;
VK2DRendererConfig config = {
    .msaa = VK2D_MSAA_8X, 
    .screenMode = VK2D_SCREEN_MODE_IMMEDIATE, 
    .filterMode = VK2D_FILTER_TYPE_NEAREST
};
vk2dRendererInit(window, config, NULL);
vec4 clearColour;
vk2dColourHex(clearColour, "#59d9d7");
bool stopRunning = false;

// Load your resources

while (!stopRunning) {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            stopRunning = true;
        }
    }

    vk2dRendererStartFrame(clearColour);

    // Draw your things

    vk2dRendererEndFrame();
}

vk2dRendererWait();

// Free your resources

vk2dRendererQuit();
SDL_DestroyWindow(window);
SDL_Quit();
```

## Running the Examples
Be sure to compile the test shader before running the `examples/main/` example with:

    glslc assets/test.frag -o assets/test.frag.spv
    glslc assets/test.vert -o assets/test.vert.spv

Then build the `CMakeLists.txt` file with the flag `-DVK2D_BUILD_EXAMPLES:BOOL=ON`.

## Roadmap

 + Stability improvements
 + Custom allocator support
 + Better shader support
 + Texture readback
