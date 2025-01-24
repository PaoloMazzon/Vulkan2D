#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

struct DrawInstance {
    vec4 texturePos;
    vec4 colour;
    uint textureIndex;
    mat4 model;
};

layout(push_constant) uniform PushBuffer {
    int cameraIndex;
} push;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 cameras[10];
} ubo;

layout(set = 3, binding = 3) readonly buffer ObjectBuffer{
    DrawInstance objects[];
} objectBuffer;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColour;
layout(location = 3) out uint textureIndex;

vec2 vertices[] = {
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 1.0f),
    vec2(0.0f, 0.0f),
};

vec2 texCoords[] = {
    vec2(0.0f, 0.0f),
    vec2(1.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(0.0f, 1.0f),
    vec2(0.0f, 0.0f),
};

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    vec2 newPos;
    int instance = gl_VertexIndex / 6;
    int vertexIndex = gl_VertexIndex % 6;
    newPos.x = vertices[vertexIndex].x * objectBuffer.objects[instance].texturePos.z;
    newPos.y = vertices[vertexIndex].y * objectBuffer.objects[instance].texturePos.w;
    gl_Position = ubo.cameras[push.cameraIndex] * objectBuffer.objects[instance].model * vec4(newPos, 1.0, 1.0);
    fragTexCoord.x = objectBuffer.objects[instance].texturePos.x + (texCoords[vertexIndex].x * objectBuffer.objects[instance].texturePos.z);
    fragTexCoord.y = objectBuffer.objects[instance].texturePos.y + (texCoords[vertexIndex].y * objectBuffer.objects[instance].texturePos.w);
    fragColour = objectBuffer.objects[instance].colour;
    textureIndex = objectBuffer.objects[instance].textureIndex;
}