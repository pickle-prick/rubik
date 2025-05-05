#version 450

/////////////////////////////////////////////////////////////////////////////////////////
// input & output

layout(location = 0)       in vec2  texcoord;
layout(location = 1)       in vec4  color;
layout(location = 2)  flat in uvec2 id;
layout(location = 3)  flat in uint  has_texture;

layout(location = 0) out vec4  out_color;
layout(location = 1) out uvec2 out_id;

// texture
layout(set=1, binding=0) uniform sampler2D tex_sampler;

void main()
{
  out_color = color;
  if(has_texture > 0)
  {
    out_color = texture(tex_sampler, texcoord);
  }

  out_id = id;
}
