![basic demo](https://i.imgur.com/InP0Sou.gif)

Vulkan2D
========
VK2D is a 2D renderer using Vulkan and SDL2 primarily for C games. VK2D aims for an extremely
simple API, requiring no Vulkan experience to use. As to prevent additional baggage, default 
shaders are SPIR-V binary blobs in `VK2D/Blobs.h`. If you don't trust binary blobs, feel free
to run `shaders/genblobs.py` yourself.

As it stands right now, VK2D is mostly ready to use, recently I made [Spacelink](https://github.com/PaoloMazzon/Spacelink).
Shader support is kind of dodgy but still usable, and since the switch to VMA was made there are
no hard technical barriers. It should also be stated that TrueType fonts will likely never be
supported simply because this is meant to be a really minimal 2D renderer, and there is no way
to easily support `.ttf` files without an external library (and I'm already unhappy with the VMA
requirement, but the performance and memory benefits are too great). 

Documentation
=============
Every file in VK2D is properly documented for doxygen, just run `doxygen Doxyconfig` and an html
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
`#define VK2D_ENABLE_DEBUG`. Don't forget to disable that before building for release as the
benefits in the test scene alone are a ~65% framerate boost.

Example
=======
Using the renderer is quite simple, but there are some things to be aware of. For the sake
of brevity, error checking is removed from the following example (always error check unless
you like random crashes).

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

Testing
=======
The CMakeLists.txt is there for testing purposes, not for use in projects. If you
wish to use this in your project, just drop the VK2D directory into your project
and build it with your project. It requires SDL2 and Vulkan to build.

Features
========
Right now it can do most things you would need in a game-dev environment but is still
in development as lacks a few very important things (check the TODO).

 + Draw rectangles/circles/lines/polygons easily
 + Load and draw textures (including only parts of images for sprite sheets and the lot)
 + Simple configuration options you can change at any point
 + Simple yet powerful camera and viewport controls
 + Colour modifier built-in to change colour of anything on the fly
 + Lots of build options to better optimize the renderer for your setup
 + Small amount of required features and limits to make the renderer work on most machines
 + Loads custom default shaders from file if available (can be disabled in BuildOptions.h)
 + Render to textures easily
 + Simple shader interface to utilize custom shaders
 + Different blend modes and easy to add new ones

TODO
====

 + PostFX passes like in RetroArch (load shaders as post-effects that get applied to the final image of the frame)
 + General optimizations

Warning
=======
Similar to Vulkan, VK2D does not check to see if you are passing garbage arguments (except for 
free functions). If you pass a null pointer, you can expect a segfault. Should VK2D return a null
pointer, an error message was most likely printed (and possibly logged to a file) to accompany it.
While not as specific as the Vulkan spec, every function has its arguments documented and states 
what each argument is so there should be no issues in that sense (use doxygen to generate the docs).
Also, most things in VK2D are for  internal use and the only real user-friendly thing is `vk2dRenderer*`
functions. That said, all of the "internal" bits are properly documented and you are free to string
together your own renderer with the nice abstractions VK2D provides. The only exception to this is the
`Initializers.h/c` file, which is really unnecessary but I like it.