![basic demo](https://i.imgur.com/zz4GT5D.gif)

VK2D
====
VK2D is a 2D renderer using Vulkan and SDL2 in C. It aims to be simple and abstract most
of the nasty Vulkan things away and make it relatively simple.

As to prevent additional baggage, default shaders are SPIR-V binary blobs in `VK2D/Blobs.h`.
If you don't trust binary blobs, feel free to run `shaders/genblobs.py` yourself (that being
the tool I wrote to generate the blobs header file, its a super simple ~100 line python script).

Right now, my goal is to have lots of nice little abstractions for the renderer that still
expose the Vulkan API, and the VK2D renderer itself require no Vulkan interaction at all. This
means that if you wish to use the abstractions like `VK2DLogicalDevice` or `VK2DImage` you
would still be required to use Vulkan, but ideally `VK2DRenderer` would not have the user touch
the Vulkan API at all.

Documentation
=============
Every file in VK2D is properly documented for doxygen, just run `doxygen Doxyconfig` and an html
folder will be created containing the documentation.

Usage
=====
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

 + Draw polygons easily
 + Load and draw textures (including only parts of images for spritesheets and the lot)
 + Simple configuration options you can change at any point (you need not restart to change MSAA, for example)
 + Simple yet powerful camera and viewport controls
 + Colour modifier that can be changed mid-frame in order to make things transparent, colour white things, and so on
 + Build options to make the renderer play with your setup
 + Small amount of required features and limits to make the renderer work on most machines
 + Render to textures and/or the screen
 + Loads custom default shaders from file if available (can be disabled in BuildOptions.h)

TODO
====

 + Custom shaders/pipelines (can already load custom default shaders)
 + ***Heavy duty*** optimizations
  + Make descriptor sets given to each texture/polygon rather than created dynamically

Performance wise, the renderer is pretty good but not there yet. It struggles to render 1000 textures
on an integrated GPU @ 60fps (which is more than you'll probably ever need, but we don't strive for 
"good enough" around here). It is also worth noting that while testing rendering 1000 textures, the CPU
usage was non-existent and the GPU was the one not keeping up, so even in its current state, the renderer
is not about to impede your game's logic very much.

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