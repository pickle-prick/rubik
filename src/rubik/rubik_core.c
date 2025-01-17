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
rk_ui_draw()
{
    Temp scratch = scratch_begin(0,0);

    // DEBUG mouse
    if(0)
    {
        R_Rect2DInst *cursor = d_rect(r2f32p(ui_state->mouse.x-15,ui_state->mouse.y-15, ui_state->mouse.x+15,ui_state->mouse.y+15), v4f32(0,0.3,1,0.3), 15, 0.0, 0.7);
    }

    // Recusivly drawing boxes
    UI_Box *box = ui_root_from_state(ui_state);
    while(!ui_box_is_nil(box))
    {
        UI_BoxRec rec = ui_box_rec_df_post(box, &ui_nil_box);

        if(box->transparency != 0)
        {
            d_push_transparency(box->transparency);
        }

        //- k: draw drop_shadw
        if(box->flags & UI_BoxFlag_DrawDropShadow)
        {
            Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
            Vec4F32 drop_shadow_color = rk_rgba_from_theme_color(RK_ThemeColor_DropShadow);
            d_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
        }

        //- k: draw background
        if(box->flags & UI_BoxFlag_DrawBackground)
        {
            // Main rectangle
            R_Rect2DInst *inst = d_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);

            if(box->flags & UI_BoxFlag_DrawHotEffects)
            {
                F32 effective_active_t = box->active_t;
                if (!(box->flags & UI_BoxFlag_DrawActiveEffects))
                {
                    effective_active_t = 0;
                }
                F32 t = box->hot_t * (1-effective_active_t);

                // Brighten
                {
                    R_Rect2DInst *inst = d_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                    color.w *= t*0.2f;
                    inst->colors[Corner_00] = color;
                    inst->colors[Corner_01] = color;
                    inst->colors[Corner_10] = color;
                    inst->colors[Corner_11] = color;
                    inst->colors[Corner_10].w *= t;
                    inst->colors[Corner_11].w *= t;
                    MemoryCopyArray(inst->corner_radii, box->corner_radii);
                }

                // rjf: slight emboss fadeoff
                if(0)
                {
                    Rng2F32 rect = r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y1);
                    R_Rect2DInst *inst = d_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
                    inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
                    inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
                    inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
                    MemoryCopyArray(inst->corner_radii, box->corner_radii);
                }

                // Active effect extension
                if(box->flags & UI_BoxFlag_DrawActiveEffects)
                {
                    Vec4F32 shadow_color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                    shadow_color.x *= 0.3f;
                    shadow_color.y *= 0.3f;
                    shadow_color.z *= 0.3f;
                    shadow_color.w *= 0.5f*box->active_t;

                    Vec2F32 shadow_size =
                    {
                        (box->rect.x1 - box->rect.x0)*0.60f*box->active_t,
                        (box->rect.y1 - box->rect.y0)*0.60f*box->active_t,
                    };
                    shadow_size.x = Clamp(0, shadow_size.x, box->font_size*2.f);
                    shadow_size.y = Clamp(0, shadow_size.y, box->font_size*2.f);

                    // rjf: top -> bottom dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: bottom -> top light effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y1 - shadow_size.y, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.4f, 0.4f, 0.4f, 0.4f*box->active_t);
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: left -> right dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x0 + shadow_size.x, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_00] = shadow_color;
                        inst->colors[Corner_01] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: right -> left dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_11] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }
                }
            }
        }

        // Draw string
        if(box->flags & UI_BoxFlag_DrawText)
        {
            // TODO: handle font color
            Vec2F32 text_position = ui_box_text_position(box);

            // Max width
            F32 max_x = 100000.0f;
            F_Run ellipses_run = {0};

            if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
            {
                max_x = (box->rect.x1-box->text_padding);
                ellipses_run = f_push_run_from_string(scratch.arena, box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
            }

            d_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
        }

        // NOTE(k): draw focus viz
        if(1)
        {
            B32 focused = (box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive) &&
                    box->flags & UI_BoxFlag_Clickable);
            B32 disabled = 0;
            for(UI_Box *p = box; !ui_box_is_nil(p); p = p->parent)
            {
                if(p->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                {
                    disabled = 1;
                    break;
                }
            }
            if(focused)
            {
                Vec4F32 color = v4f32(0.3f, 0.8f, 0.3f, 1.f);
                if(disabled)
                {
                    color = v4f32(0.8f, 0.3f, 0.3f, 1.f);
                }
                d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), color, 2, 0, 1);
                d_rect(box->rect, color, 2, 2, 1);
            }
            if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
            {
                if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                {
                    d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(1, 0, 0, 0.2f), 2, 0, 1);
                }
                else
                {
                    d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(0, 1, 0, 0.2f), 2, 0, 1);
                }
            }
        }

        if(box->flags & UI_BoxFlag_Clip)
        {
            Rng2F32 top_clip = d_top_clip();
            Rng2F32 new_clip = pad_2f32(box->rect, -1);
            if(top_clip.x1 != 0 || top_clip.y1 != 0)
            {
                new_clip = intersect_2f32(new_clip, top_clip);
            }
            d_push_clip(new_clip);
        }

        // k: custom draw list
        if(box->flags & UI_BoxFlag_DrawBucket)
        {
            Mat3x3F32 xform = make_translate_3x3f32(box->position_delta);
            D_XForm2DScope(xform)
            {
                d_sub_bucket(box->draw_bucket, 1);
            }
        }

        // Call custom draw callback
        if(box->custom_draw != 0)
        {
            box->custom_draw(box, box->custom_draw_user_data);
        }

        // k: pop stacks
        {
            S32 pop_idx = 0;
            for(UI_Box *b = box; !ui_box_is_nil(b) && pop_idx <= rec.pop_count; b = b->parent)
            {
                pop_idx += 1;
                if(b == box && rec.push_count != 0) continue;

                // k: pop clip
                if(b->flags & UI_BoxFlag_Clip)
                {
                    d_pop_clip();
                }

                // rjf: draw overlay
                if(b->flags & UI_BoxFlag_DrawOverlay)
                {
                    R_Rect2DInst *inst = d_rect(b->rect, b->palette->colors[UI_ColorCode_Overlay], 0, 0, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                //- k: draw the border
                if(b->flags & UI_BoxFlag_DrawBorder)
                {
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1.f), b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);

                    // rjf: hover effect
                    if(b->flags & UI_BoxFlag_DrawHotEffects)
                    {
                        Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                        color.w *= b->hot_t;
                        R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), color, 0, 1.f, 1.f);
                        MemoryCopyArray(inst->corner_radii, b->corner_radii);
                    }
                }

                // k: debug border rendering
                if(0)
                {
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: draw sides
                {
                    Rng2F32 r = b->rect;
                    F32 half_thickness = 1.f;
                    F32 softness = 0.5f;
                    if(b->flags & UI_BoxFlag_DrawSideTop)
                    {
                        d_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideBottom)
                    {
                        d_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideLeft)
                    {
                        d_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideRight)
                    {
                        d_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                }

                // rjf: draw focus overlay
                if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Focus);
                    color.w *= 0.2f*b->focus_hot_t;
                    R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 0.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: draw focus border
                if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Focus);
                    color.w *= b->focus_active_t;
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: disabled overlay
                if(b->disabled_t >= 0.005f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_DisabledOverlay);
                    color.w *= b->disabled_t;
                    R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // k: pop transparency
                if(b->transparency != 0)
                {
                    d_pop_transparency();
                }
            }
        }
        box = rec.next;
    }
    scratch_end(scratch);
}

/////////////////////////////////
// Basic Type Functions

internal U64
rk_hash_from_string(U64 seed, String8 string)
{
    U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
    return result;
}

internal String8
rk_hash_part_from_key_string(String8 string)
{
    String8 result = string;

    // k: look for ### patterns, which can replace the entirety of the part of
    // the string that is hashed.
    U64 hash_replace_signifier_pos = str8_find_needle(string, 0, str8_lit("###"), 0);
    if(hash_replace_signifier_pos < string.size)
    {
        result = str8_skip(string, hash_replace_signifier_pos);
    }

    return result;
}

internal String8
rk_display_part_from_key_string(String8 string)
{
    U64 hash_pos = str8_find_needle(string, 0, str8_lit("##"), 0);
    string.size = hash_pos;
    return string;
}

/////////////////////////////////
//~ Key

internal RK_Key
rk_key_from_string(RK_Key seed_key, String8 string)
{
    RK_Key result = {0};
    if(string.size != 0)
    {
        String8 hash_part = rk_hash_part_from_key_string(string);
        result.u64[0] = rk_hash_from_string(seed_key.u64[0], hash_part);
    }
    return result;
}

internal RK_Key
rk_key_from_stringf(RK_Key seed_key, char *fmt, ...)
{
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);

    RK_Key result = {0};
    result = rk_key_from_string(seed_key, string);
    scratch_end(scratch);
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

internal B32
rk_key_match(RK_Key a, RK_Key b)
{
    return a.u64[0] == b.u64[0];
}

internal RK_Key
rk_key_make(U64 v)
{
    RK_Key result = {v};
    return result;
}

internal RK_Key
rk_key_zero()
{
    return (RK_Key){0}; 
}

/////////////////////////////////
// Handle

internal RK_Handle
rk_handle_zero()
{
    RK_Handle ret = {0};
    return ret;
}

internal B32
rk_handle_match(RK_Handle a, RK_Handle b)
{
    return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

/////////////////////////////////
// Bucket

internal RK_NodeBucket *
rk_node_bucket_make(Arena *arena, U64 max_nodes)
{
    RK_NodeBucket *ret = push_array(arena, RK_NodeBucket, 1);
    ret->arena_ref = arena;
    ret->hash_table = push_array(arena, RK_NodeBucketSlot, max_nodes);
    ret->hash_table_size = max_nodes;
    return ret;
}

internal RK_ResourceBucket *
rk_res_bucket_make(Arena *arena, U64 max_resources)
{
    RK_ResourceBucket *ret = push_array(arena, RK_ResourceBucket, 1);
    ret->arena_ref = arena;
    ret->hash_table = push_array(arena, RK_ResourceBucketSlot, max_resources);
    ret->hash_table_size = max_resources;
    return ret;
}

internal void
rk_res_bucket_release(RK_ResourceBucket *res_bucket)
{
    for(U64 i = 0; i < res_bucket->hash_table_size; i++)
    {
        RK_ResourceBucketSlot *slot = &res_bucket->hash_table[i];

        for(RK_Resource *res = slot->first; res != 0; res = res->next)
        {
            rk_res_release(res);
        }
    }
}

internal RK_Node *
rk_node_from_key(RK_Key key)
{
    RK_Node *result = 0;
    RK_NodeBucket *node_bucket = rk_top_node_bucket();
    U64 slot_idx = key.u64[0] % node_bucket->hash_table_size;
    for(RK_Node *node = node_bucket->hash_table[slot_idx].first; node != 0; node = node->hash_next)
    {
        if(rk_key_match(node->key, key))
        {
            result = node;
            break;
        }
    }
    return result;
}

//~ Resourcea Type Functions

internal RK_Key
rk_res_key_from_string(RK_ResourceKind kind, RK_Key seed, String8 string)
{
    RK_Key ret;
    RK_Key src_key = rk_key_from_string(seed, string);
    ret = rk_key_merge(rk_key_make(kind), src_key);
    return ret;
}

internal RK_Key
rk_res_key_from_stringf(RK_ResourceKind kind, RK_Key seed, char* fmt, ...)
{
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);

    RK_Key result = {0};
    result = rk_res_key_from_string(kind, seed, string);
    scratch_end(scratch);
    return result;
}

internal RK_Handle
rk_resh_alloc(RK_Key key, RK_ResourceKind kind, B32 acquire)
{
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    RK_Resource *ret = res_bucket->first_free_res;

    if(ret != 0)
    {
        U64 generation = ret->generation;
        SLLStackPop(res_bucket->first_free_res);
        MemoryZeroStruct(ret);
        ret->generation = generation;
    }
    else
    {
        ret = push_array(res_bucket->arena_ref, RK_Resource, 1);
    }

    // fill info
    ret->kind = kind;
    ret->key = key;
    ret->rc = !!acquire;

    // insert into hash table
    U64 slot_idx = key.u64[0] % res_bucket->hash_table_size;
    RK_ResourceBucketSlot *slot = &res_bucket->hash_table[slot_idx];
    DLLPushBack_NP(slot->first, slot->last, ret, hash_next, hash_prev);
    res_bucket->resource_count++;
    return rk_handle_from_res(ret);
}

internal void
rk_res_release(RK_Resource *res)
{
    // TODO
    NotImplemented;

    RK_ResourceBucket *res_bucket = 0;
    if(res != 0)
    {
        res_bucket = res->owner_bucket;
    }
}

