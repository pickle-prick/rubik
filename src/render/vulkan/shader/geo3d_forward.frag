#version 450

/////////////////////////////////////////////////////////////////////////////////////////
// input & output

layout(location = 0)       in vec2  texcoord;
layout(location = 1)       in vec4  color;
layout(location = 2)  flat in vec2  id;
layout(location = 3)  flat in float omit_light;
layout(location = 4)       in vec3  nor_world;
layout(location = 5)       in vec3  nor_view;
layout(location = 6)       in vec3  pos_world;
layout(location = 7)       in vec3  pos_view;
layout(location = 8)       in mat4  nor_mat;
layout(location = 12) flat in uint  draw_edge;
layout(location = 13) flat in uint  depth_test;
layout(location = 14) flat in uint  mat_idx;

layout(location = 0) out vec4  out_color;
layout(location = 1) out vec4  out_normal_depth;
layout(location = 2) out vec4  out_id;

/////////////////////////////////////////////////////////////////////////////////////////
// basic types

// Constants
// #define BLOCK_SIZE 16
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

struct Light
{
  vec4 position_ws;
  vec4 direction_ws;
  vec4 position_vs;
  vec4 direction_vs;
  vec4 color;
  vec4 attenuation; // x: constant, y: linear, z: quadratic
  float spot_angle;
  float range;
  float intensity;
  uint kind;
};

struct TileLights
{
  uint offset;
  uint light_count;
  vec2 _padding_0; // required for std140
};

struct LightResult
{
  vec4 diffuse;
  vec4 specular;
};

struct Material
{
  // textures
  uint has_ambient_texture;
  uint has_emissive_texture;
  uint has_diffuse_texture;
  uint has_specular_texture;
  uint has_specular_power_texture;
  uint has_normal_texture;
  uint has_bump_texture;
  uint has_opacity_texture;

  // color
  vec4 ambient_color;
  vec4 emissive_color;
  vec4 diffuse_color;
  vec4 specular_color;
  vec4 reflectance;

  // sample_channel maps
  mat4 diffuse_sample_channel_map;

  // f32
  float opacity;
  float specular_power;
  // for transparent materials, IOR > 0
  float index_of_refraction;
  float bump_intensity;
  float specular_scale;
  float alpha_cutoff;
  vec2 _padding_0;
};

/////////////////////////////////////////////////////////////////////////////////////////
// ubo/push/tex/sbo

layout(set=0, binding=0) uniform UniformBufferObject
{
  mat4 view;
  mat4 view_inv;
  mat4 proj;
  mat4 proj_inv;
  uint show_grid;
  vec3 _padding_0;
} ubo;

layout(push_constant) uniform PushConstants
{
  vec2 viewport;
  uvec2 light_grid_size;
} push;

// texture
// There are equivalent sampler1D and sampler3D types for other types of images
layout(set=2, binding=0) uniform sampler2D texSampler;

// global lights
layout(std140, set=3, binding=0) readonly buffer Lights
{
  Light array[];
} lights;

// global light indices
layout(std140, set=4, binding=0) readonly buffer LightIndices
{
  uint array[]; // first one will be used as indice_count
} light_indices;

// TODO(XXX): we need two lists, one for opaque geometry, one for transparent geometry
// tile lights (2D grid)
layout(std140, set=5, binding=0) readonly buffer TileLightsArray
{
  TileLights array[];
} tile_lights;

// materials
layout(std140, set=6, binding=0) readonly buffer MaterialArray
{
  Material array[];
} materials;

/////////////////////////////////////////////////////////////////////////////////////////
// light calculations

vec4 do_diffuse(Light light, vec4 L, vec4 N)
{
  float NdotL = max(dot(N,L), 0);
  return light.color * NdotL;
}

vec4 do_specular(Light light, Material mat, vec4 V, vec4 L, vec4 N)
{
  vec4 R = normalize(reflect(-L, N));
  float RdotV = max(dot(R,V), 0);
  return light.color * pow(RdotV, mat.specular_power);
}

float do_attenuation(Light light, float d)
{
  return 1.0f - smoothstep(light.range * 0.75f, light.range, d);
}

float do_spot_cone(Light light, vec4 L)
{
  float min_cos = cos(light.spot_angle);
  float max_cos = mix(min_cos, 1, 0.5);
  float cos_angle = dot(light.direction_vs, -L);
  return smoothstep(min_cos, max_cos, cos_angle);
}

LightResult do_directional_light(Light light, Material mat, vec4 V, vec4 P, vec4 N)
{
  LightResult ret;

  vec4 L = normalize(-light.direction_vs);
  ret.diffuse = do_diffuse(light, L, N) * light.intensity;
  ret.specular = do_specular(light, mat, V, L, N) * light.intensity;
  return ret;
}

LightResult do_point_light(Light light, Material mat, vec4 V, vec4 P, vec4 N)
{
  LightResult ret;
  // direction from current pixel view position to light source
  vec4 L = light.position_vs - P;
  float distance = length(L);
  L = L / distance;
  float attenuation = do_attenuation(light, distance);

  ret.diffuse = do_diffuse(light, L, N) * attenuation * light.intensity;
  ret.specular = do_specular(light, mat, V, L, N) * attenuation * light.intensity;
  return ret;
}

