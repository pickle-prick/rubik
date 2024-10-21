#define G_StackTopImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.top->v;

#define G_StackBottomImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.bottom_val;

#define G_StackPushImpl(state, name_upper, name_lower, type, new_value) \
G_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(g_state->arena, G_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define G_StackPopImpl(state, name_upper, name_lower) \
G_##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
SLLStackPop(state->name_lower##_stack.top);\
SLLStackPush(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define G_StackSetNextImpl(state, name_upper, name_lower, type, new_value) \
G_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(g_state->arena, G_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;

#include "generated/game.meta.c"

internal void
g_init(OS_Handle os_wnd) {
    Arena *arena = arena_alloc();
    g_state = push_array(arena, G_State, 1);
    g_state->arena = arena;
    g_state->node_bucket = g_bucket_make(arena, 3000);
    g_state->os_wnd = os_wnd;

    G_InitStacks(g_state)
    G_InitStackNils(g_state)
}

/////////////////////////////////
// Key

internal G_Key
g_key_from_string(String8 string)
{
    G_Key result = {0};
    result.u64[0] = g_hash_from_string(5381, string);
    return result;
}

internal B32
g_key_match(G_Key a, G_Key b)
{
    return a.u64[0] == b.u64[0];
}

internal G_Key
g_key_zero()
{
    return (G_Key){0};
}

/////////////////////////////////
// Bucket

internal G_Node *
g_node_from_string(G_Bucket *bucket, String8 string)
{
    G_Key key = g_key_from_string(string);
    return g_node_from_key(bucket, key);
}

internal G_Node *
g_node_from_key(G_Bucket *bucket, G_Key key)
{
    G_Node *result = 0;
    U64 slot_idx = key.u64[0] % bucket->node_hash_table_size;
    for(G_Node *node = bucket->node_hash_table[slot_idx].first; node != 0; node = node->hash_next) {
        if(g_key_match(node->key, key)) {
            result = node;
            break;
        }
    }
    return result;
}

internal G_Bucket *
g_bucket_make(Arena *arena, U64 hash_table_size)
{
    G_Bucket *result = push_array(arena, G_Bucket, 1);
    result->node_hash_table = push_array(arena, G_BucketSlot, hash_table_size);
    result->node_hash_table_size = hash_table_size;
    result->arena = arena;
    return result;
}

/////////////////////////////////
// AnimatedSprite2D

// internal G_SpriteSheet2D *
// g_spritesheet_2d_from_file(Arena *arena, String8 path, String8 meta_path) {
//     G_SpriteSheet2D *result = 0;
//     result = push_array(arena, G_SpriteSheet2D, 1);
// 
//     int x,y,n; 
//     U8 *pixels = stbi_load((char *)path.str, &x, &y, &n, 4);
//     R_Handle texture = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Nearest, v2s32(x,y), R_Tex2DFormat_RGBA8, pixels);
//     stbi_image_free(pixels);
// 
//     // Parse meta file 
//     G_Sprite2D_FrameTag *tags  = 0;
//     U64 tag_count   = 0;
//     G_Sprite2D_Frame *frames   = 0;
//     U64 frame_count = 0;
// 
//     {
//         Temp scratch = scratch_begin(0,0);
//         U8 *s;
//         U64 size;
//         FileReadAll(scratch.arena, meta_path, &s, &size);
//         cJSON *root = cJSON_ParseWithLength((char *)s, size);
// 
//         // Find frames and meta obj
//         cJSON *frames_json = 0;
//         cJSON *meta_json   = 0;
//         for(cJSON *child = root->child; child != 0; child = child->next) {
//             if(cJSON_IsObject(child) && strcmp(child->string, "frames") == 0) {
//                 frames_json = child;
//             }
//             if(cJSON_IsObject(child) && strcmp(child->string, "meta") == 0) {
//                 meta_json = child;
//             }
//         }
// 
//         // Parse meta
//         for(cJSON *meta = meta_json->child; meta != 0; meta = meta->next) {
//             if(cJSON_IsArray(meta) && strcmp(meta->string, "frameTags") == 0) {
//                 for(cJSON *tag = meta->child; tag != 0; tag = tag->next) { tag_count++; }
//                 tags = push_array(arena, G_Sprite2D_FrameTag, tag_count);
//                 U64 tag_idx = 0;
//                 for(cJSON *tag = meta->child; tag != 0; tag = tag->next, tag_idx++) {
//                     for(cJSON *key = tag->child; key != 0; key = key->next) {
//                         if(cJSON_IsString(key) && strcmp(key->string, "name") == 0) {
//                             tags[tag_idx].name = push_str8_copy(arena, str8_cstring(key->valuestring));
//                         }
// 
//                         if(cJSON_IsNumber(key) && strcmp(key->string, "from") == 0) {
//                             tags[tag_idx].from = key->valueint;
//                         }
// 
//                         if(cJSON_IsNumber(key) && strcmp(key->string, "to") == 0) { 
//                             tags[tag_idx].to = key->valueint;
//                         }
//                     }
//                 }
//             }
//         }
// 
//         // Find out frame count
//         AssertAlways(tag_count > 0);
//         frame_count = tags[tag_count-1].to + 1;
//         AssertAlways(frame_count > 0 && frame_count < 1000);
//         frames = push_array(arena, G_Sprite2D_Frame, frame_count);
// 
//         // Parse frames
//         U64 frame_idx = 0; 
//         for(cJSON *frame = frames_json->child; frame != 0; frame = frame->next, frame_idx++) {
//             AssertAlways(cJSON_IsObject(frame));
//             frames[frame_idx].texture = texture;
// 
//             for(cJSON *key = frame->child; key != 0; key = key->next) {
//                 if(cJSON_IsNumber(key) && strcmp(key->string, "duration") == 0) {
//                     frames[frame_idx].duration = key->valueint;
//                 }
// 
//                 if(cJSON_IsObject(key) && strcmp(key->string, "frame") == 0) {
//                     for(cJSON *frame_key = key->child; frame_key != 0; frame_key = frame_key->next) {
//                         if(cJSON_IsNumber(frame_key) && strcmp(frame_key->string, "x") == 0) {
//                             frames[frame_idx].x = frame_key->valueint;
//                         }
//                         if(cJSON_IsNumber(frame_key) && strcmp(frame_key->string, "y") == 0) {
//                             frames[frame_idx].y = frame_key->valueint;
//                         }
//                         if(cJSON_IsNumber(frame_key) && strcmp(frame_key->string, "w") == 0) {
//                             frames[frame_idx].w = frame_key->valueint;
//                         }
//                         if(cJSON_IsNumber(frame_key) && strcmp(frame_key->string, "h") == 0) {
//                             frames[frame_idx].h = frame_key->valueint;
//                         }
//                     }
//                 }
//             }
//         }
//         scratch_end(scratch);
//         cJSON_Delete(root);
//     }
// 
//     result->texture     = texture;
//     result->w           = x;
//     result->h           = y;
//     result->tags        = tags;
//     result->tag_count   = tag_count;
//     result->frames      = frames;
//     result->frame_count = frame_count;
//     return result;
// }


/////////////////////////////////
// Node build api

internal G_Node *
g_build_node_from_string(String8 string) {
    Arena *arena = g_top_bucket()->arena;
    G_Key key = g_key_from_string(string);
    G_Node *node = g_build_node_from_key(key);
    node->name = push_str8_copy(arena, string);
    return node;
}

internal G_Node *
g_build_node_from_key(G_Key key) {
    G_Node *parent = g_top_parent();
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    G_Node *result = push_array(arena, G_Node, 1);
    // Fill info
    {
        result->key         = key;
        result->pos         = v3f32(0,0,0);
        result->pos_delta   = v3f32(0,0,0);
        result->rot         = make_indentity_quat_f32();
        result->rot_delta   = make_indentity_quat_f32();
        result->scale       = v3f32(1,1,1);
        result->scale_delta = v3f32(0,0,0);
        result->base_xform  = mat_4x4f32(1.0f);
        result->parent      = parent;
    }

    // Insert to the bucket
    U64 slot_idx = result->key.u64[0] % bucket->node_hash_table_size;
    DLLPushBack_NP(bucket->node_hash_table[slot_idx].first, bucket->node_hash_table[slot_idx].last, result, hash_next, hash_prev);
    bucket->node_count++;

    // Insert to the parent tree
    if(parent != 0)
    {
        DLLPushBack_NP(parent->first, parent->last, result, next, prev);
    }
    return result;
}

internal G_Node *
g_node_camera3d_alloc(String8 string)
{
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    G_Node *result = g_build_node_from_string(string);
    {
        result->kind           = G_NodeKind_Camera3D;
    }
    return result;
}

internal G_Node *
g_node_camera_mesh_inst3d_alloc(String8 string) 
{
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    G_Node *result = g_build_node_from_string(string);
    {
        result->kind = G_NodeKind_MeshInstance3D;
    }
    return result;
}

internal G_Mesh *g_mesh_alloc() 
{
    G_Scene *scene = g_top_scene();
    G_Mesh *result = scene->first_free_mesh;
    if(result == 0)
    {
        result = push_array_no_zero(scene->arena, G_Mesh, 1);
    }
    else 
    {
        SLLStackPop(scene->first_free_mesh);
    }
    MemoryZeroStruct(result);
    return result;
}

// internal G_Node *
// g_n2d_anim_sprite2d_from_spritesheet(Arena *arena, String8 string, G_SpriteSheet2D *spritesheet2d) {
//     G_Node *result = g_build_node2d_from_string(arena, string);
//     result->kind = G_NodeKind_AnimatedSprite2D;
// 
//     U64 anim2d_hash_table_size = 3000;
//     G_Animation2DHashSlot *anim2d_hash_table = push_array(arena, G_Animation2DHashSlot, anim2d_hash_table_size);
// 
//     for(U64 i = 0; i < spritesheet2d->tag_count; i++) {
//         G_Sprite2D_FrameTag *tag = &spritesheet2d->tags[i];
//         U64 animation_key = g_hash_from_string(5381, tag->name);
//         U64 slot_idx = animation_key % anim2d_hash_table_size;
// 
//         G_Animation2D *anim2d = push_array(arena, G_Animation2D, 1);
// 
//         F32 total_duration = 0;
//         U64 frame_count = tag->to - tag->from + 1;
//         G_Sprite2D_Frame *first_frame = spritesheet2d->frames + tag->from;
//         for(U64 frame_idx = 0; frame_idx < frame_count; frame_idx++) {
//             total_duration += (first_frame+frame_idx)->duration;
//         }
// 
//         anim2d->key         = animation_key;
//         anim2d->frames      = first_frame;
//         anim2d->frame_count = frame_count;
//         anim2d->duration    = total_duration;
//         DLLPushBack_NP(anim2d_hash_table[slot_idx].first, anim2d_hash_table[slot_idx].last, anim2d, hash_next, hash_prev);
//         AssertAlways(total_duration > 0);
//     }
// 
//     result->anim_sprite2d.flip_h      = 0;
//     result->anim_sprite2d.flip_v      = 0;
//     result->anim_sprite2d.speed_scale = 1.0f;
//     result->anim_sprite2d.anim2d_hash_table_size = anim2d_hash_table_size;
//     result->anim_sprite2d.anim2d_hash_table = anim2d_hash_table;
//     return result;
// }

/////////////////////////////////
// Node Type Functions

internal G_Node *
g_node_df(G_Node *n, G_Node* root, U64 sib_member_off, U64 child_member_off)
{
    G_Node *result = 0;
    if(*MemberFromOffset(G_Node**, n, child_member_off) != 0)
    {
        result = *MemberFromOffset(G_Node **, n, child_member_off);
    } 
    else for(G_Node *p = n; p != root; p = p->parent)
    {
        if(*MemberFromOffset(G_Node **, p, sib_member_off) != 0)
        {
            result = *MemberFromOffset(G_Node **, p, sib_member_off);
            break;
        }
    }
    return result;
}

internal Mat4x4F32 g_xform_from_node(G_Node *node)
{
    Mat4x4F32 result = node->base_xform;

    QuatF32 rot   = mul_quat_f32(node->rot_delta, node->rot);
    Vec3F32 pos   = add_3f32(node->pos_delta, node->pos);
    Vec3F32 scale = add_3f32(node->scale_delta, node->scale);

    Mat4x4F32 rot_m = mat_4x4f32_from_quat_f32(rot);
    Mat4x4F32 tra_m = make_translate_4x4f32(pos);
    Mat4x4F32 sca_m = make_scale_4x4f32(scale);

    result = mul_4x4f32(sca_m, result);
    result = mul_4x4f32(rot_m, result);
    result = mul_4x4f32(tra_m, result);

    if(node->parent != 0)
    {
        result = mul_4x4f32(result, node->parent->fixed_xform);
    }
    return result;
}

internal void
g_local_coord_from_node(G_Node *n, Vec3F32 *f, Vec3F32 *s, Vec3F32 *u)
{
    Mat4x4F32 xform = n->fixed_xform;
    // NOTE: remove translate, only rotation matters
    xform.v[3][0] = xform.v[3][1] = xform.v[3][2] = 0;
    Vec4F32 i_hat = v4f32(1,0,0,1);
    Vec4F32 j_hat = v4f32(0,1,0,1);
    Vec4F32 k_hat = v4f32(0,0,1,1);

    i_hat = mat_4x4f32_transform_4f32(xform, i_hat);
    j_hat = mat_4x4f32_transform_4f32(xform, j_hat);
    k_hat = mat_4x4f32_transform_4f32(xform, k_hat);

    MemoryCopy(f, &k_hat, sizeof(Vec3F32));
    MemoryCopy(s, &i_hat, sizeof(Vec3F32));
    MemoryCopy(u, &j_hat, sizeof(Vec3F32));

    // Vec4F32 side    = v4f32(1,0,0,0);
    // Vec4F32 up      = v4f32(0,-1,0,0);
    // Vec4F32 forward = v4f32(0,0,1,0);

    // side    = mul_quat_f32(rot_conj,side);
    // side    = mul_quat_f32(side,rot);
    // up      = mul_quat_f32(rot_conj,up);
    // up      = mul_quat_f32(up,rot);
    // forward = mul_quat_f32(rot_conj,forward);
    // forward = mul_quat_f32(forward,rot);

    // MemoryCopy(f, &forward, sizeof(Vec3F32));
    // MemoryCopy(s, &side, sizeof(Vec3F32));
    // MemoryCopy(u, &up, sizeof(Vec3F32));
}

internal Mat4x4F32
g_view_from_node(G_Node *node)
{
    return inverse_4x4f32(node->fixed_xform);
}

/////////////////////////////////
// Physics

// internal void
// g_physics_dynamic_root(G_Node *node, F32 dt) {
//     // TODO: update physics dynamic in timely manner to avoid vary delta t
//     switch(node->rigidbody2d.physics_kind)
//     {
//         default:{}break;
//         case G_PhysicsKind_Dynamic:
//         {
//             // Euler Integration
//             Vec2F32 d_acc = {0};
//             d_acc.x = node->rigidbody2d.force.x / node->rigidbody2d.mass;
//             d_acc.y = node->rigidbody2d.force.y / node->rigidbody2d.mass;
// 
//             node->rigidbody2d.velocity.x += d_acc.x * dt;
//             node->rigidbody2d.velocity.y += d_acc.y * dt;
// 
//             Vec2F32 d_pos = {0, 0};
//             d_pos.x = node->rigidbody2d.velocity.x * dt;
//             d_pos.y = node->rigidbody2d.velocity.y * dt;
// 
//             // Update position
//             node->pos.x += d_pos.x;
//             node->pos.y += d_pos.y;
// 
//         }break;
//         case G_PhysicsKind_Kinematic:
//         {
//             // TODO: this is wrong
//             // Velocity Verlet
//             G_RigidBody2D *body = &node->rigidbody2d;
//             Vec2F32 d_pos = add_2f32(scale_2f32(body->velocity, dt),
//                                     (scale_2f32(body->acc, 0.5f*dt*dt)));
//             node->pos = add_2f32(node->pos, d_pos);
//             Vec2F32 d_velocity = scale_2f32(body->acc, dt);
//             body->velocity = add_2f32(body->velocity, d_velocity);
//         }break;
//     }
// 
//     for(G_Node *child = node->first; child != 0; child = child->next) {
//         g_physics_dynamic_root(child, dt);
//     }
// }

/////////////////////////////////
// Magic

G_SHAPE_SUPPORT_FN(G_SHAPE_RECT_SUPPORT_FN) {
    Rng2F32 *rect = (Rng2F32 *)shape_data; 

    Vec2F32 p_00 = rect->p0;
    Vec2F32 p_10 = v2f32(rect->p1.x, rect->p0.y);
    Vec2F32 p_01 = v2f32(rect->p0.x, rect->p1.y);
    Vec2F32 p_11 = rect->p1;

    F32 dot_00 = dot_2f32(p_00, direction);
    F32 dot_10 = dot_2f32(p_10, direction);
    F32 dot_01 = dot_2f32(p_01, direction);
    F32 dot_11 = dot_2f32(p_11, direction);

    Vec2F32 result = p_00;
    F32 max_dot = dot_00;

    if(dot_10 > max_dot) { result = p_10; max_dot = dot_10; }
    if(dot_01 > max_dot) { result = p_01; max_dot = dot_01; }
    if(dot_11 > max_dot) { result = p_11; max_dot = dot_11; }
    return result;
}

// internal B32 
// gjk(Vec2F32 s1_center, Vec2F32 s2_center, void *s1_data, void *s2_data, G_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, G_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec2F32 simplex[3])
// {
//     B32 result = 0;
// 
//     // Pick a starting direction
//     // NOTE(k): we may want to use middle point of the two shapes as the starting direction, especially for rect
//     Vec2F32 d = sub_2f32(s2_center, s1_center);
//     if(d.x == 0 && d.y == 0)
//     {
//         d.x = 1.0f;
//     }
// 
//     Vec2F32 *C = &simplex[0];
//     Vec2F32 *B = &simplex[1];
//     Vec2F32 *A = &simplex[2];
//     Vec2F32 O = {0,0};
// 
//     // Find C
//     *C = sub_2f32(s1_support_fn(s1_data, d), s2_support_fn(s2_data, negate_2f32(d))); // C
//     if(dot_2f32(d,*C) <= 0)
//     {
//         return result;
//     }
// 
//     // Find B
//     d = sub_2f32(O,*C); // CO->
//     d = normalize_2f32(d);
//     *B = sub_2f32(s1_support_fn(s1_data, d), s2_support_fn(s2_data, negate_2f32(d))); // B
//     // CO dot OB < 0
//     if(dot_2f32(d,*B) <= 0)
//     {
//         return result;
//     }
// 
//     while(1)
//     {
//         // (BC X BO) X BC
//         Vec2F32 BC = sub_2f32(*C,*B);
//         Vec2F32 BO = sub_2f32(O,*B);
//         // NOTE: cb_perp could be (0,0) if the origin lies on the BC
//         Vec2F32 cb_perp = triple_product_2f32(BC, BO, BC);
//         AssertAlways(!(cb_perp.x == 0 && cb_perp.y == 0));
//         cb_perp = normalize_2f32(cb_perp);
// 
//         // Find point A
//         *A = sub_2f32(s1_support_fn(s1_data, cb_perp), s2_support_fn(s2_data, negate_2f32(cb_perp))); // A
//         AssertAlways(!(A->y == 0 && B->y == 0 && C->y == 0));
// 
//         // cb_perp dot OA < 0
//         if(dot_2f32(cb_perp, *A) < 0)
//         {
//             break;
//         }
// 
//         // Check if origin is within the simplex
//         // Since we have more info about how we find the simplex, we can simplified this
//         Vec2F32 AB = sub_2f32(*B,*A);
//         Vec2F32 AC = sub_2f32(*C,*A);
//         // (AC X AB) X AB
//         Vec2F32 ab_perp = triple_product_2f32(AC, AB, AB);
//         // AC X (AC X AB)
//         Vec2F32 ac_perp = triple_product_2f32(AB, AC, AC);
// 
//         Vec2F32 AO = sub_2f32(O,*A);
//         // origin is in AB area
//         if(dot_2f32(AO, ab_perp) > 0)
//         {
//             // BA is the new CB
//             *C = *B;
//             *B = *A;
//             continue;
//         }
//         // origin is in AC area
//         Vec2F32 CO = sub_2f32(O,*C);
//         if(dot_2f32(CO, ac_perp) > 0)
//         {
//             *B = *A;
//             continue;
//         }
// 
//         result = 1;
//         break;
//     }
//     return result;
// }

internal Vec2F32
triple_product_2f32(Vec2F32 a, Vec2F32 b, Vec2F32 c)
{
    // Triple product expansion is used to calculate perpendicular normal vectors 
    // which kinda 'prefer' pointing towards the Origin in Minkowski space
    Vec2F32 r;
    
    float ac = a.x * c.x + a.y * c.y; // perform a.dot(c)
    float bc = b.x * c.x + b.y * c.y; // perform b.dot(c)
    
    // perform b * a.dot(c) - a * b.dot(c)
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;

    // Version -1
    // Vec3F32 a = {A.x, A.y, 0};
    // Vec3F32 b = {B.x, B.y, 0};
    // Vec3F32 c = {C.x, C.y, 0};

    // Vec3F32 r = cross_3f32(a,b);
    // r = cross_3f32(r, c);
    // return v2f32(r.x, r.y);
}

internal B32
p_in_triangle(Vec2F32 P, Vec2F32 A, Vec2F32 B, Vec2F32 C) {
    // Compute vectors        
    Vec2F32 v0 = sub_2f32(C,A);
    Vec2F32 v1 = sub_2f32(B,A);
    Vec2F32 v2 = sub_2f32(P,A);

    // Compute dot products
    F32 dot00 = dot_2f32(v0, v0);
    F32 dot01 = dot_2f32(v0, v1);
    F32 dot02 = dot_2f32(v0, v2);
    F32 dot11 = dot_2f32(v1, v1);
    F32 dot12 = dot_2f32(v1, v2);

    // Compute barycentric coordinates
    F32 invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    F32 u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    F32 v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v < 1);
}

// internal G_Contact2D
// epa(Vec2F32 simplex[3], void *s1_data, void *s2_data,
//     G_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, G_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn)
// {
//     // NOTE(k): there are edge cases when three points all reside on the x axis 
//     // TODO: fix it later
//     // Assert(p_in_triangle(v2f32(0,0), simplex[0], simplex[1], simplex[2]));
// 
//     G_Contact2D result = {0};
//     if(!(p_in_triangle(v2f32(0,0), simplex[0], simplex[1], simplex[2]))) { return result; }
//     Temp temp = scratch_begin(0,0);
// 
//     G_Vertex2DNode *first_vertex = 0;
//     G_Vertex2DNode *last_vertex  = 0;
// 
//     for(U64 i = 0; i < 3; i++) 
//     {
//         G_Vertex2DNode *n = push_array(temp.arena, G_Vertex2DNode, 1);
//         n->v = simplex[i];
//         DLLPushBack(first_vertex, last_vertex, n);
//     }
//     last_vertex->next  = first_vertex;
//     first_vertex->prev = last_vertex;
// 
//     U64 vertices_count = 3;
//     while(1)
//     {
//         F32 min_dist       = FLT_MAX;
//         Vec2F32 normal     = {0};
//         G_Vertex2DNode *aa = 0;
//         G_Vertex2DNode *bb = 0;
// 
//         G_Vertex2DNode *a = first_vertex;
//         G_Vertex2DNode *b = a->next;
//         do
//         {
//             AssertAlways(vertices_count < 100);
//             Vec2F32 edge = sub_2f32(b->v, a->v);
//             Vec2F32 edge_perp = normalize_2f32(v2f32(edge.y, -edge.x));
//             F32 dist = dot_2f32(edge_perp, a->v);
// 
//             if(dist < 0)
//             {
//                 dist *= -1;
//                 edge_perp.x *= -1;
//                 edge_perp.y *= -1;
//             }
// 
//             if(dist < min_dist)
//             {
//                 min_dist = dist; 
//                 normal = edge_perp;
//                 aa = a;
//                 bb = b;
//             }
//             a = b;
//             b = b->next;
//         } while(a != first_vertex);
//         AssertAlways(min_dist >= 0);
// 
//         Vec2F32 support = sub_2f32(s1_support_fn(s1_data, normal), s2_support_fn(s2_data, negate_2f32(normal)));
//         F32 s_dist      = dot_2f32(support, normal);
//         AssertAlways(s_dist >= 0 || s_dist >= -0.01);
// 
//         if(abs_f32(s_dist-min_dist) > 0.1f)
//         {
//             G_Vertex2DNode *n = push_array(temp.arena, G_Vertex2DNode, 1);
//             n->v = support;
//             aa->next = n;
//             bb->prev = n;
//             n->prev = aa;
//             n->next = bb;
//             vertices_count++; 
//         }
//         else
//         {
//             result.normal = normal;
//             result.length = min_dist;
//             break;
//         }
//     }
// 
//     scratch_end(temp);
//     return result;
// }

/////////////////////////////////
// Camera

/////////////////////////////////
// Helpers

internal U64
g_hash_from_string(U64 seed, String8 string)
{
    U64 result = seed;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}