internal RK_Handle
rk_resh_from_key(RK_Key key)
{
    RK_Resource *ret = 0;
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    U64 slot_idx = key.u64[0] % res_bucket->hash_table_size;
    for(RK_Resource *res = res_bucket->hash_table[slot_idx].first; res != 0; res = res->hash_next)
    {
        if(rk_key_match(res->key, key))
        {
            ret = res;
            break;
        }
    }
    return rk_handle_from_res(ret);
}

internal RK_Handle
rk_resh_acquire_from_key(RK_Key key)
{
    RK_Resource *ret = 0;
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    U64 slot_idx = key.u64[0] % res_bucket->hash_table_size;
    for(RK_Resource *res = res_bucket->hash_table[slot_idx].first; res != 0; res = res->hash_next)
    {
        if(rk_key_match(res->key, key))
        {
            ret = res;
            break;
        }
    }
    if(ret!=0) ret->rc++;
    return rk_handle_from_res(ret);
}

internal RK_Handle
rk_resh_acquire(RK_Handle handle)
{
    RK_Resource *res = rk_res_from_handle(handle);
    if(res !=0 )
    {
        res->rc++;
    }
    return handle;
}

internal void rk_resh_return(RK_Handle handle)
{
    RK_Resource *res = rk_res_from_handle(handle);
    if(res != 0)
    {
        res->rc--;

        if(res->rc == 0 && res->auto_free)
        {
            rk_res_release(res);
        }
    }
}

internal RK_Resource *
rk_res_from_handle(RK_Handle handle)
{
    RK_Resource *ret = (RK_Resource *)handle.u64[0];
    U64 generation = handle.u64[1];
    if(ret != 0 && generation == ret->generation)
    {
        return ret;
    }
    return 0;
}

internal void *
rk_res_data_from_handle(RK_Handle handle)
{
    void *ret = 0;
    RK_Resource *res = rk_res_from_handle(handle);
    if(res != 0)
    {
        ret = &res->v;
    }
    return ret;
}

internal RK_Handle
rk_handle_from_res(RK_Resource *res)
{
    RK_Handle ret = {0};
    ret.u64[0] = (U64)res;
    if(res) ret.u64[1] = res->generation;
    return ret;
}

//- High-lelve Resource Allocation Functions

internal RK_Handle
rk_material_from_color(String8 name, Vec4F32 color)
{
    RK_Key res_key = rk_res_key_from_string(RK_ResourceKind_Material, rk_key_zero(), name);
    RK_Handle ret = rk_resh_from_key(res_key);
    if(rk_handle_is_zero(ret))
    {
        ret = rk_resh_alloc(res_key, RK_ResourceKind_Material, 0);
        RK_Material *material = rk_res_data_from_handle(ret);
        material->base_clr_factor = color;
        material->name = push_str8_copy_static(name, material->name_buffer, ArrayCount(material->name_buffer));
    }
    return ret;
}

/////////////////////////////////
//- Resourcea Building Helpers (subresource management etc...)

internal RK_Animation *
rk_animation_alloc()
{
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    RK_Animation *ret = res_bucket->first_free_animation;

    if(ret != 0)
    {
        SLLStackPop(res_bucket->first_free_animation);
    }
    else
    {
        ret = push_array_no_zero(res_bucket->arena_ref, RK_Animation, 1);
    }
    MemoryZeroStruct(ret);
    return ret;
}
internal RK_Track *
rk_track_alloc()
{
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    RK_Track *ret = res_bucket->first_free_track;
    if(ret != 0)
    {
        SLLStackPop(res_bucket->first_free_track);
    }
    else
    {
        ret = push_array_no_zero(res_bucket->arena_ref, RK_Track, 1);
    }
    MemoryZeroStruct(ret);
    return ret;
}
internal RK_TrackFrame *
rk_track_frame_alloc()
{
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();
    RK_TrackFrame *ret = res_bucket->first_free_track_frame;
    if(ret != 0)
    {
        SLLStackPop(res_bucket->first_free_track_frame);
    }
    else
    {
        ret = push_array_no_zero(res_bucket->arena_ref, RK_TrackFrame, 1);
    }
    MemoryZeroStruct(ret);
    return ret;
}

/////////////////////////////////
// State accessor/mutator

internal void
rk_init(OS_Handle os_wnd)
{
    Arena *arena = arena_alloc();
    rk_state = push_array(arena, RK_State, 1);
    {
        rk_state->arena           = arena;
        for(U64 i = 0; i < ArrayCount(rk_state->frame_arenas); i++)
        {
            rk_state->frame_arenas[i] = arena_alloc();
        }
        rk_state->node_bucket     = rk_node_bucket_make(arena, 4096-1);
        rk_state->res_node_bucket = rk_node_bucket_make(arena, 4096-1);
        rk_state->res_bucket      = rk_res_bucket_make(arena, 4096-1);
        rk_state->os_wnd          = os_wnd;
        rk_state->dpi             = os_dpi_from_window(os_wnd);
        rk_state->last_dpi        = rk_state->last_dpi;
        for(U64 i = 0; i < ArrayCount(rk_state->gizmo_drawlists); i++)
        {
            // TODO(k): some device offers 256MB memory which is both cpu visiable and device local
            rk_state->gizmo_drawlists[i] = rk_drawlist_alloc(arena, MB(8), MB(8));
        }
    }

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

internal Arena *
rk_frame_arena()
{
    return rk_state->frame_arenas[rk_state->frame_counter % ArrayCount(rk_state->frame_arenas)];
}

internal RK_DrawList *
rk_frame_gizmo_drawlist()
{
    return rk_state->gizmo_drawlists[rk_state->frame_counter % ArrayCount(rk_state->gizmo_drawlists)];
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
//~ Node build api

internal RK_Node *
rk_node_alloc(RK_NodeTypeFlags type_flags)
{
    RK_Node *ret = 0;
    RK_NodeBucket *node_bucket = rk_top_node_bucket();

    if(node_bucket->first_free_node != 0)
    {
        ret = node_bucket->first_free_node;
        U64 generation = ret->generation;
        MemoryZeroStruct(ret);
        ret->generation = generation;
        SLLStackPop(node_bucket->first_free_node);
    }
    else
    {
        ret = push_array(node_bucket->arena_ref, RK_Node, 1);
    }

    ret->owner_bucket = node_bucket;

    // some default values
    {
        ret->fixed_xform = mat_4x4f32(1.f);
    }

    if(type_flags & RK_NodeTypeFlag_Node2D)
    {
        RK_Node2D *node2d;
        if(node_bucket->first_free_node2d != 0)
        {
            node2d = node_bucket->first_free_node2d;
            SLLStackPop(node_bucket->first_free_node2d);
        }
        else
        {
            node2d = push_array_no_zero(node_bucket->arena_ref, RK_Node2D, 1);
        }
        MemoryZeroStruct(node2d);
        ret->node2d = node2d;
    }

    if(type_flags & RK_NodeTypeFlag_Node3D)
    {
        RK_Node3D *node3d;
        if(node_bucket->first_free_node3d != 0)
        {
            node3d = node_bucket->first_free_node3d;
            SLLStackPop(node_bucket->first_free_node3d);
        }
        else
        {
            node3d = push_array_no_zero(node_bucket->arena_ref, RK_Node3D, 1);
        }
        MemoryZeroStruct(node3d);
        node3d->transform.rotation = make_indentity_quat_f32();
        node3d->transform.scale = v3f32(1,1,1);
        ret->node3d = node3d;
    }

    if(type_flags & RK_NodeTypeFlag_Camera3D)
    {
        RK_Camera3D *camera3d;
        if(node_bucket->first_free_camera3d != 0)
        {
            camera3d = node_bucket->first_free_camera3d;
            SLLStackPop(node_bucket->first_free_camera3d);
        }
        else
        {
            camera3d = push_array_no_zero(node_bucket->arena_ref, RK_Camera3D, 1);
        }
        MemoryZeroStruct(camera3d);
        ret->camera3d = camera3d;
    }

    if(type_flags & RK_NodeTypeFlag_MeshInstance3D)
    {
        RK_MeshInstance3D *mesh_inst3d;
        if(node_bucket->first_free_mesh_inst3d != 0)
        {
            mesh_inst3d = node_bucket->first_free_mesh_inst3d;
            SLLStackPop(node_bucket->first_free_mesh_inst3d);
        }
        else
        {
            mesh_inst3d = push_array_no_zero(node_bucket->arena_ref, RK_MeshInstance3D, 1);
        }
        MemoryZeroStruct(mesh_inst3d);
        ret->mesh_inst3d = mesh_inst3d;
    }

    if(type_flags & RK_NodeTypeFlag_AnimationPlayer)
    {
        RK_AnimationPlayer *animation_player;
        if(node_bucket->first_free_animation_player != 0)
        {
            animation_player = node_bucket->first_free_animation_player;
            SLLStackPop(node_bucket->first_free_animation_player);
        }
        else
        {
            animation_player = push_array_no_zero(node_bucket->arena_ref, RK_AnimationPlayer, 1);
        }
        MemoryZeroStruct(animation_player);
        ret->animation_player = animation_player;
    }
    return ret;
}

internal void
rk_node_release(RK_Node *node)
{
    // recursive first
    for(RK_Node *child = node->first; child != 0; child = child->next)
    {
        rk_node_release(child);
    }

    RK_NodeBucket *bucket = node->owner_bucket;
    node->generation++;

    // Remove from node tree
    if(node->parent != 0)
    {
        DLLRemove(node->parent->first, node->parent->last, node);
    }

    // Remove from node bucket
    {
        U64 slot_idx = node->key.u64[0] % bucket->hash_table_size;
        RK_NodeBucketSlot *slot = &bucket->hash_table[slot_idx];
        DLLRemove_NP(slot->first, slot->last, node, hash_next, hash_prev);
    }

    // free equipments
    {
        if(node->type_flags & RK_NodeTypeFlag_Node2D)
        {
            SLLStackPush(bucket->first_free_node2d, node->node2d);
        }

        if(node->type_flags & RK_NodeTypeFlag_Node3D)
        {
            SLLStackPush(bucket->first_free_node3d, node->node3d);
        }

        if(node->type_flags & RK_NodeTypeFlag_Camera3D)
        {
            SLLStackPush(bucket->first_free_camera3d, node->camera3d);
        }

        if(node->type_flags & RK_NodeTypeFlag_MeshInstance3D)
        {
            SLLStackPush(bucket->first_free_mesh_inst3d, node->mesh_inst3d);
        }

        if(node->type_flags & RK_NodeTypeFlag_AnimationPlayer)
        {
            SLLStackPush(bucket->first_free_animation_player, node->animation_player);
        }
    }

    // free update fn node
    for(RK_UpdateFnNode *fn_node = node->first_update_fn; fn_node != 0; fn_node = fn_node->next)  
    {
        SLLStackPush(bucket->first_free_update_fn_node, fn_node);
    }

    // free node
    SLLStackPush(bucket->first_free_node, node);
}

internal RK_Node *
rk_build_node_from_string(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, String8 string)
{
    Arena *arena = rk_top_node_bucket()->arena_ref;
    RK_Key key = rk_key_from_string(rk_active_seed_key(), string);
    RK_Node *node = rk_build_node_from_key(type_flags, flags, key);

    rk_node_equip_display_string(node, string);
    return node;
}

internal RK_Node *
rk_build_node_from_stringf(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, char *fmt, ...)
{
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);
    RK_Node *ret = rk_build_node_from_string(type_flags, flags, string);
    scratch_end(scratch);
    return ret;
}

internal RK_Node *
rk_build_node_from_key(RK_NodeTypeFlags type_flags, RK_NodeFlags flags, RK_Key key)
{
    RK_Node *result = rk_node_alloc(type_flags);
    RK_Node *parent = rk_top_parent();
    RK_NodeBucket *node_bucket = rk_top_node_bucket();

    // fill base info
    result->key        = key;
    result->flags      = flags;
    result->type_flags = type_flags;
    result->parent     = parent;

    // Insert to the bucket
    U64 slot_idx = result->key.u64[0] % node_bucket->hash_table_size;
    RK_NodeBucketSlot *slot = &node_bucket->hash_table[slot_idx];
    DLLPushBack_NP(slot->first, slot->last, result, hash_next, hash_prev);
    node_bucket->node_count++;

    // Insert to the parent tree
    if(parent != 0)
    {
        DLLPushBack_NP(parent->first, parent->last, result, next, prev);
        parent->children_count++;
    }
    return result;
}

internal void
rk_node_equip_display_string(RK_Node* node, String8 string)
{
    Arena *arena = node->owner_bucket->arena_ref;
    node->name = push_str8_copy_static(rk_display_part_from_key_string(string), node->name_buffer, ArrayCount(node->name_buffer));
}

/////////////////////////////////
// Node Type Functions

internal RK_NodeRec
rk_node_df(RK_Node *n, RK_Node* root, U64 sib_member_off, U64 child_member_off, B32 force_leaf)
{
    RK_NodeRec result = {0};
    if(!force_leaf && *MemberFromOffset(RK_Node**, n, child_member_off) != 0)
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

// internal void
// rk_local_coord_from_node(RK_Node *n, Vec3F32 *f, Vec3F32 *s, Vec3F32 *u)
// {
//     ProfBeginFunction();
//     Mat4x4F32 xform = n->fixed_xform;
//     // NOTE: remove translate, only rotation matters
//     xform.v[3][0] = xform.v[3][1] = xform.v[3][2] = 0;
//     Vec4F32 i_hat = v4f32(1,0,0,1);
//     Vec4F32 j_hat = v4f32(0,1,0,1);
//     Vec4F32 k_hat = v4f32(0,0,1,1);
// 
//     i_hat = mat_4x4f32_transform_4f32(xform, i_hat);
//     j_hat = mat_4x4f32_transform_4f32(xform, j_hat);
//     k_hat = mat_4x4f32_transform_4f32(xform, k_hat);
// 
//     MemoryCopy(f, &k_hat, sizeof(Vec3F32));
//     MemoryCopy(s, &i_hat, sizeof(Vec3F32));
//     MemoryCopy(u, &j_hat, sizeof(Vec3F32));
// 
//     *f = normalize_3f32(*f);
//     *s = normalize_3f32(*s);
//     *u = normalize_3f32(*u);
// 
//     // Vec4F32 side    = v4f32(1,0,0,0);
//     // Vec4F32 up      = v4f32(0,-1,0,0);
//     // Vec4F32 forward = v4f32(0,0,1,0);
// 
//     // side    = mul_quat_f32(rot_conj,side);
//     // side    = mul_quat_f32(side,rot);
//     // up      = mul_quat_f32(rot_conj,up);
//     // up      = mul_quat_f32(up,rot);
//     // forward = mul_quat_f32(rot_conj,forward);
//     // forward = mul_quat_f32(forward,rot);
// 
//     // MemoryCopy(f, &forward, sizeof(Vec3F32));
//     // MemoryCopy(s, &side, sizeof(Vec3F32));
//     // MemoryCopy(u, &up, sizeof(Vec3F32));
//     ProfEnd();
// }

internal void
rk_node_push_fn(RK_Node *n, RK_NodeCustomUpdateFunctionType *fn)
{
    RK_NodeBucket *node_bucket = n->owner_bucket;
    Arena *arena = node_bucket->arena_ref;

    RK_UpdateFnNode *fn_node = node_bucket->first_free_update_fn_node;
    if(fn_node != 0)
    {
        fn_node = node_bucket->first_free_update_fn_node;
        SLLStackPop(node_bucket->first_free_update_fn_node);
    }
    else
    {
        fn_node = push_array_no_zero(arena, RK_UpdateFnNode, 1);
    }
    MemoryZeroStruct(fn_node);

    fn_node->f = fn;
    DLLPushBack(n->first_update_fn, n->last_update_fn, fn_node);
}

internal RK_Handle
rk_handle_from_node(RK_Node *n)
{
    RK_Handle ret = {0};
    ret.u64[0] = (U64)n;
    if(n) ret.u64[1] = n->generation;
    return ret;
}

internal RK_Node*
rk_node_from_handle(RK_Handle handle)
{
    RK_Node *ret = 0;
    RK_Node *node = (RK_Node*)handle.u64[0];
    U64 generation = handle.u64[1];
    if(node != 0 && node->generation == generation) ret = node;
    return ret;
}

internal B32
rk_node_is_active(RK_Node *node)
{
    B32 ret = 0; 
    RK_Scene *scene = rk_top_scene();
    for(; node != 0 && !(node->flags & RK_NodeFlag_NavigationRoot); node = node->parent) {}

    if(node && node == rk_node_from_handle(scene->active_node))
    {
        ret = 1;
    }
    return ret;
}

/////////////////////////////////
// Node base scripting

RK_NODE_CUSTOM_UPDATE(base_fn)
{
}

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
#if 1
    // Triple product expansion is used to calculate perpendicular normal vectors 
    // which kinda 'prefer' pointing towards the Origin in Minkowski space
    Vec2F32 r;
    
    float ac = a.x * c.x + a.y * c.y; // perform a.dot(c)
    float bc = b.x * c.x + b.y * c.y; // perform b.dot(c)
    
    // perform b * a.dot(c) - a * b.dot(c)
    r.x = b.x * ac - a.x * bc;
    r.y = b.y * ac - a.y * bc;
    return r;
#else
    // Version -1
    Vec3F32 a = {A.x, A.y, 0};
    Vec3F32 b = {B.x, B.y, 0};
    Vec3F32 c = {C.x, C.y, 0};

    Vec3F32 r = cross_3f32(a,b);
    r = cross_3f32(r, c);
    return v2f32(r.x, r.y);
#endif
}

internal B32
v2_in_triangle(Vec2F32 P, Vec2F32 A, Vec2F32 B, Vec2F32 C) {
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
    F32 dpi = rk_state->dpi;
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
        Rng2F32 rect;
    };

    RK_View *view = rk_view_from_kind(RK_ViewKind_Stats);
    RK_Stats_State *stats = view->custom_data;

    if(stats == 0)
    {
        stats = push_array(view->arena, RK_Stats_State, 1);
        view->custom_data = stats;

        stats->show = 1;
        stats->rect = rk_state->window_rect;
        {
            F32 default_width = rk_state->window_dim.x * 0.3f;
            F32 default_height = rk_state->window_dim.x * 0.15f;
            F32 default_margin = ui_top_font_size()*1.3;
            stats->rect.x0 = stats->rect.x1 - default_width;
            stats->rect.y1 = stats->rect.y0 + default_height;
            stats->rect = pad_2f32(stats->rect, -default_margin);
        }
    }

    // collect ui box cache count
    U64 ui_cache_count = 0;
    for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx++)
    {
        for(UI_Box *box = ui_state->box_table[slot_idx].hash_first; !ui_box_is_nil(box); box = box->hash_next)
        {
            AssertAlways(!ui_key_match(box->key, ui_key_zero()));
            ui_cache_count++;
        }
    }

    UI_Transparency(0.1) RK_UI_Pane(&stats->rect, &stats->show, str8_lit("STATS###stats"))
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
                ui_labelf("geo_hot_key");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%I64u", scene->hot_key.u64[0]);
            }
            UI_Row
            {
                ui_labelf("ui_last_build_box_count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", ui_state->last_build_box_count);
            }
            UI_Row
            {
                ui_labelf("ui_cache_count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", ui_cache_count);
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
                ui_labelf("window");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.2f, %.2f", rk_state->window_dim.x, rk_state->window_dim.y);
            }
            UI_Row
            {
                ui_labelf("drag start mouse");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%.2f, %.2f", ui_state->drag_start_mouse.x, ui_state->drag_start_mouse.y);
            }
            UI_Row
            {
                ui_labelf("scene node count");
                ui_spacer(ui_pct(1.0, 0.0));
                ui_labelf("%lu", scene->node_bucket->node_count);
            }
        }
}

