VK2D
====
VK2D is a 2D renderer using Vulkan and SDL2 in C. It aims to be simple and abstract most
of the nasty Vulkan things away and make it relatively simple.

As to prevent additional baggage, default shaders are SPIR-V binary blobs in `VK2D/Blobs.h`.
If you don't trust binary blobs, feel free to run `shaders/genblobs.py` yourself (that being
the tool I wrote to generate the blobs header file, its a super simple ~100 line python script).

Documentation
=============
Every file in VK2D is properly documented for doxygen, just run `doxygen Doxyconfig` and an html
folder will be created containing the documentation.

Usage
=====
The CMakeLists.txt is there for testing purposes, not for use in projects. If you
wish to use this in your project, just drop the VK2D directory into your project
and build it with your project. It requires SDL2 and Vulkan to build.

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