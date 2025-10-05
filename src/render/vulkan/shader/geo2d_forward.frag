#version 450

/////////////////////////////////////////////////////////////////////////////////////////
// input & output

layout(location = 0)       in vec2  texcoord;
layout(location = 1)       in vec4  color;
layout(location = 2)  flat in vec2  id;
layout(location = 3)  flat in uint  has_texture;
layout(location = 4)  flat in uint  draw_edge;

layout(location = 0) out vec4  out_color;
layout(location = 1) out vec4  out_id;
layout(location = 2) out vec4  out_edge;

// texture
layout(set=1, binding=0) uniform sampler2D tex_sampler;

void main()
{
  out_color = color;
  if(has_texture > 0)
  {
    out_color = texture(tex_sampler, texcoord);
  }

  if(draw_edge > 0)
  {
    // out_edge[0] = uintBitsToFloat(id[0]);
    // out_edge[1] = uintBitsToFloat(id[1]);
    // out_edge[2] = uintBitsToFloat(id[0]);
    // out_edge[3] = uintBitsToFloat(id[1]);
    out_edge = out_color;
  }
  else
  {
    out_edge = vec4(0,0,0,0);
  }

  // out_edge = out_color;
  // out_edge[0] = uintBitsToFloat(id[0]);
  // out_edge[1] = uintBitsToFloat(id[1]);
  // out_edge[2] = uintBitsToFloat(id[0]);
  // out_edge[3] = uintBitsToFloat(id[1]);

  out_id = vec4(id.xy, 0, 1.0);
}
