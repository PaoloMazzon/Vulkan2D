#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    vec4 textureCoords;
} pushBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec2 newPos;
    newPos.x = inPosition.x * pushBuffer.textureCoords.z;
    newPos.y = inPosition.y * pushBuffer.textureCoords.w;
    gl_Position = ubo.proj * ubo.view * pushBuffer.model * vec4(newPos, 1.0, 1.0);
    fragColor = inColor;
    fragTexCoord.x = pushBuffer.textureCoords.x + (inTexCoord.x * pushBuffer.textureCoords.z);
    fragTexCoord.y = pushBuffer.textureCoords.y + (inTexCoord.y * pushBuffer.textureCoords.w);
}