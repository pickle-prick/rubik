#version 450

layout(location = 0) out      vec4  frag_nearp_w;
layout(location = 1) out      vec4  frag_farp_w;
layout(location = 2) out flat float frag_znear;
layout(location = 3) out flat float frag_zfar;
layout(location = 4) out flat uint  frag_show_grid;
layout(location = 5) out flat mat4  frag_proj_view;
layout(location = 9) out flat mat4  frag_view;

// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots
// That means that the index after it must be at least 2 higher

layout(std140, set=0, binding=0) uniform UniformBufferObject
{
  mat4  view;
  mat4  view_inv;
  mat4  proj;
  mat4  proj_inv;
  uint  show_grid;
  vec3  _padding_0;
} ubo;

const vec3 grid_plane[6] = vec3[](
  vec3(-1, -1, 0), vec3(+1, -1, 0), vec3(-1, +1, 0),
  vec3(-1, +1, 0), vec3(+1, -1, 0), vec3(+1, +1, 0)
);

vec4 unproject_point(float x, float y, float z, mat4 xform_inv)
{
  vec4 unproject_point = xform_inv * vec4(x, y, z, 1.0);
  return unproject_point / unproject_point.w;
}

void main() 
{
  vec3 p = grid_plane[gl_VertexIndex].xyz;
  gl_Position = vec4(p, 1.0);

  mat4 proj_view_inv = ubo.view_inv * ubo.proj_inv;

  // Fragment output
  frag_nearp_w   = unproject_point(p.x, p.y, 0.0, proj_view_inv);
  frag_farp_w    = unproject_point(p.x, p.y, 1.0, proj_view_inv);
  frag_znear     = unproject_point(0,0,0, ubo.proj_inv).z;
  frag_zfar      = unproject_point(0,0,1, ubo.proj_inv).z;
  frag_show_grid = ubo.show_grid;
  frag_proj_view = ubo.proj * ubo.view;
  frag_view      = ubo.view;

  // Extract near and far from proj matrix
  // float znear = -proj[3][2] / proj[2][2];
  // float zfar  = (proj[2][2]*near) / (proj[2][2]-1);
}
