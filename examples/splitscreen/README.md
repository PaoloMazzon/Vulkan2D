![example](example.gif)

In this demo, multiple Vulkan2D cameras are used at the same time to create
a split-screen effect. Use the mouse to grab any of the individual screens
and move it around.

This effect is achieved through creating multiple cameras and setting up their
`xOnScreen`, `yOnScreen`, `wOnScreen`, and `hOnScreen` such that each of the
4 cameras are in each corner of the window.