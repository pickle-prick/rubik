#version 450

// Input
layout(location = 0)       in   vec3 near_p;
layout(location = 1)       in   vec3 far_p;
layout(location = 2)  flat in   mat4 view;
layout(location = 6)  flat in   mat4 proj;
layout(location = 10) flat in   vec4 gizmos_origin;
layout(location = 11) flat in   mat4 gizmos_xform;
layout(location = 15) flat in   uint show_grid;
layout(location = 16) flat in   uint show_gizmos;

// Output
layout(location = 0) out vec4  out_color;
layout(location = 1) out uvec2 out_id;

// Predefined keys
uvec2 key_gizmos_ihat = uvec2(0xFFFFFFFF-0, 0xFFFFFFFF);
uvec2 key_gizmos_jhat = uvec2(0xFFFFFFFF-1, 0xFFFFFFFF);
uvec2 key_gizmos_khat = uvec2(0xFFFFFFFF-2, 0xFFFFFFFF);
uvec2 key_grid        = uvec2(0xFFFFFFFF-3, 0xFFFFFFFF);

//////////////////////////////
//~ Transform

mat4 translate(vec3 t)
{
    return mat4(
        1.0, 0.0, 0.0,   0,
        0.0, 1.0, 0.0,   0,
        0.0, 0.0, 1.0,   0,
        t.x, t.y, t.z, 1.0
    );
}

float ndc_depth(vec3 pos)
{
    vec4 clip_pos = proj * view * vec4(pos.xyz, 1.0);
    return clip_pos.z / clip_pos.w;
}

float view_depth(vec3 pos)
{
    return (view * vec4(pos.xyz, 1.0)).z;
}

//////////////////////////////
//~ SDF

float sd_sphere(vec3 p, float s)
{
    return length(p)-s;
}

//- Cone-exact
float sd_cone(vec3 p, vec2 c, float h)
{
    // c is the sin/cos of the angle, h is height
    // Alternatively pass q instead of (c,h),
    // which is the point at the base in 2D
    vec2 q = h*vec2(c.x/c.y,-1.0);

    vec2 w = vec2( length(p.xz), p.y );
    vec2 a = w - q*clamp( dot(w,q)/dot(q,q), 0.0, 1.0 );
    vec2 b = w - q*vec2( clamp( w.x/q.x, 0.0, 1.0 ), 1.0 );
    float k = sign( q.y );
    float d = min(dot( a, a ),dot(b, b));
    float s = max( k*(w.x*q.y-w.y*q.x),k*(w.y-q.y)  );
    return sqrt(d)*sign(s);
}

float sd_capped_cylinder(vec3 p, vec3 a, vec3 b, float r)
{
    vec3  ba = b - a;
    vec3  pa = p - a;
    float baba = dot(ba,ba);
    float paba = dot(pa,ba);
    float x = length(pa*baba-ba*paba) - r*baba;
    float y = abs(paba-baba*0.5)-baba*0.5;
    float x2 = x*x;
    float y2 = y*y*baba;
    float d = (max(x,y)<0.0)?-min(x2,y2):(((x>0.0)?x2:0.0)+((y>0.0)?y2:0.0));
    return sign(d)*sqrt(abs(d))/baba;
}

float sdf_rect(in vec2 p, in vec2 b)
{
    vec2 d = abs(p)-b;
    return length(max(d,0.0)) + min(max(d.x,d.y),0.0);
}

float sdf_segment(in vec2 p, in vec2 a, in vec2 b)
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0);
    return length(pa - ba*h);
}

