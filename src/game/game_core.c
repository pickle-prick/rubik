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
g_init(OS_Handle os_wnd)
{
    Arena *arena = arena_alloc();
    g_state = push_array(arena, G_State, 1);
    g_state->arena = arena;
    g_state->frame_arena = arena_alloc();
    g_state->node_bucket = g_bucket_make(arena, 3000);
    g_state->os_wnd = os_wnd;
    g_state->mesh_cache_table.slot_count = 1000;
    g_state->mesh_cache_table.arena = arena;
    g_state->mesh_cache_table.slots = push_array(arena, G_MeshCacheSlot, g_state->mesh_cache_table.slot_count);

    // Fonts
    g_state->cfg_font_tags[G_FontSlot_Main]  = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
    g_state->cfg_font_tags[G_FontSlot_Code]  = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
    g_state->cfg_font_tags[G_FontSlot_Icons] = f_tag_from_path(str8_lit("./fonts/icons.ttf"));

    // Theme 
    // MemoryCopy(g_state->cfg_theme_target.colors, rd_theme_preset_colors__vs_dark, sizeof(rd_theme_preset_colors__vs_dark));
    // MemoryCopy(g_state->cfg_theme.colors, rd_theme_preset_colors__vs_dark, sizeof(rd_theme_preset_colors__vs_dark));
    MemoryCopy(g_state->cfg_theme_target.colors, rd_theme_preset_colors__handmade_hero, sizeof(rd_theme_preset_colors__handmade_hero));
    MemoryCopy(g_state->cfg_theme.colors, rd_theme_preset_colors__handmade_hero, sizeof(rd_theme_preset_colors__handmade_hero));
    // MemoryCopy(g_state->cfg_theme_target.colors, rd_theme_preset_colors__far_manager, sizeof(rd_theme_preset_colors__far_manager));
    // MemoryCopy(g_state->cfg_theme.colors, rd_theme_preset_colors__far_manager, sizeof(rd_theme_preset_colors__far_manager));

    //////////////////////////////
    //- k: compute ui palettes from theme
    {
        G_Theme *current = &g_state->cfg_theme;
        for(EachEnumVal(G_PaletteCode, code))
        {
            g_state->cfg_ui_debug_palettes[code].null       = v4f32(1, 0, 1, 1);
            g_state->cfg_ui_debug_palettes[code].cursor     = current->colors[RD_ThemeColor_Cursor];
            g_state->cfg_ui_debug_palettes[code].selection  = current->colors[RD_ThemeColor_SelectionOverlay];
        }
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Base].background = current->colors[RD_ThemeColor_BaseBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Base].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Base].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Base].border     = current->colors[RD_ThemeColor_BaseBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_MenuBar].background = current->colors[RD_ThemeColor_MenuBarBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_MenuBar].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_MenuBar].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_MenuBar].border     = current->colors[RD_ThemeColor_MenuBarBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Floating].background = current->colors[RD_ThemeColor_FloatingBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Floating].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Floating].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Floating].border     = current->colors[RD_ThemeColor_FloatingBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ImplicitButton].background = current->colors[RD_ThemeColor_ImplicitButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ImplicitButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ImplicitButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ImplicitButton].border     = current->colors[RD_ThemeColor_ImplicitButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PlainButton].background = current->colors[RD_ThemeColor_PlainButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PlainButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PlainButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PlainButton].border     = current->colors[RD_ThemeColor_PlainButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PositivePopButton].background = current->colors[RD_ThemeColor_PositivePopButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PositivePopButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PositivePopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_PositivePopButton].border     = current->colors[RD_ThemeColor_PositivePopButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NegativePopButton].background = current->colors[RD_ThemeColor_NegativePopButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NegativePopButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NegativePopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NegativePopButton].border     = current->colors[RD_ThemeColor_NegativePopButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NeutralPopButton].background = current->colors[RD_ThemeColor_NeutralPopButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NeutralPopButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NeutralPopButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_NeutralPopButton].border     = current->colors[RD_ThemeColor_NeutralPopButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ScrollBarButton].background = current->colors[RD_ThemeColor_ScrollBarButtonBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ScrollBarButton].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ScrollBarButton].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_ScrollBarButton].border     = current->colors[RD_ThemeColor_ScrollBarButtonBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Tab].background = current->colors[RD_ThemeColor_TabBackground];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Tab].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Tab].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_Tab].border     = current->colors[RD_ThemeColor_TabBorder];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_TabInactive].background = current->colors[RD_ThemeColor_TabBackgroundInactive];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_TabInactive].text       = current->colors[RD_ThemeColor_Text];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_TabInactive].text_weak  = current->colors[RD_ThemeColor_TextWeak];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_TabInactive].border     = current->colors[RD_ThemeColor_TabBorderInactive];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_DropSiteOverlay].background = current->colors[RD_ThemeColor_DropSiteOverlay];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_DropSiteOverlay].text       = current->colors[RD_ThemeColor_DropSiteOverlay];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_DropSiteOverlay].text_weak  = current->colors[RD_ThemeColor_DropSiteOverlay];
        g_state->cfg_ui_debug_palettes[G_PaletteCode_DropSiteOverlay].border     = current->colors[RD_ThemeColor_DropSiteOverlay];
    }

    g_state->function_hash_table_size = 1000;
    g_state->function_hash_table = push_array(arena, G_FunctionSlot, g_state->function_hash_table_size);

    G_InitStacks(g_state)
    G_InitStackNils(g_state)
}

