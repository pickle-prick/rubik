#define RK_StackTopImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.top->v;

#define RK_StackBottomImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.bottom_val;

#define RK_StackPushImpl(state, name_upper, name_lower, type, new_value) \
RK_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(rk_state->arena, RK_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define RK_StackPopImpl(state, name_upper, name_lower) \
RK_##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
SLLStackPop(state->name_lower##_stack.top);\
SLLStackPush(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define RK_StackSetNextImpl(state, name_upper, name_lower, type, new_value) \
RK_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(rk_state->arena, RK_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;

#include "generated/rubik.meta.c"

internal void
rk_init(OS_Handle os_wnd)
{
    Arena *arena = arena_alloc();
    rk_state = push_array(arena, RK_State, 1);
    rk_state->arena = arena;
    rk_state->frame_arena = arena_alloc();
    rk_state->node_bucket = rk_bucket_make(arena, 3000);
    rk_state->os_wnd = os_wnd;
    rk_state->mesh_cache_table.slot_count = 1000;
    rk_state->mesh_cache_table.arena = arena;
    rk_state->mesh_cache_table.slots = push_array(arena, RK_MeshCacheSlot, rk_state->mesh_cache_table.slot_count);
    rk_state->last_dpi = os_dpi_from_window(os_wnd);

    // Settings
    for EachEnumVal(RK_SettingCode, code)
    {
        rk_state->setting_vals[code] = rk_setting_code_default_val_table[code];
    }
    rk_state->setting_vals[RK_SettingCode_MainFontSize].s32 = rk_state->setting_vals[RK_SettingCode_MainFontSize].s32 * (rk_state->last_dpi/96.f);
    rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32 = rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32 * (rk_state->last_dpi/96.f);
    rk_state->setting_vals[RK_SettingCode_MainFontSize].s32 = ClampBot(rk_state->setting_vals[RK_SettingCode_MainFontSize].s32, rk_setting_code_default_val_table[RK_SettingCode_MainFontSize].s32);
    rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32 = ClampBot(rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32, rk_setting_code_default_val_table[RK_SettingCode_CodeFontSize].s32);

    // Fonts
    rk_state->cfg_font_tags[RK_FontSlot_Main]  = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
    rk_state->cfg_font_tags[RK_FontSlot_Code]  = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
    rk_state->cfg_font_tags[RK_FontSlot_Icons] = f_tag_from_path(str8_lit("./fonts/icons.ttf"));

    // Theme 
    MemoryCopy(rk_state->cfg_theme_target.colors, rk_theme_preset_colors__handmade_hero, sizeof(rk_theme_preset_colors__handmade_hero));
    MemoryCopy(rk_state->cfg_theme.colors, rk_theme_preset_colors__handmade_hero, sizeof(rk_theme_preset_colors__handmade_hero));

    //////////////////////////////
    //- k: compute ui palettes from theme
    {
        RK_Theme *current = &rk_state->cfg_theme;
        for EachEnumVal(RK_PaletteCode, code)
        {
            rk_state->cfg_ui_debug_palettes[code].null       = v4f32(1, 0, 1, 1);
            rk_state->cfg_ui_debug_palettes[code].cursor     = current->colors[RK_ThemeColor_Cursor];
            rk_state->cfg_ui_debug_palettes[code].selection  = current->colors[RK_ThemeColor_SelectionOverlay];
        }
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Base].background = current->colors[RK_ThemeColor_BaseBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Base].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Base].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Base].border     = current->colors[RK_ThemeColor_BaseBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_MenuBar].background = current->colors[RK_ThemeColor_MenuBarBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_MenuBar].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_MenuBar].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_MenuBar].border     = current->colors[RK_ThemeColor_MenuBarBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Floating].background = current->colors[RK_ThemeColor_FloatingBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Floating].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Floating].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Floating].border     = current->colors[RK_ThemeColor_FloatingBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ImplicitButton].background = current->colors[RK_ThemeColor_ImplicitButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ImplicitButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ImplicitButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ImplicitButton].border     = current->colors[RK_ThemeColor_ImplicitButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PlainButton].background = current->colors[RK_ThemeColor_PlainButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PlainButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PlainButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PlainButton].border     = current->colors[RK_ThemeColor_PlainButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PositivePopButton].background = current->colors[RK_ThemeColor_PositivePopButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PositivePopButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PositivePopButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_PositivePopButton].border     = current->colors[RK_ThemeColor_PositivePopButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NegativePopButton].background = current->colors[RK_ThemeColor_NegativePopButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NegativePopButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NegativePopButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NegativePopButton].border     = current->colors[RK_ThemeColor_NegativePopButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NeutralPopButton].background = current->colors[RK_ThemeColor_NeutralPopButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NeutralPopButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NeutralPopButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_NeutralPopButton].border     = current->colors[RK_ThemeColor_NeutralPopButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ScrollBarButton].background = current->colors[RK_ThemeColor_ScrollBarButtonBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ScrollBarButton].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ScrollBarButton].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_ScrollBarButton].border     = current->colors[RK_ThemeColor_ScrollBarButtonBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Tab].background = current->colors[RK_ThemeColor_TabBackground];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Tab].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Tab].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_Tab].border     = current->colors[RK_ThemeColor_TabBorder];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_TabInactive].background = current->colors[RK_ThemeColor_TabBackgroundInactive];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_TabInactive].text       = current->colors[RK_ThemeColor_Text];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_TabInactive].text_weak  = current->colors[RK_ThemeColor_TextWeak];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_TabInactive].border     = current->colors[RK_ThemeColor_TabBorderInactive];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_DropSiteOverlay].background = current->colors[RK_ThemeColor_DropSiteOverlay];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_DropSiteOverlay].text       = current->colors[RK_ThemeColor_DropSiteOverlay];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_DropSiteOverlay].text_weak  = current->colors[RK_ThemeColor_DropSiteOverlay];
        rk_state->cfg_ui_debug_palettes[RK_PaletteCode_DropSiteOverlay].border     = current->colors[RK_ThemeColor_DropSiteOverlay];
    }

    // Views
    for(U64 i = 0; i < RK_ViewKind_COUNT; i++)
    {
        RK_View *view = &rk_state->views[i];
        view->arena = arena_alloc();
    }

    rk_state->function_hash_table_size = 1000;
    rk_state->function_hash_table = push_array(arena, RK_FunctionSlot, rk_state->function_hash_table_size);

    RK_InitStacks(rk_state)
    RK_InitStackNils(rk_state)
}

/////////////////////////////////
//~ Key

internal RK_Key
rk_key_from_string(RK_Key seed_key, String8 string)
{
    RK_Key result = {0};
    result.u64[0] = rk_hash_from_string(seed_key.u64[0], string);
    return result;
}

