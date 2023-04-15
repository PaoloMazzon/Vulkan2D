![example](example.gif)

This demo shows off the instancing feature in Vulkan2D which makes drawing a massive
amount of the same texture at once possible performance-wise. The example creates a
large list of `VK2DDrawInstace` at startup with random features and moves everything
in the list every frame. They tend to move to the top-left of the window because the
random number generator is trash. The gif above has 6399 instances being rendered
each frame without any optimizations turned on.