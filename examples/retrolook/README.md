![example](example.gif)

This is a demo scene meant to show a method of giving your program a retro feel.
The scene is rendered to a texture target with the PS1's resolution, then integer
up-scaled to the center of the screen. 1x msaa is used because retro hardware
wouldn't have access to anything more than that, and you could take it a step
further by locking the framerate.