#version 450

layout(location = 0) in vec2 tex;
layout(location = 0) out vec4 out_color;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 0, binding = 0) uniform sampler2D geo3d_sampler;

void main()
{
    out_color = texture(geo3d_sampler, tex);
}
