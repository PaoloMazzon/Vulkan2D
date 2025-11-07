![basic demo](https://i.imgur.com/InP0Sou.gif)

![gif](https://github.com/user-attachments/assets/ae0f05fd-7679-4061-ac89-dce66e163232)

![example](https://github.com/user-attachments/assets/186de874-6f2f-47c4-8a58-52c14148bb46)

![example](https://github.com/user-attachments/assets/9a43483b-0bdd-4511-98a3-9ca839b070d5)

Vulkan2D
========
[Vulkan2D](https://github.com/PaoloMazzon/Vulkan2D) is a 2D renderer using Vulkan and SDL3 primarily for C games. VK2D 
aims for an extremely simple API, requiring no Vulkan experience to use. [Astro](https://github.com/PaoloMazzon/Astro) 
and more recently [Sea of Clouds](https://devplo.itch.io/sea-of-clouds) internally use Vulkan2D for rendering. My other 
projects [Bedlam](https://github.com/PaoloMazzon/Bedlam), [Spacelink](https://github.com/PaoloMazzon/Spacelink), and 
[Peace & Liberty](https://github.com/PaoloMazzon/PeacenLiberty) also used Vulkan2D, although a much older version of it.
Check out the [quick-start](docs/QuickStart.md) guide.

Features
========

 + Simple, fast, and intuitive API built on top of SDL3
 + Draw shapes/textures/3D models/arbitrary polygons to the screen or to other textures
 + Fast, built with Vulkan 1.2 and doesn't require any device features (but it can make use of some)
 + Simple and fully-featured cameras, allowing for multiple concurrent cameras
 + Powerful and very simple shader interface
 + Simple access to the Vulkan implementation through `VK2D/VulkanInterface.h`
 + Hardware-accelerated 2D light and shadows

Documentation
=============
Check out the [documentation website](https://paolomazzon.github.io/Vulkan2D/index.html).

Usage
=====
The only officially supported way to use VK2D is via `add_subdirectory`. For the least painful
and most out-of-the-box way to use VK2D, include as a Git submodule and use `add_subdirectory`
in your CMakeLists.txt file.

```bash
git submodule add --recursive https://github.com/PaoloMazzon/Vulkan2D.git
```

```cmake
add_subdirectory(Vulkan2D/)
# your cmake script
target_link_libraries(my-game PRIVATE Vulkan2D SDL3::SDL3)
```

Example
=======

```c
SDL_Init(SDL_INIT_EVENTS);
SDL_Window *window = SDL_CreateWindow("VK2D", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
SDL_Event e;
VK2DRendererConfig config = {
    .msaa = VK2D_MSAA_32X, 
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

Running the Examples
====================
All examples are tested to work on Windows and Ubuntu. The `CMakeLists.txt` at the root
directory will generate build systems for each example. Be sure to compile the test 
shader before running the `examples/main/` example with:

    glslc assets/test.frag -o assets/test.frag.spv
    glslc assets/test.vert -o assets/test.vert.spv

Roadmap
=======

 + Stability improvements
 + Custom allocator support
 + Better shader support
 + GPU readback/screenshots
