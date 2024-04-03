#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
} ubo;

layout(push_constant) uniform PushBuffer {
    vec2 lightSource; // Where the light is
} pushBuffer;

layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 fragColor; // not currently used

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    int mod = gl_VertexID % 6;
    if (mod == 1 || mod == 4 || mod == 3) {
        // One of the vertices that needs to be projected
        vec4 vertex = vec4(
            inPosition.x - pushBuffer.lightSource.x,
            inPosition.y - pushBuffer.lightSource.y,
            1.0,
            0.0
        );
        gl_Position = ubo.viewproj * vertex;
    } else {
        // Static vertices
        gl_Position = ubo.viewproj * vec4(inPosition, 1.0);
    }
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}