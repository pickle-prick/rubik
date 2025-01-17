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
layout(location = 7)   in mat4  model;
layout(location = 11)  in mat4  model_inv;

layout(location = 15) in uvec2 id;
layout(location = 16) in vec4  color_texture;
layout(location = 17) in uint  draw_edge;
layout(location = 18) in uint  joint_count;
layout(location = 19) in uint  first_joint;
layout(location = 20) in uint  depth_test;
layout(location = 21) in uint  omit_light;

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
    mat4 view_inv;
    mat4 proj;
    mat4 proj_inv;
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

void main()
{
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

        // Diffuse light model
        float intensity = 1.0;
        if(omit_light == 0)
        {
            const float ambient = 0.2;

            // TODO(k): forget how these two lines work
            // float light_alignment = dot(-ubo.global_light.xyz, (model*normal).xyz);
            // intensity = 0.5*light_alignment + 0.5;

            // no "Indirect Illumination" for now, use "Ambient Lighting" instead
            // TODO(XXX): directly multiply model matrix with normal is only correct if scale is uniform (sx == sy == sz), prove it later
            // REF: Resource by Jason L. McKesson:
            // Learning Modern 3D Graphics Programming -Normal Transformation
            // intensity = ambient + max(dot(-ubo.global_light.xyz, (model*normal).xyz), 0);

            // TODO(XXX): calculating the inverse in shader (any kind)  can be expensive and should be avoided
            // we could computed it once per frame on cpu, then pass it to the uniform
            // to furthur optimize this computation, we can ignore the translate part of matrix, since normal shouldn't be affected by translation
            mat4 normal_mat = transpose(model_inv);
            vec4 normal_word = normalize(normal_mat * normal);
            intensity = ambient + max(dot(-ubo.global_light.xyz, normal_word.xyz), 0);
        }

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