/////////////////////////////////
// Key

internal G_Key
g_key_from_string(G_Key seed_key, String8 string)
{
    G_Key result = {0};
    result.u64[0] = g_hash_from_string(seed_key.u64[0], string);
    return result;
}

internal G_Key
g_active_seed_key(void)
{
    G_Key key = g_key_zero();
    for(G_Node *n = g_top_parent(); n != 0; n = n->parent)
    {
        if(!g_key_match(g_key_zero(), n->key))
        {
            key = n->key;
            break;
        }
    }
    return key;
}

internal G_Key
g_key_merge(G_Key a, G_Key b)
{
    return g_key_from_string(a, str8((U8*)(&b), 8));
}

internal U64
g_hash_from_string(U64 seed, String8 string)
{
    U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
    return result;
}

internal B32 g_key_match(G_Key a, G_Key b)
{
    return a.u64[0] == b.u64[0];
}

internal G_Key g_key_zero()
{
    return (G_Key){0}; 
}

/////////////////////////////////
// Bucket

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
// State accessor/mutator

internal void
g_set_active_key(G_Key key)
{
    g_state->active_key = key;
    // G_Node *node = g_node_from_key(key);
    // g_state->active_key = g_key_zero();
    // for(G_Node *n = node; n != 0; n = n->parent)
    // {
    //     if(n->flags & G_NodeFlags_NavigationRoot)
    //     {
    //         g_state->active_key = n->key;
    //     }
    // }
}

internal G_Node *
g_node_from_key(G_Key key)
{
    G_Node *result = 0;
    G_Bucket *bucket = g_top_bucket();
    U64 slot_idx = key.u64[0] % bucket->node_hash_table_size;
    for(G_Node *node = bucket->node_hash_table[slot_idx].first; node != 0; node = node->hash_next)
    {
        if(g_key_match(node->key, key))
        {
            result = node;
            break;
        }
    }
    return result;
}

internal void 
g_register_function(String8 string, void *ptr)
{
    G_Key key = g_key_from_string(g_key_zero(), string);
    G_FunctionNode *node = push_array(g_state->arena, G_FunctionNode, 1);
    {
        node->key   = key;
        node->alias = push_str8_copy(g_state->arena, string);
        node->ptr   = ptr;
    }

    U64 slot_idx = key.u64[0] % g_state->function_hash_table_size;
    G_FunctionSlot *slot = &g_state->function_hash_table[slot_idx];
    DLLPushBack(slot->first, slot->last, node);
}

internal G_FunctionNode *
g_function_from_string(String8 string)
{
    G_FunctionNode *ret = 0;
    G_Key key = g_key_from_string(g_key_zero(), string);
    U64 slot_idx = key.u64[0] % g_state->function_hash_table_size;
    G_FunctionSlot *slot = &g_state->function_hash_table[slot_idx];

    for(G_FunctionNode *n = slot->first; n != 0; n = n->next)
    {
        if(g_key_match(n->key, key))
        {
            ret = n;
            break;
        }
    }
    return ret;
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
g_build_node_from_string(G_NodeFlags flags, String8 string) {
    Arena *arena = g_top_bucket()->arena;
    G_Key key = g_key_from_string(g_active_seed_key(), string);
    G_Node *node = g_build_node_from_key(flags, key);
    node->name = push_str8_copy(arena, string);
    return node;
}

internal G_Node *
g_build_node_from_stringf(G_NodeFlags flags, char *fmt, ...) {
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);
    G_Node *ret = g_build_node_from_string(flags, string);
    scratch_end(scratch);
    return ret;
}

