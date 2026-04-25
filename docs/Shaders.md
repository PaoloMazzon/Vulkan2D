# Shaders
Shaders are an advanced tool to allow users to create hardware-accelerated effects on images they draw to
the screen. In Vulkan2D, shaders are required to be written in [Slang](https://shader-slang.org/) and
you may only provide a fragment (pixel) shader. 
Shaders in VK2D can be accessed via the `vk2dSlangLoad` and `vk2dSlangFrom` functions and eventually
used with `vk2dRendererDrawShader`. Vulkan2D will take [Slang](https://shader-slang.org/) source code 
and compile it when you call those functions, additionally validating that they are in the correct format. 
Shaders loaded in this way are expected to have a few bindings,

 + Push constant buffer with a very specific type (see below), named `push` of type `Push`.
 + Sampler at binding 1, set 1, named `sampler` of type `SamplerState`.
 + Texture array at binding 2, set 2, named `textures` of type `Texture2D<float4>`.
 + Optional user input data at binding 3, set 3, name doesn't matter here as long as it's a `ConstantBuffer`.

Vulkan2D will check that all of those types are present and in the right binding in your shader. **The names
of the bindings are required.** 

## User input data
Users may provide their own input constants to shaders via the addition of another binding at 3,3, ie

```c
[[vk::binding(3,3)]] ConstantBuffer<UserData> userData;
```

If you do set one of these, be aware they are aligned as std430, basically meaning they are vec4 (16-byte)
aligned. When you are using a user data block, you are expected to give a buffer identically structured from
the C side when calling `vk2dRendererDrawShader`.

For example, if you had in your shader

```c
struct UserData {
    float time;
    VK2DVec4 colour;
};
[[vk::binding(3,3)]] ConstantBuffer<UserData> userData;
```

the C-equivalent struct would be

```c
struct UserData {
    float time;
    VK2DVec3 _padding0;
    VK2DVec4 colour;
};
```

Vulkan2D will find the size of your user data struct in the shader automatically.

## Example Shader
This is a basic example shader that you could drop in your project and use as a base for your effect
development.

```c
// Per-drawcall values
struct Push {
    // Not needed in pixel shader
    int cameraIndex;

    // Index of the texture to draw
    uint textureIndex;

    // Where in texture to draw (x, y, w, h)
    float4 texturePos;

    // Current colour modifier
    float4 colour;

    // Not needed in pixel shader
    float4x4 model;
};
[[vk::push_constant]] Push push;

// Texture and sampler
[[vk::binding(1,1)]] SamplerState sampler;
[[vk::binding(2,2)]] Texture2D<float4> textures[];

// Optional user data
struct UserData {
    float time; // whatever you want
};
[[vk::binding(3,3)]] ConstantBuffer<UserData> userData;

// This is your main function
[shader("pixel")]
float4 PixelShader(float2 uv) {
    float4 uv_colour = textures[push.textureIndex].Sample(sampler, uv);
    return push.colour * uv_colour;
}

```