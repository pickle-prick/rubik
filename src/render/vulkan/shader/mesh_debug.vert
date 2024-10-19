#version 450

// The order of uniform and input decleration does not matter here
layout(location = 0)  out vec3 frag_near_p;
layout(location = 1)  out vec3 frag_far_p;
layout(location = 2)  flat out mat4 frag_view;
layout(location = 6)  flat out mat4 frag_proj;
layout(location = 10) flat out vec4 frag_gizmos_origin;
layout(location = 11) flat out mat4 frag_gizmos_xform;
layout(location = 15) flat out uint frag_show_grid;
layout(location = 16) flat out uint frag_show_gizmos;

// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots
// That means that the index after it must be at least 2 higher

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4  view;
    mat4  proj;
    // vec4 light_pos;
    vec4  gizmos_origin;
    mat4  gizmos_xform;
    uint  show_grid;
    uint  show_gizmos;
} ubo;

// For debug only
//vec2 positions[3] = vec2[](
//        vec2( 0.0, -0.5),
//        vec2( 0.5,  0.5),
//        vec2(-0.5,  0.5)
//);

vec3 grid_plane[6] = vec3[](
    vec3(-1, -1, 0), vec3(+1, -1, 0), vec3(-1, +1, 0),
    vec3(-1, +1, 0), vec3(+1, -1, 0), vec3(+1, +1, 0)
);

mat4 translate(vec3 t) {
    return mat4(
        1.0, 0.0, 0.0,   0,
        0.0, 1.0, 0.0,   0,
        0.0, 0.0, 1.0,   0,
        t.x, t.y, t.z, 1.0
    );
    // return mat4(
    //         1.0, 0.0, 0.0, t.x,
    //         0.0, 1.0, 0.0, t.y,
    //         0.0, 0.0, 1.0, t.z,
    //         0.0, 0.0, 0.0, 1.0
    // );
}

mat4 scale(float s) {
    return mat4(
        s,   0.0, 0.0, 0.0,
        0.0, s,   0.0, 0.0,
        0.0, 0.0, s,   0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat4 rotateX(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0,   c,   s, 0.0,
        0.0,  -s,   c, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat4 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
          c, 0.0,  -s, 0.0,
        0.0, 1.0, 0.0, 0.0,
          s, 0.0,   c, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat4 rotateZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
          c,   s, 0.0, 0.0,
         -s,   c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat4 indentity() {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0);
}

mat4 rotate(vec3 rotation) {
    return rotateZ(rotation.z) * rotateY(rotation.y) * rotateX(rotation.x);
}

vec3 unproject_point(float x, float y, float z, mat4 xform_inv) {
    vec4 unproject_point = xform_inv * vec4(x, y, z, 1.0);
    return unproject_point.xyz / unproject_point.w;
}

void main() {
    vec3 p = grid_plane[gl_VertexIndex].xyz;
    mat4 xform = ubo.proj * ubo.view;
    mat4 xform_inv = inverse(xform);
    gl_Position = vec4(p, 1.0);

    // Fragment output
    frag_near_p        = unproject_point(p.x, p.y, 0.0, xform_inv).xyz;
    frag_far_p         = unproject_point(p.x, p.y, 1.0, xform_inv).xyz;
    frag_view          = ubo.view;
    frag_proj          = ubo.proj;
    frag_gizmos_origin = ubo.gizmos_origin;
    frag_gizmos_xform  = ubo.gizmos_xform;
    frag_show_grid     = ubo.show_grid;
    frag_show_gizmos   = ubo.show_gizmos;
}
