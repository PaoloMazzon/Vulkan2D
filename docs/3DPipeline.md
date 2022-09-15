3D Rendering Pipeline
=====================
VK2D is a 2D renderer meant for 2D games, but it provides basic functionality to render 3D models in case
your 2D project wants some 3D models in it somewhere.

Overview
--------
There are a few steps involved in rendering models.

 1. Create a camera in either perspective or orthographic rendering mode
 2. Load a 3D model
 3. Enable/switch to the 3D camera then draw it