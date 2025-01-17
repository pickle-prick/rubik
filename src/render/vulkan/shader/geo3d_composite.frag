#version 450

layout(location = 0) in  vec2 tex;
layout(location = 0) out vec4 out_color;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 0, binding = 0) uniform sampler2D geo3d_sampler;
layout(set = 1, binding = 0) uniform sampler2D geo3d_normal_depth_sampler;

/////////////////////////////////
//~ Sobel Filter

vec2 offsets[9] = vec2[](
    vec2(-1, -1), vec2( 0, -1), vec2( 1, -1),
    vec2(-1,  0), vec2( 0,  0), vec2( 1,  0),
    vec2(-1,  1), vec2( 0,  1), vec2( 1,  1)
);

float kernel_x[9] = float[](
    1.0,  0.0,  -1.0,
    2.0,  0.0,  -2.0,
    1.0,  0.0,  -1.0
);

float kernel_y[9] = float[](
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0
);

/////////////////////////////////
//~ Edge detection

float edge_from_normal(vec2 texel)
{
    vec3 Gx = vec3(0);
    vec3 Gy = vec3(0);

    for(int i = 0; i < 9; i++)
    {
        Gx += kernel_x[i] * texture(geo3d_normal_depth_sampler, tex + offsets[i] * texel).rgb;
        Gy += kernel_y[i] * texture(geo3d_normal_depth_sampler, tex + offsets[i] * texel).rgb;
    }

    float edge = sqrt(dot(Gx,Gx) + dot(Gy,Gy));
    return edge;
}

float edge_from_depth(vec2 texel)
{
    float Gx = 0;
    float Gy = 0;

    for(int i = 0; i < 9; i++)
    {
        Gx += kernel_x[i] * texture(geo3d_normal_depth_sampler, tex + offsets[i] * texel).a;
        Gy += kernel_y[i] * texture(geo3d_normal_depth_sampler, tex + offsets[i] * texel).a;
    }

    float edge = sqrt(dot(Gx,Gx) + dot(Gy,Gy));
    return edge;
}

/////////////////////////////////

vec4 alpha_blend(vec4 src, vec4 dst)
{
    // Blend formula: result = src * src.a + dst * (1-src.a)
    vec3 ret = src.rgb * src.a + dst.rgb * (1.0 - src.a);
    return vec4(ret.xyz, dst.a);
}

void main()
{
    // edge detection (using sobel filter)
    vec2 tex_size = textureSize(geo3d_normal_depth_sampler, 0);
    float texel_scale = 1.f;
    vec2 texel = texel_scale / tex_size;

    float normal_edge = edge_from_normal(texel);
    float depth_edge = edge_from_depth(texel);
    float edge = max(normal_edge, depth_edge);

    vec4 edge_clr = vec4(0.18039, 0.313725, 0.46666, edge);
    // vec4 edge_clr = vec4(0, 0, 0, edge);

    out_color = texture(geo3d_sampler, tex);
    out_color = alpha_blend(edge_clr, out_color);
}
