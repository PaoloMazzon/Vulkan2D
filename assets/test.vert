#version 450
#extension GL_ARB_separate_shader_objects : enable

const vec2 VERTICES[] = {
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 1.0f),
    vec2(0.0f, 0.0f),
};

const vec2 TEX_COORDS[] = {
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 1.0f),
    vec2(0.0f, 0.0f),
};

layout(push_constant) uniform PushBuffer {
    int cameraIndex;
    uint textureIndex;
    vec4 texturePos;
    vec4 colour;
    mat4 model;
} push;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj[10];
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};


layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColour;

layout(set = 3, binding = 3) uniform UserData {
    float colour;
} userData;

void main() {
    vec2 newPos;
    newPos.x = VERTICES[gl_VertexIndex].x * push.texturePos.z;
    newPos.y = VERTICES[gl_VertexIndex].y * push.texturePos.w;
    gl_Position = ubo.viewproj[push.cameraIndex] * push.model * vec4(newPos, 1.0, 1.0);
    fragTexCoord.x = push.texturePos.x + (TEX_COORDS[gl_VertexIndex].x * push.texturePos.z);
    fragTexCoord.y = push.texturePos.y + (TEX_COORDS[gl_VertexIndex].y * push.texturePos.w);
    fragColour = vec4(1, 1, 1, 1);
}