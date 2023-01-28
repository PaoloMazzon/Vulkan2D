![basic demo](https://i.imgur.com/InP0Sou.gif)

![another demo](assets/gif.gif)

Vulkan2D
========
VK2D is a 2D renderer using Vulkan and SDL2 primarily for C games. VK2D aims for an extremely
simple API, requiring no Vulkan experience to use. For examples of Vulkan2D in use, [Peace & Liberty](https://github.com/PaoloMazzon/PeacenLiberty)
and [Spacelink](https://github.com/PaoloMazzon/Spacelink) are two jam games I wrote in a weekend using Vulkan2D.

Documentation
=============
Every file in VK2D is properly documented for doxygen, run `doxygen` in `Vulkan2D/` and an html
folder will be created containing the documentation.

Usage
=====
There are two parts to building it with your project: you must build VK2D and also VMA since
VK2D needs VMA to function. Simply put, you'll likely need to do something like this in CMake:

    set(VMA_FILES VK2D/VulkanMemoryAllocator/src/vk_mem_alloc.h VK2D/VulkanMemoryAllocator/src/VmaUsage.cpp)
    file(GLOB VK2D_FILES VK2D/VK2D/*.c)
    ...
    include_directories(... VK2D/)
    add_executable(... ${VK2D_FILES} ${VMA_FILES})
   
You also need to link/include SDL2 and Vulkan but that is kind of implied. There will be no
instructions on how to do that here since there are much better guides elsewhere.

Build Options
-------------
Typically you would want to just include repo in your repo as a submodule (`git submodule add https://github.com/PaoloMazzon/Vulkan2D`)
to keep up to date on improvements in this repo. This is good, but you should also look at
and modify `VK2D/BuildOptions.h` before release or just not include that one and use your own or
something. There are several little optimization options, but most importantly there is
`#define VK2D_ENABLE_DEBUG`. Don't forget to disable that before building for release as the debug layers
are not present on most PCs.

Example
=======
Using the renderer is quite simple, but there are some things to be aware of. For the sake
of brevity, error checking is removed from the following example

    SDL_Window *window = SDL_CreateWindow("VK2D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
   	SDL_Event e;
   	VK2DRendererConfig config = {msaa_32x, sm_TripleBuffer, ft_Nearest};
    vk2dRendererInit(window, config);
    vec4 clearColour = {0.0, 0.0, 0.0, 1.0}; // Black
    bool stopRunning = false;
    
    // Load your resources
    
   	while (!stopRunning) {
   		while (SDL_PollEvent(&e))
   			if (e.type == SDL_QUIT)
   				stopRunning = true;
    
   		vk2dRendererStartFrame(clearColour);
   		
   		// Draw your things
   		
   		vk2dRendererEndFrame();
   	}
    
   	vk2dRendererWait();
   	
   	// Free your resources
   	
   	vk2dRendererQuit();
   	SDL_DestroyWindow(window);

SDL functions were included the example to remove any confusion on how the two integrate, but
SDL is very simple to use and won't be discussed here beyond don't forget to give the window the
`SDL_WINDOW_VULKAN` flag. You first initialize the renderer with some configuration (which can
be changed whenever), then at the start of the frame you call `vk2dRendererStartFrame()` and at
the end of the frame you call `vk2dRendererEndFrame()`. Other than that, its crucial that you
call `vk2dRendererWait()` before you start free your resources in case they're still in use by
the GPU (and of course call `vk2dRendererQuit()` after). Check the documentation on Renderer.h
to see all the fun stuff you can do.

Features
========
For a complete list of functions, generate the documentation and look at `VK2D/Renderer.h`

 + Simple and intuitive API built on top of SDL (you still control the window)
 + Draw shapes/textures/arbitrary polygons to the screen or other textures
 + Fast, built with Vulkan 1.2 without any device extension requirements
 + Simple and fully-featured camera
 + Multiple camera support
 + External SPIR-V shader support
 + Optional blend mode support

TODO
====

 + More detailed 3D rendering
 + Error checking on common user mistakes instead of relying on Vulkan layers