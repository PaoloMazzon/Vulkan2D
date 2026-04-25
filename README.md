![basic demo](https://i.imgur.com/InP0Sou.gif)

![gif](https://github.com/user-attachments/assets/ae0f05fd-7679-4061-ac89-dce66e163232)

![example](https://github.com/user-attachments/assets/186de874-6f2f-47c4-8a58-52c14148bb46)

![example](https://github.com/user-attachments/assets/499ce5ce-5783-4bfe-a16c-e694df15944c)

# Vulkan2D
[Vulkan2D](https://github.com/PaoloMazzon/Vulkan2D) is a 2D renderer using Vulkan and SDL3 primarily for C games. VK2D 
aims for an extremely simple API, requiring no Vulkan experience to use. This project
initially started out aiming to be a more feature complete drop-in replacement for
the SDL renderer, but since then has grown in scope. Vulkan2D requires C++20, C11,
and Vulkan 1.2+.

## Features

 + Intuitive API built on top of SDL3
 + Very fine-grained camera control
 + Direct access to Vulkan (at your own risk)
 + Hardware-accelerated 2D light and shadows
 + High-performance sprite batching
 + Simple and user-friendly shader interface
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

You may also disable shader support entirely by supplying 

```cmake
set(VK2D_DISABLE_SHADERS ON)
add_subdirectory(Vulkan2D/)
```

This is an option because compiling Slang is a lengthy process and for projects that don't
require them this can save users a significant amount of time.

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
VK2DVec4 clearColour;
vk2dColourHex(clearColour, "#59d9d7");
bool stopRunning = false;

// Load your resources

while (!stopRunning) {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            stopRunning = true;
        }
    }

    // Draw your things

    vk2dRendererPresent();
}

vk2dRendererWait();

// Free your resources

vk2dRendererQuit();
SDL_DestroyWindow(window);
SDL_Quit();
```

To run the examples in the `examples/` folder, build the `CMakeLists.txt` file with the flag `-DVK2D_BUILD_EXAMPLES:BOOL=ON`.

## Roadmap

 + Texture readback
 + Stability improvements
 + Remove or improve 3D
 + API lock


