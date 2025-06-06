#version 450

// in
layout(location = 0) in vec2 in_tex;

// out
layout(location = 0) out vec4 out_color;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 0, binding = 0) uniform sampler2D stage_sampler;
layout(set = 1, binding = 0) uniform sampler2D edge_sampler;

layout(push_constant) uniform PushConstants
{
  float time;
} push;

/////////////////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////////////////
//~ Edge detection

float edge_from_texel(vec2 texel)
{
  vec4 Gx = vec4(0);
  vec4 Gy = vec4(0);

  for(int i = 0; i < 9; i++)
  {
    Gx += kernel_x[i] * texture(edge_sampler, in_tex + offsets[i] * texel);
    Gy += kernel_y[i] * texture(edge_sampler, in_tex + offsets[i] * texel);
  }

  float edge = sqrt(dot(Gx,Gx) + dot(Gy,Gy));
  return edge;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Helpers

vec4 alpha_blend(vec4 src, vec4 dst)
{
  // Blend formula: result = src * src.a + dst * (1-src.a)
  vec3 ret = src.rgb * src.a + dst.rgb * (1.0 - src.a);
  return vec4(ret.xyz, dst.a);
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Main

void main()
{
  // vec2 st = gl_FragCoord.xy/push.resolution.xy;

  vec2 tex_size = textureSize(edge_sampler, 0);
  float texel_scale = 1.f;
  vec2 texel = texel_scale / tex_size;

  float edge = edge_from_texel(texel);

  // vec4 edge_clr = vec4(0.18039, 0.313725, 0.46666, edge);
  vec4 edge_clr = vec4(0, 1, 0, edge);

  vec4 tex_clr = texture(stage_sampler, in_tex);
  out_color = tex_clr;
  out_color = alpha_blend(edge_clr, out_color);
}
