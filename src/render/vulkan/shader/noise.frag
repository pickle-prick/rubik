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

float random(vec2 st)
{
  return fract(sin(dot(st.xy, vec2(12.9898,78.233)))* 43758.5453123);
}

void main()
{
  vec2 st = gl_FragCoord.xy/push.resolution.xy;
  float rnd = random(st);
  vec4 noise = vec4(vec3(rnd),0.0);

  vec4 tex_clr = out_color = texture(stage_sampler, in_tex);
  out_color = noise*0.1 + tex_clr;
}
