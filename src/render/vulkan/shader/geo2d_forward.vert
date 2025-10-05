#version 450

// The order of uniform and input decleration does not matter here
layout(location = 0)  in vec3   pos;
layout(location = 1)  in vec3   nor;
layout(location = 2)  in vec2   tex;
layout(location = 3)  in vec3   tan;
layout(location = 4)  in vec4   col;
layout(location = 5)  in uvec4  joints;
layout(location = 6)  in vec4   weights;

// Instance buffer
layout(location = 7)  in  mat4  model;
layout(location = 11) in  mat4  model_inv;
layout(location = 15) in  vec2  id;
layout(location = 16) in  uint  has_texture;
layout(location = 17) in  vec4  color;
layout(location = 18) in  uint  has_color;
layout(location = 19) in  uint  draw_edge;

// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots
// That means that the index after it must be at least 2 higher
layout(location = 0)       out  vec2  frag_texcoord;
layout(location = 1)       out  vec4  frag_color;
layout(location = 2)  flat out  vec2  frag_id;
layout(location = 3)  flat out  uint  frag_has_texture;
layout(location = 4)  flat out  uint  frag_draw_edge;

/////////////////////////////////////////////////////////////////////////////////////////
// ubo/sbo

layout(std140, set=0, binding=0) uniform UniformBufferObject
{
  mat4 proj;
  mat4 proj_inv;
  mat4 view;
  mat4 view_inv;
} ubo;

void main()
{
  vec4 pos_view = ubo.view * model * vec4(pos,1.0);
  vec4 pos_world = ubo.proj * pos_view;
  gl_Position = pos_world;

  // output
  frag_texcoord    = tex;
  frag_color       = has_color > 0 ? color : col;
  frag_id          = id;
  frag_has_texture = has_texture;
  frag_draw_edge   = draw_edge;
}
