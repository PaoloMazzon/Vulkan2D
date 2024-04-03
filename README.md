![basic demo](https://i.imgur.com/InP0Sou.gif)

![another demo](assets/gif.gif)

![another demo](examples/retrolook/example.gif)

![another demo](examples/shadows/example.gif)

Vulkan2D
========
Vulkan2D is a 2D renderer using Vulkan and SDL2 primarily for C games. VK2D aims for an extremely
simple API, requiring no Vulkan experience to use. [Astro](https://github.com/PaoloMazzon/Astro)
and more recently [Bedlam](https://github.com/PaoloMazzon/Bedlam) internally use Vulkan2D for
rendering. My other projects [Spacelink](https://github.com/PaoloMazzon/Spacelink) and
[Peace & Liberty](https://github.com/PaoloMazzon/PeacenLiberty) also used Vulkan2D, although
a much older version of it. Check out the [quick-start](docs/QuickStart.md) guide.

Features
========

 + Simple and intuitive API built on top of SDL
 + Draw shapes/textures/3D models/arbitrary polygons to the screen or other textures
 + Fast, built with Vulkan 1.2 and doesn't require any device features (but it can make use of some)
 + Simple and fully-featured cameras, allowing for multiple concurrent cameras
 + Powerful and very simple shader interface
 + Simple access to the Vulkan implementation through `VK2D/VulkanInterface.h`
 + Easily load multiple resources in the background while your application gets prepared

Documentation
=============
Every file in VK2D is properly documented for doxygen, run `doxygen` in `Vulkan2D/` or check
the header files. Additionally, there is a few miscellaneous pieces of documentation,
[quick-start](docs/QuickStart.md) and [cameras](docs/Cameras.md). As a general overview,

Usage
=====
There are two parts to building it with your project: you must build VK2D and also VMA since
VK2D needs VMA to function. You'll likely need to do something like this in CMake:

```cmake
set(VMA_FILES VK2D/VulkanMemoryAllocator/src/vk_mem_alloc.h VK2D/VulkanMemoryAllocator/src/VmaUsage.cpp)
file(GLOB VK2D_FILES VK2D/VK2D/*.c)
...
include_directories(... Vulkan2D/ VulkanMemoryAllocator/src/)
add_executable(... ${VK2D_FILES} ${VMA_FILES})
```

Vulkan2D also requires the following external dependencies:

    SDL2, 2.0+
    Vulkan 1.2+
    C11 + C standard library
    C++17 (VMA uses C++17)

Example
=======
Using the renderer is quite simple, but there are some things to be aware of. For the sake
of brevity, error checking is removed from the following example

```c
SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
SDL_Event e;
VK2DRendererConfig config = {VK2D_MSAA_32X, VK2D_SCREEN_MODE_TRIPLE_BUFFER, VK2D_FILTER_TYPE_NEAREST};
vk2dRendererInit(window, config, NULL);
vec4 clearColour;
vk2dColourHex(clearColour, "#59d9d7");
bool stopRunning = false;

// Load your resources

while (!stopRunning) {
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
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
```

Running the Examples
====================
All examples are tested to work on Windows and Ubuntu. The `CMakeLists.txt` at the root
directory will generate build systems for each example. Be sure to compile the test 
shader before running the `examples/main/` example with:

    glslc assets/test.frag -o assets/tex.frag.spv
    glslc assets/test.vert -o assets/tex.vert.spv

You may also compile the binary shader blobs with the long command

    genblobs.py colour.vert colour.frag instanced.vert instanced.frag model.vert model.frag shadows.vert shadows.frag tex.vert tex.frag

run from the `shaders/` folder (requires Python).

TODO
====

 + 3D shaders
