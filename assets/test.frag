#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#define pi 3.141592635

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

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColour;

layout(set = 1, binding = 1) uniform sampler texSampler;
layout(set = 2, binding = 2) uniform texture2D tex[];

layout(location = 0) out vec4 outColor;

layout(set = 3, binding = 3) uniform UserData {
    float colour;
} userData;

void main() {
    float a = 1;
    float b = 0.3;
    float val1 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour)) + (((a - b) / 2) + b);
    float val2 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour + (pi * 0.25))) + (((a - b) / 2) + b);
    float val3 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour + (pi * 0.5))) + (((a - b) / 2) + b);
    vec4 colour = texture(sampler2D(tex[push.textureIndex], texSampler), fragTexCoord);
    outColor = vec4(
    (0.5 * colour.r) * push.colour.r * (val1 * 2),//fragColour.r,
    (0.5 * colour.g) * push.colour.g * (val2 * 2),//fragColour.g,
    (0.5 * colour.b) * push.colour.b * (val3 * 2),//fragColour.b,
    (colour.a) * push.colour.a * fragColour.a);
}