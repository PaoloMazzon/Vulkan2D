#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj[10];
} ubo;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    int cameraIndex;
} pushBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

out gl_PerVertex {
    vec4 gl_Position;
};

const mat4 UNIT_VIEW = {
    { 2.0, 0.0, 0.0, 0.0 },
    { 0.0, 2.0, 0.0, 0.0 },
    { 0.0, 0.0, 0.10101, 0.0 },
    { -1.0, -1.0, 0.191919, 1.0 }
};

void main() {
    mat4 camera = pushBuffer.cameraIndex == -1 ? UNIT_VIEW : ubo.viewproj[pushBuffer.cameraIndex];
    gl_Position = camera * pushBuffer.model * vec4(inPosition, 1.0);
    fragColor = inColor;
}