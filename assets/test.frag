#version 450
#extension GL_ARB_separate_shader_objects : enable
#define pi 3.141592635

layout(set = 1, binding = 1) uniform sampler texSampler;
layout(set = 2, binding = 2) uniform texture2D tex;

layout(set = 3, binding = 3) uniform UserData {
    float colour;
} userData;

layout(push_constant) uniform PushBuffer {
    mat4 model;
    vec4 colourMod;
    vec4 textureCoords;
} pushBuffer;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 fragColour;

layout(location = 0) out vec4 outColor;

void main() {
    //0.45*sin(x)+0.55
    float a = 1;
    float b = 0.3;
    float val1 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour)) + (((a - b) / 2) + b);
    float val2 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour + (pi * 0.25))) + (((a - b) / 2) + b);
    float val3 = ((a - b) / 2)*sin(((fragTexCoord.y + fragTexCoord.x) / 2) + (userData.colour + (pi * 0.5))) + (((a - b) / 2) + b);
    vec4 colour = texture(sampler2D(tex, texSampler), fragTexCoord);
    outColor = vec4(
    (0.5 * colour.r) * pushBuffer.colourMod.r * (val1 * 2),//fragColour.r,
    (0.5 * colour.g) * pushBuffer.colourMod.g * (val2 * 2),//fragColour.g,
    (0.5 * colour.b) * pushBuffer.colourMod.b * (val3 * 2),//fragColour.b,
    (colour.a) * pushBuffer.colourMod.a * fragColour.a);
}