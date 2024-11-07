#version 450

layout(location = 0) in vec4  position;
layout(location = 1) in vec2  rect_half_size_px;
layout(location = 2) in vec2  texcoord_pct;
layout(location = 3) in vec2  sdf_sample_pos;
layout(location = 4) in vec4  tint;
layout(location = 5) in float corner_radius_px;
layout(location = 6) in float border_thickness_px;
layout(location = 7) in float softness_px;
layout(location = 8) in float omit_texture;

layout(location = 0) out vec4 out_color;
layout(set = 0, binding = 0) uniform Globals {
    vec2 viewport_size_px;                // Vec2F32 viewport_size;
    float opacity;                        // F32 opacity;
    float _padding0_;                     // F32 _padding0_;
    mat4 texture_sample_channel_map;      // Vec4F32 texture_sample_channel_map[4];
    vec2 texture_t2d_size;                // Vec2F32 texture_t2d_size;
    vec2 translate;                       // Vec2F32 translate;
    mat3 xform;                           // Vec4F32 xform[3];
    vec2 xform_scale;                     // Vec2F32 xform_scale;
    float _padding1_[2];                  // F32 _padding1_;
} globals;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 1, binding = 0) uniform sampler2D tex_sampler;

float rect_sdf(vec2 pos, vec2 rect_size, float r) {
    return length(max(abs(pos)-rect_size+r, 0.0)) - r;
}

void main() {
    // Sample texture
    vec4 albedo_sample = vec4(1, 1, 1, 1);
    if(omit_texture < 1.0)
    {
        // xform for sample channel map 
        albedo_sample = globals.texture_sample_channel_map * texture(tex_sampler, texcoord_pct);
    }

    // Sample for borders
    // TODO(@k): support border color
    float border_sdf_t = 1.0;
    if(border_thickness_px > 0)
    {
        float border_sdf_s = rect_sdf(sdf_sample_pos,
                                      rect_half_size_px - 2*softness_px - border_thickness_px,
                                      max(border_thickness_px-corner_radius_px, 0));
        border_sdf_t = smoothstep(0, 2*softness_px, border_sdf_s);
    }

    // Sample for corners
    float corner_sdf_t = 1.0;
    if(corner_radius_px > 0 || softness_px > 0.75)
    {
        float corner_sdf_s = rect_sdf(sdf_sample_pos, rect_half_size_px - 2*softness_px, corner_radius_px);
        corner_sdf_t = 1.0 - smoothstep(0, 2*softness_px, corner_sdf_s);
    }

    // Form+Return final color
    out_color = albedo_sample;
    out_color *= tint;
    out_color.a *= globals.opacity;
    out_color.a *= border_sdf_t;
    out_color.a *= corner_sdf_t;

    // if (out_color.a == 0) { discard; }
}
