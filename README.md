![basic demo](https://i.imgur.com/InP0Sou.gif)

![gif](https://github.com/user-attachments/assets/ae0f05fd-7679-4061-ac89-dce66e163232)

![example](https://github.com/user-attachments/assets/186de874-6f2f-47c4-8a58-52c14148bb46)

![example](https://github.com/user-attachments/assets/9a43483b-0bdd-4511-98a3-9ca839b070d5)

Vulkan2D
========
[Vulkan2D](https://github.com/PaoloMazzon/Vulkan2D) is a 2D renderer using Vulkan and SDL2 primarily for C games. VK2D aims for an extremely
simple API, requiring no Vulkan experience to use. [Astro](https://github.com/PaoloMazzon/Astro)
and more recently [Sea of Clouds](https://devplo.itch.io/sea-of-clouds) internally use Vulkan2D for
rendering. My other projects [Bedlam](https://github.com/PaoloMazzon/Bedlam), 
[Spacelink](https://github.com/PaoloMazzon/Spacelink), and
[Peace & Liberty](https://github.com/PaoloMazzon/PeacenLiberty) also used Vulkan2D, although
a much older version of it. Check out the [quick-start](docs/QuickStart.md) guide.

Features
========

 + Simple and intuitive API built on top of SDL
 + Draw shapes/textures/3D models/arbitrary polygons to the screen or other textures
 + Fast, built with Vulkan 1.1 and doesn't require any device features (but it can make use of some)
 + Simple and fully-featured cameras, allowing for multiple concurrent cameras
 + Powerful and very simple shader interface
 + Simple access to the Vulkan implementation through `VK2D/VulkanInterface.h`
 + Hardware-accelerated 2D light and shadows
 + Easily load multiple resources in the background while your application gets prepared

Documentation
=============
Check out the [documentation website](https://paolomazzon.github.io/Vulkan2D/index.html).

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

    SDL2, 2.0.6+
    Vulkan 1.1+
    C11 + C standard library
    C++17 (VMA uses C++17)

Example
=======

By default the program automatically crashes on fatal errors, but you may specify Vulkan2D to not do
that and check for errors on your own. The following example uses default settings meaning that if there
is an error in VK2D, it will print the status to `vk2derror.txt` and quit. 

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

If you don't want VK2D to crash on errors you may specify that in the struct `VK2DStartupOptions` passed to
`vk2dRendererInit` and check for errors yourself with `vk2dStatus`, `vk2dStatusMessage`, and
`vk2dStatusFatal`. Any VK2D function can raise fatal errors but unless you pass bad pointers
to VK2D functions, they will not crash if there is a fatal error and will instead simply do
nothing.

Running the Examples
====================
All examples are tested to work on Windows and Ubuntu. The `CMakeLists.txt` at the root
directory will generate build systems for each example. Be sure to compile the test 
shader before running the `examples/main/` example with:

    glslc assets/test.frag -o assets/tex.frag.spv
    glslc assets/test.vert -o assets/tex.vert.spv

If you don't trust binary blobs you may also compile the binary shader blobs with the command

    genblobs.py colour.vert colour.frag instanced.vert instanced.frag model.vert model.frag shadows.vert shadows.frag spritebatch.comp

run from the `shaders/` folder (requires Python).

Roadmap
=======

 + Sprite batching
 + Built-in imgui support
 + Better multi-threaded loading support
 + Soft shadows
 + More flexibility with shader uniforms
 + 3D shaders
