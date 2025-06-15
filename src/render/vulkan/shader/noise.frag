#version 450

layout(location = 0) in vec2 in_tex;
layout(location = 0) out vec4 out_color;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 0, binding = 0) uniform sampler2D stage_sampler;

layout(push_constant) uniform PushConstants
{
  vec2 resolution;
  vec2 mouse;
  float time;
} push;

#ifdef GL_ES
precision mediump float;
#endif

float random(vec2 st)
{
  return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123*push.time);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise(vec2 _st)
{
  vec2 i = floor(_st);
  vec2 f = fract(_st);

  // Four corners in 2D of a tile
  float a = random(i);
  float b = random(i + vec2(1.0, 0.0));
  float c = random(i + vec2(0.0, 1.0));
  float d = random(i + vec2(1.0, 1.0));

  vec2 u = f * f * (3.0 - 2.0 * f);
  return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

#define NUM_OCTAVES 5

float fbm(vec2 _st)
{
  float v = 0.0;
  float a = 0.5;
  vec2 shift = vec2(100.0);
  // Rotate to reduce axial bias
  mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.50));
  for(int i = 0; i < NUM_OCTAVES; ++i)
  {
    v += a * noise(_st);
    _st = rot * _st * 2.0 + shift;
    a *= 0.5;
  }
  return v;
}

void main()
{
  vec2 st = gl_FragCoord.xy/push.resolution.xy;
  float rnd = random(st);
  vec4 noise = vec4(vec3(rnd),0.0);
  vec4 tex_clr = texture(stage_sampler, in_tex);
  out_color = noise*0.05 + tex_clr;
}