internal RK_Key
rk_active_seed_key(void)
{
    RK_Key key = rk_key_zero();
    for(RK_Node *n = rk_top_parent(); n != 0; n = n->parent)
    {
        if(!rk_key_match(rk_key_zero(), n->key))
        {
            key = n->key;
            break;
        }
    }
    return key;
}

internal RK_Key
rk_key_merge(RK_Key a, RK_Key b)
{
    return rk_key_from_string(a, str8((U8*)(&b), 8));
}

internal U64
rk_hash_from_string(U64 seed, String8 string)
{
    U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
    return result;
}

internal B32 rk_key_match(RK_Key a, RK_Key b)
{
    return a.u64[0] == b.u64[0];
}

internal RK_Key rk_key_zero()
{
    return (RK_Key){0}; 
}

/////////////////////////////////
// Bucket

internal RK_Bucket *
rk_bucket_make(Arena *arena, U64 hash_table_size)
{
    RK_Bucket *result = push_array(arena, RK_Bucket, 1);
    result->node_hash_table = push_array(arena, RK_BucketSlot, hash_table_size);
    result->node_hash_table_size = hash_table_size;
    result->arena = arena;
    return result;
}

/////////////////////////////////
// State accessor/mutator

internal void
rk_set_active_key(RK_Key key)
{
    rk_state->active_key = key;
    // RK_Node *node = rk_node_from_key(key);
    // rk_state->active_key = rk_key_zero();
    // for(RK_Node *n = node; n != 0; n = n->parent)
    // {
    //     if(n->flags & RK_NodeFlags_NavigationRoot)
    //     {
    //         rk_state->active_key = n->key;
    //     }
    // }
}

internal RK_Node *
rk_node_from_key(RK_Key key)
{
    RK_Node *result = 0;
    RK_Bucket *bucket = rk_top_bucket();
    U64 slot_idx = key.u64[0] % bucket->node_hash_table_size;
    for(RK_Node *node = bucket->node_hash_table[slot_idx].first; node != 0; node = node->hash_next)
    {
        if(rk_key_match(node->key, key))
        {
            result = node;
            break;
        }
    }
    return result;
}

internal void 
rk_register_function(String8 string, void *ptr)
{
    RK_Key key = rk_key_from_string(rk_key_zero(), string);
    RK_FunctionNode *node = push_array(rk_state->arena, RK_FunctionNode, 1);
    {
        node->key   = key;
        node->alias = push_str8_copy(rk_state->arena, string);
        node->ptr   = ptr;
    }

    U64 slot_idx = key.u64[0] % rk_state->function_hash_table_size;
    RK_FunctionSlot *slot = &rk_state->function_hash_table[slot_idx];
    DLLPushBack(slot->first, slot->last, node);
}

internal RK_FunctionNode *
rk_function_from_string(String8 string)
{
    RK_FunctionNode *ret = 0;
    RK_Key key = rk_key_from_string(rk_key_zero(), string);
    U64 slot_idx = key.u64[0] % rk_state->function_hash_table_size;
    RK_FunctionSlot *slot = &rk_state->function_hash_table[slot_idx];

    for(RK_FunctionNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            ret = n;
            break;
        }
    }
    return ret;
}

internal RK_View *
rk_view_from_kind(RK_ViewKind kind)
{
    return &rk_state->views[kind];
}

/////////////////////////////////
// AnimatedSprite2D

// internal RK_SpriteSheet2D *
// rk_spritesheet_2d_from_file(Arena *arena, String8 path, String8 meta_path) {
//     RK_SpriteSheet2D *result = 0;
//     result = push_array(arena, RK_SpriteSheet2D, 1);
// 
//     int x,y,n; 
//     U8 *pixels = stbi_load((char *)path.str, &x, &y, &n, 4);
//     R_Handle texture = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Nearest, v2s32(x,y), R_Tex2DFormat_RGBA8, pixels);
//     stbi_image_free(pixels);
// 
//     // Parse meta file 
//     RK_Sprite2D_FrameTag *tags  = 0;
//     U64 tag_count   = 0;
//     RK_Sprite2D_Frame *frames   = 0;
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
//                 tags = push_array(arena, RK_Sprite2D_FrameTag, tag_count);
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
//         frames = push_array(arena, RK_Sprite2D_Frame, frame_count);
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

internal RK_Node *
rk_build_node_from_string(RK_NodeFlags flags, String8 string) {
    Arena *arena = rk_top_bucket()->arena;
    RK_Key key = rk_key_from_string(rk_active_seed_key(), string);
    RK_Node *node = rk_build_node_from_key(flags, key);
    node->name = push_str8_copy(arena, string);
    return node;
}

internal RK_Node *
rk_build_node_from_stringf(RK_NodeFlags flags, char *fmt, ...) {
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);
    RK_Node *ret = rk_build_node_from_string(flags, string);
    scratch_end(scratch);
    return ret;
}

internal RK_Node *
rk_build_node_from_key(RK_NodeFlags flags, RK_Key key) {
    RK_Node *parent = rk_top_parent();
    RK_Bucket *bucket = rk_top_bucket();
    Arena *arena = bucket->arena;

    // TODO(k): reuse free node
    RK_Node *result = push_array(arena, RK_Node, 1);

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

internal void
rk_node_release()
{
    // TODO
    NotImplemented;
}

/////////////////////////////////
// Node Type Functions

internal RK_NodeRec
rk_node_df(RK_Node *n, RK_Node* root, U64 sib_member_off, U64 child_member_off)
{
    RK_NodeRec result = {0};
    if(*MemberFromOffset(RK_Node**, n, child_member_off) != 0)
    {
        result.next = *MemberFromOffset(RK_Node **, n, child_member_off);
        result.push_count++;
    } 
    else for(RK_Node *p = n; p != root; p = p->parent)
    {
        if(*MemberFromOffset(RK_Node **, p, sib_member_off) != 0)
        {
            result.next = *MemberFromOffset(RK_Node **, p, sib_member_off);
            break;
        }
        result.pop_count++;
    }
    return result;
}

internal void
rk_local_coord_from_node(RK_Node *n, Vec3F32 *f, Vec3F32 *s, Vec3F32 *u)
{
    ProfBeginFunction();
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
    ProfEnd();
}

internal Mat4x4F32
rk_view_from_node(RK_Node *node)
{
    return inverse_4x4f32(node->fixed_xform);
}

internal Vec3F32
rk_pos_from_node(RK_Node *node)
{
    Vec3F32 c = {
        node->fixed_xform.v[3][0],
        node->fixed_xform.v[3][1],
        node->fixed_xform.v[3][2],
    };
    return c;
}

internal void
rk_node_delta_commit(RK_Node *node)
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
    rk_trs_from_matrix(&loc_m, &node->pos, &node->rot, &node->scale);
}

internal void
rk_node_push_fn(Arena *arena, RK_Node *n, RK_NodeCustomUpdateFunctionType *fn, String8 name)
{
    RK_UpdateFnNode *fn_node = push_array(arena, RK_UpdateFnNode, 1);
    fn_node->f = fn;
    fn_node->name = push_str8_copy(arena, name);
    DLLPushBack(n->first_update_fn, n->last_update_fn, fn_node);
}

