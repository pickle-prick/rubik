#version 450

// input
layout(location = 0)  in vec4  position;
layout(location = 1)  in vec2  rect_half_size_px;
layout(location = 2)  in vec2  texcoord_pct;
layout(location = 3)  in vec2  sdf_sample_pos;
layout(location = 4)  in vec4  tint;
layout(location = 5)  in float corner_radius_px;
layout(location = 6)  in float border_thickness_px;
layout(location = 7)  in float softness_px;
layout(location = 8)  in float line_thickness_px;
layout(location = 9)  in float white_texture_override;
layout(location = 10) in float omit_texture;
layout(location = 11) in float has_pixel_id;
layout(location = 12) in vec2  pixel_id;
layout(location = 13) in vec4  line;

// output
layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_id;

layout(set = 0, binding = 0) uniform Globals
{
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

layout(set = 1, binding = 0) uniform sampler2D tex_sampler;

float rect_sdf(vec2 pos, vec2 rect_size, float r)
{
  return length(max(abs(pos)-rect_size+r, 0.0)) - r;
}

float line_sdf(vec2 p, vec2 a, vec2 b, float r)
{
  vec2 pa = p-a, ba = b-a;
  float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0);
  return length(pa-ba*h) - r;
}

void main()
{
  // Sample texture
  vec4 albedo_sample = vec4(1, 1, 1, 1);
  if(omit_texture < 1.0)
  {
    // xform for sample channel map 
    albedo_sample = globals.texture_sample_channel_map * texture(tex_sampler, texcoord_pct);
    if(white_texture_override > 0.0)
    {
      albedo_sample.xyz = vec3(1.0,1.0,1.0);
    }
  }

  // Sample for line
  float line_sdf_t = 1.0;
  vec2 a = line.xy;
  vec2 b = line.zw;
  bool draw_line = !all(equal(a,b));
  if(draw_line)
  {
    float line_sdf_s = line_sdf(sdf_sample_pos, a, b, line_thickness_px/2.0-softness_px*2.0);
    line_sdf_t = 1.0 - smoothstep(0, softness_px*2.0, line_sdf_s);
  }

  // Sample for borders
  float border_sdf_t = 1.0;
  if(border_thickness_px > 0 && !(draw_line))
  {
    float border_sdf_s = rect_sdf(sdf_sample_pos,
                                  rect_half_size_px - 2*softness_px - border_thickness_px,
                                  max(border_thickness_px-corner_radius_px, 0));
    border_sdf_t = smoothstep(0, 2*softness_px, border_sdf_s);
  }

  // Sample for corners
  float corner_sdf_t = 1.0;
  if(corner_radius_px > 0 || softness_px > 0.75)
  // if(corner_radius_px > 0 || softness_px > 0)
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
  out_color.a *= line_sdf_t;

  // FIXME: move this into some dedicated color grading pass or just implement 3D LUT
  // warmer
  mat3 warm_mat = mat3(
    1.10,  0.05, -0.02,
    0.00,  1.00,  0.00,
    -0.05, -0.05,  0.95
  );
  vec3 c = warm_mat*out_color.rgb;
  // desaturate slightly
  float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
  c = mix(vec3(lum), c, 0.9);
  // slight fade blacks / lift shadows
  c = mix(vec3(0.02), c, vec3(0.98));
  c = clamp(c, 0.0, 1.0);
  out_color.rgb = c;

  vec4 id = vec4(pixel_id.xy,0.0,1.0);
  if(has_pixel_id == 0.0)
  {
    id.w = 0;
  }
  out_id = id;
}
