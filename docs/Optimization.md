Optimization Tips and Tricks
============================
*Quick note*: if you're just making jam games or something really small
it is very likely that you don't even have many resources in your game
and these optimization techniques won't really yield you any noticeable
improvements. 

Use Sprite Sheets
-----------------
Should you draw a whole bunch of textures sequentially that all originate
from the same image the renderer will not need to bind a new image to the
draw command each time. To that end, it is also best to draw things that come
from the same sprite sheet at the same time but this likely already happens
in most game-dev environments and isn't really worth going out of your way
for in most cases.

Draw All Your Shapes/Textures at Once
-------------------------------------
Similar to the previous tip, drawing all of the shapes at once means that
the pipeline need not be bound each draw call. For example, if you call
drawing functions 500 times in a single frame but you do all of your texture
drawing before drawing shapes you could very well only be binding a pipeline
twice that frame (wireframe shapes and filled shapes use a different pipeline,
also switching render targets causes everything to get rebound).

Don't Switch Target Too Much
============================
Switching the render target causes a render pass to end and a new one to begin,
which can be a very hefty thing on some graphics cards in addition to forcing
everything to be rebound. Render targets are also persistent and you only need
to draw to them once, use a bool or something to make sure its only drawn once -- 
but remember, you cannot draw before `vk2dRendererStartFrame` and after `vk2dRendererEndFrame`,
including drawing to targets.

Check BuildOptions.h and Constants.h
====================================
There are lots of little optimizations you can get by just changing a few 
constants or build options. Highlights of these would be
 
 + `VK2D_UNIT_GENERATION` If you're not using the`vk2dRendererDrawRectangle*` or `vk2dRendererDrawCircle*` functions, disabling that option will net you a bit of video memory, host memory, and performance when starting up.
 + `VK2D_ENABLE_DEBUG` While the program won't crash if it fails to find validation layers, should the end user have validation layers installed they will be missing out on some performance
 + `VK2D_CIRCLE_VERTICES` This depends on what kind of game you're making. If you're making a pixel art game, you may not need very many vertices on circles to make a good looking circle, where high-resolution games will require lots of vertices. More vertices is poorer performance so play around with this one.

Also don't forget to build with `-O2` when in release mode.