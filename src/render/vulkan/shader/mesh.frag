#version 450

layout(location = 0) in vec2  texcoord;
layout(location = 1) in vec3  color;
layout(location = 2) flat in uvec2 id;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4  out_color;
layout(location = 1) out uvec2 out_id;

void main() {
    // You can use these iamges as inputs to implement cool effects like post-processing and camera displays within the 3D world
    // outColor = texture(texSampler, fragTexCoord);
    // outColor = vec4(texcoord, 0.0, 0.6);
    out_color = vec4(color, 0.7);

    // // Assuming you have a 64-bit unsigned integer object ID
    // uint64_t object_id = ...; // Your 64-bit object ID

    // // Split the 64-bit ID into two 32-bit unsigned integers
    // uint id_low  = uint(object_id & 0xFFFFFFFFu);
    // uint id_high = uint(object_id >> 32);

    // // Assign to the output variable
    // out_id = uvec2(id_low, id_high);
    out_id = id;
}