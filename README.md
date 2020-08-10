VK2D
====
VK2D is a 2D renderer using Vulkan and SDL2 in C. It aims to be simple and abstract most
of the nasty Vulkan things away and make it relatively simple.

As to prevent additional baggage, default shaders are SPIR-V binary blobs in `VK2D/Blobs.h`.
If you don't trust binary blobs, feel free to run `shaders/genblobs.py` yourself (that being
the tool I wrote to generate the blobs header file, its a super simple ~100 line python script).

Documentation
=============
Every file in VK2D is properly documented for doxygen.

Usage
=====
The CMakeLists.txt is there for testing purposes, not for use in projects. If you
wish to use this in your project, just drop the VK2D directory into your project
and build it with your project. It requires SDL2 and Vulkan to build.