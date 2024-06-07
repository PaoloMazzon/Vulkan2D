#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj;
} ubo;

layout(push_constant) uniform PushBuffer {
    mat4 model; // Model for this shadow object
    vec2 lightSource; // Where the light is
    vec2 _alignment; // ignore
    vec4 colour; // Colour of the shadows
} pushBuffer;

layout(location = 0) in vec3 inPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    int mod = gl_VertexIndex % 6;
    if (mod == 1 || mod == 4 || mod == 5) {
        // One of the vertices that needs to be projected
        vec4 vertex = vec4(
            (inPosition.x - pushBuffer.lightSource.x) * 1000,
            (inPosition.y - pushBuffer.lightSource.y) * 1000,
            1.0,
            0.0
        );
        gl_Position = ubo.viewproj * pushBuffer.model * vertex;
    } else {
        // Static vertices
        gl_Position = ubo.viewproj * pushBuffer.model * vec4(inPosition, 1.0);
    }
}