vec4 grid(vec3 pos, float scale) {
    vec2 coord = pos.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord-0.5)-0.5) / (derivative*2);
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    // minimumz *= 19;
    // minimumx *= 89;

    vec4 color = vec4(0.2, 0.2, 0.2, 1.0-min(line, 1.0));

    // z axis
    // if(pos.x > -0.01 && pos.x < 0.01) {
    if(pos.x > -4 * minimumx && pos.x < 4 * minimumx) {
        color.r = 1.0;
    }

    // x axis
    if(pos.z > -4 * minimumz && pos.z < 4 * minimumz) {
    // if(pos.z > -0.1 * minimumz && pos.z < 0.1 * minimumz) {
        color.b = 1.0;
    }

    // z axis
    // if(pos.x > -0.1 && pos.x < 0.1) {
    //     color.b = 1.0;
    //     color.a = 1.0;
    // }

    // // x axis
    // if(pos.z > -0.1 && pos.z < 0.1) {
    //     color.x = 1.0;
    //     color.a = 1.0;
    // }

    return color;
}

float plane_intersect(vec3 ray_start, vec3 ray_end, vec3 N, vec3 p_plane)
{
    // return dot(N,(p_plane-ray_start)) / dot(N,(ray_end-ray_start));
    vec3 ray_dir = ray_end - ray_start;
    float denom = dot(N, ray_dir);

    // Avoid division by zero
    if (abs(denom) > 1e-6)
    {
        float t = dot(N, p_plane - ray_start) / denom;

        // Check if t is within the valid range [0, 1]
        if (t >= 0.0 && t <= 1.0)
        {
            return t;
        }
        else
        {
            return -1.0; // Intersection is outside the ray segment
        }
    }
    else
    {
        return -1.0; // Ray is parallel to the plane
    }
}

