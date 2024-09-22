#version 450
#extension GL_ARB_separate_shader_objects : enable

struct DrawInstance {
    vec4 texturePos;
    vec4 colour;
    uint textureIndex;
    mat4 model;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj[10];
} ubo;

layout(set = 3, binding = 3) readonly buffer drawInstances {
    DrawInstance draws[];
};

layout(set = 4, binding = 4) uniform UserData {
    float colour;
} userData;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColour;

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
    newPos.x = vertices[gl_VertexIndex].x * pushBuffer.textureCoords.z;
    newPos.y = vertices[gl_VertexIndex].y * pushBuffer.textureCoords.w;
    gl_Position = ubo.viewproj * pushBuffer.model * vec4(newPos, 1.0, 1.0);
    fragTexCoord.x = pushBuffer.textureCoords.x + (texCoords[gl_VertexIndex].x * pushBuffer.textureCoords.z);
    fragTexCoord.y = pushBuffer.textureCoords.y + (texCoords[gl_VertexIndex].y * pushBuffer.textureCoords.w);
    /*float val = (sin(userData.colour) + 1) / 2;
    fragColour = vec4(val, val, val, 1);*/
    fragColour = vec4(1, 1, 1, 1);
}