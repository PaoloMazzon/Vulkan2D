#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PushBuffer {
    vec2 lightSource; // Where the light is
    vec2 _alignment; // ignore
    vec4 colour; // Colour of the shadows
} pushBuffer;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(pushBuffer.colour);
}