vec4 gizmos(inout float depth, inout uvec2 key)
{
    vec4 color = vec4(0);
    depth = 1.0;

    vec3 i_hat = normalize(gizmos_xform[0].xyz); // First column
    vec3 j_hat = normalize(gizmos_xform[1].xyz); // Second column
    vec3 k_hat = normalize(gizmos_xform[2].xyz); // Third column

    vec3 ro = (inverse(view) * vec4(0,0,0,1)).xyz; // Eye position
    vec3 rd = normalize(far_p-near_p); // Ray direction

    // Raymarching
    float t        = 0; // Total distance travelled (also known depth from eye standpoint)
    int hit        = 0;
    vec4 hit_color = vec4(0);
    uvec2 hit_key  = uvec2(0);

    float dist     = length(view*gizmos_origin);
    vec3 origin    = gizmos_origin.xyz;
    float center_r = 0.04 * dist;
    float axis_r   = center_r/2.0;
    float axis_l   = 0.3 * dist;
    vec3 i_end     = origin+(i_hat*axis_l);
    vec3 j_end     = origin-(j_hat*axis_l);
    vec3 k_end     = origin+(k_hat*axis_l);

    for(int i = 0; i < 30; i++)
    {
        vec3 p = ro + rd*t;
        vec3 relative_p = p - origin;

        float min_d    = 99999;
        vec4 min_color = vec4(0.0);
        uvec2 min_key  = uvec2(0);

        // Distance to the scene
        {
            // Origin point
            float d_sphere = sd_sphere(relative_p, center_r);
            min_d          = d_sphere;
            min_color      = vec4(0.953,0.776,0.137,1.0);

            // Y Axis
            float d_cyl_y    = sd_capped_cylinder(p, origin, j_end, axis_r);
            bool is_closer_y = d_cyl_y < min_d;
            min_d            = is_closer_y ? d_cyl_y : min_d;
            min_color        = is_closer_y ? vec4(0,1,0,1) : min_color;
            min_key          = is_closer_y ? key_gizmos_jhat : min_key;

            // X Axis
            float d_cyl_x    = sd_capped_cylinder(p, origin, i_end, axis_r);
            bool is_closer_x = d_cyl_x < min_d;
            min_d            = is_closer_x ? d_cyl_x : min_d;
            min_color        = is_closer_x ? vec4(0,0,1,1) : min_color;
            min_key          = is_closer_x ? key_gizmos_ihat : min_key;

            // Z Axis
            float d_cyl_z    = sd_capped_cylinder(p, origin, k_end, axis_r);
            bool is_closer_z = d_cyl_z < min_d;
            min_d            = is_closer_z ? d_cyl_z : min_d;
            min_color        = is_closer_z ? vec4(1,0,0,1) : min_color;
            min_key          = is_closer_z ? key_gizmos_khat : min_key;
        }

        t+=min_d; // "Marching" the ray

        // TODO: we may need to pass the near and far here
        if(min_d < 0.01)
        {
            hit       = 1;
            hit_color = min_color;
            hit_key   = min_key;
            break;
        }

        if(min_d > 200) { break; }
    }

    color = hit > 0 ? hit_color : color;
    key = hit > 0 ? hit_key : key;
    depth = hit > 0 ? 0.0f : depth;
    return color;

    // NOTE(k): old code for plane intersection
    // ZY plane
    // float i_t = plane_intersect(near_p, far_p, i_hat, gizmos_origin.xyz);
    // XZ plane
    // float j_t = plane_intersect(near_p, far_p, j_hat, gizmos_origin.xyz);
    // XY plane
    // float k_t = plane_intersect(near_p, far_p, k_hat, gizmos_origin.xyz);

    // // Transform coordinate to local
    // mat4 m = mat4(
    //     i_hat.x, i_hat.y, i_hat.z, 0,
    //     j_hat.x, j_hat.y, j_hat.z, 0,
    //     k_hat.x, k_hat.y, k_hat.z, 0,
    //     0,       0,       0,       1
    // );
    // m = translate(origin) * m;
    // mat4 m_inv = inverse(gizmos_xform);

    // ZY plane
    // if(i_t>0.001)
    // {
    //     vec3 p = near_p + i_t*(far_p-near_p);
    //     float p_depth = ndc_depth(p);
    //     if(p_depth < depth)
    //     {
    //         vec3 loc_p = (m_inv*vec4(p.xyz, 1.0)).xyz;
    //         // vec3 dpdy = dFdy(loc_p);
    //         // float dy = dpdy.y;
    //         vec3 dp= fwidth(loc_p);
    //         // TODO: we may need to clmap to some range
    //         float dz = dp.z;

    //         float half_line_width = 9;
    //         // Y-Axis in ZY plane
    //         if(loc_p.y < 0.0 && loc_p.y > -3 &&
    //            loc_p.z > -(half_line_width*dz) && loc_p.z < (half_line_width*dz) &&
    //            loc_p.x > -0.01 && loc_p.x < 0.01)
    //         {
    //             float a = smoothstep(0,half_line_width*dz,loc_p.z);
    //             color = vec4(1,0,0,1-a);
    //             depth = p_depth;
    //         }
    //     }
    // }
}

void main() {
    float depth = 0;
    uvec2 key = uvec2(0);

    // Grid
    if(show_grid > 0)
    {
        float t = -near_p.y / (far_p.y-near_p.y);
        vec3 intersect = near_p + t*(far_p-near_p);
        // Update ndc depth
        depth = ndc_depth(intersect);
        // out_color = grid(intersect,1) * float(t>0);
        out_color = grid(intersect,1);

        // Extract near and far from proj matrix
        float near = -proj[3][2] / proj[2][2];
        float far  = (proj[2][2]*near) / (proj[2][2]-1);

        // Blend, fade out when it's too far from eye
        float view_z = view_depth(intersect);
        float alpha = 1-smoothstep(near, far/3.f, view_z);
        out_color.a *= alpha;
        key = key_grid;
    }

    // Gizmos
    if(show_gizmos > 0)
    {
        float local_depth;
        uvec2 local_key; 
        vec4 color = gizmos(local_depth, local_key);
        if(color.a > 0.0)
        {
            out_color = color;
            // Update ndc depth
            depth = local_depth;
            key = local_key;
        }
    }
    
    out_id = key;
    gl_FragDepth = depth;
}
