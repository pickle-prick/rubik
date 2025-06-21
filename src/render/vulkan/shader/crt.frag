#version 450

// in
layout(location = 0) in vec2 in_tex;

// out
layout(location = 0) out vec4 out_color;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 0, binding = 0) uniform sampler2D stage_sampler;

layout(push_constant) uniform PushConstants
{
  vec2 resolution;
  float warp;
  float scan;
  float time;
} push;

/////////////////////////////////////////////////////////////////////////////////////////
//~ Main

void main()
{
  vec2 uv = gl_FragCoord.xy/push.resolution.xy;
  vec2 dc = abs(0.5-uv);
  dc *= dc;

  float warp = push.warp;
  float scan = push.scan;

  // warp the fragment coordinates
  uv.x -= 0.5; uv.x *= 1.0+(dc.y*(0.3*warp)); uv.x += 0.5;
  uv.y -= 0.5; uv.y *= 1.0+(dc.x*(0.4*warp)); uv.y += 0.5;

  // sample inside boundaries, otherwise set to black
  if(uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0)
  {
    out_color = vec4(0.0,0.0,0.0,1.0);
  }
  else
  {
    // determine if we are drawing in a scanline
    float apply = abs(sin(gl_FragCoord.y)*0.5*scan);
    // sample the texture
    out_color = vec4(mix(texture(stage_sampler, uv).rgb,vec3(0.0),apply),1.0);
  }
}
