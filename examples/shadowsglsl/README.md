![example](example.gif)

This demo is a demonstrates how to use Vulkan2D's hardware-accelerated 2D shadow
functions in a program. It is reasonably simple,

 1. Create a `ShadowEnvironment` with `vk2dShadowEnvironmentCreate`
 2. Add your edges to it with `vk2DShadowEnvironmentAddEdge`
 3. Flush those edges to the GPU with `vk2DShadowEnvironmentFlushVBO`
 4. Draw the shadows with `vk2dRendererDrawShadows`, providing a colour and light source
 position

This implementation is fairly simple and straight forward, for multiple overlapping light
sources you would need to `vk2dRendererDrawShadows` for each light source with a different
position/colour.