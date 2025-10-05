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
layout(location = 7)   in mat4  model;
layout(location = 11)  in mat4  model_inv;

layout(location = 15) in vec2  id;
layout(location = 16) in uint  material_idx;
layout(location = 17) in uint  draw_edge;
layout(location = 18) in uint  joint_count;
layout(location = 19) in uint  first_joint;
layout(location = 20) in uint  depth_test;
layout(location = 21) in uint  omit_light;

layout(std140, set=0, binding=0) uniform UniformBufferObject {
    mat4 view;
    mat4 view_inv;
    mat4 proj;
    mat4 proj_inv;
    uint show_grid;
    vec3 _padding_0;
} ubo;

layout(std140, set = 1, binding = 0) readonly buffer Joints {
    mat4 xforms[];
} global_joints;

void main()
{
  vec4 pos_local = vec4(pos, 1.0);

  if(joint_count > 0)
  {
    mat4 skin_mat = 
      weights[0] * global_joints.xforms[first_joint+joints[0]] + 
      weights[1] * global_joints.xforms[first_joint+joints[1]] + 
      weights[2] * global_joints.xforms[first_joint+joints[2]] + 
      weights[3] * global_joints.xforms[first_joint+joints[3]];

    pos_local = skin_mat * pos_local;
  }

  vec4 pos_world = ubo.proj * ubo.view * model * pos_local;
  gl_Position = pos_world;
}
