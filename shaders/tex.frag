#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 1) uniform sampler texSampler;
layout(set = 2, binding = 2) uniform texture2D tex;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    vec4 textureCoords;
} pushBuffer;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 colour = texture(sampler2D(tex, texSampler), fragTexCoord);
    outColor = vec4(
            colour.r * pushBuffer.colourMod.r * fragColor.r,
            colour.g * pushBuffer.colourMod.g * fragColor.g,
            colour.b * pushBuffer.colourMod.b * fragColor.b,
            colour.a * pushBuffer.colourMod.a * fragColor.a);
}