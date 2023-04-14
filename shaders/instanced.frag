#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 1, binding = 1) uniform sampler texSampler;
layout(set = 2, binding = 2) uniform texture2D tex;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    vec4 textureCoords;
} pushBuffer;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColour;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 colour = texture(sampler2D(tex, texSampler), fragTexCoord);
    outColor = vec4(
            colour.r * fragColour.r,
            colour.g * fragColour.g,
            colour.b * fragColour.b,
            colour.a * fragColour.a);
}