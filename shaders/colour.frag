#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    uint cameraIndex;
} pushBuffer;

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(
            fragColor.r  * pushBuffer.colourMod.r,
            fragColor.g  * pushBuffer.colourMod.g,
            fragColor.b  * pushBuffer.colourMod.b,
            fragColor.a  * pushBuffer.colourMod.a);
}