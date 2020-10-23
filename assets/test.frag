#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
} pushBuffer;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 colour = texture(texSampler, fragTexCoord);
    outColor = colour;
}