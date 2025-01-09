#version 450

// The order of uniform and input decleration does not matter here
layout(location = 0)  in vec3   pos;
layout(location = 1)  in vec3   nor;
layout(location = 2)  in vec2   tex;
layout(location = 3)  in vec3   tan;
layout(location = 4)  in vec3   col;
layout(location = 5)  in uvec4  joints;
layout(location = 6)  in vec4   weights;

// Instance buffer
layout(location = 7)  in mat4  model;
layout(location = 11) in uvec2 id;
layout(location = 12) in vec4  color_texture;
layout(location = 13) in uint  draw_edge;
layout(location = 14) in uint  joint_count;
layout(location = 15) in uint  first_joint;
layout(location = 16) in uint  depth_test;

// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots
// That means that the index after it must be at least 2 higher
layout(location = 0)      out  vec2  frag_texcoord;
layout(location = 1)      out  vec4  frag_color;
layout(location = 2) flat out  uvec2 frag_id;
layout(location = 3) flat out  float frag_omit_texture;
layout(location = 4) flat out  vec3  frag_normal;
layout(location = 5) flat out  uint  frag_draw_edge;
layout(location = 6) flat out  uint  frag_depth_test;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 global_light;
} ubo;

layout(std430, set = 1, binding = 0) readonly buffer Joints {
    mat4 xforms[];
} global_joints;

// struct instance_t {
//     vec3 pos;
//     vec3 rot;
//     float scale;
//     uint text_index;
//     uint generation_index;
// };
// 
// layout(std430, set = 1, binding = 0) readonly buffer InstanceData {
//     instance_t instances[];
// };

// For debug only
//vec2 positions[3] = vec2[](
//        vec2( 0.0, -0.5),
//        vec2( 0.5,  0.5),
//        vec2(-0.5,  0.5)
//);

void main() {
        // instance_t instance = instances[gl_InstanceIndex];
        vec4 position = vec4(pos, 1.0);
        vec4 normal = vec4(nor, 0.0);

        // Method 1
        if(joint_count > 0)
        {
            mat4 skin_mat = 
                weights[0] * global_joints.xforms[first_joint+joints[0]] + 
                weights[1] * global_joints.xforms[first_joint+joints[1]] + 
                weights[2] * global_joints.xforms[first_joint+joints[2]] + 
                weights[3] * global_joints.xforms[first_joint+joints[3]];
            
            position = skin_mat * position;
            normal = normalize(skin_mat * normal);
        }

        // Method 2
        // vec4 ori_pos = vec4(pos, 1.0);
        // vec4 position = vec4(0.0);
        // if(joint_count > 0)
        // {
        //     for(int i = 0; i < 4; i++)
        //     {
        //         position += weights[i] * (global_joints.xforms[first_joint+joints[i]] * ori_pos);
        //     }
        // }

        gl_Position = ubo.proj * ubo.view * model * position;

        float light_alignment = dot(-ubo.global_light.xyz, (model*normal).xyz);
        // float light_alignment = dot(-ubo.global_light.xyz, normal.xyz);
        float intensity = 0.5*light_alignment + 0.5;
        intensity = intensity < 0.3 ? 0.3 : intensity;

        vec4 color = color_texture.a > 0 ? color_texture : vec4(col.xyz, 1.0);

        // Output
        frag_texcoord     = tex;
        frag_color        = vec4((color*intensity).xyz, color.a);
        frag_id           = id;
        frag_omit_texture = color_texture.a > 0 ? 1.0 : 0.0;
        frag_normal       = nor;
        frag_draw_edge    = draw_edge;
        frag_depth_test   = depth_test;
}
