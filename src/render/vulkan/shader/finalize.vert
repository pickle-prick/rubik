#version 450

layout(location = 0) out vec2 out_tex;

vec2 positions[4] = vec2[](
    vec2(-1, -1), // Top-left
    vec2( 1, -1), // Top-Right
    vec2(-1,  1), // Bottom-Left
    vec2( 1,  1)  // Bottom-Right
);

vec2 tex_coords[4] = vec2[](
    vec2(0, 0), // Top-left
    vec2(1, 0), // Top-Right
    vec2(0, 1), // Bottom-Left
    vec2(1, 1)  // Bottom-Right
);

void main() {
        gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
        out_tex = tex_coords[gl_VertexIndex];
}
