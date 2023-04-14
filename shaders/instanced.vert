#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
} ubo;

struct SingleDraw {
    vec2 pos;
    vec4 uv;
    vec4 colour;
};

layout(set = 3, binding = 3, scalar) readonly buffer DrawInstance {
    SingleDraw draws[];
} drawInstance;

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
    newPos.x = vertices[gl_VertexIndex].x * drawInstance.draws[gl_InstanceIndex].uv.z;
    newPos.y = vertices[gl_VertexIndex].y * drawInstance.draws[gl_InstanceIndex].uv.w;
    gl_Position = ubo.viewproj * pushBuffer.model * vec4(newPos + drawInstance.draws[gl_InstanceIndex].pos, 1.0, 1.0);
    fragTexCoord.x = drawInstance.draws[gl_InstanceIndex].uv.x + (texCoords[gl_VertexIndex].x * drawInstance.draws[gl_InstanceIndex].uv.z);
    fragTexCoord.y = drawInstance.draws[gl_InstanceIndex].uv.y + (texCoords[gl_VertexIndex].y * drawInstance.draws[gl_InstanceIndex].uv.w);
    fragColour = drawInstance.draws[gl_InstanceIndex].colour;
}