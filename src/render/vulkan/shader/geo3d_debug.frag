#version 450

// Input
layout(location = 0) in      vec4  nearp_w;
layout(location = 1) in      vec4  farp_w;
layout(location = 2) in flat float znear;
layout(location = 3) in flat float zfar;
layout(location = 4) in flat uint  show_grid;
layout(location = 5) in flat mat4  proj_view;
layout(location = 9) in flat mat4  view;

// Output
layout(location = 0) out vec4  out_color;
layout(location = 1) out vec4  out_normal_depth;
layout(location = 2) out vec4 out_id;

#define red      vec3(1.,0.271,0.271)
#define green    vec3(0.,0.416,0.404)
#define blue     vec3(0.,0.192,0.38)
#define base_clr vec3(0,0,0)
#define line_clr vec3(0.965,0.988,0.875)

vec4 grid(vec4 pos_w, float scale) {
    vec2 coord = pos_w.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord-0.5)-0.5) / (derivative*1.5);
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);

    vec4 color = vec4(mix(base_clr, line_clr*0.1, 1.0-min(line, 1.0)).xyz, 1.0);

    // z axis
    // if(pos_w.x > -0.01 && pos_w.x < 0.01) {
    if(pos_w.x > -1 * minimumx && pos_w.x < 1 * minimumx)
    {
        color.b = 1.0;
    }

    // x axis
    // if(pos_w.z > -0.1 * minimumz && pos_w.z < 0.1 * minimumz) {
    if(pos_w.z > -1 * minimumz && pos_w.z < 1 * minimumz)
    {
        color.r = 1.0;
    }

    return color;
}

// float plane_intersect(vec3 ray_start, vec3 ray_end, vec3 N, vec3 p_plane)
// {
//     // return dot(N,(p_plane-ray_start)) / dot(N,(ray_end-ray_start));
// 
//     vec3 ray_dir = ray_end - ray_start;
//     float denom = dot(N, ray_dir);
// 
//     // Avoid division by zero
//     if (abs(denom) > 1e-6)
//     {
//         float t = dot(N, p_plane-ray_start) / denom;
// 
//         // Check if t is within the valid range [0, 1]
//         if (t >= 0.0 && t <= 1.0)
//         {
//             return t;
//         }
//         else
//         {
//             return -1.0; // Intersection is outside the ray segment
//         }
//     }
//     else
//     {
//         return -1.0; // Ray is parallel to the plane
//     }
// }

void main()
{
    float depth_ndc = 1;
    vec4 colr = vec4(0,0,0,0);

    // Grid
    if(show_grid > 0)
    {
        // intersection
        float t = -nearp_w.y / (farp_w.y-nearp_w.y);
        vec4 intersect_w = nearp_w + t*(farp_w-nearp_w);
        vec4 intersect_v = view * intersect_w;

        // Update ndc depth
        vec4 ndc = proj_view * intersect_w;
        ndc /= ndc.w;
        depth_ndc = ndc.z;
        // out_color = grid(intersect_vs, 1) * float(t>0);
        colr = grid(intersect_w,1);

        // Blend, fade out when it's too far from eye
        float alpha = 1-smoothstep(znear, zfar, intersect_v.z);
        // float alpha = 1-smoothstep(0, 100, intersect_v.z);
        colr.a *= alpha;
    }
    else
    {
        discard;
    }

    out_id = vec4(0,0,0,0);
    out_normal_depth = vec4(0,0,0,1);
    out_color = colr;
    gl_FragDepth = depth_ndc;
}
