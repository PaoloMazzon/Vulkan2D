#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 viewproj[10];
} ubo;

layout(push_constant) uniform PushBuffer {
    mat4 model; // Model for this shadow object
    vec2 lightSource; // Where the light is
    vec2 _alignment; // ignore
    vec4 colour; // Colour of the shadows
    int cameraIndex;
} pushBuffer;

layout(location = 0) in vec3 inPosition;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    int mod = gl_VertexIndex % 6;
    vec4 position = pushBuffer.model * vec4(inPosition, 1.0);
    if (mod == 1 || mod == 4 || mod == 5) {
        // One of the vertices that needs to be projected
        vec4 vertex = vec4(
            (position.x - pushBuffer.lightSource.x) * 1000,
            (position.y - pushBuffer.lightSource.y) * 1000,
            1.0,
            0.0
        );
        gl_Position = ubo.viewproj[pushBuffer.cameraIndex] * vertex;
    } else {
        // Static vertices
        gl_Position = ubo.viewproj[pushBuffer.cameraIndex] * position;
    }
}