LightResult do_spot_light(Light light, Material mat, vec4 V, vec4 P, vec4 N)
{
  LightResult ret;
  vec4 L = light.position_vs - P;
  float distance = length(L);

  L = L / distance;
  float attenuation = do_attenuation(light, distance);
  float spot_intensity = do_spot_cone(light, L);

  ret.diffuse = do_diffuse(light, L, N) * attenuation * spot_intensity * light.intensity;
  ret.specular = do_specular(light, mat, V, L, N) * attenuation * spot_intensity * light.intensity;
  return ret;
}

void main()
{
  // // Assuming you have a 64-bit unsigned integer object ID
  // uint64_t object_id = ...; // Your 64-bit object ID
  // // Split the 64-bit ID into two 32-bit unsigned integers
  // uint id_low  = uint(object_id & 0xFFFFFFFFu);
  // uint id_high = uint(object_id >> 32);

  /////////////////////////////////////////////////////////////////////////////////////
  // output "id"

  // Assign to the output variable
  // out_id = uvec2(id_low, id_high);
  out_id = vec4(id.xy, 0, 1);

  // NOTE(k): normal is interpolated using berrycentric, so it's not guarenteed to be unit vector
  out_normal_depth.rgb = draw_edge > 0 ? normalize(nor_world) : vec3(0,0,0);
  out_normal_depth.a = draw_edge > 0 ? gl_FragCoord.z : 1.0f;

  /////////////////////////////////////////////////////////////////////////////////////
  // disable depth if asked

  // NOTE(k): https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_FragDepth.xhtml
  // If a shader statically assigns to gl_FragDepth, then the value of the fragment's depth may be undefined for executions of the shader that don't take that path
  // TODO(k): this will disblae EARLY_FRAGMENT_TEST, thus effect the performance, do we really need this
  gl_FragDepth = depth_test == 1 ? gl_FragCoord.z : 0.0f;

  /////////////////////////////////////////////////////////////////////////////////////
  // material

  Material mat = materials.array[mat_idx];

  /////////////////////////////////////////////////////////////////////////////////////
  // light acc

  vec4 diffuse = mat.diffuse_color;
  if(mat.has_diffuse_texture > 0)
  {
    diffuse *= mat.diffuse_sample_channel_map * texture(texSampler, texcoord);
  }

  // TODO(XXX): not working properly, fix it later
  // vec4 specular = mat.specular_color;
  // vec4 ambient = mat.ambient_color;
  // vec4 emissive = mat.emissive_color;
  vec4 specular = vec4(0,0,0,0);
  vec4 ambient = vec4(0,0,0,0);
  vec4 emissive = vec4(0,0,0,0);
  float alpha = diffuse.a;

  /////////////////////////////////////////////////////////////////////////////////////
  // lighting

  if(omit_light == 0)
  {
    // NOTE(k): since nor_view is interpolated using berrycentric, it's not guaranteed to be unit vector
    vec4 N = vec4(normalize(nor_view), 0.0);
    vec4 P = vec4(pos_view, 1.0); // view pos for current pixel/fragment

    // eye position in view
    vec4 E = vec4(0,0,0,1.0);

    // the view vector (V) is computed from the eye position and the position of the shaded pixel in view space
    vec4 V = normalize(E - P);

    // get the index of the current pixel in the light grid
    vec2 pct = gl_FragCoord.xy / push.viewport;
    ivec2 tile_coord = ivec2(pct * push.light_grid_size);
    uint tile_idx = tile_coord.y * push.light_grid_size.x + tile_coord.x;

    // get the start position and offset of the light in the light index list
    uint start_offset = tile_lights.array[tile_idx].offset;
    uint light_count = tile_lights.array[tile_idx].light_count;

    LightResult acc = LightResult(vec4(0,0,0,0), vec4(0,0,0,0));
    for(uint i = 0; i < light_count; i++)
    {
      uint light_idx = light_indices.array[start_offset+i];
      Light light = lights.array[light_idx];
      LightResult ret = LightResult(vec4(0,0,0,0), vec4(0,0,0,0));

      switch(light.kind)
      {
        case DIRECTIONAL_LIGHT:
        {
          ret = do_directional_light(light, mat, V, P, N);
        }break;
        case POINT_LIGHT:
        {
          ret = do_point_light(light, mat, V, P, N);
        }break;
        case SPOT_LIGHT:
        {
          ret = do_spot_light(light, mat, V, P, N);
        }break;
      }

      acc.diffuse += ret.diffuse;
      acc.specular += ret.specular;
    }

    // discard the alpha value from the lighting calculations
    diffuse *= vec4(acc.diffuse.rgb, 1.0f);
    specular *= acc.specular;
  }

  out_color = vec4((ambient+emissive+diffuse+specular).rgb, alpha*mat.opacity);
}
