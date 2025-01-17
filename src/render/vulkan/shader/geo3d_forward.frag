#version 450

layout(location = 0)       in vec2  texcoord;
layout(location = 1)       in vec4  color;
layout(location = 2)  flat in uvec2 id;
layout(location = 3)  flat in float omit_texture;
layout(location = 4)  flat in float omit_light;
layout(location = 5)       in vec3  nor_world;
layout(location = 6)       in vec3  pos_world;
layout(location = 7)       in mat4  nor_mat;
layout(location = 11) flat in uint  draw_edge;
layout(location = 12) flat in uint  depth_test;

// There are equivalent sampler1D and sampler3D types for other types of images
layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(location = 0) out vec4  out_color;
layout(location = 1) out vec4  out_normal_depth;
layout(location = 2) out uvec2 out_id;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 view_inv;
    mat4 proj;
    mat4 proj_inv;
    vec4 global_light;
} ubo;

const float ambient = 0.1;

void main()
{
    float intensity = 1.0;
    if(omit_light == 0)
    {
        // TODO(k): forget how these two lines work
        // float light_alignment = dot(-ubo.global_light.xyz, (model*normal).xyz);
        // intensity = 0.5*light_alignment + 0.5;

        // no "Indirect Illumination" for now, use "Ambient Lighting" instead
        // NOTE(k): directly multiply model matrix with normal is only correct if scale is uniform (sx == sy == sz), prove it later
        // REF: Resource by Jason L. McKesson:
        // Learning Modern 3D Graphics Programming -Normal Transformation
        // intensity = ambient + max(dot(-ubo.global_light.xyz, (model*normal).xyz), 0);

        // NOTE(k): calculating the inverse in shader (any kind)  can be expensive and should be avoided
        // we could computed it once per frame on cpu, then pass it to the uniform
        // to furthur optimize this computation, we can ignore the translate part of matrix, since normal shouldn't be affected by translation
        // mat4 normal_mat = transpose(model_inv);

        // Diffuse light model for direction light
        vec3 nor = normalize(mat3(nor_mat) * nor_world);
        intensity = ambient + max(dot(-ubo.global_light.xyz, nor), 0);
    }

    vec4 colr = omit_texture > 0.0f ? color : texture(texSampler, texcoord);
    out_color = vec4(colr.rgb * intensity, colr.a);

    // // Assuming you have a 64-bit unsigned integer object ID
    // uint64_t object_id = ...; // Your 64-bit object ID

    // // Split the 64-bit ID into two 32-bit unsigned integers
    // uint id_low  = uint(object_id & 0xFFFFFFFFu);
    // uint id_high = uint(object_id >> 32);

    // Assign to the output variable
    // out_id = uvec2(id_low, id_high);
    out_id = id;

    out_normal_depth.rgb = draw_edge > 0 ? nor_world : vec3(0,0,0);
    out_normal_depth.a = draw_edge > 0 ? gl_FragCoord.z : 1.0f;

    // NOTE(k): https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_FragDepth.xhtml
    // If a shader statically assigns to gl_FragDepth, then the value of the fragment's depth may be undefined for executions of the shader that don't take that path
    gl_FragDepth = depth_test == 1 ? gl_FragCoord.z : 0.0f;
}
