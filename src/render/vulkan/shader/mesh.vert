#version 450

// The order of uniform and input decleration does not matter here
layout(location = 0)  in vec3   pos;
layout(location = 1)  in vec3   nor;
layout(location = 2)  in vec2   tex;
layout(location = 3)  in vec3   col;
layout(location = 4)  in uvec4  joints;
layout(location = 5)  in vec4   weights;

// Instance buffer
layout(location = 6)  in mat4   model;
layout(location = 10) in uvec2  id;
layout(location = 11) in uint   first_joint;
layout(location = 12) in uint   joint_count;

// It is important to know that some types, like dvec3 64 bit vectors, use multiple slots
// That means that the index after it must be at least 2 higher
layout(location = 0) out  vec2 frag_texcoord;
layout(location = 1) out  vec3 frag_color;
layout(location = 2) flat out  uvec2 frag_id;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    // vec4 light_pos;
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
        0.0, 0.0, 0.0, 1.0
    );
}

mat4 rotateZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat4(
          c,   s, 0.0, 0.0,
         -s,   c, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
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

void main() {
        // instance_t instance = instances[gl_InstanceIndex];
        vec4 position = vec4(pos, 1.0);

        // Method 1
        if(joint_count > 0)
        {
            mat4 skin_mat = 
                weights[0] * global_joints.xforms[first_joint+joints[0]] + 
                weights[1] * global_joints.xforms[first_joint+joints[1]] + 
                weights[2] * global_joints.xforms[first_joint+joints[2]] + 
                weights[3] * global_joints.xforms[first_joint+joints[3]];
            
            position = skin_mat * position;
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

        // Output
        frag_color    = vec3(1, 1, 1);
        frag_texcoord = tex;
        frag_id       = id;
}