internal void rk_ui_inspector(void)
{
    ProfBeginFunction();
    RK_Scene *scene = rk_top_scene();

    // unpack active camera
    RK_Node *camera_node = rk_node_from_handle(scene->active_camera);
    AssertAlways(camera_node != 0 && "No active camera was found");
    RK_Camera3D *camera = camera_node->camera3d;
    RK_Transform3D *camera_transform = &camera_node->node3d->transform;
    Mat4x4F32 camera_xform = camera_node->fixed_xform;

    // unpack active node
    RK_Node *active_node = rk_node_from_handle(scene->active_node);

    typedef struct RK_Inspector_State RK_Inspector_State;
    struct RK_Inspector_State
    {
        B32 show;
        B32 show_scene_cfg;
        B32 show_camera_cfg;
        B32 show_gizmo_cfg;
        B32 show_light_cfg;
        B32 show_node_cfg;

        Rng2F32 rect;

        String8 scene_path_to_save;
        U64 scene_path_to_save_buffer_size;
        S64 last_active_row; // -1 is used to indicate no selection

        // txt input buffer
        TxtPt txt_cursor;
        TxtPt txt_mark;
        B32 txt_has_draft;
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

        inspector->show  = 1;
        inspector->show_scene_cfg  = 1;
        inspector->show_camera_cfg = 1;
        inspector->show_gizmo_cfg  = 1;
        inspector->show_light_cfg  = 1;
        inspector->show_node_cfg   = 1;

        inspector->scene_path_to_save_buffer_size = Max(scene->save_path.size*2, 1000);
        inspector->scene_path_to_save.str = push_array(inspector_view->arena, U8, inspector->scene_path_to_save_buffer_size);
        inspector->scene_path_to_save.size = scene->save_path.size;
        MemoryCopy(inspector->scene_path_to_save.str, scene->save_path.str, scene->save_path.size);
        inspector->last_active_row = -1;

        inspector->txt_cursor           = (TxtPt){0};
        inspector->txt_mark             = (TxtPt){0};
        inspector->txt_has_draft        = 0;
        inspector->txt_edit_buffer_size = 100;
        inspector->txt_edit_buffer      = push_array(inspector_view->arena, U8, inspector->txt_edit_buffer_size);
        inspector->txt_edit_string_size = 0;

        inspector->rect = rk_state->window_rect;
        {
            F32 default_width  = 800;
            F32 default_margin = 30;
            inspector->rect.x1 = 800;
            inspector->rect = pad_2f32(inspector->rect, -default_margin);
        }
    }

    // Animation rate
    F32 fast_rate = 1 - pow_f32(2, (-40.f * rk_state->dt_sec));

    // Build top-level panel container
    UI_Box *pane;
    UI_Transparency(0.1)
    {
        pane = rk_ui_pane_begin(&inspector->rect, &inspector->show, str8_lit("INSPECTOR"));
    }

    ui_spacer(ui_em(0.215, 0.f));

    {
        // Scene
        ui_set_next_pref_size(Axis2_Y, ui_children_sum(1.0));
        ui_set_next_child_layout_axis(Axis2_Y);
        UI_Box *scene_tree_box = ui_build_box_from_key(0, ui_key_zero());
        UI_Parent(scene_tree_box)
        {
            // Header
            {
                ui_set_next_child_layout_axis(Axis2_X);
                ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
                UI_Box *header_box = ui_build_box_from_key(0, ui_key_zero());
                UI_Parent(header_box)
                {
                    ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
                    if(ui_clicked(ui_expanderf(inspector->show_scene_cfg, "###show_scene_cfg")))
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
            UI_Box *scene_cfg = ui_build_box_from_key(0, ui_key_zero());
            UI_Parent(scene_cfg)
            {
                UI_Flags(UI_BoxFlag_ClickToFocus)
                {
                    if(ui_committed(ui_line_edit(&inspector->txt_cursor, &inspector->txt_mark, inspector->scene_path_to_save.str, inspector->scene_path_to_save_buffer_size, &inspector->txt_edit_string_size, inspector->scene_path_to_save, str8_lit("###scene_path"))))
                    {
                        inspector->scene_path_to_save.size = inspector->txt_edit_string_size;                        
                    }
                }

                UI_Row
                {
                    rk_ui_dropdown_begin(str8_lit("Load Template"));
                    for(U64 i = 0; i < rk_state->template_count; i++)
                    {
                        RK_SceneTemplate t = rk_state->templates[i];
                        if(ui_clicked(ui_button(t.name)))
                        {
                            RK_Scene *new_scene = t.fn();
                            SLLStackPush(rk_state->first_to_free_scene, scene);
                            rk_state->active_scene = new_scene;

                            U64 name_size = ClampTop(inspector->scene_path_to_save_buffer_size, new_scene->name.size);
                            inspector->scene_path_to_save.size = name_size;
                            MemoryCopy(inspector->scene_path_to_save.str, new_scene->name.str, name_size);
                            rk_ui_dropdown_hide();
                        }
                    }
                    rk_ui_dropdown_end();

                    if(ui_clicked(ui_buttonf("Save")))
                    {
                        // TODO
                    }
                    if(ui_clicked(ui_buttonf("Reload")))
                    {
                        // TODO
                    }
                }
            }

            // Scene tree
            {
                F32 row_height = ui_top_font_size()*1.3;
                F32 list_height = row_height*30.f;
                ui_set_next_pref_size(Axis2_Y, ui_px(list_height, 0.0));
                ui_set_next_child_layout_axis(Axis2_Y);
                if(!inspector->show_scene_cfg)
                {
                    ui_set_next_flags(UI_BoxFlag_Disabled);
                }
                UI_Box *container_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder, "##container");
                container_box->pref_size[Axis2_Y].value = mix_1f32(list_height, 0, container_box->disabled_t);

                // scroll params
                UI_ScrollListParams scroll_list_params = {0};
                {
                    Vec2F32 rect_dim = dim_2f32(container_box->rect);
                    scroll_list_params.flags = UI_ScrollListFlag_All;
                    scroll_list_params.row_height_px = row_height;
                    scroll_list_params.dim_px = rect_dim;
                    scroll_list_params.item_range = r1s64(0, scene->node_bucket->node_count);
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
                    RK_Node *root = rk_node_from_handle(scene->root);
                    U64 row_idx = 0;
                    S64 active_row = -1;
                    U64 level = 0;
                    U64 indent_size = 2;
                    Temp scratch = scratch_begin(0,0);
                    while(root != 0)
                    {
                        RK_NodeRec rec = rk_node_df_pre(root, 0, 0);

                        B32 is_active = root == active_node;

                        if(row_idx >= visible_row_rng.min && row_idx <= visible_row_rng.max)
                        {
                            String8 indent = str8(0, level*indent_size);
                            indent.str = push_array(scratch.arena, U8, indent.size);
                            MemorySet(indent.str, ' ', indent.size);

                            String8 string = push_str8f(scratch.arena, "%S%S###%d", indent, root->name, row_idx);
                            if(is_active)
                            {
                                ui_set_next_palette(ui_build_palette(ui_top_palette(), .overlay = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)));
                                ui_set_next_flags(UI_BoxFlag_DrawOverlay);
                            }
                            UI_Signal label = ui_button(string);

                            if(ui_clicked(label) && !is_active)
                            {
                                rk_scene_active_node_set(scene, root->key, 0);
                                inspector->last_active_row = row_idx;
                                active_row = row_idx;
                            }
                        }

                        if(active_row == -1 && is_active)
                        {
                            active_row = row_idx;
                        }

                        level += (rec.push_count-rec.pop_count);
                        root = rec.next;
                        row_idx++;
                    }
                    scratch_end(scratch);

                    if(active_row != inspector->last_active_row && active_row >= 0)
                    {
                        inspector->last_active_row = active_row;
                        ui_scroll_pt_target_idx(&scroller, active_row);
                    }
                }
            }
        }

        ui_spacer(ui_em(0.215, 0.f));

        // Camera
        {
            UI_Box *camera_cfg_box;
            UI_PrefSize(Axis2_Y, ui_children_sum(1.0)) UI_ChildLayoutAxis(Axis2_Y)
            {
                camera_cfg_box = ui_build_box_from_key(0, ui_key_zero());
            }
            UI_Parent(camera_cfg_box)
            {
                ui_set_next_child_layout_axis(Axis2_X);
                ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
                UI_Box *header_box = ui_build_box_from_key(0, ui_key_zero());
                UI_Parent(header_box)
                {
                    ui_set_next_pref_size(Axis2_X, ui_em(2.f, 0.f));
                    if(ui_clicked(ui_expanderf(inspector->show_camera_cfg, "###show_camera_cfg")))
                    {
                        inspector->show_camera_cfg = !inspector->show_camera_cfg;
                    }
                    ui_labelf("Camera");
                }

                ui_spacer(ui_em(0.2f, 0.f));

                UI_Row
                {
                    ui_labelf("show_grid");
                    ui_spacer(ui_pct(1.f,0.f));
                    rk_ui_checkbox(&camera->show_grid);
                }

                ui_spacer(ui_em(0.2f, 0.f));

                UI_Row
                {
                    ui_labelf("shading");
                    ui_spacer(ui_pct(1.0, 0.0));
                    // TODO(k): radio button
                    if(ui_clicked(ui_buttonf("wireframe"))) {camera->viewport_shading = RK_ViewportShadingKind_Wireframe;};
                    if(ui_clicked(ui_buttonf("solid")))     {camera->viewport_shading = RK_ViewportShadingKind_Solid;};
                    if(ui_clicked(ui_buttonf("material")))  {camera->viewport_shading = RK_ViewportShadingKind_Material;};
                    ui_spacer(ui_em(0.5, 1.0));
                }
            }
        }

        ui_spacer(ui_em(0.2f, 0.f));

        // Gizmo
        {
            UI_Box *gizmo_cfg_box;
            UI_PrefSize(Axis2_Y, ui_children_sum(1.0)) UI_ChildLayoutAxis(Axis2_Y)
            {
                gizmo_cfg_box = ui_build_box_from_key(0, ui_key_zero());
            }
            UI_Parent(gizmo_cfg_box)
            {
                ui_set_next_child_layout_axis(Axis2_X);
                ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
                UI_Box *header_box = ui_build_box_from_key(0, ui_key_zero());
                UI_Parent(header_box)
                {
                    ui_set_next_pref_size(Axis2_X, ui_em(2.f, 0.f));
                    if(ui_clicked(ui_expanderf(inspector->show_camera_cfg, "###show_gizmo_cfg")))
                    {
                        inspector->show_gizmo_cfg = !inspector->show_gizmo_cfg;
                    }
                    ui_labelf("Gizmo");
                }

                ui_spacer(ui_em(0.2f, 0.f));

                UI_Row
                {
                    ui_labelf("hide_gizmo");
                    ui_spacer(ui_pct(1.f,0.f));
                    rk_ui_checkbox(&scene->gizmo3d_disabled);
                }

                ui_spacer(ui_em(0.2f, 0.f));

                UI_Row
                {
                    ui_labelf("mode");
                    ui_spacer(ui_pct(1.0, 0.0));
                    // TODO(k): radio button
                    if(ui_clicked(ui_buttonf("translate"))) {scene->gizmo3d_mode = RK_Gizmo3dMode_Translate;};
                    if(ui_clicked(ui_buttonf("rotation")))  {scene->gizmo3d_mode = RK_Gizmo3dMode_Rotation;};
                    if(ui_clicked(ui_buttonf("scale")))     {scene->gizmo3d_mode = RK_Gizmo3dMode_Scale;};
                    ui_spacer(ui_em(0.5, 1.0));
                }
            }

        }

        ui_spacer(ui_em(0.2f, 0.f));

        // Active Node
        {
            UI_Box *node_cfg_box;
            UI_PrefSize(Axis2_Y, ui_children_sum(1.0)) UI_ChildLayoutAxis(Axis2_Y)
            {
                node_cfg_box = ui_build_box_from_key(0, ui_key_zero());
            }
            if(active_node != 0) UI_Parent(node_cfg_box)
            {
                ui_set_next_child_layout_axis(Axis2_X);
                ui_set_next_flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder);
                UI_Box *header_box = ui_build_box_from_key(0, ui_key_zero());
                UI_Parent(header_box)
                {
                    ui_set_next_pref_size(Axis2_X, ui_em(2.f, 0.f));
                    if(ui_clicked(ui_expanderf(inspector->show_node_cfg, "###show_node_cfg")))
                    {
                        inspector->show_node_cfg = !inspector->show_node_cfg;
                    }
                    ui_labelf("Node");
                }

                ui_spacer(ui_em(0.2f, 0.f));

                // basic info
                {
                    ui_set_next_child_layout_axis(Axis2_X);
                    UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                    UI_Parent(header_box)
                    {
                        ui_spacer(ui_em(0.3f,0.f));
                        ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
                        ui_labelf("basic");
                    }
                    
                    UI_Row
                    {
                        ui_labelf("name");
                        ui_spacer(ui_pct(1.f,0.f));
                        ui_label(active_node->name);
                    }

                    UI_Row
                    {
                        ui_labelf("key");
                        ui_spacer(ui_pct(1.f,0.f));
                        ui_labelf("%lu", active_node->key);
                    }

                    if(active_node->parent) UI_Row
                    {
                        ui_labelf("parent");
                        ui_spacer(ui_pct(1.f,0.f));
                        ui_label(active_node->parent->name);
                    }

                    UI_Row
                    {
                        ui_labelf("children_count");
                        ui_spacer(ui_pct(1.f,0.f));
                        ui_labelf("%lu", active_node->children_count);
                    }
                }

                ui_spacer(ui_em(0.9, 0.f));

                ////////////////////////////////
                //~ equipment info

                ////////////////////////////////
                //- node2d

                if(active_node->type_flags & RK_NodeTypeFlag_Node2D)
                {
                    NotImplemented;
                }

                ////////////////////////////////
                //- node3d

                if(active_node->type_flags & RK_NodeTypeFlag_Node3D)
                {
                    RK_Transform3D *transform3d = &active_node->node3d->transform;

                    ui_set_next_child_layout_axis(Axis2_X);
                    UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
                    UI_Parent(header_box)
                    {
                        ui_spacer(ui_em(0.3f,0.f));
                        ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
                        ui_labelf("node3d");
                    }

                    UI_Row
                    {
                        ui_labelf("position");
                        ui_spacer(ui_pct(1.0, 0.0));
                        ui_f32_edit(&transform3d->position.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###pos_x"));
                        ui_f32_edit(&transform3d->position.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###pos_y"));
                        ui_f32_edit(&transform3d->position.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###pos_z"));
                    }

                    UI_Row
                    {
                        ui_labelf("scale");
                        ui_spacer(ui_pct(1.0, 0.0));
                        ui_f32_edit(&transform3d->scale.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###scale_x"));
                        ui_f32_edit(&transform3d->scale.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###scale_y"));
                        ui_f32_edit(&transform3d->scale.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###scale_z"));
                    }

                    UI_Row
                    {
                        ui_labelf("rotation");
                        ui_spacer(ui_pct(1.0, 0.0));
                        ui_f32_edit(&transform3d->rotation.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###rot_x"));
                        ui_f32_edit(&transform3d->rotation.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###rot_y"));
                        ui_f32_edit(&transform3d->rotation.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###rot_z"));
                        ui_f32_edit(&transform3d->rotation.w, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("W###rot_w"));
                    }
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
        Rng2F32 rect;
        UI_ScrollPt table_scroller;
    };

    RK_View *view = rk_view_from_kind(RK_ViewKind_Profiler);
    RK_Profiler_State *profiler = view->custom_data;
    if(profiler == 0)
    {
        profiler = push_array(view->arena, RK_Profiler_State, 1);
        view->custom_data = profiler;

        profiler->show = 1;
        profiler->rect = rk_state->window_rect;
        {
            F32 default_width = rk_state->window_dim.x * 0.6f;
            F32 default_height = rk_state->window_dim.x * 0.15f;
            F32 default_margin = ui_top_font_size()*1.3;
            profiler->rect.x0 = profiler->rect.x1 - default_width;
            profiler->rect.y0 = profiler->rect.y1 - default_height;
            profiler->rect = pad_2f32(profiler->rect, -default_margin);
        }
    }

    // Top-level pane 
    UI_Box *container_box;
    UI_Transparency(0.1)
    {
        container_box = rk_ui_pane_begin(&profiler->rect, &profiler->show, str8_lit("PROFILER"));
    }

    ui_spacer(ui_px(ui_top_font_size()*0.215, 0.f));

    F32 row_height = ui_top_font_size()*1.3f;
    ui_push_pref_height(ui_px(row_height, 0));

    // tag + total_cycles + call_count + cycles_per_call + total_us + us_per_call
    U64 col_count = 6;

    // Table header
    ui_set_next_child_layout_axis(Axis2_X);
    // NOTE(k): width - scrollbar width
    ui_set_next_pref_width(ui_px(container_box->fixed_size.x-ui_top_font_size()*0.9f, 0.f));
    UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
    UI_Parent(header_box) UI_PrefWidth(ui_pct(1.f,0.f)) UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow) UI_TextAlignment(UI_TextAlign_Center)
    {
        ui_labelf("Tag");
        ui_labelf("Cycles n/p");
        ui_labelf("Calls");
        ui_labelf("Cycles per call");
        ui_labelf("US n/p");
        ui_labelf("US per call");
    }

    Rng2F32 content_rect = container_box->rect;
    content_rect.y0 = header_box->rect.y1;

    // Content
    ProfTickInfo *pf_tick = ProfTickPst();
    U64 prof_node_count = 0;
    if(pf_tick != 0 && pf_tick->node_hash_table != 0)
    {
        for(U64 slot_idx = 0; slot_idx < pf_table_size; slot_idx++)
        {
            prof_node_count += pf_tick->node_hash_table[slot_idx].count;
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
        if(pf_tick != 0 && pf_tick->node_hash_table != 0)
        {
            U64 row_idx = 0;
            for(U64 slot_idx = 0; slot_idx < pf_table_size; slot_idx++)
            {
                for(ProfNode *n = pf_tick->node_hash_table[slot_idx].first; n != 0; n = n->hash_next)
                {
                    if(row_idx >= visible_row_rng.min && row_idx <= visible_row_rng.max)
                    {
                        UI_Box *row = 0;
                        UI_ChildLayoutAxis(Axis2_X) UI_Flags(UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_MouseClickable|UI_BoxFlag_DrawBackground) UI_PrefWidth(ui_pct(1.f,0.f))
                        {
                            row = ui_build_box_from_stringf(0,"###row_%d", row_idx);
                            ui_signal_from_box(row);
                        }
                        UI_Parent(row) UI_PrefWidth(ui_pct(1.0,0.f)) UI_Flags(UI_BoxFlag_DrawSideRight)
                        {
                            ui_label(n->tag);
                            UI_Row
                            {
                                ui_labelf("%lu", n->total_cycles);
                                ui_spacer(ui_pct(1.f,0.f));
                                ui_labelf("%.2f", (F32)n->total_cycles/pf_tick->cycles);
                            }
                            ui_labelf("%lu", n->call_count);
                            ui_labelf("%lu", n->cycles_per_call);
                            UI_Row
                            {
                                ui_labelf("%lu", n->total_us);
                                ui_spacer(ui_pct(1.f,0.f));
                                ui_labelf("%.2f", (F32)n->total_us/pf_tick->us);
                            }
                            ui_labelf("%lu", n->us_per_call);
                        }
                    }
                    row_idx++;
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

internal D_Bucket *
rk_frame(RK_Scene *scene, OS_EventList os_events, U64 dt_us, U64 hot_key)
{
    ProfBeginFunction();

    /////////////////////////////////
    // Begin of the frame
    {
        rk_push_node_bucket(scene->node_bucket);
        rk_push_res_bucket(scene->res_bucket);
        rk_push_scene(scene);

        rk_state->dt_us = dt_us;
        rk_state->dt_sec = dt_us/1000000.0f;
        rk_state->dt_ms = dt_us/1000.0f;
        rk_state->window_rect = os_client_rect_from_window(rk_state->os_wnd);
        rk_state->window_dim = dim_2f32(rk_state->window_rect);
        rk_state->last_cursor = rk_state->cursor;
        rk_state->cursor = os_mouse_from_window(rk_state->os_wnd);
        rk_state->last_dpi = rk_state->dpi;
        rk_state->dpi = os_dpi_from_window(rk_state->os_wnd);

        // clear frame gizmo drawlist
        rk_drawlist_reset(rk_frame_gizmo_drawlist());
    }

    /////////////////////////////////
    // Unpack active camera

    RK_Node *camera_node = rk_node_from_handle(scene->active_camera);
    AssertAlways(camera_node != 0 && "No active camera was found");
    RK_Camera3D *camera = camera_node->camera3d;
    Mat4x4F32 camera_xform = camera_node->fixed_xform;

    //- Unpack camera settings
    B32 show_grid   = camera->show_grid;
    B32 projection  = camera->projection;

    // polygon mode
    R_GeoPolygonKind polygon_mode;
    switch(camera->viewport_shading)
    {
        case RK_ViewportShadingKind_Wireframe:       {polygon_mode = R_GeoPolygonKind_Line;}break;
        case RK_ViewportShadingKind_Solid:           {polygon_mode = R_GeoPolygonKind_Fill;}break;
        case RK_ViewportShadingKind_Material:        {polygon_mode = R_GeoPolygonKind_Fill;}break;
        default:                                     {InvalidPath;}break;
    }

    //- view/projection (NOTE(k): behind by one frame)
    Mat4x4F32 view_m = inverse_4x4f32(camera_xform);
    Mat4x4F32 projection_m;
    switch(camera->projection)
    {
        case RK_ProjectionKind_Perspective:
        {
            projection_m = make_perspective_vulkan_4x4f32(camera->perspective.fov, rk_state->window_dim.x/rk_state->window_dim.y, camera->zn, camera->zf);
        }break;
        case RK_ProjectionKind_Orthographic:
        {
            projection_m = make_orthographic_vulkan_4x4f32(camera->orthographic.left, camera->orthographic.right, camera->orthographic.bottom, camera->orthographic.top, camera->zn, camera->zf);
        }break;
        default:{InvalidPath;}break;
    }

    /////////////////////////////////
    // Remake drawing buckets every frame

    rk_state->bucket_rect = d_bucket_make();
    for(U64 kind = 0; kind < RK_GeoBucketKind_COUNT; kind++)
    {
        D_Bucket **bucket = &rk_state->bucket_geo3d[kind];
        *bucket = d_bucket_make();
        d_push_bucket(*bucket);

        Rng2F32 viewport;
        Mat4x4F32 view;
        Mat4x4F32 projection;
        B32 grid;
        Vec3F32 global_light;

        switch(kind)
        {
            case RK_GeoBucketKind_Back:
            {
                grid = show_grid;
                viewport = rk_state->window_rect;
                view = view_m;
                projection = projection_m;
                global_light = v3f32(0,1,0);
            }break;
            case RK_GeoBucketKind_Front:
            {
                grid = 0;
                viewport = rk_state->window_rect;
                view = view_m;
                projection = projection_m;
                global_light = v3f32(1,0,0);
            }break;
            case RK_GeoBucketKind_Screen:
            {
                grid = 0;
                viewport = rk_state->window_rect;
                view = mat_4x4f32(1.f);
                projection = make_orthographic_vulkan_4x4f32(viewport.x0, viewport.x1, viewport.y1, viewport.y0, 0.1, 100.f);
            }break;
            default:{InvalidPath;}break;
        }
        d_geo3d_begin(viewport, view, projection, global_light, grid);
        d_pop_bucket();
    }

    /////////////////////////////////
    //~ Build ui

    //- Build event list for ui
    UI_EventList ui_events = {0};
    OS_Event *os_evt_first = os_events.first;
    OS_Event *os_evt_opl = os_events.last + 1;
    for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    {
        if(os_evt == 0) continue;

        UI_Event ui_evt = zero_struct;

        UI_EventKind kind = UI_EventKind_Null;
        switch(os_evt->kind)
        {
            default:{}break;
            case OS_EventKind_Press:       {kind = UI_EventKind_Press;}break;
            case OS_EventKind_Release:     {kind = UI_EventKind_Release;}break;
            case OS_EventKind_MouseMove:   {kind = UI_EventKind_MouseMove;}break;
            case OS_EventKind_Text:        {kind = UI_EventKind_Text;}break;
            case OS_EventKind_Scroll:      {kind = UI_EventKind_Scroll;}break;
            case OS_EventKind_FileDrop:    {kind = UI_EventKind_FileDrop;}break;
        }

        ui_evt.kind         = kind;
        ui_evt.key          = os_evt->key;
        ui_evt.modifiers    = os_evt->modifiers;
        ui_evt.string       = os_evt->character ? str8_from_32(ui_build_arena(), str32(&os_evt->character, 1)) : str8_zero();
        ui_evt.pos          = os_evt->pos;
        ui_evt.delta_2f32   = os_evt->delta;
        ui_evt.timestamp_us = os_evt->timestamp_us;

        if(ui_evt.key == OS_Key_Backspace && ui_evt.kind == UI_EventKind_Press)
        {
            ui_evt.kind       = UI_EventKind_Edit;
            ui_evt.flags      = UI_EventFlag_Delete | UI_EventFlag_KeepMark;
            ui_evt.delta_unit = UI_EventDeltaUnit_Char;
            ui_evt.delta_2s32 = v2s32(-1,0);
        }

        if(ui_evt.kind == UI_EventKind_Text)
        {
            ui_evt.flags = UI_EventFlag_KeepMark;
        }

        if(ui_evt.key == OS_Key_Return && ui_evt.kind == UI_EventKind_Press)
        {
            ui_evt.slot = UI_EventActionSlot_Accept;
        }

        if(ui_evt.key == OS_Key_A && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
        {
            ui_evt.kind       = UI_EventKind_Navigate;
            ui_evt.flags      = UI_EventFlag_KeepMark;
            ui_evt.delta_unit = UI_EventDeltaUnit_Whole;
            ui_evt.delta_2s32 = v2s32(1,0);

            UI_Event ui_evt_0 = ui_evt;
            ui_evt_0.flags      = 0;
            ui_evt_0.delta_unit = UI_EventDeltaUnit_Whole;
            ui_evt_0.delta_2s32 = v2s32(-1,0);
            ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt_0);
        }

        if(ui_evt.key == OS_Key_Left && ui_evt.kind == UI_EventKind_Press)
        {
            ui_evt.kind       = UI_EventKind_Navigate;
            ui_evt.flags      = 0;
            ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
            ui_evt.delta_2s32 = v2s32(-1,0);
        }

        if(ui_evt.key == OS_Key_Right && ui_evt.kind == UI_EventKind_Press)
        {
            ui_evt.kind       = UI_EventKind_Navigate;
            ui_evt.flags      = 0;
            ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
            ui_evt.delta_2s32 = v2s32(1,0);
        }

        ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt);

        // TODO(k): we may want to handle this somewhere else
        if(os_evt->kind == OS_EventKind_WindowClose) {rk_state->window_should_close = 1;}
    }

    // Begin build ui
    {
        // Gather font info
        F_Tag main_font = rk_font_from_slot(RK_FontSlot_Main);
        F32 main_font_size = rk_font_size_from_slot(RK_FontSlot_Main);
        F_Tag icon_font = rk_font_from_slot(RK_FontSlot_Icons);

        // Build icon info
        UI_IconInfo icon_info = {0};
        {
            icon_info.icon_font = icon_font;
            icon_info.icon_kind_text_map[UI_IconKind_RightArrow]     = rk_icon_kind_text_table[RK_IconKind_RightScroll];
            icon_info.icon_kind_text_map[UI_IconKind_DownArrow]      = rk_icon_kind_text_table[RK_IconKind_DownScroll];
            icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]      = rk_icon_kind_text_table[RK_IconKind_LeftScroll];
            icon_info.icon_kind_text_map[UI_IconKind_UpArrow]        = rk_icon_kind_text_table[RK_IconKind_UpScroll];
            icon_info.icon_kind_text_map[UI_IconKind_RightCaret]     = rk_icon_kind_text_table[RK_IconKind_RightCaret];
            icon_info.icon_kind_text_map[UI_IconKind_DownCaret]      = rk_icon_kind_text_table[RK_IconKind_DownCaret];
            icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]      = rk_icon_kind_text_table[RK_IconKind_LeftCaret];
            icon_info.icon_kind_text_map[UI_IconKind_UpCaret]        = rk_icon_kind_text_table[RK_IconKind_UpCaret];
            icon_info.icon_kind_text_map[UI_IconKind_CheckHollow]    = rk_icon_kind_text_table[RK_IconKind_CheckHollow];
            icon_info.icon_kind_text_map[UI_IconKind_CheckFilled]    = rk_icon_kind_text_table[RK_IconKind_CheckFilled];
        }

        UI_WidgetPaletteInfo widget_palette_info = {0};
        {
            widget_palette_info.tooltip_palette   = rk_palette_from_code(RK_PaletteCode_Floating);
            widget_palette_info.ctx_menu_palette  = rk_palette_from_code(RK_PaletteCode_Floating);
            widget_palette_info.scrollbar_palette = rk_palette_from_code(RK_PaletteCode_ScrollBarButton);
        }

        // Build animation info
        UI_AnimationInfo animation_info = {0};
        {
            animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;
            animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;
            animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;
            animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;
            animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;
            animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;
        }

        // Begin & push initial stack values
        ui_begin_build(rk_state->os_wnd, &ui_events, &icon_info, &widget_palette_info, &animation_info, rk_state->dt_sec);

        ui_push_font(main_font);
        ui_push_font_size(main_font_size);
        // ui_push_text_raster_flags(...);
        ui_push_text_padding(main_font_size*0.2f);
        ui_push_pref_width(ui_em(20.f, 1.f));
        ui_push_pref_height(ui_em(1.35f, 1.f));
        ui_push_palette(rk_palette_from_code(RK_PaletteCode_Base));
    }

    // Game view (debug/game ui)
    {
        rk_ui_inspector();
        rk_ui_stats();
        rk_ui_profiler();
    }

    // Process events for game logic
    // OS_Event *os_evt_first = os_events.first;
    // OS_Event *os_evt_opl = os_events.last + 1;
    // for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    // {
    //     if(os_evt == 0) continue;
    //     // if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_Space) {}
    //     if(os_evt->kind == OS_EventKind_Press)
    //     {
    //         U64 camera_idx = os_evt->key - OS_Key_F1;
    //         U64 i = 0;
    //         for(RK_CameraNode *cn = scene->first_camera; cn != 0; cn = cn->next, i++)
    //         {
    //             if(i == camera_idx)
    //             {
    //                 scene->active_camera = cn;
    //                 break;
    //             }
    //         }
    //     }
    // }

    // UI Box for game viewport (handle user interaction)
    ui_set_next_rect(rk_state->window_rect);
    UI_Box *overlay = ui_build_box_from_string(UI_BoxFlag_MouseClickable|UI_BoxFlag_ClickToFocus|UI_BoxFlag_Scroll|UI_BoxFlag_DisableFocusOverlay, str8_lit("###game_overlay"));

    RK_Node *active_node = rk_node_from_handle(scene->active_node);

    /////////////////////////////////
    //~ Update/draw node in the scene tree
    ProfScope("update/draw game")
    {
        // unpack geo back pass params
        R_Pass *geo_back_pass = 0;
        D_Bucket *geo_back_bucket = rk_state->bucket_geo3d[RK_GeoBucketKind_Back];
        geo_back_pass = r_pass_from_kind(d_thread_ctx->arena, &geo_back_bucket->passes, R_PassKind_Geo3D, 1);
        R_PassParams_Geo3D *geo_params = geo_back_pass->params_geo3d;

        RK_Node *node = rk_node_from_handle(scene->root);
        while(node != 0)
        {
            B32 force_leaf = !!(node->flags & RK_NodeFlag_Hidden);
            RK_NodeRec rec = rk_node_df_pre(node, 0, force_leaf);
            RK_Node *parent = node->parent;

            for(RK_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
            {
                fn->f(node, scene, os_events, rk_state->dt_sec, geo_params->projection, geo_params->view);
            }

            D_BucketScope(geo_back_bucket)
            {
                B32 has_transform = 0;
                // Update fixed_xform in DFS order
                if(node->type_flags & RK_NodeTypeFlag_Node2D)
                {
                    has_transform = 1;
                    NotImplemented;
                }

                // Update fixed_xform in DFS order
                if(node->type_flags & RK_NodeTypeFlag_Node3D)
                {
                    has_transform = 1;
                    node->position = node->node3d->transform.position;
                    node->scale = node->node3d->transform.scale;
                    node->rotation = node->node3d->transform.rotation;
                }

                // Update fixed_xform in DFS order 
                if(has_transform)
                {
                    Mat4x4F32 xform = mat_4x4f32(1.0);
                    Mat4x4F32 rotation_m = mat_4x4f32_from_quat_f32(node->rotation);
                    Mat4x4F32 translate_m = make_translate_4x4f32(node->position);
                    Mat4x4F32 scale_m = make_scale_4x4f32(node->scale);
                    // TRS order
                    xform = mul_4x4f32(scale_m, xform);
                    xform = mul_4x4f32(rotation_m, xform);
                    xform = mul_4x4f32(translate_m, xform);

                    if(parent != 0 && !(node->flags & RK_NodeFlag_Float))
                    {
                        xform = mul_4x4f32(parent->fixed_xform, xform);
                    }
                    node->fixed_xform = xform;
                }

                if(node->type_flags & RK_NodeTypeFlag_Camera3D)
                {
                    if(node->camera3d->is_active) 
                    {
                        rk_scene_active_camera_set(scene, node);
                    }
                }

                // Animation Player progress
                if(node->type_flags & RK_NodeTypeFlag_AnimationPlayer)
                {
                    RK_AnimationPlayer *anim_player = node->animation_player;
                    // TODO(k): TESTING, play the first animation
                    if(rk_handle_is_zero(anim_player->playback.curr))
                    {
                        anim_player->playback.curr = anim_player->animations[0]; 
                        anim_player->playback.loop = 1;
                    }
                
                    // play animation if there is any
                    if(!rk_handle_is_zero(anim_player->playback.curr))
                    {
                        anim_player->playback.pos += rk_state->dt_sec;
                        RK_Animation *animation = rk_res_data_from_handle(anim_player->playback.curr);
                        if(anim_player->playback.pos > animation->duration_sec && anim_player->playback.loop)
                        {
                            anim_player->playback.pos = mod_f32(anim_player->playback.pos, animation->duration_sec);
                        }
                        if(animation)
                        {
                            F32 pos = anim_player->playback.pos;
                            for(RK_Track *track = animation->first_track; track != 0; track = track->next)
                            {
                                RK_Node *target = rk_node_from_key(rk_key_merge(anim_player->target_seed, track->target_key));
                                AssertAlways(target != 0);

                                F32 t = 0;
                                RK_TrackFrame *prev_frame = 0;
                                BTSetFindPrev_PLRKZ(track->frame_btree_root,pos,prev_frame,parent,left,right,ts_sec,0);
                                RK_TrackFrame *next_frame = 0;
                                BTNext_PLRZ(prev_frame, next_frame, parent, left, right, 0);
                                // NOTE(k): single frame track or track is finished
                                if(next_frame == 0)
                                {
                                    next_frame = prev_frame;
                                    t = 0;
                                }
                                else
                                {
                                    Assert(next_frame->ts_sec > prev_frame->ts_sec);
                                    Assert(pos > prev_frame->ts_sec);
                                    Assert(pos <= next_frame->ts_sec);
                                    switch(track->interpolation)
                                    {
                                        case RK_InterpolationKind_Linear:
                                        {
                                            t = (pos-prev_frame->ts_sec)/(next_frame->ts_sec-prev_frame->ts_sec);
                                        }break;
                                        case RK_InterpolationKind_Cubic:
                                        case RK_InterpolationKind_Step:
                                        {
                                            NotImplemented;
                                        }break;
                                        default: {InvalidPath;}break;
                                    }
                                }

                                switch(track->target_kind)
                                {
                                    case RK_TrackTargetKind_Position3D:
                                    {
                                        target->node3d->transform.position = mix_3f32(prev_frame->v.position3d, next_frame->v.position3d, t);
                                    }break;
                                    case RK_TrackTargetKind_Rotation3D:
                                    {
                                        target->node3d->transform.rotation = mix_quat_f32(prev_frame->v.rotation3d, next_frame->v.rotation3d, t);
                                    }break;
                                    case RK_TrackTargetKind_Scale3D:
                                    {
                                        target->node3d->transform.scale = mix_3f32(prev_frame->v.scale3d, next_frame->v.scale3d, t);
                                    }break;
                                    case RK_TrackTargetKind_MorphWeight3D:
                                    default: {}break;
                                }
                            }
                        }
                    }
                }

                // Draw mesh(3d/2d)
                if(node->type_flags & RK_NodeTypeFlag_MeshInstance3D && !(node->flags & RK_NodeFlag_Hidden))
                {
                    RK_MeshInstance3D *mesh_inst3d = node->mesh_inst3d;
                    RK_Mesh *mesh = rk_res_data_from_handle(mesh_inst3d->mesh);
                    RK_Skin *skin = rk_res_data_from_handle(mesh_inst3d->skin);
                    RK_Key skin_seed = mesh_inst3d->skin_seed;
                    U64 joint_count = 0;
                    Mat4x4F32 *joint_xforms = 0;

                    if(skin != 0)
                    {
                        joint_count = skin->bind_count;
                        joint_xforms = push_array(rk_frame_arena(), Mat4x4F32, joint_count);

                        RK_Bind *bind;
                        for(U64 i = 0; i < joint_count; i++)
                        {
                            bind = &skin->binds[i];
                            RK_Key target_key = rk_key_merge(skin_seed, bind->joint);
                            RK_Node *target = rk_node_from_key(target_key);
                            AssertAlways(target != 0);
                            joint_xforms[i] = mul_4x4f32(target->fixed_xform, bind->inverse_bind_matrix);
                        }
                    }
                    
                    R_Handle albedo_tex = {0};
                    // NOTE(k): alpha != 0 => omit texture and use color texture
                    Vec4F32 clr_tex = {1,1,1,0};

                    RK_Material *material = rk_res_data_from_handle(mesh_inst3d->material_override);
                    if(material == 0)
                    {
                        material = rk_res_data_from_handle(mesh->material);
                    }

                    // TODO(k): PBR rendering
                    if(material != 0)
                    {
                        RK_Texture2D *tex2d = rk_res_data_from_handle(material->base_clr_tex);
                        if(tex2d) albedo_tex = tex2d->tex;
                        if(camera->viewport_shading != RK_ViewportShadingKind_Material)
                        {
                            clr_tex = material->base_clr_factor;
                        }
                    }
                    else
                    {
                        clr_tex = rgba_from_u32(0xF4E0AFFF);
                    }

                    if(mesh != 0)
                    {
                        B32 draw_edge = rk_node_is_active(node);
                        R_Mesh3DInst *inst = d_mesh(mesh->vertices, mesh->indices, 0,0, mesh->indice_count,
                                                    mesh->topology, polygon_mode, R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB, albedo_tex, clr_tex, 1.f,
                                                    joint_xforms, joint_count,
                                                    node->fixed_xform, node->key.u64[0], draw_edge, !mesh_inst3d->disable_depth, 0, 0);
                        inst->xform_inv = inverse_4x4f32(node->fixed_xform);
                    }
                    else
                    {
                        // NOTE(k): THIS SHOULD NOT HAPPEDN
                        InvalidPath;
                    }
                }
            }

            // pop stacks
            {
                // TODO(k): we could have some stacks here
            }
            node = rec.next;
        }
    }

    // NOTE(k): there could be ui elements within node update
    rk_state->sig = ui_signal_from_box(overlay);

    /////////////////////////////////
    // Update hot/active key
    {
        scene->hot_key = rk_key_make(hot_key);
        if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
        {
            scene->active_key = scene->hot_key;
            rk_scene_active_node_set(scene, scene->active_key, 1);
        }
        if(rk_state->sig.f & UI_SignalFlag_LeftReleased)
        {
            scene->active_key = rk_key_zero();
        }
    }

    /////////////////////////////////
    // handle cursors
    if(camera->hide_cursor && (!rk_state->cursor_hidden))
    {
        os_hide_cursor(rk_state->os_wnd);
        rk_state->cursor_hidden = 1;
    }
    if(!camera->hide_cursor && rk_state->cursor_hidden)
    {
        os_show_cursor(rk_state->os_wnd);
        rk_state->cursor_hidden = 0;
    }
    if(camera->lock_cursor)
    {
        Vec2F32 cursor = center_2f32(rk_state->window_rect);
        os_wrap_cursor(rk_state->os_wnd, cursor.x, cursor.y);
        rk_state->cursor = cursor;
    }

    // Draw gizmo3d
    if(active_node && !scene->gizmo3d_disabled)
    {
        D_BucketScope(rk_state->bucket_geo3d[RK_GeoBucketKind_Front])
        {
            Mat4x4F32 gizmo_xform_trs = active_node->fixed_xform;
            Mat4x4F32 gizmo_xform_tr = rk_xform_from_trs(active_node->position, active_node->rotation, v3f32(1,1,1));
            Vec3F32 gizmo_origin = v3f32(gizmo_xform_trs.v[3][0], gizmo_xform_trs.v[3][1], gizmo_xform_trs.v[3][2]);

            // make keys
            RK_Key i_hat_key = rk_key_from_stringf(rk_key_zero(), "gizmo_i_hat");
            RK_Key j_hat_key = rk_key_from_stringf(rk_key_zero(), "gizmo_j_hat");
            RK_Key k_hat_key = rk_key_from_stringf(rk_key_zero(), "gizmo_k_hat");
            RK_Key origin_key = rk_key_from_stringf(rk_key_zero(), "gizmo_origin");

            Vec3F32 i_hat,j_hat,k_hat;
            rk_ijk_from_matrix(gizmo_xform_tr, &i_hat, &j_hat, &k_hat);

            Vec3F32 eye = camera_node->position;
            Vec3F32 ray_eye_to_mouse_end;
            Vec3F32 ray_eye_to_gizmo = normalize_3f32(sub_3f32(gizmo_origin, eye));
            Vec3F32 ray_eye_to_center;
            {
                Mat4x4F32 view_proj_inv = inverse_4x4f32(mul_4x4f32(projection_m, view_m));

                // ray_eye_to_mouse_end
                {
                    // mouse ndc pos
                    F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
                    F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;
                    // unproject
                    Vec4F32 ray_end = mat_4x4f32_transform_4f32(view_proj_inv, v4f32(mox_ndc, moy_ndc, 1.f, 1.f));
                    ray_end = scale_4f32(ray_end, 1.f/ray_end.w);
                    ray_eye_to_mouse_end = v4f32_xyz(ray_end);
                }

                // ray_eye_center
                {
                    // unproject
                    Vec4F32 ray_end = mat_4x4f32_transform_4f32(view_proj_inv, v4f32(0, 0, 1.f, 1.f));
                    ray_end = scale_4f32(ray_end, 1.f/ray_end.w);
                    ray_eye_to_center = normalize_3f32(v4f32_xyz(ray_end));
                }
            }

            // NOTE(k): scale factor based on the distance between eye to gizmo origin
            // TODO(k): not so sure about this, is this right?
            F32 linew_scale = 1.f * (rk_state->dpi/96.f);
            F32 scale_t;
            {
                F32 base_scale = 1.f;
                F32 min_dist = 4.5f;
                F32 projected_view_dist = dot_3f32(sub_3f32(gizmo_origin, eye), ray_eye_to_center);
                // F32 view_dist = length_3f32(sub_3f32(gizmo_origin, eye));
                scale_t = (projected_view_dist*base_scale) / min_dist;
            }

            Vec4F32 axis_clrs[3] = {rgba_from_u32(0x7D0A0AFF), rgba_from_u32(0x4E9F3DFF), rgba_from_u32(0x003161FF)};
            Vec3F32 axises[3] = {i_hat,j_hat,k_hat};
            RK_Key axis_keys[3] = {i_hat_key, j_hat_key, k_hat_key};
            switch(scene->gizmo3d_mode)
            {
                case RK_Gizmo3dMode_Translate:
                {
                    // cfg
                    F32 axis_length = 1.0 * scale_t;

                    for(U64 i = 0; i < 3; i++)
                    {
                        Vec3F32 axis = axises[i];
                        RK_Key axis_key = axis_keys[i];
                        Vec4F32 axis_clr = axis_clrs[i];

                        B32 draw_edge = 0;

                        B32 is_hot = rk_key_match(scene->hot_key, axis_key);
                        B32 is_active = rk_key_match(scene->active_key, axis_key);

                        if(is_hot)
                        {
                            axis_clr.w = 0.8;
                            draw_edge = 1;
                        }
                        if(is_active)
                        {
                            axis_clr.w = 0.6;
                            draw_edge = 1;
                        }

                        if(is_active && rk_state->sig.f & UI_SignalFlag_LeftDragging)
                        {
                            typedef struct RK_Gizmo3dTranslateDragData RK_Gizmo3dTranslateDragData;
                            struct RK_Gizmo3dTranslateDragData
                            {
                                Vec3F32 start_pos;
                                Vec3F32 start_axis;
                                F32 start_dist;
                            };
                            if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
                            {
                                RK_Gizmo3dTranslateDragData drag_data = {0};
                                drag_data.start_pos = active_node->position;
                                drag_data.start_axis = axis;
                                Vec3F32 intersect;
                                {
                                    Vec3F32 next_axis = axises[(i+1)%3];
                                    F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, next_axis, gizmo_origin);
                                    intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                                }
                                drag_data.start_dist = dot_3f32(sub_3f32(intersect, active_node->position), axis);
                                ui_store_drag_struct(&drag_data);
                            }
                            RK_Gizmo3dTranslateDragData *drag_data = ui_get_drag_struct(RK_Gizmo3dTranslateDragData);

                            // choose axis other than current axis(don't care which one)
                            Vec3F32 next_axis = axises[(i+1)%3];
                            Vec3F32 intersect;
                            {
                                F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, next_axis, gizmo_origin);
                                intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                            }
                            F32 dist = dot_3f32(sub_3f32(intersect, drag_data->start_pos), drag_data->start_axis);
                            dist -= drag_data->start_dist;

                            // move active_node along the axis
                            if(dist != 0)
                            {
                                active_node->node3d->transform.position = add_3f32(drag_data->start_pos, scale_3f32(drag_data->start_axis, dist));
                            }

                            // draw axis to indicate direction
                            rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), axis_clr, 3*linew_scale, 0);
                        }

                        // drag plane
                        {
                            Vec4F32 plane_clr = axis_clr;
                            plane_clr.w = 0.6;
                            RK_Key drag_plane_key = rk_key_from_stringf(rk_key_zero(), "gizmo_translate_drag_plane_%I64d", i);


                            Vec2F32 size = scale_2f32(v2f32(axis_length, axis_length), 0.3);
                            Vec3F32 origin = gizmo_origin;
                            Vec3F32 dir = normalize_3f32(add_3f32(axises[(i+1)%3], axises[(i+2)%3]));
                            Vec3F32 dist = scale_3f32(dir, size.x*0.9);
                            origin = add_3f32(origin, dist);
                            rk_drawlist_push_plane_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), drag_plane_key, size, 1, origin, axises[(i+2)%3], axis, plane_clr, 1, 1);
                        }

                        // curr axis
                        rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, gizmo_origin, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis_clr, 6*linew_scale, draw_edge);
                        // curr axis tip
                        rk_drawlist_push_cone(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis, 0.1*scale_t, 0.3*scale_t, 39, axis_clr, 1, 1);
                    }

                    // draw a ball and circle in the middle
                    rk_drawlist_push_sphere(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, axis_length*0.1, axis_length*0.2, 19, 19, 0, v4f32(0,0,0,0.9), 0, 1);
                    rk_drawlist_push_circle(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*0.11, 69,v4f32(1,1,1,0.9), 3, 0); // billboard fashion

                    // draw a outer circle
                    rk_drawlist_push_circle(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*1.3, 69,v4f32(1,1,1,1), 3, 1);

                }break;
                case RK_Gizmo3dMode_Rotation:
                {
                    F32 ring_radius = 1*scale_t;
                    U64 ring_segments = 69;
                    F32 ring_line_width = 9;
                    F32 axis_length = 1.f*scale_t;


                    for(U64 i = 0; i < 3; i++)
                    {
                        Vec3F32 axis = axises[i];
                        RK_Key axis_key = axis_keys[i];

                        Vec4F32 axis_clr = axis_clrs[i];
                        B32 draw_edge = 0;

                        B32 is_hot = rk_key_match(scene->hot_key, axis_key);
                        B32 is_active = rk_key_match(scene->active_key, axis_key);

                        if(is_hot)
                        {
                            axis_clr.w = 0.8;
                            draw_edge = 1;
                        }
                        if(is_active)
                        {
                            axis_clr.w = 0.6;
                            draw_edge = 1;
                        }

                        if(is_active && rk_state->sig.f & UI_SignalFlag_LeftDragging)
                        {
                            typedef struct RK_Gizmo3dRotationDragData RK_Gizmo3dRotationDragData;
                            struct RK_Gizmo3dRotationDragData
                            {
                                QuatF32 start_rot;
                                Vec3F32 anchor;
                                Vec3F32 start_axis;
                            };
                            if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
                            {
                                RK_Gizmo3dRotationDragData drag_data = {0};
                                drag_data.start_rot = active_node->rotation;
                                {
                                    F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, axis, gizmo_origin);
                                    // normalize, get the point on the ring
                                    Vec3F32 intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                                    Vec3F32 dir = normalize_3f32(sub_3f32(intersect, gizmo_origin));
                                    intersect = add_3f32(gizmo_origin, scale_3f32(dir, ring_radius));
                                    drag_data.anchor = intersect;
                                    drag_data.start_axis = axis; // NOTE(k): store this to prevent numerical instability
                                }
                                ui_store_drag_struct(&drag_data);
                            }
                            RK_Gizmo3dRotationDragData *drag_data = ui_get_drag_struct(RK_Gizmo3dRotationDragData);

                            axis = drag_data->start_axis;
                            F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, axis, gizmo_origin);
                            // normalize intersection point
                            Vec3F32 intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                            Vec3F32 dir = normalize_3f32(sub_3f32(intersect, gizmo_origin));
                            intersect = add_3f32(gizmo_origin, scale_3f32(dir, ring_radius));

                            rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_make(0), intersect, gizmo_origin, v4f32(1,0,0,1), 5.f*linew_scale, 1);
                            rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_make(0), drag_data->anchor, gizmo_origin, v4f32(0,1,0,1), 5.f*linew_scale, 1);

                            Vec3F32 start_dir = normalize_3f32(sub_3f32(drag_data->anchor, gizmo_origin));
                            Vec3F32 curr_dir = normalize_3f32(sub_3f32(intersect, gizmo_origin));
                            Vec3F32 cross = cross_3f32(start_dir, curr_dir);
                            // clock wise if sign > 0
                            F32 sign = dot_3f32(cross, axis) > 0 ? 1 : -1;
                            F32 turn_abs = turns_from_radians_f32(acosf(Clamp(-1, dot_3f32(start_dir, curr_dir), 1)));
                            F32 turn = sign * turn_abs;
                            Assert((turn >= 0 && turn <= 1.f) || (turn <= 0 && turn >= -1));

                            if(turn != 0)
                            {
                                // draw arc to indicate pct
                                F32 turn_pct = fmod(abs_f32(turn), 1.f);
                                Assert(turn_pct > 0 && turn_pct < 1);
                                rk_drawlist_push_arc_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, drag_data->anchor, intersect, ring_segments*turn_pct, 1, v4f32(0,0,1,0.6), 3, 1, mat_4x4f32(1.f));

                                // draw axis
                                rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length)), axis_clr, 6.f*linew_scale, 0);
                            }

                            // change active node rotation
                            if(turn != 0)
                            {
                                QuatF32 quat = make_rotate_quat_f32(axis, turn);
                                active_node->node3d->transform.rotation = mul_quat_f32(quat, drag_data->start_rot);
                            }
                        }

                        // draw rotation circle to indicate which axis is rotating
                        rk_drawlist_push_circle(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, gizmo_origin, axis, ring_radius,ring_segments, axis_clr, ring_line_width, draw_edge);
                    }

                    // draw a cube in the inner middle
                    // rk_drawlist_push_box_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), scale_3f32(v3f32(1,1,1), axis_length*0.1), gizmo_origin, axises[1], v4f32(1,1,1,1), 1);
                    // draw a white ball in the middle (NOTE(k): it's semi transparency, so we need draw it last)
                    rk_drawlist_push_sphere(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, ring_radius*0.9, ring_radius*0.9*2, ring_segments, 19, 0, v4f32(0,0,0,0.6), 0, 1);

                    // draw a outer circle
                    // rk_drawlist_push_circle(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*1.3, 69,v4f32(0,0,0,0.1), 9, 0);
                }break;
                case RK_Gizmo3dMode_Scale:
                {
                    // cfg
                    F32 axis_length = 1.0 * scale_t;

                    typedef struct RK_Gizmo3dScaleDragData RK_Gizmo3dScaleDragData;
                    struct RK_Gizmo3dScaleDragData
                    {
                        Vec3F32 start_pos;
                        Vec3F32 start_scale;
                        F32 start_dist;
                    };

                    for(U64 i = 0; i < 3; i++)
                    {
                        Vec3F32 axis = axises[i];
                        RK_Key axis_key = axis_keys[i];
                        Vec4F32 axis_clr = axis_clrs[i];

                        B32 draw_edge = 0;

                        B32 is_hot = rk_key_match(scene->hot_key, axis_key);
                        B32 is_active = rk_key_match(scene->active_key, axis_key);

                        if(is_hot)
                        {
                            axis_clr.w = 0.8;
                            draw_edge = 1;
                        }
                        if(is_active)
                        {
                            axis_clr.w = 0.6;
                            draw_edge = 1;
                        }

                        if(is_active && rk_state->sig.f & UI_SignalFlag_LeftDragging)
                        {
                            if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
                            {
                                RK_Gizmo3dScaleDragData drag_data = {0};
                                drag_data.start_pos = active_node->position;
                                drag_data.start_scale = active_node->scale;
                                Vec3F32 intersect;
                                {
                                    Vec3F32 next_axis = axises[(i+1)%3];
                                    F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, next_axis, gizmo_origin);
                                    intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                                }
                                drag_data.start_dist = dot_3f32(sub_3f32(intersect, active_node->position), axis);
                                ui_store_drag_struct(&drag_data);
                            }

                            RK_Gizmo3dScaleDragData *drag_data = ui_get_drag_struct(RK_Gizmo3dScaleDragData);

                            // choose axis other than current axis(don't care which one)
                            Vec3F32 next_axis = axises[(i+1)%3];
                            Vec3F32 intersect;
                            {
                                F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, next_axis, gizmo_origin);
                                intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                            }
                            F32 dist = dot_3f32(sub_3f32(intersect, drag_data->start_pos), axis);
                            F32 scale = dist / drag_data->start_dist;

                            // scale active_node along the axis
                            if(scale != 0)
                            {
                                active_node->node3d->transform.scale.v[i] = drag_data->start_scale.v[i] * scale;
                            }

                            // draw axis to indicate direction
                            rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), axis_clr, 6.f*linew_scale, 0);
                            // draw a cube to indicate curr scale pos(projected)
                            Vec4F32 box_clr = axis_clr;
                            box_clr.w = 1.0f;
                            rk_drawlist_push_box_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, scale_3f32(v3f32(1,1,1), axis_length*0.1), add_3f32(gizmo_origin, scale_3f32(axis, dist)), i_hat, j_hat, box_clr, draw_edge, 1);
                        }

                        // curr axis
                        rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, gizmo_origin, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis_clr, 19*linew_scale, draw_edge);
                        // curr axis tip
                        rk_drawlist_push_box_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), axis_key, scale_3f32(v3f32(1,1,1),axis_length*0.15), add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), i_hat, j_hat, scale_4f32(axis_clr,0.9), draw_edge, 1);
                    }

                    // draw unit scale box in the middle
                    {
                        B32 is_hot = rk_key_match(scene->hot_key, origin_key);
                        B32 is_active = rk_key_match(scene->active_key, origin_key);
                        B32 draw_edge = 1;
                        Vec4F32 clr = v4f32(0,0,0,0.9);

                        if(is_hot || is_active)
                        {
                            clr.w = 0.6;
                        }

                        if(is_active && rk_state->sig.f & UI_SignalFlag_LeftDragging)
                        {
                            if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
                            {
                                RK_Gizmo3dScaleDragData drag_data = {0};
                                drag_data.start_pos = active_node->position;
                                drag_data.start_scale = active_node->scale;
                                Vec3F32 intersect;
                                {
                                    // projected to front plane(facing camera)
                                    F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, ray_eye_to_center, gizmo_origin);
                                    intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                                }
                                drag_data.start_dist = length_3f32(sub_3f32(intersect, active_node->position));
                                ui_store_drag_struct(&drag_data);
                            }

                            RK_Gizmo3dScaleDragData *drag_data = ui_get_drag_struct(RK_Gizmo3dScaleDragData);
                            Vec3F32 intersect;
                            {
                                F32 t = rk_plane_intersect(eye, ray_eye_to_mouse_end, ray_eye_to_center, gizmo_origin);
                                intersect = mix_3f32(eye, ray_eye_to_mouse_end, t);
                            }
                            F32 dist = length_3f32(sub_3f32(intersect, drag_data->start_pos));
                            F32 scale = dist / drag_data->start_dist;

                            // draw a line from gizmo_origin to intersect
                            rk_drawlist_push_line(rk_frame_arena(), rk_frame_gizmo_drawlist(), rk_key_zero(), gizmo_origin, intersect, v4f32(1,1,1,0.9), 3*linew_scale, draw_edge);

                            // scale active_node along all axises
                            if(scale != 0)
                            {
                                active_node->node3d->transform.scale = scale_3f32(drag_data->start_scale, scale);
                            }
                        }

                        // draw middle cube
                        rk_drawlist_push_box_filled(rk_frame_arena(), rk_frame_gizmo_drawlist(), origin_key, scale_3f32(v3f32(1,1,1),axis_length*0.2), gizmo_origin, i_hat, j_hat, clr, draw_edge, 1);
                    }
                }break;
                default:{InvalidPath;}break;
            }
        }

        // build gizmo drawlists
        rk_drawlist_build(rk_frame_gizmo_drawlist());
    }

    /////////////////////////////////
    // Draw ui
    ui_end_build();
    ProfScope("draw ui") D_BucketScope(rk_state->bucket_rect)
    {
        rk_ui_draw();
    }

    /////////////////////////////////
    // End of the frame (pop, release scene)
    {
        rk_pop_node_bucket();
        rk_pop_res_bucket();
        rk_pop_scene();

        RK_Scene *s = rk_state->first_to_free_scene;
        for(RK_Scene *s = rk_state->first_to_free_scene; s != 0; s = rk_state->first_to_free_scene)
        {
            SLLStackPop(rk_state->first_to_free_scene);
            rk_scene_release(s);
        }
    }

    /////////////////////////////////
    // concate draw buckets

    D_Bucket *ret = d_bucket_make();
    d_push_bucket(ret);
    // TODO(XXX): we should check there is something in passes, we don't a empty geo pass (performance issue)
    if(!d_bucket_is_empty(rk_state->bucket_geo3d[RK_GeoBucketKind_Back]))   {d_sub_bucket(rk_state->bucket_geo3d[RK_GeoBucketKind_Back], 0);}
    if(!d_bucket_is_empty(rk_state->bucket_geo3d[RK_GeoBucketKind_Front]))  {d_sub_bucket(rk_state->bucket_geo3d[RK_GeoBucketKind_Front], 0);}
    if(!d_bucket_is_empty(rk_state->bucket_geo3d[RK_GeoBucketKind_Screen])) {d_sub_bucket(rk_state->bucket_geo3d[RK_GeoBucketKind_Screen], 0);}
    if(!d_bucket_is_empty(rk_state->bucket_rect))                           {d_sub_bucket(rk_state->bucket_rect, 0);}
    d_pop_bucket();

    ProfEnd();
    rk_state->frame_counter++;
    return ret;
}

