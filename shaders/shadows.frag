#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(
        fragColor.r,
        fragColor.g,
        fragColor.b,
        fragColor.a
    );
}