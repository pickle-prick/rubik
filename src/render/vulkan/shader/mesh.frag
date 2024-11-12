#version 450

layout(location = 0)      in vec2  texcoord;
layout(location = 1)      in vec4  color;
layout(location = 2) flat in uvec2 id;
layout(location = 3) flat in float omit_texture;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4  out_color;
layout(location = 1) out uvec2 out_id;

void main()
{
    // You can use these iamges as inputs to implement cool effects like post-processing and camera displays within the 3D world
    vec4 colr = omit_texture > 0.0f ? color : texture(texSampler, texcoord);
    out_color = colr;

    // // Assuming you have a 64-bit unsigned integer object ID
    // uint64_t object_id = ...; // Your 64-bit object ID

    // // Split the 64-bit ID into two 32-bit unsigned integers
    // uint id_low  = uint(object_id & 0xFFFFFFFFu);
    // uint id_high = uint(object_id >> 32);

    // Assign to the output variable
    // out_id = uvec2(id_low, id_high);
    out_id = id;
}