internal G_Node *
g_build_node_from_key(G_NodeFlags flags, G_Key key) {
    G_Node *parent = g_top_parent();
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    // TODO(k): reuse free node
    G_Node *result = push_array(arena, G_Node, 1);

    // Fill info
    {
        result->key             = key;
        // TODO: make a flag stack
        result->flags           = flags;
        result->pos             = v3f32(0,0,0);
        result->pre_pos_delta   = v3f32(0,0,0);
        result->pst_pos_delta   = v3f32(0,0,0);
        result->rot             = make_indentity_quat_f32();
        result->pre_rot_delta   = make_indentity_quat_f32();
        result->pst_rot_delta   = make_indentity_quat_f32();
        result->scale           = v3f32(1,1,1);
        result->pre_scale_delta = v3f32(0,0,0);
        result->pst_scale_delta = v3f32(1,1,1);
        result->parent          = parent;
    }

    // Insert to the bucket
    U64 slot_idx = result->key.u64[0] % bucket->node_hash_table_size;
    DLLPushBack_NP(bucket->node_hash_table[slot_idx].first, bucket->node_hash_table[slot_idx].last, result, hash_next, hash_prev);
    bucket->node_count++;

    // Insert to the parent tree
    if(parent != 0)
    {
        DLLPushBack_NP(parent->first, parent->last, result, next, prev);
        parent->children_count++;
    }
    return result;
}

/////////////////////////////////
// Node Type Functions