/////////////////////////////////
// Dynamic drawing (in immediate mode fashion)

internal RK_DrawList *
rk_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap)
{
    RK_DrawList *ret = push_array(arena, RK_DrawList, 1);
    ret->vertex_buffer_cap = vertex_buffer_cap;
    ret->indice_buffer_cap = indice_buffer_cap;
    ret->vertex_buffer = r_buffer_alloc(R_ResourceKind_Stream, vertex_buffer_cap, 0, 0);
    ret->indice_buffer = r_buffer_alloc(R_ResourceKind_Stream, indice_buffer_cap, 0, 0);
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push(Arena *arena, RK_DrawList *drawlist)
{
    RK_DrawNode *ret = push_array(arena, RK_DrawNode, 1);
    DLLPushBack(drawlist->first, drawlist->last, ret);
    drawlist->node_count++;
    return ret;
}

internal void
rk_drawlist_build(RK_DrawList *drawlist)
{
    U64 vertex_count = 0;
    U64 indice_count = 0;
    for(RK_DrawNode *n = drawlist->first; n != 0; n = n->next)
    {
        vertex_count += n->vertex_count;
        indice_count += n->indice_count;
    }
    U64 vertex_buffer_size = vertex_count * sizeof(R_Vertex);
    U64 indice_buffer_size = indice_count * sizeof(U32);

    if(drawlist->vertex_buffer_cap < vertex_buffer_size)
    {
        r_buffer_release(drawlist->vertex_buffer);
        drawlist->vertex_buffer = r_buffer_alloc(R_ResourceKind_Stream, vertex_buffer_size*2, 0, 0);
    }
    if(drawlist->indice_buffer_cap < indice_buffer_size)
    {
        r_buffer_release(drawlist->indice_buffer);
        drawlist->indice_buffer = r_buffer_alloc(R_ResourceKind_Stream, indice_buffer_size*2, 0, 0);
    }

    R_Handle vertex_buffer = drawlist->vertex_buffer;
    R_Handle indice_buffer = drawlist->indice_buffer;

    Temp scratch = scratch_begin(0,0);
    R_Vertex *vertices = push_array(scratch.arena, R_Vertex, vertex_count);
    U64 vertex_idx = 0;
    U32 *indices = push_array(scratch.arena, U32, indice_count);
    U64 indice_idx = 0;

    U64 count = 0;
    for(RK_DrawNode *n = drawlist->first; n != 0; n = n->next)
    {
        // copy vertex
        U64 vertex_copy_size = n->vertex_count * sizeof(R_Vertex);
        U64 indice_copy_size = n->indice_count * sizeof(U32);
        MemoryCopy(vertices+vertex_idx, n->vertices, vertex_copy_size);
        MemoryCopy(indices+indice_idx, n->indices, indice_copy_size);

        // NOTE(k): we would use multiple pass here (geo3d back, geo3d front, screen space)
        D_BucketScope(n->draw_bucket)
        {
            // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
            d_mesh(vertex_buffer, indice_buffer, vertex_idx*sizeof(R_Vertex), indice_idx*sizeof(U32), n->indice_count,
                   n->topology, n->polygon, R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                   n->albedo_tex, n->color, n->line_width, 0, 0, n->xform, n->key.u64[0], n->draw_edge, !n->disable_depth, 1, n->omit_light);
        }

        vertex_idx += n->vertex_count;
        indice_idx += n->indice_count;
        count++;
    }
    Assert(count == drawlist->node_count);

    // upload buffer with new content
    r_buffer_copy(vertex_buffer, vertices, vertex_buffer_size);
    r_buffer_copy(indice_buffer, indices, indice_buffer_size);

    scratch_end(scratch);
}

internal void
rk_drawlist_reset(RK_DrawList *drawlist)
{
    // clear per-frame data
    drawlist->first = 0;
    drawlist->last = 0;
    drawlist->node_count = 0;
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

internal void
rk_ijk_from_matrix(Mat4x4F32 m, Vec3F32 *i, Vec3F32 *j, Vec3F32 *k)
{
    // NOTE(K): don't apply translation, hence 0 for w
    Vec4F32 i_hat = {1,0,0,0};
    Vec4F32 j_hat = {0,1,0,0};
    Vec4F32 k_hat = {0,0,1,0};
    i_hat = mat_4x4f32_transform_4f32(m, i_hat);
    j_hat = mat_4x4f32_transform_4f32(m, j_hat);
    k_hat = mat_4x4f32_transform_4f32(m, k_hat);

    MemoryCopy(i, &i_hat, sizeof(Vec3F32));
    MemoryCopy(j, &j_hat, sizeof(Vec3F32));
    MemoryCopy(k, &k_hat, sizeof(Vec3F32));

    *i = normalize_3f32(*i);
    *j = normalize_3f32(*j);
    *k = normalize_3f32(*k);
}

internal Mat4x4F32
rk_xform_from_transform3d(RK_Transform3D *transform)
{
    Mat4x4F32 xform = mat_4x4f32(1.0);
    Mat4x4F32 rotation_m = mat_4x4f32_from_quat_f32(transform->rotation);
    Mat4x4F32 translate_m = make_translate_4x4f32(transform->position);
    Mat4x4F32 scale_m = make_scale_4x4f32(transform->scale);
    // TRS order
    xform = mul_4x4f32(scale_m, xform);
    xform = mul_4x4f32(rotation_m, xform);
    xform = mul_4x4f32(translate_m, xform);
    return xform;
}

internal Mat4x4F32
rk_xform_from_trs(Vec3F32 translate, QuatF32 rotation, Vec3F32 scale)
{

    Mat4x4F32 xform = mat_4x4f32(1.0);
    Mat4x4F32 rotation_m = mat_4x4f32_from_quat_f32(rotation);
    Mat4x4F32 translate_m = make_translate_4x4f32(translate);
    Mat4x4F32 scale_m = make_scale_4x4f32(scale);
    // TRS order
    xform = mul_4x4f32(scale_m, xform);
    xform = mul_4x4f32(rotation_m, xform);
    xform = mul_4x4f32(translate_m, xform);
    return xform;
}

internal F32
rk_plane_intersect(Vec3F32 ray_start, Vec3F32 ray_end, Vec3F32 plane_normal, Vec3F32 plane_point)
{
    F32 t;
    Vec3F32 ray_dir = sub_3f32(ray_end, ray_start);
    F32 denom = dot_3f32(plane_normal, ray_dir);

    // avoid division by zero
    if(abs_f32(denom) > 1e-6)
    {
        // NOTE(k): if t is not within [0,1], the intersection is outside of ray, but still intersect if we consider the ray infinite long
        t = dot_3f32(plane_normal, sub_3f32(plane_point,ray_start)) / denom;
    }
    else
    {
        t = 0;
    }
    return t;
}

/////////////////////////////////
//~ Enum to string

internal String8
rk_string_from_projection_kind(RK_ProjectionKind kind)
{
    String8 ret = {0};
    switch(kind)
    {
        case RK_ProjectionKind_Perspective:  { ret = str8_lit("perspective"); }break;
        case RK_ProjectionKind_Orthographic: { ret = str8_lit("orthographic"); }break;
        default:                             { InvalidPath; }break;
    }
    return ret;
}

internal String8
rk_string_from_viewport_shading_kind(RK_ViewportShadingKind kind)
{

    String8 ret = {0};
    switch(kind)
    {
        case RK_ViewportShadingKind_Solid:     { ret = str8_lit("solid"); }break;
        case RK_ViewportShadingKind_Wireframe: { ret = str8_lit("wireframe"); }break;
        case RK_ViewportShadingKind_Material:  { ret = str8_lit("material"); }break;
        default:                               { InvalidPath; }break;
    }
    return ret;
}

internal String8
rk_string_from_polygon_kind(RK_ViewportShadingKind kind)
{
    String8 ret = {0};
    switch(kind)
    {
        case R_GeoPolygonKind_Line:  { ret = str8_lit("line"); }break;
        case R_GeoPolygonKind_Point: { ret = str8_lit("point"); }break;
        case R_GeoPolygonKind_Fill:  { ret = str8_lit("fill"); }break;
        default:                     { InvalidPath; }break;
    }
    return ret;
}

/////////////////////////////////
// Scene creation and destruction

internal RK_Scene *
rk_scene_alloc(String8 name, String8 save_path)
{
    Arena *arena = arena_alloc();
    RK_Scene *ret = push_array(arena, RK_Scene, 1);
    {
        ret->arena           = arena;
        ret->node_bucket     = rk_node_bucket_make(arena, 4096-1);
        ret->res_node_bucket = rk_node_bucket_make(arena, 4096-1);
        ret->res_bucket      = rk_res_bucket_make(arena, 4096-1);
        ret->name            = push_str8_copy(arena, name);
        ret->save_path       = push_str8_copy(arena, save_path);
    }
    return ret;
}

internal void
rk_scene_release(RK_Scene *s)
{
    // TODO: release resources 
    // NotImplemented;
    arena_release(s->arena);
}

internal void
rk_scene_active_camera_set(RK_Scene *s, RK_Node *camera_node)
{
    RK_Node *old_camera = rk_node_from_handle(s->active_camera);
    if(old_camera != 0 && old_camera != camera_node)
    {
        old_camera->camera3d->is_active = 0;
    }
    s->active_camera = rk_handle_from_node(camera_node);
}

internal void
rk_scene_active_node_set(RK_Scene *s, RK_Key key, B32 only_navigation_root)
{
    RK_Node *node = 0;
    if(only_navigation_root)
    {
        node = rk_node_from_key(key);
        for(; node != 0 && !(node->flags & RK_NodeFlag_NavigationRoot); node = node->parent) {}
    }

    if(node != 0 || rk_key_match(key, rk_key_zero()))
    {
        s->active_node = rk_handle_from_node(node);
    }
}
