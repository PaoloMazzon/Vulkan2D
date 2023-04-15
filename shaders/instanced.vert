#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
} ubo;

layout(location = 0) in vec4 instanceTexturePos;
layout(location = 1) in vec4 instanceColour;
layout(location = 2) in vec2 instancePos;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    vec4 textureCoords;
} pushBuffer;

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
    newPos.x = vertices[gl_VertexIndex].x * instanceTexturePos.z;
    newPos.y = vertices[gl_VertexIndex].y * instanceTexturePos.w;
    gl_Position = ubo.viewproj * pushBuffer.model * vec4(newPos + instancePos.xy, 1.0, 1.0);
    fragTexCoord.x = instanceTexturePos.x + (texCoords[gl_VertexIndex].x * instanceTexturePos.z);
    fragTexCoord.y = instanceTexturePos.y + (texCoords[gl_VertexIndex].y * instanceTexturePos.w);
    fragColour = instanceColour;
}