/////////////////////////////////
// Node base scripting

RK_NODE_CUSTOM_UPDATE(base_fn)
{
    ProfBegin("base_fn");
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

        if(node->parent != 0 && !(node->flags & RK_NodeFlags_Float)) 
        {
            xform = mul_4x4f32(node->parent->fixed_xform, xform);
        }

        xform = mul_4x4f32(pst_sca_delta_m, xform);
        xform = mul_4x4f32(pst_rot_delta_m, xform);
        xform = mul_4x4f32(pst_tra_delta_m, xform);
        
        node->fixed_xform = xform;
    }

    // Advance anim_dt
    if(node->flags & RK_NodeFlags_Animated) { node->anim_dt += dt_sec; }

    // Play animations
    // TODO: some weird artifacts when playing animation, fix it later
    if((node->flags & RK_NodeFlags_Animated) && (node->flags & RK_NodeFlags_AnimatedSkeleton))
    {
        // TODO: testing for now, play the first animation
        RK_Bucket *bucket = scene->bucket;
        RK_MeshSkeletonAnimation *anim = node->skeleton_anims[0];

        while(node->anim_dt > anim->duration)
        {
            node->anim_dt -= anim->duration;
        }

        RK_Key seed_key = node->key;
        for(U64 i = 0; i < anim->spline_count; i++)
        {
            RK_MeshSkeletonAnimSpline *spline = &anim->splines[i];
            RK_Key target_key_pre = spline->target_key;
            RK_Key target_key_pst = rk_key_merge(seed_key, target_key_pre);
            RK_Node *target = 0;
            RK_Bucket_Scope(bucket)
            {
                target = rk_node_from_key(target_key_pst);
            }
            AssertAlways(target != 0);

            // TODO: only linear is supported for now  (STEP, LINEAR, CUBICSPLINE)
            AssertAlways(spline->interpolation_method == RK_InterpolationMethod_Linear);

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
                case RK_TransformKind_Scale:
                {
                    target->scale = mix_3f32(spline->values.v3s[last_frame], spline->values.v3s[next_frame], t);
                }break;
                case RK_TransformKind_Rotation:
                {
                    // spherical linear
                    target->rot = mix_quat_f32(spline->values.v4s[last_frame], spline->values.v4s[next_frame], t);
                }break;
                case RK_TransformKind_Translation:
                {
                    target->pos = mix_3f32(spline->values.v3s[last_frame], spline->values.v3s[next_frame], t);
                }break;
                default: {}break;
            }
        }
    }
    ProfEnd();
}

/////////////////////////////////
// Physics

// internal void
// rk_physics_dynamic_root(RK_Node *node, F32 dt) {
//     // TODO: update physics dynamic in timely manner to avoid vary delta t
//     switch(node->rigidbody2d.physics_kind)
//     {
//         default:{}break;
//         case RK_PhysicsKind_Dynamic:
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
//         case RK_PhysicsKind_Kinematic:
//         {
//             // TODO: this is wrong
//             // Velocity Verlet
//             RK_RigidBody2D *body = &node->rigidbody2d;
//             Vec2F32 d_pos = add_2f32(scale_2f32(body->velocity, dt),
//                                     (scale_2f32(body->acc, 0.5f*dt*dt)));
//             node->pos = add_2f32(node->pos, d_pos);
//             Vec2F32 d_velocity = scale_2f32(body->acc, dt);
//             body->velocity = add_2f32(body->velocity, d_velocity);
//         }break;
//     }
// 
//     for(RK_Node *child = node->first; child != 0; child = child->next) {
//         rk_physics_dynamic_root(child, dt);
//     }
// }

/////////////////////////////////
// Magic

