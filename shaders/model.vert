#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj[10];
} ubo;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    uint textureIndex;
    int cameraIndex;
} pushBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.viewproj[pushBuffer.cameraIndex] * pushBuffer.model * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}