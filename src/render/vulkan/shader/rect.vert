#version 450

// CPU2Vertex
layout(location = 0) in vec4 dst_rect_px;
layout(location = 1) in vec4 src_rect_px;
layout(location = 2) in vec4 color00;
layout(location = 3) in vec4 color10;
layout(location = 4) in vec4 color01;
layout(location = 5) in vec4 color11;
layout(location = 6) in vec4 corner_radii_px;
layout(location = 7) in vec4 style_params_1;
layout(location = 8) in vec4 style_params_2;
layout(location = 9) in vec4 line;

layout(location = 0)       out vec4  frag_position;
layout(location = 1)  flat out vec2  frag_rect_half_size_px;
layout(location = 2)       out vec2  frag_texcoord_pct;
layout(location = 3)       out vec2  frag_sdf_sample_pos;
layout(location = 4)       out vec4  frag_tint;
layout(location = 5)  flat out float frag_corner_radius_px;
// style_1
layout(location = 6)  flat out float frag_border_thickness_px;
layout(location = 7)  flat out float frag_softness_px;
layout(location = 8)  flat out float frag_line_thickness_px;
layout(location = 9)  flat out float frag_white_texture_override;
// style_2
layout(location = 10) flat out float frag_omit_texture;
layout(location = 11) flat out float frag_has_pixel_id;
layout(location = 12) flat out vec2  frag_pixel_id;
// line
layout(location = 13) flat out vec4  frag_line;

layout(set = 0, binding = 0) uniform Globals {
  vec2 viewport_size_px;                // Vec2F32 viewport_size;
  float opacity;                        // F32 opacity;
  float _padding0_;                     // F32 _padding0_;
  vec4 texture_sample_channel_map[4];   // Vec4F32 texture_sample_channel_map[4];
  vec2 texture_t2d_size;                // Vec2F32 texture_t2d_size;
  vec2 translate;                       // Vec2F32 translate;
  mat3 xform;                           // Vec4F32 xform[3];
  vec2 xform_scale;                     // Vec2F32 xform_scale;
  float _padding1_[2];                  // F32 _padding1_;
} globals;

vec2 positions[4] = vec2[](
  vec2(-1, -1), // Top-left
  vec2( 1, -1), // Top-Right
  vec2(-1,  1), // Bottom-Left
  vec2( 1,  1)  // Bottom-Right
);

// vec2 tex_coords[4] = vec2[](
//     vec2(0, 0), // Top-left
//     vec2(1, 0), // Top-Right
//     vec2(0, 1), // Bottom-Left
//     vec2(1, 1)  // Bottom-Right
// );

vec4 src_colors[4] = vec4[4](
  color00,
  color10,
  color01,
  color11
);

void main()
{
  vec2 dst_p0_px   = dst_rect_px.xy;
  vec2 dst_p1_px   = dst_rect_px.zw;
  vec2 src_p0_px   = src_rect_px.xy;
  vec2 src_p1_px   = src_rect_px.zw;
  vec2 dst_size_px = dst_p1_px - dst_p0_px;

  // Unpack style params
  float border_thickness_px    = style_params_1.x;
  float softness_px            = style_params_1.y;
  float line_thickness_px      = style_params_1.z;
  float white_texture_override = style_params_1.w;
  float omit_texture           = style_params_2.x;
  float has_pixel_id           = style_params_2.y;
  vec2  pixel_id               = style_params_2.zw;

  vec2 rect_normal_pos = positions[gl_VertexIndex];
  vec2 pct             = (rect_normal_pos+1.0) * 0.5; // x: [0-1] y: [0-1]
  vec2 xformed_pos     = (globals.xform * vec3(mix(dst_p0_px.xy, dst_p1_px.xy, pct), 1.0)).xy;
  vec4 ndc_pos         = vec4(2.0f * xformed_pos / globals.viewport_size_px - 1.0f, 0.0, 1.0);

  // Output vertex position
  gl_Position = ndc_pos;

  // Output
  frag_position               = ndc_pos;
  frag_rect_half_size_px      = dst_size_px / 2.0f * globals.xform_scale;
  frag_texcoord_pct           = mix(src_p0_px, src_p1_px, pct) / globals.texture_t2d_size;
  frag_sdf_sample_pos         = mix(-frag_rect_half_size_px, frag_rect_half_size_px, pct);
  frag_tint                   = src_colors[gl_VertexIndex];
  frag_corner_radius_px       = corner_radii_px[gl_VertexIndex];
  frag_border_thickness_px    = border_thickness_px;
  frag_softness_px            = softness_px;
  frag_line_thickness_px      = line_thickness_px;
  frag_white_texture_override = white_texture_override;
  frag_omit_texture           = omit_texture;
  frag_has_pixel_id           = has_pixel_id;
  frag_pixel_id               = pixel_id;
  frag_line                   = vec4(line.xy*globals.xform_scale, line.zw*globals.xform_scale);
}