internal G_NodeRec
g_node_df(G_Node *n, G_Node* root, U64 sib_member_off, U64 child_member_off)
{
    G_NodeRec result = {0};
    if(*MemberFromOffset(G_Node**, n, child_member_off) != 0)
    {
        result.next = *MemberFromOffset(G_Node **, n, child_member_off);
        result.push_count++;
    } 
    else for(G_Node *p = n; p != root; p = p->parent)
    {
        if(*MemberFromOffset(G_Node **, p, sib_member_off) != 0)
        {
            result.next = *MemberFromOffset(G_Node **, p, sib_member_off);
            break;
        }
        result.pop_count++;
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

    *f = normalize_3f32(*f);
    *s = normalize_3f32(*s);
    *u = normalize_3f32(*u);

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

internal Vec3F32
g_pos_from_node(G_Node *node)
{
    Vec3F32 c = {
        node->fixed_xform.v[3][0],
        node->fixed_xform.v[3][1],
        node->fixed_xform.v[3][2],
    };
    return c;
}

internal void
g_node_delta_commit(G_Node *node)
{
    MemoryZeroStruct(&node->pre_pos_delta);
    node->pre_rot_delta = make_indentity_quat_f32();
    MemoryZeroStruct(&node->pre_scale_delta);

    MemoryZeroStruct(&node->pst_pos_delta);
    node->pst_rot_delta = make_indentity_quat_f32();
    node->pst_scale_delta = v3f32(1.0f, 1.0f, 1.0f);

    Mat4x4F32 parent_xform = mat_4x4f32(1.0f);
    if(node->parent != 0)
    {
        parent_xform = node->parent->fixed_xform;
    }
    Mat4x4F32 loc_m = mul_4x4f32(inverse_4x4f32(parent_xform), node->fixed_xform);
    g_trs_from_matrix(&loc_m, &node->pos, &node->rot, &node->scale);
}

internal void
g_node_push_fn(Arena *arena, G_Node *n, G_NodeCustomUpdateFunctionType *fn, String8 name)
{
    G_UpdateFnNode *fn_node = push_array(arena, G_UpdateFnNode, 1);
    fn_node->f = fn;
    fn_node->name = push_str8_copy(arena, name);
    DLLPushBack(n->first_update_fn, n->last_update_fn, fn_node);
}

/////////////////////////////////
// Node base scripting

G_NODE_CUSTOM_UPDATE(base_fn)
{
    // Update fixed_xform 
    {
        Mat4x4F32 xform = mat_4x4f32(1.0f);

        // QuatF32 rot   = mul_quat_f32(node->rot_delta, node->rot);
        // Vec3F32 pos   = add_3f32(node->pos_delta, node->pos);
        // Vec3F32 scale = add_3f32(node->scale_delta, node->scale);

        // Mat4x4F32 rot_m = mat_4x4f32_from_quat_f32(rot);
        // Mat4x4F32 tra_m = make_translate_4x4f32(pos);
        // Mat4x4F32 sca_m = make_scale_4x4f32(scale);

        // // TRS order
        // xform = mul_4x4f32(sca_m, xform);
        // xform = mul_4x4f32(rot_m, xform);
        // xform = mul_4x4f32(tra_m, xform);

        QuatF32 rot   = mul_quat_f32(node->pre_rot_delta, node->rot);
        Vec3F32 pos   = add_3f32(node->pre_pos_delta, node->pos);
        Vec3F32 scale = add_3f32(node->pre_scale_delta, node->scale);

        Mat4x4F32 rot_m = mat_4x4f32_from_quat_f32(rot);
        Mat4x4F32 tra_m = make_translate_4x4f32(pos);
        Mat4x4F32 sca_m = make_scale_4x4f32(scale);

        Mat4x4F32 pst_rot_delta_m = mat_4x4f32_from_quat_f32(node->pst_rot_delta);
        Mat4x4F32 pst_tra_delta_m = make_translate_4x4f32(node->pst_pos_delta);
        Mat4x4F32 pst_sca_delta_m = make_scale_4x4f32(node->pst_scale_delta);

        // TRS order
        xform = mul_4x4f32(sca_m, xform);
        xform = mul_4x4f32(rot_m, xform);
        xform = mul_4x4f32(tra_m, xform);

        if(node->parent != 0 && !(node->flags & G_NodeFlags_Float)) 
        {
            xform = mul_4x4f32(node->parent->fixed_xform, xform);
        }

        xform = mul_4x4f32(pst_sca_delta_m, xform);
        xform = mul_4x4f32(pst_rot_delta_m, xform);
        xform = mul_4x4f32(pst_tra_delta_m, xform);
        
        node->fixed_xform = xform;
    }

    // Advance anim_dt
    if(node->flags & G_NodeFlags_Animated) { node->anim_dt += dt_sec; }

    // Play animations
    // TODO: some weird artifacts when playing animation, fix it later
    if((node->flags & G_NodeFlags_Animated) && (node->flags & G_NodeFlags_AnimatedSkeleton))
    {
        // TODO: testing for now, play the first animation
        G_Bucket *bucket = scene->bucket;
        G_MeshSkeletonAnimation *anim = node->skeleton_anims[0];

        while(node->anim_dt > anim->duration)
        {
            node->anim_dt -= anim->duration;
        }

        G_Key seed_key = node->key;
        for(U64 i = 0; i < anim->spline_count; i++)
        {
            G_MeshSkeletonAnimSpline *spline = &anim->splines[i];
            G_Key target_key_pre = spline->target_key;
            G_Key target_key_pst = g_key_merge(seed_key, target_key_pre);
            G_Node *target = 0;
            G_Bucket_Scope(bucket)
            {
                target = g_node_from_key(target_key_pst);
            }
            AssertAlways(target != 0);

            // TODO: only linear is supported for now  (STEP, LINEAR, CUBICSPLINE)
            AssertAlways(spline->interpolation_method == G_InterpolationMethod_Linear);

            U64 last_frame = 0;
            U64 next_frame = 0;
            F32 t = 0;

            // Interpolation between frames
            for(U64 frame_idx = 0; frame_idx < spline->frame_count; frame_idx++)
            {
                F32 curr_ts = spline->timestamps[frame_idx];
                if(node->anim_dt < curr_ts)
                {
                    F32 last_ts = spline->timestamps[frame_idx-1];
                    t = (node->anim_dt-last_ts) / curr_ts;
                    last_frame = frame_idx-1;
                    next_frame = frame_idx;
                    break;
                }
            }

            switch(spline->transform_kind)
            {
                case G_TransformKind_Scale:
                {
                    target->scale = mix_3f32(spline->values.v3s[last_frame], spline->values.v3s[next_frame], t);
                }break;
                case G_TransformKind_Rotation:
                {
                    // spherical linear
                    target->rot = mix_quat_f32(spline->values.v4s[last_frame], spline->values.v4s[next_frame], t);
                }break;
                case G_TransformKind_Translation:
                {
                    target->pos = mix_3f32(spline->values.v3s[last_frame], spline->values.v3s[next_frame], t);
                }break;
                default: {}break;
            }
        }
    }
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
// UI

internal F_Tag
g_font_from_slot(G_FontSlot slot)
{
    return g_state->cfg_font_tags[slot];
}

internal UI_Palette *
g_palette_from_code(G_PaletteCode code)
{
    UI_Palette *result = &g_state->cfg_ui_debug_palettes[code];
    return result;
}
internal Vec4F32
g_rgba_from_theme_color(RD_ThemeColor color)
{
    return g_state->cfg_theme.colors[color];
}

/////////////////////////////////
// State

internal void
g_begin(G_Scene *scene, U64 dt, OS_EventList os_events, U64 hot_key)
{
    arena_clear(g_state->frame_arena);
    g_push_bucket(scene->bucket);
    g_push_scene(scene);

    g_state->dt = dt;
    g_state->dt_sec = dt/1000000.0f;
    g_state->dt_ms = dt/1000.0f;
    g_state->hot_key = (G_Key){ hot_key };
    g_state->window_rect = os_client_rect_from_window(g_state->os_wnd);
    g_state->window_dim = dim_2f32(g_state->window_rect);

    // Remake bucket every frame
    g_state->bucket_rect = d_bucket_make();
    g_state->bucket_geo3d = d_bucket_make();
}

internal void g_end(void)
{
    g_pop_bucket();
    g_pop_scene();

    G_Scene *s = g_state->first_to_free_scene;
    for(G_Scene *s = g_state->first_to_free_scene; s != 0; s = g_state->first_to_free_scene)
    {
        SLLStackPop(g_state->first_to_free_scene);
        SLLStackPush(g_state->first_free_scene, s);
    }
}

/////////////////////////////////
// Helpers

internal void
g_trs_from_matrix(Mat4x4F32 *m, Vec3F32 *trans, QuatF32 *rot, Vec3F32 *scale)
{
    // Translation
    trans->x = m->v[3][0];
    trans->y = m->v[3][1];
    trans->z = m->v[3][2];

    Vec3F32 i_hat = { m->v[0][0], m->v[0][1], m->v[0][2] };
    Vec3F32 j_hat = { m->v[1][0], m->v[1][1], m->v[1][2] };
    Vec3F32 k_hat = { m->v[2][0], m->v[2][1], m->v[2][2] };

    scale->x = length_3f32(i_hat);
    scale->y = length_3f32(j_hat);
    scale->z = length_3f32(k_hat);

    i_hat = normalize_3f32(i_hat);
    j_hat = normalize_3f32(j_hat);
    k_hat = normalize_3f32(k_hat);

    Mat4x4F32 rot_m =
    {
        {
            { i_hat.x, i_hat.y, i_hat.z, 0.f },
            { j_hat.x, j_hat.y, j_hat.z, 0.f },
            { k_hat.x, k_hat.y, k_hat.z, 0.f },
            { 0,       0,       0,       1.f },
        }
    };
    *rot = quat_f32_from_4x4f32(rot_m);
}

/////////////////////////////////
// Scene creation and destruction

internal G_Scene *
g_scene_alloc()
{
    Arena *arena = 0;
    G_Scene *scene = 0;
    if(g_state->first_free_scene == 0)
    {
        arena = arena_alloc(.reserve_size = MB(64), .commit_size = MB(64));
    }
    else
    {
        arena = g_state->first_free_scene->arena;
        arena_clear(arena);
    }
    scene = push_array(arena, G_Scene, 1);
    scene->arena = arena;
    scene->bucket = g_bucket_make(arena, 1000);
    scene->mesh_cache_table.slot_count = 1000;
    scene->mesh_cache_table.arena      = arena;
    scene->mesh_cache_table.slots = push_array(arena, G_MeshCacheSlot, scene->mesh_cache_table.slot_count);
    return scene;
}