RK_SHAPE_SUPPORT_FN(RK_SHAPE_RECT_SUPPORT_FN) {
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
// gjk(Vec2F32 s1_center, Vec2F32 s2_center, void *s1_data, void *s2_data, RK_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, RK_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn, Vec2F32 simplex[3])
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

// internal RK_Contact2D
// epa(Vec2F32 simplex[3], void *s1_data, void *s2_data,
//     RK_SHAPE_CUSTOM_SUPPORT_FN s1_support_fn, RK_SHAPE_CUSTOM_SUPPORT_FN s2_support_fn)
// {
//     // NOTE(k): there are edge cases when three points all reside on the x axis 
//     // TODO: fix it later
//     // Assert(p_in_triangle(v2f32(0,0), simplex[0], simplex[1], simplex[2]));
// 
//     RK_Contact2D result = {0};
//     if(!(p_in_triangle(v2f32(0,0), simplex[0], simplex[1], simplex[2]))) { return result; }
//     Temp temp = scratch_begin(0,0);
// 
//     RK_Vertex2DNode *first_vertex = 0;
//     RK_Vertex2DNode *last_vertex  = 0;
// 
//     for(U64 i = 0; i < 3; i++) 
//     {
//         RK_Vertex2DNode *n = push_array(temp.arena, RK_Vertex2DNode, 1);
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
//         RK_Vertex2DNode *aa = 0;
//         RK_Vertex2DNode *bb = 0;
// 
//         RK_Vertex2DNode *a = first_vertex;
//         RK_Vertex2DNode *b = a->next;
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
//             RK_Vertex2DNode *n = push_array(temp.arena, RK_Vertex2DNode, 1);
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
//~ Colors, Fonts, Config

/////////////////////////////////
//- fonts/sizes

//- colors
internal Vec4F32
rk_rgba_from_theme_color(RK_ThemeColor color)
{
    return rk_state->cfg_theme.colors[color];
}

//- code -> palette
internal UI_Palette *
rk_palette_from_code(RK_PaletteCode code)
{
    UI_Palette *result = &rk_state->cfg_ui_debug_palettes[code];
    return result;
}

//- fonts/sizes
internal F_Tag
rk_font_from_slot(RK_FontSlot slot)
{
    return rk_state->cfg_font_tags[slot];
}

internal F32
rk_font_size_from_slot(RK_FontSlot slot)
{
    F32 result = 0;
    F32 dpi = os_dpi_from_window(rk_state->os_wnd);
    if(dpi != rk_state->last_dpi)
    {
        F32 old_dpi = rk_state->last_dpi;
        F32 new_dpi = dpi;
        rk_state->last_dpi = dpi;
        S32 *pt_sizes[] =
        {
            &rk_state->setting_vals[RK_SettingCode_MainFontSize].s32,
            &rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32,
        };
        for(U64 idx = 0; idx < ArrayCount(pt_sizes); idx++)
        {
            F32 ratio = pt_sizes[idx][0] / old_dpi;
            F32 new_pt_size = ratio*new_dpi;
            pt_sizes[idx][0] = (S32)new_pt_size;
        }
    }

    switch(slot)
    {
        case RK_FontSlot_Code:
        {
            result = (F32)rk_state->setting_vals[RK_SettingCode_CodeFontSize].s32;
        }break;
        default:
        case RK_FontSlot_Main:
        case RK_FontSlot_Icons:
        {
            result = (F32)rk_state->setting_vals[RK_SettingCode_MainFontSize].s32;
        }break;
    }
    return result;
}

/////////////////////////////////
//~ UI widget functions

internal void rk_ui_stats(void)
{
    RK_Scene *scene = rk_top_scene();
    typedef struct RK_Stats_State RK_Stats_State;
    struct RK_Stats_State
    {
        B32 show;
    };

    RK_View *view = rk_view_from_kind(RK_ViewKind_Stats);
    RK_Stats_State *stats = view->custom_data;

    if(stats == 0)
    {
        stats = push_array(view->arena, RK_Stats_State, 1);
        view->custom_data = stats;
        
        stats->show = 1;
    }

    local_persist B32 show_stats = 1;
    ui_set_next_focus_hot(UI_FocusKind_Root);
    ui_set_next_focus_active(UI_FocusKind_Root);
    RK_UI_Pane(r2f32p(rk_state->window_rect.p1.x-710, rk_state->window_rect.p0.y+10, rk_state->window_rect.p1.x-10, rk_state->window_rect.p0.y+590), &show_stats, str8_lit("STATS###stats"))
        UI_TextAlignment(UI_TextAlign_Left)
        UI_TextPadding(9)
        {
            UI_Row
            {
                ui_labelf("frame time ms");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.3f", (rk_state->debug.frame_dt_us/1000.0));
            }
            UI_Row
            {
                ui_labelf("cpu time ms");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.3f", (rk_state->debug.cpu_dt_us/1000.0));
            }
            UI_Row
            {
                ui_labelf("gpu time ms");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.3f", (rk_state->debug.gpu_dt_us/1000.0));
            }
            UI_Row
            {
                ui_labelf("fps");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.2f", 1 / (rk_state->debug.frame_dt_us/1000000.0));
            }
            UI_Row
            {
                ui_labelf("ui_hot_key");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", ui_state->hot_box_key.u64[0]);
            }
            UI_Row
            {
                ui_labelf("ui_last_build_box_count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", ui_state->last_build_box_count);
            }
            UI_Row
            {
                ui_labelf("ui_build_box_count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", ui_state->build_box_count);
            }
            UI_Row
            {
                ui_labelf("mouse");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.2f, %.2f", ui_state->mouse.x, ui_state->mouse.y);
            }
            UI_Row
            {
                ui_labelf("drag start mouse");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.2f, %.2f", ui_state->drag_start_mouse.x, ui_state->drag_start_mouse.y);
            }
            UI_Row
            {
                ui_labelf("geo3d hot id");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", rk_state->active_key.u64[0]);
            }
            UI_Row
            {
                ui_labelf("scene node count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", scene->bucket->node_count);
            }
        }
}

internal void rk_ui_inspector(void)
{
    ProfBeginFunction();
    RK_Scene *scene = rk_top_scene();

    typedef struct RK_Inspector_State RK_Inspector_State;
    struct RK_Inspector_State
    {
        B32 show_inspector;
        B32 show_scene_cfg;
        B32 show_camera_cfg;
        B32 show_light_cfg;
        B32 show_node_cfg;

        TxtPt txt_cursor;
        TxtPt txt_mark;
        U8 *txt_edit_buffer;
        U8 txt_edit_buffer_size;
        U64 txt_edit_string_size;
    };

    RK_View *inspector_view = rk_view_from_kind(RK_ViewKind_SceneInspector);
    RK_Inspector_State *inspector = inspector_view->custom_data;
    if(inspector == 0)
    {
        inspector = push_array(inspector_view->arena, RK_Inspector_State, 1);
        inspector_view->custom_data = inspector;

        inspector->show_inspector  = 1;
        inspector->show_scene_cfg  = 1;
        inspector->show_camera_cfg = 1;
        inspector->show_light_cfg  = 1;
        inspector->show_node_cfg   = 1;

        inspector->txt_cursor           = (TxtPt){0};
        inspector->txt_mark             = (TxtPt){0};
        inspector->txt_edit_buffer_size = 100;
        inspector->txt_edit_buffer      = push_array(inspector_view->arena, U8, inspector->txt_edit_buffer_size);
        inspector->txt_edit_string_size = 0;
    }

    RK_Node *camera = scene->active_camera->v;
    RK_Node *active_node = rk_node_from_key(rk_state->active_key);

    // Animation rate
    F32 fast_rate = 1 - pow_f32(2, (-40.f * rk_state->dt_sec));

    // Build top-level panel container
    UI_Box *pane;
    UI_FocusActive(UI_FocusKind_Root) UI_FocusHot(UI_FocusKind_Root) UI_Transparency(0.1)
    {
        Rng2F32 panel_rect = rk_state->window_rect;
        panel_rect.p1.x = 800;
        panel_rect = pad_2f32(panel_rect,-30);
        pane = rk_ui_pane_begin(panel_rect, &inspector->show_inspector, str8_lit("INSPECTOR"));
    }

    {
        // Scene
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        ui_set_next_flags(UI_BoxFlag_DrawBorder);
        UI_Box *scene_tree_box = ui_build_box_from_stringf(0, "###scene_tree");
        UI_Parent(scene_tree_box)
        {
            // Header
            {
                ui_set_next_child_layout_axis(Axis2_X);
                ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
                UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
                UI_Parent(header_box)
                {
                    ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                    if(ui_clicked(ui_expanderf(inspector->show_scene_cfg, "###scene_cfg")))
                    {
                        inspector->show_scene_cfg = !inspector->show_scene_cfg;
                    }
                    ui_set_next_pref_size(Axis2_X, ui_text_dim(3, 0.0));
                    ui_labelf("Scene");
                }
            }

            // Scene cfg
            ui_set_next_child_layout_axis(Axis2_Y);
            ui_set_next_pref_height(ui_children_sum(0));
            ui_set_next_flags(UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideRight);
            UI_Box *scene_cfg = ui_build_box_from_stringf(0, "###scene_cfg");
            UI_Parent(scene_cfg)
            {
                // TODO(k): move these into some state struct
                local_persist TxtPt cursor = {0};
                local_persist TxtPt mark   = {0};
                local_persist U8 edit_buffer[300] = {0};
                local_persist String8 scene_path = {edit_buffer, 0};

                if(scene_path.size == 0)
                {
                    U64 size = Min(scene->path.size, ArrayCount(edit_buffer));
                    MemoryCopy(edit_buffer, scene->path.str, size);
                    scene_path.size = size;
                }

                UI_Flags(UI_BoxFlag_ClickToFocus)
                {
                    ui_line_edit(&cursor, &mark, edit_buffer, ArrayCount(edit_buffer), &scene_path.size, scene_path, str8_lit("###scene_path"));
                }

                UI_Row
                {
                    if(ui_clicked(ui_buttonf("Load Default")))
                    {
                        RK_Scene *new_scene = rk_default_scene();
                        SLLStackPush(rk_state->first_to_free_scene, scene);
                        rk_state->active_scene = new_scene;
                        scene_path.size = 0;
                    }
                    if(ui_clicked(ui_buttonf("Save")))
                    {
                        rk_scene_to_file(scene, scene_path);
                    }
                    if(ui_clicked(ui_buttonf("Reload")))
                    {
                        RK_Scene *new_scene = rk_scene_from_file(scene_path);
                        SLLStackPush(rk_state->first_to_free_scene, scene);
                        rk_state->active_scene = new_scene;
                        scene_path.size = 0;
                    }
                }

                UI_Row RK_MeshCacheTable_Scope(&scene->mesh_cache_table)
                {
                    RK_Node *parent = active_node;
                    if(parent == 0) parent = scene->root;

                    rk_ui_dropdown_begin(str8_lit("Create"));
                    RK_Bucket_Scope(scene->bucket) RK_Parent_Scope(parent)
                    {
                        if(ui_clicked(ui_buttonf("box")))
                        {
                            // TODO: check name collision
                            RK_Node *n = rk_build_node_from_stringf(0, "box");
                            n->kind = RK_NodeKind_MeshRoot;
                            n->v.mesh_root.kind = RK_MeshKind_Box;
                            RK_Parent_Scope(n)
                            {
                                rk_box_node_default(str8_lit("box"));
                            }
                            rk_ui_dropdown_hide();
                            rk_set_active_key(n->key);
                        }
                        if(ui_clicked(ui_buttonf("plane")))
                        {
                            // TODO: check name collision
                            RK_Node *n = rk_build_node_from_stringf(0, "plane");
                            n->kind = RK_NodeKind_MeshRoot;
                            n->v.mesh_root.kind = RK_MeshKind_Plane;
                            RK_Parent_Scope(n)
                            {
                                rk_plane_node_default(str8_lit("plane"));
                            }
                            rk_ui_dropdown_hide();
                            rk_set_active_key(n->key);
                        }
                        if(ui_clicked(ui_buttonf("sphere")))
                        {
                            // TODO: check name collision
                            RK_Node *n = rk_build_node_from_stringf(0, "sphere");
                            n->kind = RK_NodeKind_MeshRoot;
                            n->v.mesh_root.kind = RK_MeshKind_Sphere;
                            RK_Parent_Scope(n)
                            {
                                rk_sphere_node_default(str8_lit("sphere"));
                            }
                            rk_ui_dropdown_hide();
                            rk_set_active_key(n->key);
                        }
                        if(ui_clicked(ui_buttonf("cylinder")))
                        {
                            // TODO: check name collision
                            RK_Node *n = rk_build_node_from_stringf(0, "cylinder");
                            n->kind = RK_NodeKind_MeshRoot;
                            n->v.mesh_root.kind = RK_MeshKind_Cylinder;
                            RK_Parent_Scope(n)
                            {
                                rk_cylinder_node_default(str8_lit("cylinder"));
                            }
                            rk_ui_dropdown_hide();
                            rk_set_active_key(n->key);
                        }
                        if(ui_clicked(ui_buttonf("capsule")))
                        {
                            // TODO: check name collision
                            RK_Node *n = rk_build_node_from_stringf(0, "capsule");
                            n->kind = RK_NodeKind_MeshRoot;
                            n->v.mesh_root.kind = RK_MeshKind_Capsule;
                            RK_Parent_Scope(n)
                            {
                                rk_capsule_node_default(str8_lit("capsule"));
                            }
                            rk_ui_dropdown_hide();
                            rk_set_active_key(n->key);
                        }
                        if(ui_clicked(ui_buttonf("mesh"))) {}
                    }
                    rk_ui_dropdown_end();

                    if(ui_clicked(ui_buttonf("Delete")))
                    {
                        if(active_node != 0)
                        {
                            DLLRemove(active_node->parent->first, active_node->parent->last, active_node);
                            // TODO: free node
                        }
                    }
                }
            }

            // Scene tree
            {
                F32 size = ui_top_font_size()*30.f;
                ui_set_next_pref_size(Axis2_Y, ui_px(size, 0.0));
                ui_set_next_child_layout_axis(Axis2_Y);
                if(!inspector->show_scene_cfg)
                {
                    ui_set_next_flags(UI_BoxFlag_Disabled);
                }
                UI_Box *container_box = ui_build_box_from_stringf(UI_BoxFlag_DrawOverlay, "###container");
                container_box->pref_size[Axis2_Y].value = mix_1f32(size, 0, container_box->disabled_t);

                // scroll params
                UI_ScrollListParams scroll_list_params = {0};
                {
                    Vec2F32 rect_dim = dim_2f32(container_box->rect);
                    scroll_list_params.flags = UI_ScrollListFlag_All;
                    scroll_list_params.row_height_px = ui_top_font_size()*1.3;
                    scroll_list_params.dim_px = rect_dim;
                    scroll_list_params.item_range = r1s64(0, scene->bucket->node_count);
                    scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 0;
                }
                UI_ScrollListSignal scroll_list_sig = {0};
                // TODO: move these to some kind of view state
                local_persist UI_ScrollPt scroller = {0};
                scroller.off -= scroller.off * fast_rate;
                if(abs_f32(scroller.off) < 0.01) scroller.off = 0;
                Rng1S64 visible_row_rng = {0};

                UI_Parent(container_box) UI_ScrollList(&scroll_list_params, &scroller, 0, 0, &visible_row_rng, &scroll_list_sig)
                {
                    RK_Node *root = scene->root;
                    U64 row_idx = 0;
                    U64 level = 0;
                    U64 indent_size = 2;
                    while(root != 0)
                    {
                        RK_NodeRec rec = rk_node_df_pre(root, 0);

                        if(row_idx >= visible_row_rng.min && row_idx <= visible_row_rng.max)
                        {
                            String8 indent = str8(0, level*indent_size);
                            indent.str = push_array(ui_build_arena(), U8, indent.size);
                            MemorySet(indent.str, ' ', indent.size);

                            String8 string = push_str8f(ui_build_arena(), "%S%S###%d", indent, root->name, row_idx);
                            if(active_node == root)
                            {
                                  ui_set_next_palette(ui_build_palette(ui_top_palette(), .overlay = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)));
                                  ui_set_next_flags(UI_BoxFlag_DrawOverlay);
                            }
                            UI_Signal label = ui_button(string);
                            if(ui_clicked(label)) rk_set_active_key(root->key);
                        }

                        // if(root == active_node && scroller.idx != row_idx && scroller.off == 0)
                        // {
                        //     ui_scroll_pt_target_idx(&scroller, row_idx);
                        // }

                        level += (rec.push_count-rec.pop_count);
                        root = rec.next;
                        row_idx++;
                    }
                }
            }
        }

        // Camera
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        ui_set_next_flags(UI_BoxFlag_DrawBorder);
        UI_Box *camera_cfg_box = ui_build_box_from_stringf(0, "###camera_cfg");
        UI_Parent(camera_cfg_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expanderf(inspector->show_camera_cfg, "###camera_cfg")))
                {
                    inspector->show_camera_cfg = !inspector->show_camera_cfg;
                }
                ui_labelf("Camera");
            }

            // Active cameras
            UI_Row
            {
                ui_labelf("active camera");
                ui_spacer(ui_pct(1.0, 0.0));
                rk_ui_dropdown_begin(scene->active_camera->v->name);
                for(RK_CameraNode *c = scene->first_camera; c!=0; c = c->next)
                {
                    if(ui_clicked(ui_button(c->v->name)))
                    {
                        scene->active_camera = c;
                        rk_ui_dropdown_hide();
                    }
                }
                rk_ui_dropdown_end();
            }

            // Viewport shading
            UI_Row
            {
                ui_labelf("shading");
                ui_spacer(ui_pct(1.0, 0.0));
                if(ui_clicked(ui_buttonf("wireframe"))) {scene->viewport_shading = RK_ViewportShadingKind_Wireframe;};
                if(ui_clicked(ui_buttonf("solid")))     {scene->viewport_shading = RK_ViewportShadingKind_Solid;};
                if(ui_clicked(ui_buttonf("material")))  {scene->viewport_shading = RK_ViewportShadingKind_MaterialPreview;};
            }

            if(inspector->show_camera_cfg)
            {
                UI_Row
                {
                    ui_labelf("pos");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->pos.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###pos_x"));
                    ui_f32_edit(&camera->pos.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###pos_y"));
                    ui_f32_edit(&camera->pos.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###pos_z"));
                }
                UI_Row
                {
                    ui_labelf("scale");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->scale.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###scale_x"));
                    ui_f32_edit(&camera->scale.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###scale_y"));
                    ui_f32_edit(&camera->scale.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###scale_z"));
                }
                UI_Row
                {
                    ui_labelf("rot");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&camera->rot.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###rot_x"));
                    ui_f32_edit(&camera->rot.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###rot_y"));
                    ui_f32_edit(&camera->rot.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###rot_z"));
                    ui_f32_edit(&camera->rot.w, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("W###rot_w"));
                }
            }
        }

        ui_spacer(ui_px(30, 1.0));

        // Light
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        ui_set_next_flags(UI_BoxFlag_DrawBorder);
        UI_Box *light_cfg_box = ui_build_box_from_stringf(0, "###light_cfg");
        UI_Parent(light_cfg_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expanderf(inspector->show_light_cfg, "###light_cfg")))
                {
                    inspector->show_light_cfg = !inspector->show_light_cfg;
                }
                ui_labelf("Light");
            }

            if(inspector->show_light_cfg)
            {
                UI_Row
                {
                    ui_labelf("global_light");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&scene->global_light.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###pos_x"));
                    ui_f32_edit(&scene->global_light.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###pos_y"));
                    ui_f32_edit(&scene->global_light.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###pos_z"));
                }
            }
        }

        // Node Properties
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        UI_Box *node_cfg_box = ui_build_box_from_stringf(0, "###node_cfg");
        UI_Parent(node_cfg_box)
        {
            // Header
            ui_set_next_child_layout_axis(Axis2_X);
            ui_set_next_flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow);
            UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
            UI_Parent(header_box)
            {
                ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                if(ui_clicked(ui_expander(inspector->show_node_cfg, str8_lit("###node_cfg"))))
                {
                    inspector->show_node_cfg = !inspector->show_node_cfg;
                }
                ui_labelf("Node Properties");
            }

            if(inspector->show_node_cfg && active_node != 0)
            {
                UI_Row
                {
                    ui_labelf("name");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%S", active_node->name);
                }
                if(active_node->parent != 0)
                {
                    UI_Row
                    {
                        ui_labelf("parent");
                        ui_spacer(ui_pct(1.0, 0.0));
                        ui_labelf("%S", active_node->parent->name);
                    }
                }
                UI_Row
                {
                    ui_labelf("pos");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->pos.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###pos_x"));
                    ui_f32_edit(&active_node->pos.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###pos_y"));
                    ui_f32_edit(&active_node->pos.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###pos_z"));
                }
                UI_Row
                {
                    ui_labelf("scale");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->scale.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###scale_x"));
                    ui_f32_edit(&active_node->scale.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###scale_y"));
                    ui_f32_edit(&active_node->scale.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###scale_z"));
                }
                UI_Row
                {
                    ui_labelf("rot");
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_f32_edit(&active_node->rot.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("X###rot_x"));
                    ui_f32_edit(&active_node->rot.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Y###rot_y"));
                    ui_f32_edit(&active_node->rot.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("Z###rot_z"));
                    ui_f32_edit(&active_node->rot.w, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, str8_lit("W###rot_w"));
                }
            }
        }
    }
    rk_ui_pane_end();
    ProfEnd();
}

internal void rk_ui_profiler(void)
{
    ProfBeginFunction();
    typedef struct RK_Profiler_State RK_Profiler_State;
    struct RK_Profiler_State
    {
        B32 show;
        UI_ScrollPt table_scroller;
    };

    RK_View *view = rk_view_from_kind(RK_ViewKind_Profiler);
    RK_Profiler_State *profiler = view->custom_data;
    if(profiler == 0)
    {
        profiler = push_array(view->arena, RK_Profiler_State, 1);
        view->custom_data = profiler;

        profiler->show = 1;
    }

    // Top-level pane 
    UI_Box *container_box;
    UI_FocusActive(UI_FocusKind_Root) UI_FocusHot(UI_FocusKind_Root) UI_Transparency(0.1)
    {
        Rng2F32 panel_rect = rk_state->window_rect;
        panel_rect.p0.x = panel_rect.p1.x - 1100;
        panel_rect.p0.y = panel_rect.p1.y - 1300;
        panel_rect = pad_2f32(panel_rect,-30);
        container_box = rk_ui_pane_begin(panel_rect, &profiler->show, str8_lit("PROFILER"));
    }

    F32 row_height = ui_top_font_size()*1.3f;
    ui_push_pref_height(ui_px(row_height, 0));

    // tag + total_cycles + call_count + cycles_per_call + total_us + us_per_call
    U64 col_count = 6;

    // Table header
    ui_set_next_child_layout_axis(Axis2_X);
    // NOTE(k): width - scrollbar width
    ui_set_next_pref_width(ui_px(container_box->fixed_size.x-ui_top_font_size()*0.9f, 0.f));
    UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
    UI_Parent(header_box) UI_PrefWidth(ui_pct(1.f,0.f)) UI_Flags(UI_BoxFlag_DrawBorder) UI_TextAlignment(UI_TextAlign_Center)
    {
        ui_labelf("Tag");
        ui_labelf("Cycles");
        ui_labelf("Calls");
        ui_labelf("Cycles per call");
        ui_labelf("US");
        ui_labelf("US per call");
    }

    Rng2F32 content_rect = container_box->rect;
    content_rect.y0 = header_box->rect.y1;

    // Content
    ProfNodeSlot *prof_table = ProfTablePst();
    U64 prof_node_count = 0;
    if(prof_table != 0)
    {
        for(U64 slot_idx = 0; slot_idx < prof_hash_table_size; slot_idx++)
        {
            prof_node_count += prof_table[slot_idx].count;
        }
    }

    UI_ScrollListParams scroll_list_params = {0};
    {
        Vec2F32 rect_dim = dim_2f32(content_rect);
        scroll_list_params.flags = UI_ScrollListFlag_All;
        scroll_list_params.row_height_px = row_height;
        scroll_list_params.dim_px = rect_dim;
        scroll_list_params.item_range = r1s64(0, prof_node_count);
        scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 0;
    }

    // Animation rate
    F32 fast_rate = 1 - pow_f32(2, (-40.f * rk_state->dt_sec));

    // Animating scroller pt
    // TODO(k): we should move this part into RK_View
    profiler->table_scroller.off -= profiler->table_scroller.off * fast_rate;
    if(abs_f32(profiler->table_scroller.off) < 0.01) profiler->table_scroller.off = 0;

    UI_ScrollListSignal scroll_list_sig = {0};
    Rng1S64 visible_row_rng = {0};

    UI_ScrollList(&scroll_list_params, &profiler->table_scroller, 0, 0, &visible_row_rng, &scroll_list_sig)
    {
        if(prof_table != 0)
        {
            for(U64 slot_idx = 0; slot_idx < prof_hash_table_size; slot_idx++)
            {
                for(ProfNode *n = prof_table[slot_idx].first; n != 0; n = n->hash_next)
                {
                    UI_Row UI_PrefWidth(ui_pct(1.f, 0.f)) UI_Flags(UI_BoxFlag_DrawBorder)
                    {
                        ui_label(n->tag);
                        ui_labelf("%lu", n->total_cycles);
                        ui_labelf("%lu", n->call_count);
                        ui_labelf("%lu", n->cycles_per_call);
                        ui_labelf("%lu", n->total_us);
                        ui_labelf("%lu", n->us_per_call);
                    }
                }
            }
        }
    }

    ui_pop_pref_height();
    rk_ui_pane_end();
    ProfEnd();
}

/////////////////////////////////
// Frame

internal void
rk_frame(RK_Scene *scene, OS_EventList os_events, U64 dt, U64 hot_key)
{
    ProfBeginFunction();
    // Begin of the frame
    {
        arena_clear(rk_state->frame_arena);
        rk_push_bucket(scene->bucket);
        rk_push_scene(scene);

        rk_state->dt = dt;
        rk_state->dt_sec = dt/1000000.0f;
        rk_state->dt_ms = dt/1000.0f;
        rk_state->hot_key = (RK_Key){ hot_key };
        rk_state->window_rect = os_client_rect_from_window(rk_state->os_wnd);
        rk_state->window_dim = dim_2f32(rk_state->window_rect);
        rk_state->last_cursor = rk_state->cursor;
        rk_state->cursor = os_mouse_from_window(rk_state->os_wnd);
    }

    // Remake bucket every frame
    rk_state->bucket_rect = d_bucket_make();
    rk_state->bucket_geo3d = d_bucket_make();

    D_BucketScope(rk_state->bucket_rect)
    {
        rk_ui_inspector();
        rk_ui_stats();
        rk_ui_profiler();
    }

    // Process events
    OS_Event *os_evt_first = os_events.first;
    OS_Event *os_evt_opl = os_events.last + 1;
    for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    {
        if(os_evt == 0) continue;
        // if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_Space) {}
        if(os_evt->kind == OS_EventKind_Press)
        {
            U64 camera_idx = os_evt->key - OS_Key_F1;
            U64 i = 0;
            for(RK_CameraNode *cn = scene->first_camera; cn != 0; cn = cn->next, i++)
            {
                if(i == camera_idx)
                {
                    scene->active_camera = cn;
                    break;
                }
            }
        }
    }

    // Unpack camera
    RK_Node *camera = scene->active_camera->v;

    // RK_Node *hot_node = 0;
    RK_Node *active_node = rk_node_from_key(rk_state->active_key);

    // UI Box for game viewport (Handle user interaction)
    ui_set_next_rect(rk_state->window_rect);
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *overlay = ui_build_box_from_string(UI_BoxFlag_MouseClickable|UI_BoxFlag_ClickToFocus|UI_BoxFlag_Scroll|UI_BoxFlag_DisableFocusOverlay, str8_lit("###game_overlay"));
    rk_state->sig = ui_signal_from_box(overlay);

    B32 show_grid   = 1;
    B32 show_gizmos = 0;
    d_push_bucket(rk_state->bucket_geo3d);
    R_PassParams_Geo3D *geo3d_pass = d_geo3d_begin(overlay->rect, mat_4x4f32(1.f), mat_4x4f32(1.f), normalize_3f32(scene->global_light), show_grid, show_gizmos, mat_4x4f32(1.f), v3f32(0,0,0));
    d_pop_bucket();

    R_GeoPolygonKind polygon_mode;
    switch(scene->viewport_shading)
    {
        case RK_ViewportShadingKind_Wireframe:       {polygon_mode = R_GeoPolygonKind_Line;}break;
        case RK_ViewportShadingKind_Solid:           {polygon_mode = R_GeoPolygonKind_Fill;}break;
        case RK_ViewportShadingKind_MaterialPreview: {polygon_mode = R_GeoPolygonKind_Fill;}break;
        default:                                    {InvalidPath;}break;
    }

    // Update/draw node in the scene tree
    {
        RK_Node *node = scene->root;
        while(node != 0)
        {
            RK_NodeRec rec = rk_node_df_pre(node, 0);

            base_fn(node, scene, os_events, rk_state->dt_sec);
            for(RK_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
            {
                fn->f(node, scene, os_events, rk_state->dt_sec);
            }

            D_BucketScope(rk_state->bucket_geo3d)
            {
                switch(node->kind)
                {
                    default: {}break;
                    case RK_NodeKind_MeshPrimitive:
                    {
                        Mat4x4F32 *joint_xforms = node->parent->v.mesh_grp.joint_xforms;
                        U64 joint_count = node->parent->v.mesh_grp.joint_count;
                        R_Mesh3DInst *inst = d_mesh(node->v.mesh_primitive.vertices, node->v.mesh_primitive.indices,
                                                    R_GeoTopologyKind_Triangles, polygon_mode,
                                                    R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB, node->v.mesh_primitive.albedo_tex,
                                                    joint_xforms, joint_count,
                                                    mat_4x4f32(1.f), node->key.u64[0]);
                        inst->xform = node->fixed_xform;
                        if(r_handle_match(node->v.mesh_primitive.albedo_tex, r_handle_zero()) || scene->viewport_shading == RK_ViewportShadingKind_Solid)
                        {
                            inst->white_texture_override = 1.0f;
                        }
                        else 
                        {
                            inst->white_texture_override = 0.0f;
                        }
                    }break;
                }
            }
            node = rec.next;
        }
    }

    geo3d_pass->view = rk_view_from_node(camera);
    geo3d_pass->projection = make_perspective_vulkan_4x4f32(camera->v.camera.fov, rk_state->window_dim.x/rk_state->window_dim.y, camera->v.camera.zn, camera->v.camera.zf);
    Mat4x4F32 xform_m = mul_4x4f32(geo3d_pass->projection, geo3d_pass->view);

    if(active_node != 0) 
    {
        geo3d_pass->show_gizmos = 1;
        geo3d_pass->gizmos_xform = active_node->fixed_xform;
        geo3d_pass->gizmos_origin = v4f32(geo3d_pass->gizmos_xform.v[3][0],
                                          geo3d_pass->gizmos_xform.v[3][1],
                                          geo3d_pass->gizmos_xform.v[3][2],
                                          1);
    }

    // Update hot/active
    {
        if(rk_state->sig.f & UI_SignalFlag_LeftReleased)
        {
            rk_state->is_dragging = 0;
        }

        if((rk_state->sig.f & UI_SignalFlag_LeftReleased) && active_node != 0)
        {
            rk_node_delta_commit(active_node);
        }

        if((rk_state->sig.f & UI_SignalFlag_LeftPressed) &&
            rk_state->hot_key.u64[0] != RK_SpecialKeyKind_GizmosIhat &&
            rk_state->hot_key.u64[0] != RK_SpecialKeyKind_GizmosJhat &&
            rk_state->hot_key.u64[0] != RK_SpecialKeyKind_GizmosKhat)
        {
            rk_set_active_key(rk_state->hot_key);
        }

        if(active_node != 0)
        {
            // Gizmos dragging
            Vec3F32 local_i;
            Vec3F32 local_j;
            Vec3F32 local_k;
            rk_local_coord_from_node(active_node, &local_k, &local_i, &local_j);

            // Dragging started
            if((rk_state->sig.f & UI_SignalFlag_LeftDragging) &&
                rk_state->is_dragging == 0 &&
                (rk_state->hot_key.u64[0] == RK_SpecialKeyKind_GizmosIhat || rk_state->hot_key.u64[0] == RK_SpecialKeyKind_GizmosJhat  || rk_state->hot_key.u64[0] == RK_SpecialKeyKind_GizmosKhat))
            {
                Vec3F32 direction = {0};
                switch(rk_state->hot_key.u64[0])
                {
                    default: {InvalidPath;}break;
                    case RK_SpecialKeyKind_GizmosIhat: {direction = local_i;}break;
                    case RK_SpecialKeyKind_GizmosJhat: {direction = local_j;}break;
                    case RK_SpecialKeyKind_GizmosKhat: {direction = local_k;}break;
                }
                rk_state->is_dragging = 1;
                rk_state->drag_start_direction = direction;
            }

            // Dragging
            if(rk_state->is_dragging)
            {
                Vec2F32 delta = ui_drag_delta();
                Vec4F32 dir = v4f32(rk_state->drag_start_direction.x,
                                    rk_state->drag_start_direction.y,
                                    rk_state->drag_start_direction.z,
                                    1.0);
                Vec4F32 projected_start = mat_4x4f32_transform_4f32(xform_m, v4f32(0,0,0,1.0));
                if(projected_start.w != 0)
                {
                    projected_start.x /= projected_start.w;
                    projected_start.y /= projected_start.w;
                }
                Vec4F32 projected_end = mat_4x4f32_transform_4f32(xform_m, dir);
                if(projected_end.w != 0)
                {
                    projected_end.x /= projected_end.w;
                    projected_end.y /= projected_end.w;
                }
                Vec2F32 projected_dir = sub_2f32(v2f32(projected_end.x, projected_end.y), v2f32(projected_start.x, projected_start.y));

                // TODO: negating direction could cause rapid changes on the position, we would want to avoid that 
                S32 n = dot_2f32(delta, projected_dir) > 0 ? 1 : -1;

                // 10.0 in word coordinate per pixel (scaled by the distance to the eye)
                F32 speed = 1.0/(rk_state->window_dim.x*length_2f32(projected_dir));
                active_node->pst_pos_delta = scale_3f32(rk_state->drag_start_direction, length_2f32(delta)*speed*n);
            }
        }
    }

    if(camera->v.camera.hide_cursor && (!rk_state->cursor_hidden))
    {
        os_hide_cursor(rk_state->os_wnd);
        rk_state->cursor_hidden = 1;
    }

    if(!camera->v.camera.hide_cursor && rk_state->cursor_hidden)
    {
        os_show_cursor(rk_state->os_wnd);
        rk_state->cursor_hidden = 0;
    }
    if(camera->v.camera.lock_cursor)
    {
        Vec2F32 cursor = center_2f32(rk_state->window_rect);
        os_wrap_cursor(rk_state->os_wnd, cursor.x, cursor.y);
        rk_state->cursor = cursor;
    }

    // End of the frame
    {
        rk_pop_bucket();
        rk_pop_scene();

        RK_Scene *s = rk_state->first_to_free_scene;
        for(RK_Scene *s = rk_state->first_to_free_scene; s != 0; s = rk_state->first_to_free_scene)
        {
            rk_scene_release(s);
        }
    }
    ProfEnd();
}

/////////////////////////////////
// Helpers

internal void
rk_trs_from_matrix(Mat4x4F32 *m, Vec3F32 *trans, QuatF32 *rot, Vec3F32 *scale)
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

internal RK_Scene *
rk_scene_alloc()
{
    Arena *arena = 0;
    RK_Scene *scene = 0;
    if(rk_state->first_free_scene == 0)
    {
        arena = arena_alloc(.reserve_size = MB(64), .commit_size = MB(64));
    }
    else
    {
        arena = rk_state->first_free_scene->arena;
        arena_clear(arena);
    }
    scene = push_array(arena, RK_Scene, 1);
    scene->arena = arena;
    scene->bucket = rk_bucket_make(arena, 1000);
    scene->mesh_cache_table.slot_count = 1000;
    scene->mesh_cache_table.arena      = arena;
    scene->mesh_cache_table.slots = push_array(arena, RK_MeshCacheSlot, scene->mesh_cache_table.slot_count);
    return scene;
}

internal void
rk_scene_release(RK_Scene *s)
{
    SLLStackPop(rk_state->first_to_free_scene);
    SLLStackPush(rk_state->first_free_scene, s);
    // TODO: there should something else to do here
}
