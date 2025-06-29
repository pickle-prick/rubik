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

    // draw drop_shadw
    if(box->flags & UI_BoxFlag_DrawDropShadow)
    {
      Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
      Vec4F32 drop_shadow_color = rk_rgba_from_theme_color(RK_ThemeColor_DropShadow);
      d_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
    }

    // draw background
    if(box->flags & UI_BoxFlag_DrawBackground)
    {
      // main rectangle
      R_Rect2DInst *inst = d_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
      MemoryCopyArray(inst->corner_radii, box->corner_radii);

      if(box->flags & UI_BoxFlag_DrawHotEffects)
      {
        F32 effective_active_t = box->active_t;
        if(!(box->flags & UI_BoxFlag_DrawActiveEffects))
        {
          effective_active_t = 0;
        }
        F32 t = box->hot_t * (1-effective_active_t);

        // brighten
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

        // active effect extension
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

    // draw string
    if(box->flags & UI_BoxFlag_DrawText)
    {
      // TODO: handle font color
      Vec2F32 text_position = ui_box_text_position(box);

      // max width
      F32 max_x = 100000.0f;
      F_Run ellipses_run = {0};

      if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
      {
        max_x = (box->rect.x1-box->text_padding);
        ellipses_run = f_push_run_from_string(scratch.arena, box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
      }

      d_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
    }

    // draw image
    if(box->flags & UI_BoxFlag_DrawImage)
    {
      d_img(box->rect, box->src, box->albedo_tex, v4f32(1,1,1,1), 0., 0., 0.);
    }

    // NOTE(k): draw focus viz
    if(0)
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
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

internal RK_Key
rk_key_make(U64 a, U64 b)
{
  RK_Key result = {a,b};
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
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1] &&
         a.u64[2] == b.u64[2] && a.u64[3] == b.u64[3] &&
         a.u64[4] == b.u64[4];
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
  // TODO(MemoryLeak)
  for(U64 i = 0; i < res_bucket->hash_table_size; i++)
  {
    RK_ResourceBucketSlot *slot = &res_bucket->hash_table[i];

    for(RK_Resource *res = slot->first; res != 0; res = res->hash_next)
    {
      switch(res->kind)
      {
        case RK_ResourceKind_Mesh:
        {
          // RK_Mesh *mesh = res->data;
          // r_buffer_release(mesh->vertices);
          // r_buffer_release(mesh->indices);
          // TODO
        }break;
        case RK_ResourceKind_Material:
        case RK_ResourceKind_Skin:
        case RK_ResourceKind_Animation:
        case RK_ResourceKind_PackedScene:
        case RK_ResourceKind_SpriteSheet:
        {
          // noop
        }break;
        case RK_ResourceKind_Texture2D:
        {
          // RK_Texture2D *tex2d = res->data;
          // r_tex2d_release(tex2d->tex);
        }break;
        case RK_ResourceKind_TileSet:
        {
          // TODO(MemoryLeak)
        }break;
        default:{InvalidPath;}break;
      }
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
  RK_Key ret = rk_key_from_string(seed, string);
  ret.u64[1] = (U64)kind;
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

internal RK_Resource *
rk_res_alloc(RK_Key key)
{
  RK_Resource *ret = 0;
  RK_ResourceBucket *res_bucket = rk_top_res_bucket();

  if(res_bucket->first_free_resource != 0)
  {
    ret = res_bucket->first_free_resource;
    U64 generation = ret->generation;
    MemoryZeroStruct(ret);
    ret->generation = generation;
    SLLStackPop(res_bucket->first_free_resource);
  }
  else
  {
    ret = push_array(res_bucket->arena_ref, RK_Resource, 1);
  }
  ret->kind = key.u64[1];
  ret->owner_bucket = res_bucket;
  ret->key = key;

  U64 slot_idx = ret->key.u64[0] % res_bucket->hash_table_size;
  RK_ResourceBucketSlot *slot = &res_bucket->hash_table[slot_idx];
#if BUILD_DEBUG
  for(RK_Resource *r = slot->first; r != 0; r = r->hash_next)
  {
    if(rk_key_match(r->key, ret->key))
    {
      Assert(false && "resource key collision");
    }
  }
#endif
  DLLPushBack_NP(slot->first, slot->last, ret, hash_next, hash_prev);
  res_bucket->res_count++;
  return ret;
}

internal void
rk_res_release(RK_Resource *res)
{
  RK_ResourceBucket *res_bucket = res->owner_bucket;
  NotImplemented;
}

internal RK_Resource *
rk_res_from_key(RK_Key key)
{
  RK_Resource *ret = 0;
  RK_ResourceBucket *res_bucket = rk_top_res_bucket();
  U64 slot_idx = key.u64[0] % res_bucket->hash_table_size;
  RK_ResourceBucketSlot *slot = &res_bucket->hash_table[slot_idx];
  for(RK_Resource *r = slot->first; r != 0; r = r->hash_next)
  {
    if(rk_key_match(key, r->key))
    {
      ret = r;
      break;
    }
  }
  return ret;
}

internal RK_Handle
rk_handle_from_res(RK_Resource *res)
{
  RK_Handle ret = {0};
  ret.u64[0] = (U64)res;
  if(res != 0)
  {
    ret.u64[1] = res->generation;
    ret.u64[2] = res->key.u64[0];
    ret.u64[3] = res->key.u64[1];
    ret.u64[4] = rk_top_handle_seed();
  }
  return ret;
}

internal RK_Resource *
rk_res_from_handle(RK_Handle *handle)
{
  RK_Resource *ret = 0;

  U64 run_seed = handle->u64[4];
  if(run_seed != rk_top_handle_seed())
  {
    // update
    RK_Key key = rk_key_make(handle->u64[2], handle->u64[3]);
    RK_Handle new = rk_handle_from_res(rk_res_from_key(key));
    MemoryCopy(handle, &new, sizeof(RK_Handle));
  }

  RK_Resource *dst = (RK_Resource*)handle->u64[0];
  U64 generation = handle->u64[1];
  if(dst != 0 && dst->generation == generation)
  {
    ret = dst;
  }
  return ret;
}

/////////////////////////////////
//- Resourcea Building Helpers (subresource management etc...)

/////////////////////////////////
// State accessor/mutator

internal void
rk_init(OS_Handle os_wnd, R_Handle r_wnd)
{
  Arena *arena = arena_alloc();
  rk_state = push_array(arena, RK_State, 1);
  {
    rk_state->arena = arena;
    for(U64 i = 0; i < ArrayCount(rk_state->frame_arenas); i++)
    {
      rk_state->frame_arenas[i] = arena_alloc();
    }
    rk_state->node_bucket     = rk_node_bucket_make(arena, 4096-1);
    rk_state->res_node_bucket = rk_node_bucket_make(arena, 4096-1);
    rk_state->res_bucket      = rk_res_bucket_make(arena, 4096-1);
    rk_state->os_wnd          = os_wnd;
    rk_state->r_wnd           = r_wnd;
    rk_state->dpi             = os_dpi_from_window(os_wnd);
    rk_state->last_dpi        = rk_state->last_dpi;
    rk_state->window_rect     = os_client_rect_from_window(os_wnd, 1);
    rk_state->window_dim      = dim_2f32(rk_state->window_rect);

    for(U64 i = 0; i < ArrayCount(rk_state->drawlists); i++)
    {
      // TODO(k): some device offers 256MB memory which is both cpu visiable and device local
      rk_state->drawlists[i] = rk_drawlist_alloc(arena, MB(64), MB(64));
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
  rk_state->cfg_font_tags[RK_FontSlot_Main]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  rk_state->cfg_font_tags[RK_FontSlot_Code]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  rk_state->cfg_font_tags[RK_FontSlot_Icons]  = f_tag_from_path(str8_lit("./fonts/icons.ttf"));
  rk_state->cfg_font_tags[RK_FontSlot_Game]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));

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

  // UI Views
  for(U64 i = 0; i < RK_ViewKind_COUNT; i++)
  {
    RK_View *view = &rk_state->views[i];
    view->arena = arena_alloc();
  }

  if(BUILD_DEBUG)
  {
    rk_state->views_enabled[RK_ViewKind_Stats] = 1;
  }

  rk_state->function_registry_size = 1000;
  rk_state->function_registry = push_array(arena, RK_FunctionSlot, rk_state->function_registry_size);

  RK_InitStacks(rk_state)
  RK_InitStackNils(rk_state)
}

internal Arena *
rk_frame_arena()
{
  return rk_state->frame_arenas[rk_state->frame_index % ArrayCount(rk_state->frame_arenas)];
}

internal RK_DrawList *
rk_frame_drawlist()
{
  return rk_state->drawlists[rk_state->frame_index % ArrayCount(rk_state->drawlists)];
}

internal void
rk_register_function(String8 name, RK_NodeCustomUpdateFunctionType *ptr)
{
  RK_Key key = rk_key_from_string(rk_key_zero(), name);
  RK_FunctionNode *node = push_array(rk_state->arena, RK_FunctionNode, 1);
  {
    node->key   = key;
    node->name  = push_str8_copy(rk_state->arena, name);
    node->ptr   = ptr;
  }

  U64 slot_idx = key.u64[0] % rk_state->function_registry_size;
  RK_FunctionSlot *slot = &rk_state->function_registry[slot_idx];
  DLLPushBack(slot->first, slot->last, node);
}

internal RK_FunctionNode *
rk_function_from_string(String8 string)
{
  RK_FunctionNode *ret = 0;
  RK_Key key = rk_key_from_string(rk_key_zero(), string);
  U64 slot_idx = key.u64[0] % rk_state->function_registry_size;
  RK_FunctionSlot *slot = &rk_state->function_registry[slot_idx];

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

internal void
rk_ps_push_particle3d(PH_Particle3D *p)
{
  RK_Scene *scene = rk_top_scene();
  PH_Particle3DSystem *system = &scene->particle3d_system;
  SLLQueuePush(system->first_particle, system->last_particle, p);
  p->idx = system->particle_count++;
}

internal void
rk_ps_push_force3d(PH_Force3D *force)
{
  RK_Scene *scene = rk_top_scene();
  PH_Particle3DSystem *system = &scene->particle3d_system;
  SLLQueuePush(system->first_force, system->last_force, force);
  force->idx = system->force_count++;
}

internal void
rk_ps_push_constraint3d(PH_Constraint3D *c)
{
  RK_Scene *scene = rk_top_scene();
  PH_Particle3DSystem *system = &scene->particle3d_system;
  SLLQueuePush(system->first_constraint, system->last_constraint, c);
  c->idx = system->constraint_count++;
}

internal void
rk_rs_push_rigidbody3d(PH_Rigidbody3D *rb)
{
  RK_Scene *scene = rk_top_scene();
  PH_Rigidbody3DSystem *system = &scene->rigidbody3d_system;
  SLLQueuePush(system->first_body, system->last_body, rb);
  rb->idx = system->body_count++;
}

internal void
rk_rs_push_force3d(PH_Force3D *force)
{
  RK_Scene *scene = rk_top_scene();
  PH_Rigidbody3DSystem *system = &scene->rigidbody3d_system;
  SLLQueuePush(system->first_force, system->last_force, force);
  force->idx = system->force_count++;
}

internal void
rk_rs_push_constraint3d(PH_Constraint3D *c)
{
  RK_Scene *scene = rk_top_scene();
  PH_Rigidbody3DSystem *system = &scene->rigidbody3d_system;
  SLLQueuePush(system->first_constraint, system->last_constraint, c);
  c->idx = system->constraint_count++;
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
    SLLStackPop_N(node_bucket->first_free_node, free_next);
  }
  else
  {
    ret = push_array(node_bucket->arena_ref, RK_Node, 1);
  }

  ret->owner_bucket = node_bucket;

  // some default values
  ret->rotation = make_indentity_quat_f32();
  ret->scale = v3f32(1,1,1);
  ret->fixed_xform = mat_4x4f32(1.f);
  ret->fixed_scale = v3f32(1,1,1);
  ret->fixed_rotation = make_indentity_quat_f32();

  rk_node_equip_type_flags(ret, type_flags);
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

  // Remove from node bucket cache map
  U64 slot_idx = node->key.u64[0] % bucket->hash_table_size;
  RK_NodeBucketSlot *slot = &bucket->hash_table[slot_idx];
  DLLRemove_NP(slot->first, slot->last, node, hash_next, hash_prev);
  bucket->node_count--;

  // TODO(MemoryLeak): free custom data

  // free equipments
  rk_node_unequip_type_flags(node, node->type_flags);

  // free update fn node
  for(RK_UpdateFnNode *fn_node = node->first_update_fn; fn_node != 0;)  
  {
    RK_UpdateFnNode *next = fn_node->next;
    // NOTE(k): don't need to pop from stack, since we are freeing the node
    SLLStackPush(bucket->first_free_update_fn_node, fn_node);
    fn_node = next;
  }

  // add it to free list
  SLLStackPush_N(bucket->first_free_node, node, free_next);
}

internal void
rk_node_equip_type_flags(RK_Node *n, RK_NodeTypeFlags type_flags)
{
  RK_EquipTypeFlagsImpl(n, type_flags);
}

internal void
rk_node_unequip_type_flags(RK_Node *n, RK_NodeTypeFlags type_flags)
{
  RK_UnequipTypeFlagsImpl(n, type_flags);
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
  node->name = push_str8_copy_static(rk_display_part_from_key_string(string), node->name_buffer);
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
rk_node_push_fn_(RK_Node *n, String8 fn_name)
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

  RK_FunctionNode *src = rk_function_from_string(fn_name);

  AssertAlways(src != 0 && "add function to state registry please");
  fn_node->f = src->ptr;
  fn_node->name = src->name;
  DLLPushBack(n->first_update_fn, n->last_update_fn, fn_node);
}

internal RK_Handle
rk_handle_from_node(RK_Node *node)
{
  RK_Handle ret = {0};
  ret.u64[0] = (U64)node;
  if(node != 0)
  {
    ret.u64[1] = node->generation;
    ret.u64[2] = node->key.u64[0];
    ret.u64[3] = node->key.u64[1];
    ret.u64[4] = rk_top_handle_seed();
  }
  return ret;
}

internal RK_Node*
rk_node_from_handle(RK_Handle *handle)
{
  RK_Node *ret = 0;

  U64 run_seed = handle->u64[4];
  if(run_seed != rk_top_handle_seed())
  {
    // update
    RK_Key key = rk_key_make(handle->u64[2], handle->u64[3]);
    RK_Handle new = rk_handle_from_node(rk_node_from_key(key));
    MemoryCopy(handle, &new, sizeof(RK_Handle));
  }

  RK_Node *node = (RK_Node*)handle->u64[0];
  U64 generation = handle->u64[1];
  if(node != 0 && node->generation == generation) ret = node;
  return ret;
}

internal B32
rk_node_is_active(RK_Node *node)
{
  B32 ret = 0;
  RK_Scene *scene = rk_top_scene();
  for(; node != 0 && !(node->flags & RK_NodeFlag_NavigationRoot); node = node->parent) {}

  if(node && node == rk_node_from_handle(&scene->active_node))
  {
    ret = 1;
  }
  return ret;
}

/////////////////////////////////
// Magic

RK_SHAPE_SUPPORT_FN(RK_SHAPE_RECT_SUPPORT_FN)
{
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
  struct RK_Stats_State { };

  RK_View *view = rk_view_from_kind(RK_ViewKind_Stats);
  RK_Stats_State *stats = view->custom_data;

  if(stats == 0)
  {
    stats = push_array(view->arena, RK_Stats_State, 1);
    view->custom_data = stats;
  }

  // collect ui box cache count
  // U64 ui_cache_count = 0;
  // for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx++)
  // {
  //   for(UI_Box *box = ui_state->box_table[slot_idx].hash_first; !ui_box_is_nil(box); box = box->hash_next)
  //   {
  //     AssertAlways(!ui_key_match(box->key, ui_key_zero()));
  //     ui_cache_count++;
  //   }
  // }

  B32 enabled = rk_state->views_enabled[RK_ViewKind_Stats];
  if(!enabled) return;

  UI_Box *container = 0;
  UI_Rect(rk_state->window_rect)
    UI_ChildLayoutAxis(Axis2_X)
  {
    container = ui_build_box_from_stringf(0, "###stats_container");
  }

  UI_Parent(container)
  {
    ui_spacer(ui_pct(1.0, 0.0));

    // stats, push to the right side of screen  
    UI_Box *stats_container;
    UI_ChildLayoutAxis(Axis2_Y)
      UI_PrefWidth(ui_px(800, 1.0))
      UI_PrefHeight(ui_children_sum(0.0))
      UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow)
    {
      stats_container = ui_build_box_from_stringf(0, "###stats_body");
    }

    UI_Parent(stats_container)
      UI_TextAlignment(UI_TextAlign_Left)
      UI_TextPadding(9)
      UI_Transparency(0.1)
    {
      // collect some values
      U64 last_frame_index = rk_state->frame_index > 0 ? rk_state->frame_index-1 : 0;
      U64 last_frame_us = rk_state->frame_time_us_history[last_frame_index%ArrayCount(rk_state->frame_time_us_history)];

      UI_Row
      {
        ui_labelf("frame");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.3fms", (F32)last_frame_us/1000.0);
      }
      UI_Row
      {
        ui_labelf("pre cpu time");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.3fms", (F32)rk_state->pre_cpu_time_us/1000.0);
      }
      UI_Row
      {
        ui_labelf("cpu time");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.3fms", (F32)rk_state->cpu_time_us/1000.0);
      }
      UI_Row
      {
        ui_labelf("fps");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.2f", 1.0 / (last_frame_us/1000000.0));
      }
      UI_Row
      {
        ui_labelf("node_count");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%lu", rk_top_node_bucket()->node_count);
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
        ui_labelf("hot_pixel_key");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%I64u", rk_state->hot_pixel_key);
      }
      UI_Row
      {
        ui_labelf("ui_last_build_box_count");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%lu", ui_state->last_build_box_count);
      }
      // UI_Row
      // {
      //   ui_labelf("ui_cache_count");
      //   ui_spacer(ui_pct(1.0, 0.0));
      //   ui_labelf("%lu", ui_cache_count);
      // }
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
}

internal void rk_ui_inspector(void)
{
  RK_Scene *scene = rk_top_scene();

  // unpack active camera
  RK_Node *camera_node = rk_node_from_handle(&scene->active_camera);
  RK_Camera3D *camera = camera_node->camera3d;
  RK_Transform3D *camera_transform = &camera_node->node3d->transform;
  Mat4x4F32 camera_xform = camera_node->fixed_xform;

  // unpack active node
  RK_Node *active_node = rk_node_from_handle(&scene->active_node);

  typedef struct RK_Inspector_State RK_Inspector_State;
  struct RK_Inspector_State
  {
    B32 collapsed;

    B32 show_scene_cfg;
    B32 show_tree;
    B32 show_camera_cfg;
    B32 show_gizmo_cfg;
    B32 show_light_cfg;
    B32 show_node_cfg;

    Rng2F32 rect;

    String8 scene_path_to_save;
    U8 scene_path_to_save_buffer[256];
    S64 last_active_row; // -1 is used to indicate no selection

    // txt input buffer
    TxtPt txt_cursor;
    TxtPt txt_mark;

    B32 txt_has_draft;
    U8 txt_edit_buffer[300];
    U64 txt_edit_string_size;
  };

  RK_View *view = rk_view_from_kind(RK_ViewKind_SceneInspector);

  // init view data
  RK_Inspector_State *state = view->custom_data;
  if(state == 0)
  {
    state = push_array(view->arena, RK_Inspector_State, 1);
    view->custom_data = state;

    state->collapsed       = 0;
    state->show_scene_cfg  = 1;
    state->show_tree       = 1;
    state->show_camera_cfg = 1;
    state->show_gizmo_cfg  = 1;
    state->show_light_cfg  = 1;
    state->show_node_cfg   = 1;

    state->last_active_row = -1;

    state->txt_cursor           = (TxtPt){0};
    state->txt_mark             = (TxtPt){0};
    state->txt_has_draft        = 0;

    state->rect = rk_state->window_rect;
    {
      F32 default_width  = 800;
      F32 default_margin = 30;
      state->rect.x1 = 800;
      state->rect = pad_2f32(state->rect, -default_margin);
    }
  }

  B32 *open = &rk_state->views_enabled[RK_ViewKind_SceneInspector];
  if(!(*open)) return;

  // TODO: move rect into screen

  // build top-level panel container
  UI_Box *pane;
  UI_Transparency(0.1)
  {
    pane = rk_ui_pane_begin(&state->rect, open, &state->collapsed, str8_lit("INSPECTOR"));
  }

  ui_spacer(ui_em(0.215, 0.f));

  ///////////////////////////////////////////////////////////////////////////////////////
  // scene

  RK_UI_Tab(str8_lit("scene"), &state->show_scene_cfg, ui_em(0.1,0), ui_em(0.1,0))
  {
    // scene path
    UI_Flags(UI_BoxFlag_ClickToFocus)
    {
      if(ui_committed(ui_line_edit(&state->txt_cursor,
                                   &state->txt_mark,
                                   state->scene_path_to_save.str,
                                   ArrayCount(state->scene_path_to_save_buffer),
                                   &state->txt_edit_string_size,
                                   state->scene_path_to_save, str8_lit("###scene_path"))))
      {
        String8 string = str8(state->scene_path_to_save_buffer, state->txt_edit_string_size);
        state->scene_path_to_save = push_str8_copy_static(string, state->scene_path_to_save_buffer);
        state->scene_path_to_save.size = state->txt_edit_string_size;                        
      }
    }

    UI_Row
    {
      // templates selection
      // {
      //   rk_ui_dropdown_begin(str8_lit("load"));
      //   for(U64 i = 0; i < rk_state->template_count; i++)
      //   {
      //     RK_SceneTemplate t = rk_state->templates[i];
      //     if(ui_clicked(ui_button(t.name)))
      //     {
      //       RK_Scene *new_scene = t.fn();
      //       SLLStackPush(rk_state->first_to_free_scene, scene);
      //       rk_state->next_active_scene = new_scene;

      //       // TODO(XXX): to be removed
      //       // U64 name_size = ClampTop(inspector->scene_path_to_save_buffer_size, new_scene->name.size);
      //       // inspector->scene_path_to_save.size = name_size;
      //       // MemoryCopy(inspector->scene_path_to_save.str, new_scene->name.str, name_size);
      //       rk_ui_dropdown_hide();
      //     }
      //   }
      //   rk_ui_dropdown_end();
      // }

      if(ui_clicked(ui_buttonf("save")))
      {
        rk_scene_to_tscn(scene);
      }

      if(ui_clicked(ui_buttonf("default")))
      {
        if(scene->default_fn)
        {
          RK_Scene *new = scene->default_fn();
          SLLStackPush(rk_state->first_to_free_scene, scene);
          rk_state->next_active_scene = new;
        }
      }

      if(ui_clicked(ui_buttonf("reload")))
      {
        SLLStackPush(rk_state->first_to_free_scene, scene);
        RK_Scene *new = rk_scene_from_tscn(scene->save_path);
        rk_state->next_active_scene = new;
      }
    }

    ui_divider(ui_em(0.5,0));

    UI_Row
    {
      ui_labelf("omit_grid");
      ui_spacer(ui_pct(1.f,0.f));
      rk_ui_checkbox(&scene->omit_grid);
    }

    UI_Row
    {
      ui_labelf("omit_light");
      ui_spacer(ui_pct(1.f,0.f));
      rk_ui_checkbox(&scene->omit_light);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // scene tree

  RK_UI_Tab(str8_lit("tree"), &state->show_tree, ui_em(0.1,0), ui_em(0.1,0))
  {
    F32 row_height = ui_top_font_size()*1.3;
    // F32 list_height = row_height*30.f;
    F32 list_height = rk_state->window_dim.y*0.3;
    ui_set_next_pref_size(Axis2_Y, ui_px(list_height, 0.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    if(!state->show_scene_cfg)
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
    // TODO(k): move these to some kind of view state
    local_persist UI_ScrollPt scroller = {0};
    scroller.off -= scroller.off * rk_state->animation.fast_rate;
    if(abs_f32(scroller.off) < 0.01) scroller.off = 0;
    Rng1S64 visible_row_rng = {0};

    UI_Parent(container_box) UI_ScrollList(&scroll_list_params, &scroller, 0, 0, &visible_row_rng, &scroll_list_sig)
    {
      RK_Node *root = rk_node_from_handle(&scene->root);
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
            state->last_active_row = row_idx;
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

      if(active_row != state->last_active_row && active_row >= 0)
      {
        state->last_active_row = active_row;
        ui_scroll_pt_target_idx(&scroller, active_row);
      }
    }
  }

  ui_spacer(ui_em(0.215, 0.f));

  ///////////////////////////////////////////////////////////////////////////////////////
  // camera settings

  // RK_UI_Tab(str8_lit("camera"), &state->show_camera_cfg, ui_em(0.1,0), ui_em(0.6,0))
  // {
  //   UI_Row
  //   {
  //     ui_labelf("shading");
  //     ui_spacer(ui_pct(1.0, 0.0));

  //     UI_TextAlignment(UI_TextAlign_Center) for(U64 k = 0; k < RK_ViewportShadingKind_COUNT; k++)
  //     {
  //       if(camera->viewport_shading == k)
  //       {
  //         ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)));
  //       }
  //       if(ui_clicked(ui_button(rk_viewport_shading_kind_display_string_table[k]))) {camera->viewport_shading = k;}
  //     }
  //   }

  //   ui_spacer(ui_em(0.5, 1.0));

  //   UI_Row
  //   {
  //     ui_labelf("viewport");
  //     ui_spacer(ui_pct(1.0, 0.0));
  //     ui_f32_edit(&camera->viewport.x0, -100, 100, &state->txt_cursor, &state->txt_mark, state->txt_edit_buffer, state->txt_edit_buffer_size, &state->txt_edit_string_size, &state->txt_has_draft, str8_lit("X0###viewport_x0"));
  //     ui_f32_edit(&camera->viewport.x1, -100, 100, &state->txt_cursor, &state->txt_mark, state->txt_edit_buffer, state->txt_edit_buffer_size, &state->txt_edit_string_size, &state->txt_has_draft, str8_lit("X1###viewport_x1"));
  //     ui_f32_edit(&camera->viewport.y0, -100, 100, &state->txt_cursor, &state->txt_mark, state->txt_edit_buffer, state->txt_edit_buffer_size, &state->txt_edit_string_size, &state->txt_has_draft, str8_lit("Y0###viewport_y0"));
  //     ui_f32_edit(&camera->viewport.y1, -100, 100, &state->txt_cursor, &state->txt_mark, state->txt_edit_buffer, state->txt_edit_buffer_size, &state->txt_edit_string_size, &state->txt_has_draft, str8_lit("Y1###viewport_y1"));
  //   }
  // }

  // ui_spacer(ui_em(0.2f, 0.f));

  // ///////////////////////////////////////////////////////////////////////////////////////
  // // gizmo settings

  // RK_UI_Tab(str8_lit("camera"), &inspector->show_gizmo_cfg, ui_em(0.1,0), ui_em(0.6,0))
  // {
  //   UI_Row
  //   {
  //     ui_labelf("omit_gizmo");
  //     ui_spacer(ui_pct(1.f,0.f));
  //     rk_ui_checkbox(&scene->omit_gizmo3d);
  //   }

  //   ui_spacer(ui_em(0.2f, 0.f));

  //   UI_Row
  //   {
  //     ui_labelf("mode");
  //     ui_spacer(ui_pct(1.0, 0.0));
  //     UI_TextAlignment(UI_TextAlign_Center) for(U64 k = 0; k < RK_Gizmo3DMode_COUNT; k++)
  //     {
  //       if(scene->gizmo3d_mode == k)
  //       {
  //         ui_set_next_palette(ui_build_palette(ui_top_palette(), .background = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)));
  //       }
  //       if(ui_clicked(ui_button(rk_gizmo3d_mode_display_string_table[k]))) {scene->gizmo3d_mode = k;}
  //     }
  //     ui_spacer(ui_em(0.5, 1.0));
  //   }
  // }

  // ui_spacer(ui_em(0.2f, 0.f));

  // ///////////////////////////////////////////////////////////////////////////////////////
  // // active Node

  // RK_UI_Tab(str8_lit("node"), &inspector->show_node_cfg, ui_em(0.1,0), ui_em(0.6,0)) if(active_node)
  // {
  //   // basic info
  //   {
  //     ui_set_next_child_layout_axis(Axis2_X);
  //     UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //     UI_Parent(header_box)
  //     {
  //       ui_spacer(ui_em(0.3f,0.f));
  //       ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //       ui_labelf("basic");
  //     }

  //     UI_Row
  //     {
  //       ui_labelf("name");
  //       ui_spacer(ui_pct(1.f,0.f));
  //       ui_label(active_node->name);
  //     }

  //     UI_Row
  //     {
  //       ui_labelf("key");
  //       ui_spacer(ui_pct(1.f,0.f));
  //       ui_labelf("%lu", active_node->key);
  //     }

  //     if(active_node->parent) UI_Row
  //     {
  //       ui_labelf("parent");
  //       ui_spacer(ui_pct(1.f,0.f));
  //       ui_label(active_node->parent->name);
  //     }

  //     UI_Row
  //     {
  //       ui_labelf("children_count");
  //       ui_spacer(ui_pct(1.f,0.f));
  //       ui_labelf("%lu", active_node->children_count);
  //     }
  //   }

  //   ui_spacer(ui_em(0.9, 0.f));

  //   /////////////////////////////////////////////////////////////////////////////////////
  //   // equipment info

  //   ////////////////////////////////
  //   // node2d

  //   if(active_node->type_flags & RK_NodeTypeFlag_Node2D)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "node2d_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_Transform2D *transform2d = &active_node->node2d->transform;

  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("node2d");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("position");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform2d->position.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###pos_x"));
  //         ui_f32_edit(&transform2d->position.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###pos_y"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("scale");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform2d->scale.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###scale_x"));
  //         ui_f32_edit(&transform2d->scale.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###scale_y"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("rotation");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform2d->rotation, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###rot"));
  //       }
  //     }
  //   }

  //   ////////////////////////////////
  //   // node3d

  //   if(active_node->type_flags & RK_NodeTypeFlag_Node3D)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "node3d_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_Transform3D *transform3d = &active_node->node3d->transform;

  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("node3d");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("position");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform3d->position.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###pos_x"));
  //         ui_f32_edit(&transform3d->position.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###pos_y"));
  //         ui_f32_edit(&transform3d->position.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###pos_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("scale");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform3d->scale.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###scale_x"));
  //         ui_f32_edit(&transform3d->scale.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###scale_y"));
  //         ui_f32_edit(&transform3d->scale.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###scale_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("rotation");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&transform3d->rotation.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###rot_x"));
  //         ui_f32_edit(&transform3d->rotation.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###rot_y"));
  //         ui_f32_edit(&transform3d->rotation.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###rot_z"));
  //         ui_f32_edit(&transform3d->rotation.w, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("W###rot_w"));
  //       }
  //     }
  //   }

  //   ////////////////////////////////
  //   // directional light

  //   if(active_node->type_flags & RK_NodeTypeFlag_DirectionalLight)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "directionlgith_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_DirectionalLight *light = active_node->directional_light;
  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("directional light");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("direction");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###direction_x"));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###direction_y"));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###direction_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("color");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->color.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_x"));
  //         ui_f32_edit(&light->color.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_y"));
  //         ui_f32_edit(&light->color.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_z"));
  //       }
  //     }
  //   }

  //   ////////////////////////////////
  //   // point light

  //   if(active_node->type_flags & RK_NodeTypeFlag_PointLight)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "pointlight_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_PointLight *light = active_node->point_light;
  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("point light");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("color");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->color.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_x"));
  //         ui_f32_edit(&light->color.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_y"));
  //         ui_f32_edit(&light->color.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("attenuation");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->attenuation.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("C###constant"));
  //         ui_f32_edit(&light->attenuation.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("L###linear"));
  //         ui_f32_edit(&light->attenuation.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Q###quadratic"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("range");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->range, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("R###range"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("intensity");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->intensity, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("I###intensity"));
  //       }
  //     }
  //   }

  //   ////////////////////////////////
  //   // spot light

  //   if(active_node->type_flags & RK_NodeTypeFlag_SpotLight)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "spotlight_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_SpotLight *light = active_node->spot_light;
  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("spot light");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("color");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->color.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_x"));
  //         ui_f32_edit(&light->color.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_y"));
  //         ui_f32_edit(&light->color.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###color_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("attenuation");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->attenuation.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("C###attenuation_constant"));
  //         ui_f32_edit(&light->attenuation.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("L###attenuation_linear"));
  //         ui_f32_edit(&light->attenuation.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Q###attenuation_quadratic"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("direction");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###direction_x"));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###direction_y"));
  //         ui_f32_edit(&light->direction.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###direction_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("range");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->range, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("R###range"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("intensity");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&light->intensity, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("I###intensity"));
  //       }
  //     }
  //   }

  //   ////////////////////////////////
  //   // particle 3d

  //   if(active_node->type_flags & RK_NodeTypeFlag_Particle3D)
  //   {
  //     UI_Box *container;
  //     UI_ChildLayoutAxis(Axis2_Y) UI_PrefHeight(ui_children_sum(0.f))
  //     {
  //       container = ui_build_box_from_stringf(0, "particle3d_detail");
  //     }

  //     UI_Parent(container)
  //     {
  //       RK_Particle3D *p = active_node->particle3d;
  //       ui_set_next_child_layout_axis(Axis2_X);
  //       UI_Box *header_box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder, ui_key_zero());
  //       UI_Parent(header_box)
  //       {
  //         ui_spacer(ui_em(0.3f,0.f));
  //         ui_set_next_pref_width(ui_text_dim(3.f, 0.f));
  //         ui_labelf("particle3d");
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("position");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&p->v.x.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###pos_x"));
  //         ui_f32_edit(&p->v.x.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###pos_y"));
  //         ui_f32_edit(&p->v.x.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###pos_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("velocity");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&p->v.v.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###vel_x"));
  //         ui_f32_edit(&p->v.v.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###vel_y"));
  //         ui_f32_edit(&p->v.v.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###vel_z"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("mass");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&p->v.m, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("M###mass"));
  //       }

  //       UI_Row
  //       {
  //         ui_labelf("force");
  //         ui_spacer(ui_pct(1.0, 0.0));
  //         ui_f32_edit(&p->v.f.x, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("X###force_x"));
  //         ui_f32_edit(&p->v.f.y, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Y###force_y"));
  //         ui_f32_edit(&p->v.f.z, -100, 100, &inspector->txt_cursor, &inspector->txt_mark, inspector->txt_edit_buffer, inspector->txt_edit_buffer_size, &inspector->txt_edit_string_size, &inspector->txt_has_draft, str8_lit("Z###force_z"));
  //       }
  //     }
  //   }
  // }
  rk_ui_pane_end();
}

internal void rk_ui_profiler(void)
{
  // typedef struct RK_Profiler_State RK_Profiler_State;
  // struct RK_Profiler_State
  // {
  //   B32 collapsed;
  //   Rng2F32 rect;
  //   UI_ScrollPt table_scroller;
  // };

  // RK_View *view = rk_view_from_kind(RK_ViewKind_Profiler);
  // RK_Profiler_State *profiler = view->custom_data;
  // if(profiler == 0)
  // {
  //   profiler = push_array(view->arena, RK_Profiler_State, 1);
  //   view->custom_data = profiler;

  //   profiler->rect = rk_state->window_rect;
  //   {
  //     F32 default_width = rk_state->window_dim.x * 0.6f;
  //     F32 default_height = rk_state->window_dim.x * 0.15f;
  //     F32 default_margin = ui_top_font_size()*1.3;
  //     profiler->rect.x0 = profiler->rect.x1 - default_width;
  //     profiler->rect.y0 = profiler->rect.y1 - default_height;
  //     profiler->rect = pad_2f32(profiler->rect, -default_margin);
  //   }
  // }

  // B32 *open = &rk_state->views_enabled[RK_ViewKind_Profiler];
  // if(!(*open)) return;

  // // TODO: handle window resizing

  // // Top-level pane 
  // UI_Box *container_box;
  // UI_Transparency(0.1)
  // {
  //   container_box = rk_ui_pane_begin(&profiler->rect, open, &profiler->collapsed, str8_lit("PROFILER"));
  // }

  // ui_spacer(ui_px(ui_top_font_size()*0.215, 0.f));

  // F32 row_height = ui_top_font_size()*1.3f;
  // ui_push_pref_height(ui_px(row_height, 0));

  // // tag + total_cycles + call_count + cycles_per_call + total_us + us_per_call
  // U64 col_count = 6;

  // // Table header
  // ui_set_next_child_layout_axis(Axis2_X);
  // // NOTE(k): width - scrollbar width
  // ui_set_next_pref_width(ui_px(container_box->fixed_size.x-ui_top_font_size()*0.9f, 0.f));
  // UI_Box *header_box = ui_build_box_from_stringf(0, "###header");
  // UI_Parent(header_box) UI_PrefWidth(ui_pct(1.f,0.f)) UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow) UI_TextAlignment(UI_TextAlign_Center)
  // {
  //   ui_labelf("Tag");
  //   ui_labelf("Cycles n/p");
  //   ui_labelf("Calls");
  //   ui_labelf("Cycles per call");
  //   ui_labelf("MS n/p");
  //   ui_labelf("MS per call");
  // }

  // Rng2F32 content_rect = container_box->rect;
  // content_rect.y0 = header_box->rect.y1;

  // // content
  // ProfTickInfo *pf_tick = ProfTickPst();
  // U64 prof_node_count = 0;
  // if(pf_tick != 0 && pf_tick->node_hash_table != 0)
  // {
  //   for(U64 slot_idx = 0; slot_idx < pf_table_size; slot_idx++)
  //   {
  //     prof_node_count += pf_tick->node_hash_table[slot_idx].count;
  //   }
  // }

  // UI_ScrollListParams scroll_list_params = {0};
  // {
  //   Vec2F32 rect_dim = dim_2f32(content_rect);
  //   scroll_list_params.flags = UI_ScrollListFlag_All;
  //   scroll_list_params.row_height_px = row_height;
  //   scroll_list_params.dim_px = rect_dim;
  //   scroll_list_params.item_range = r1s64(0, prof_node_count);
  //   scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 0;
  // }

  // // animating scroller pt
  // profiler->table_scroller.off -= profiler->table_scroller.off * rk_state->animation.fast_rate;
  // if(abs_f32(profiler->table_scroller.off) < 0.01) profiler->table_scroller.off = 0;

  // UI_ScrollListSignal scroll_list_sig = {0};
  // Rng1S64 visible_row_rng = {0};

  // UI_ScrollList(&scroll_list_params, &profiler->table_scroller, 0, 0, &visible_row_rng, &scroll_list_sig)
  // {
  //   if(pf_tick != 0 && pf_tick->node_hash_table != 0)
  //   {
  //     U64 row_idx = 0;
  //     for(U64 slot_idx = 0; slot_idx < pf_table_size; slot_idx++)
  //     {
  //       for(ProfNode *n = pf_tick->node_hash_table[slot_idx].first; n != 0; n = n->hash_next)
  //       {
  //         if(row_idx >= visible_row_rng.min && row_idx <= visible_row_rng.max)
  //         {
  //           UI_Box *row = 0;
  //           UI_ChildLayoutAxis(Axis2_X) UI_Flags(UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_MouseClickable|UI_BoxFlag_DrawBackground) UI_PrefWidth(ui_pct(1.f,0.f))
  //           {
  //             row = ui_build_box_from_stringf(0,"###row_%d", row_idx);
  //             ui_signal_from_box(row);
  //           }
  //           UI_Parent(row) UI_PrefWidth(ui_pct(1.0,0.f)) UI_Flags(UI_BoxFlag_DrawSideRight)
  //           {
  //             ui_label(n->tag);
  //             UI_Row
  //             {
  //               ui_labelf("%lu", n->total_cycles);
  //               ui_spacer(ui_pct(1.f,0.f));
  //               ui_labelf("%.2f", (F32)n->total_cycles/pf_tick->cycles);
  //             }
  //             ui_labelf("%lu", n->call_count);
  //             ui_labelf("%lu", n->cycles_per_call);
  //             UI_Row
  //             {
  //               ui_labelf("%.2f", n->total_us/1000.0);
  //               ui_spacer(ui_pct(1.f,0.f));
  //               ui_labelf("%.2f", (F32)n->total_us/pf_tick->us);
  //             }
  //             ui_labelf("%.2f", n->us_per_call/1000.0);
  //           }
  //         }
  //         row_idx++;
  //       }
  //     }
  //   }
  // }

  // ui_pop_pref_height();
  // rk_ui_pane_end();
}

internal void
rk_ui_terminal(void)
{
  typedef struct RK_Terminal_State RK_Terminal_State;
  struct RK_Terminal_State
  {
    F32 height;
    B32 last_open;
    Arena *history_arena;
    UI_ScrollPt scroller;
    String8List history;
    Rng1S64 visible_row_rng;
    U64 edit_line_size;
    U8 edit_line_buffer[300];

    // txt input buffer
    TxtPt txt_cursor;
    TxtPt txt_mark;
  };
  RK_View *view = rk_view_from_kind(RK_ViewKind_Terminal);
  RK_Terminal_State *state = view->custom_data;
  if(state == 0)
  {
    state = push_array(view->arena, RK_Terminal_State, 1);
    state->height = 500;
    state->history_arena = arena_alloc();
    state->txt_cursor.column = 1;
    state->txt_mark.column = 1;

    view->custom_data = state;
  }

  B32 *open = &rk_state->views_enabled[RK_ViewKind_Terminal];
  state->last_open = *open;

  {
    OS_Event *os_evt_first = rk_state->os_events.first;
    OS_Event *os_evt_opl = rk_state->os_events.last + 1;
    for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    {
      if(os_evt == 0) continue;
      B32 taken = 0;

      if(os_evt->key == OS_Key_Tick && os_evt->kind == OS_EventKind_Press && (os_evt->modifiers & OS_Modifier_Ctrl))
      {
        *open = !(*open);
        taken = 1;
      }

      if(taken) os_eat_event(&rk_state->os_events, os_evt);
    }
  }

  // focus input ui box if it was disabled last frame
  B32 focus = !state->last_open;

  if(*open)
  {
    // container
    UI_Box *container_box;
    Rng2F32 rect = {0,0, rk_state->window_dim.x, state->height};
    UI_Rect(rect)
      UI_Transparency(0.3)
      UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_KeyboardClickable|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBackground)
      UI_ChildLayoutAxis(Axis2_Y)
    {
      container_box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "terminal");
    }

    UI_Parent(container_box)
      UI_PrefWidth(ui_pct(1.0,1.0))
    {
      ///////////////////////////////////////////////////////////////////////////////////
      // history scroll area

      {
        F32 pref_row_height = ui_top_font_size()*1.3f;
        F32 scroll_area_height = state->height-ui_top_pref_width().value;
        U64 visible_row_count = scroll_area_height/pref_row_height;
        F32 row_height = scroll_area_height/round_f32(visible_row_count);

        UI_ScrollListParams scroll_list_params = {0};
        {
          Vec2F32 rect_dim = v2f32(rk_state->window_dim.x, scroll_area_height);
          scroll_list_params.flags = UI_ScrollListFlag_All;
          scroll_list_params.row_height_px = row_height;
          scroll_list_params.dim_px = rect_dim;
          scroll_list_params.item_range = r1s64(0, state->history.node_count);
          scroll_list_params.cursor_min_is_empty_selection[Axis2_Y] = 0;
        }

        state->scroller.off -= state->scroller.off * rk_state->animation.fast_rate;
        if(abs_f32(state->scroller.off) < 0.01) state->scroller.off = 0;

        UI_ScrollListSignal scroll_list_sig = {0};
        U64 row_index = 0;
        UI_ScrollList(&scroll_list_params, &state->scroller, 0, 0, &state->visible_row_rng, &scroll_list_sig)
        {
          for(String8Node *line = state->history.first; line != 0; line = line->next)
          {
            if(row_index >= state->visible_row_rng.min && row_index <= state->visible_row_rng.max)
            {
              UI_Signal sig = ui_label(line->string);
              UI_Box *b = sig.box;
            }
            row_index++;
          }
        }
      }

      ///////////////////////////////////////////////////////////////////////////////////
      // input area

      UI_Flags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow)
      {
        UI_Flags(UI_BoxFlag_ClickToFocus)
        {
          UI_Signal sig = ui_line_edit(&state->txt_cursor,
                                       &state->txt_mark,
                                       state->edit_line_buffer,
                                       ArrayCount(state->edit_line_buffer),
                                       &state->edit_line_size,
                                       str8_lit(""),
                                       str8_lit("###line_edit"));
          UI_Box *line_edit_box = sig.box;

          if(focus)
          {
            ui_set_auto_focus_active_key(line_edit_box->key);
          }

          if(ui_committed(sig))
          {
            // scroll to bottom
            U64 scroll_diff = 0;
            if(state->history.node_count > 0)
            {
              U64 last_row_index = state->history.node_count-1;
              if(last_row_index >= state->visible_row_rng.max)
              {
                scroll_diff = last_row_index-state->visible_row_rng.max;
              }
            }

            String8 line = push_str8_copy(state->history_arena, str8(state->edit_line_buffer, state->edit_line_size));
            str8_list_push(state->history_arena, &state->history, line);

            // handle cmd
            if(str8_match(line, str8_lit("show inspector"), 0))
            {
              rk_state->views_enabled[RK_ViewKind_SceneInspector] = !rk_state->views_enabled[RK_ViewKind_SceneInspector];
            }
            if(str8_match(line, str8_lit("show profiler"), 0))
            {
              rk_state->views_enabled[RK_ViewKind_Profiler] = !rk_state->views_enabled[RK_ViewKind_Profiler];
            }

            // clear edit_buffer
            state->edit_line_size = 0;

            if(scroll_diff > 0)
            {
              ui_scroll_pt_target_idx(&state->scroller, state->visible_row_rng.min+scroll_diff);
            }
          }
        }
      }
    }
  }
}

internal B32
rk_frame(void)
{
  Temp scratch = scratch_begin(0,0); 
  RK_Scene *scene = rk_state->active_scene;

  ///////////////////////////////////////////////////////////////////////////////////////
  // free scenes & nodes

  ProfScope("free")
  {
    RK_Node *n = scene->node_bucket->first_to_free_node;
    while(n != 0)
    {
      RK_Node *next = n->free_next;
      SLLStackPop_N(scene->node_bucket->first_to_free_node, free_next);
      rk_node_release(n);
      n = next;
    }

    n = rk_state->node_bucket->first_to_free_node;
    while(n != 0)
    {
      RK_Node *next = n->free_next;
      SLLStackPop_N(rk_state->node_bucket->first_to_free_node, free_next);
      rk_node_release(n);
      n = next;
    }
  }

  {
    RK_Scene *s = rk_state->first_to_free_scene;
    if(s != 0 && (rk_state->frame_index-s->frame_index) > MAX_FRAMES_IN_FLIGHT)
    {
      while(s)
      {
        RK_Scene *next = s->next;
        SLLStackPop(rk_state->first_to_free_scene);
        rk_scene_release(s);
        s = next;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // prepare current scene

  // set next scene if we have any
  if(rk_state->next_active_scene != 0)
  {
    scene = rk_state->next_active_scene;
    rk_state->active_scene = scene;
    SLLStackPop(rk_state->next_active_scene);
  }

  scene->frame_index = rk_state->frame_index;
  rk_push_scene(scene);
  rk_push_node_bucket(scene->node_bucket);
  rk_push_res_bucket(scene->res_bucket);
  rk_push_handle_seed(scene->handle_seed);

  ///////////////////////////////////////////////////////////////////////////////////////
  // do per-frame resets

  rk_drawlist_reset(rk_frame_drawlist());
  arena_clear(rk_frame_arena());

  // clear physics states
  scene->particle3d_system.first_particle   = 0;
  scene->particle3d_system.last_particle    = 0;
  scene->particle3d_system.particle_count   = 0;
  scene->particle3d_system.first_force      = 0;
  scene->particle3d_system.last_force       = 0;
  scene->particle3d_system.force_count      = 0;
  scene->particle3d_system.first_constraint = 0;
  scene->particle3d_system.last_constraint  = 0;
  scene->particle3d_system.constraint_count = 0;

  scene->rigidbody3d_system.first_body       = 0;
  scene->rigidbody3d_system.last_body        = 0;
  scene->rigidbody3d_system.body_count       = 0;
  scene->rigidbody3d_system.first_force      = 0;
  scene->rigidbody3d_system.last_force       = 0;
  scene->rigidbody3d_system.force_count      = 0;
  scene->rigidbody3d_system.first_constraint = 0;
  scene->rigidbody3d_system.last_constraint  = 0;
  scene->rigidbody3d_system.constraint_count = 0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // get events from os
  
  rk_state->os_events          = os_get_events(scratch.arena, 0);
  rk_state->last_window_rect   = rk_state->window_rect;
  rk_state->last_window_dim    = dim_2f32(rk_state->last_window_rect);
  rk_state->window_res_changed = rk_state->window_rect.x0 == rk_state->last_window_rect.x0 && rk_state->window_rect.x1 == rk_state->last_window_rect.x1 && rk_state->window_rect.y0 == rk_state->last_window_rect.y0 && rk_state->window_rect.y1 == rk_state->last_window_rect.y1;
  rk_state->window_rect        = os_client_rect_from_window(rk_state->os_wnd, 0);
  rk_state->window_dim         = dim_2f32(rk_state->window_rect);
  rk_state->last_cursor        = rk_state->cursor;
  {
    Vec2F32 cursor = os_mouse_from_window(rk_state->os_wnd);
    if(cursor.x >= 0 && cursor.x <= rk_state->window_dim.x &&
       cursor.y >= 0 && cursor.y <= rk_state->window_dim.y)
    {
      rk_state->cursor = cursor;
    }
  }
  rk_state->cursor_delta       = sub_2f32(rk_state->cursor, rk_state->last_cursor);
  rk_state->last_dpi           = rk_state->dpi;
  rk_state->dpi                = os_dpi_from_window(rk_state->os_wnd);

  // animation
  rk_state->animation.vast_rate = 1 - pow_f32(2, (-60.f * ui_state->animation_dt));
  rk_state->animation.fast_rate = 1 - pow_f32(2, (-50.f * ui_state->animation_dt));
  rk_state->animation.fish_rate = 1 - pow_f32(2, (-40.f * ui_state->animation_dt));
  rk_state->animation.slow_rate = 1 - pow_f32(2, (-30.f * ui_state->animation_dt));
  rk_state->animation.slug_rate = 1 - pow_f32(2, (-15.f * ui_state->animation_dt));
  rk_state->animation.slaf_rate = 1 - pow_f32(2, (-8.f * ui_state->animation_dt));

  ///////////////////////////////////////////////////////////////////////////////////////
  // calculate avg length in us of last many frames

  U64 frame_time_history_avg_us = 0;
  {
    U64 num_frames_in_history = Min(ArrayCount(rk_state->frame_time_us_history), rk_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    if(num_frames_in_history > 0)
    {
      for(U64 i = 0; i < num_frames_in_history; i++)
      {
        frame_time_history_sum_us += rk_state->frame_time_us_history[i];
      }
      frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // pick target hz

  // pick among a number of sensible targets to snap to, given how well we've been performing
  F32 target_hz = os_get_gfx_info()->default_refresh_rate;
  // F32 target_hz = 30.0;
  if(rk_state->frame_index > 32)
  {
    F32 possible_alternate_hz_targets[] = {target_hz, 60.f, 120.f, 144.f, 240.f};
    F32 best_target_hz = target_hz;
    S64 best_target_hz_frame_time_us_diff = max_S64;
    for(U64 idx = 0; idx < ArrayCount(possible_alternate_hz_targets); idx += 1)
    {
      F32 candidate = possible_alternate_hz_targets[idx];
      if(candidate <= target_hz)
      {
        U64 candidate_frame_time_us = 1000000/(U64)candidate;
        S64 frame_time_us_diff = (S64)frame_time_history_avg_us - (S64)candidate_frame_time_us;
        if(abs_s64(frame_time_us_diff) < best_target_hz_frame_time_us_diff &&
           frame_time_history_avg_us < candidate_frame_time_us + candidate_frame_time_us/4)
        {
          best_target_hz = candidate;
          best_target_hz_frame_time_us_diff = frame_time_us_diff;
        }
      }
    }
    target_hz = best_target_hz;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // frame_dt

  rk_state->frame_dt = 1.f/target_hz;

  // begin measuring actual per-frame work
  U64 begin_time_us = os_now_microseconds();

  ///////////////////////////////////////////////////////////////////////////////////////
  // unpack current camera

  RK_Node *camera_node = rk_node_from_handle(&scene->active_camera);
  AssertAlways(camera_node != 0 && "No active camera was found");
  RK_Camera3D *camera = camera_node->camera3d;
  Mat4x4F32 camera_xform = camera_node->fixed_xform;

  // polygon mode
  R_GeoPolygonKind polygon_mode;
  switch(camera->viewport_shading)
  {
    case RK_ViewportShadingKind_Wireframe:       {polygon_mode = R_GeoPolygonKind_Line;}break;
    case RK_ViewportShadingKind_Solid:           {polygon_mode = R_GeoPolygonKind_Fill;}break;
    case RK_ViewportShadingKind_Material:        {polygon_mode = R_GeoPolygonKind_Fill;}break;
    default:                                     {InvalidPath;}break;
  }

  // view/projection (NOTE(k): behind by one frame)
  Mat4x4F32 view_m = inverse_4x4f32(camera_xform);
  Mat4x4F32 proj_m;
  Vec3F32 eye = camera_node->fixed_position;
  Rng2F32 viewport = camera->viewport;
  Vec2F32 viewport_dim = dim_2f32(camera->viewport);
  if(viewport_dim.x == 0 || viewport_dim.y == 0)
  {
    viewport = rk_state->window_rect;
    viewport_dim = dim_2f32(viewport);
  }

  switch(camera->projection)
  {
    case RK_ProjectionKind_Perspective:
    {
      proj_m = make_perspective_vulkan_4x4f32(camera->perspective.fov, viewport_dim.x/viewport_dim.y, camera->zn, camera->zf);
    }break;
    case RK_ProjectionKind_Orthographic:
    {
      proj_m = make_orthographic_vulkan_4x4f32(camera->orthographic.left, camera->orthographic.right, camera->orthographic.bottom, camera->orthographic.top, camera->zn, camera->zf);
    }break;
    default:{InvalidPath;}break;
  }
  Mat4x4F32 proj_view_m = mul_4x4f32(proj_m, view_m);
  Mat4x4F32 proj_view_inv_m = inverse_4x4f32(proj_view_m);

  ///////////////////////////////////////////////////////////////////////////////////////
  // remake drawing buckets every frame

  d_begin_frame();

  // rect
  rk_state->bucket_rect = d_bucket_make();

  // geo2d
  {
    D_Bucket **bucket = &rk_state->bucket_geo[RK_GeoBucketKind_Geo2D];
    *bucket = d_bucket_make();
    D_BucketScope(*bucket)
    {
      d_geo2d_begin(viewport, view_m, proj_m);
    }
  }

  // geo3d back
  {
    D_Bucket **bucket = &rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Back];
    *bucket = d_bucket_make();
    D_BucketScope(*bucket)
    {
      R_PassParams_Geo3D *pass_params = d_geo3d_begin(viewport, view_m, proj_m);
      pass_params->omit_light = scene->omit_light;
      pass_params->omit_grid = scene->omit_grid;
      pass_params->lights = push_array(rk_frame_arena(), R_Light3D, MAX_LIGHTS_PER_PASS);
      pass_params->materials = push_array(rk_frame_arena(), R_Material3D, MAX_MATERIALS_PER_PASS);
      pass_params->textures = push_array(rk_frame_arena(), R_PackedTextures, MAX_MATERIALS_PER_PASS);

      // load default material
      pass_params->materials[0].diffuse_color = v4f32(1,1,1,1);
      pass_params->materials[0].opacity = 1.0f;
      pass_params->material_count = 1;
    }
  }

  // geo front, for example we want gizmo drawn on top of scene
  {
    D_Bucket **bucket = &rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
    *bucket = d_bucket_make();
    D_BucketScope(*bucket)
    {
      R_PassParams_Geo3D *pass_params = d_geo3d_begin(viewport, view_m, proj_m);
      pass_params->omit_light = 1;
      pass_params->omit_grid = 1;
      pass_params->lights = push_array(rk_frame_arena(), R_Light3D, MAX_LIGHTS_PER_PASS);
      pass_params->materials = push_array(rk_frame_arena(), R_Material3D, MAX_MATERIALS_PER_PASS);
      pass_params->textures = push_array(rk_frame_arena(), R_PackedTextures, MAX_MATERIALS_PER_PASS);

      // load default material
      pass_params->materials[0].diffuse_color = v4f32(1,1,1,1);
      pass_params->materials[0].opacity = 1.0f;
      pass_params->material_count = 1;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // build ui

  // build event list for ui
  UI_EventList ui_events = {0};
  OS_Event *os_evt_first = rk_state->os_events.first;
  OS_Event *os_evt_opl = rk_state->os_events.last + 1;
  for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
  {
    // if(os_window_is_focused(window))
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

  // begin build ui
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
    ui_begin_build(rk_state->os_wnd, &ui_events, &icon_info, &widget_palette_info, &animation_info, rk_state->frame_dt);

    ui_push_font(main_font);
    ui_push_font_size(main_font_size);
    // ui_push_text_raster_flags(...);
    ui_push_text_padding(main_font_size*0.2f);
    ui_push_pref_width(ui_em(20.f, 1.f));
    ui_push_pref_height(ui_em(1.35f, 1.f));
    ui_push_palette(rk_palette_from_code(RK_PaletteCode_Base));
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // draw game view

  // TODO: change drawing order if cursor is clicked on one of the views
  ProfScope("rk ui drawing")
  {
    rk_ui_inspector();
    rk_ui_stats();
    rk_ui_profiler();
    rk_ui_terminal();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // process events for game logic

  {
    OS_Event *os_evt_first = rk_state->os_events.first;
    OS_Event *os_evt_opl = rk_state->os_events.last + 1;
    for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
    {
      if(os_evt == 0) continue;
      B32 taken = 0;
      // if(os_evt->kind == OS_EventKind_Text && os_evt->key == OS_Key_Space) {}
      if(os_evt->key == OS_Key_S && os_evt->kind == OS_EventKind_Press && (os_evt->modifiers & OS_Modifier_Ctrl))
      {
        rk_scene_to_tscn(scene);
        taken = 1;
      }

      if(taken) os_eat_event(&rk_state->os_events, os_evt);
    }
  }

  /////////////////////////////////////////////////////////////////////////////////////
  // update & draw scene

  // UI Box for game viewport (handle user interaction)
  ui_set_next_rect(rk_state->window_rect);
  UI_Box *overlay = ui_build_box_from_string(UI_BoxFlag_MouseClickable|UI_BoxFlag_ClickToFocus|UI_BoxFlag_Scroll|UI_BoxFlag_DisableFocusOverlay, str8_lit("###game_overlay"));
  RK_Node *active_node = rk_node_from_handle(&scene->active_node);

  {
    RK_FrameContext ctx =
    {
      .eye = eye,
      .proj_m = proj_m,
      .view_m = view_m,
      .proj_view_m = proj_view_m,
      .proj_view_inv_m = proj_view_inv_m,
    };

    // unpack pass/params
    D_Bucket *geo3d_back_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Back];
    R_Pass *geo3d_back_pass = r_pass_from_kind(d_thread_ctx->arena, &geo3d_back_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *geo_back_params = geo3d_back_pass->params_geo3d;

    D_Bucket *geo2d_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo2D];
    R_Pass *geo2d_pass = r_pass_from_kind(d_thread_ctx->arena, &geo2d_bucket->passes, R_PassKind_Geo2D, 1);
    R_PassParams_Geo2D *geo2d_params = geo2d_pass->params_geo2d;

    // D_Bucket *geo_screen_bucket = rk_state->bucket_geo3d[RK_GeoBucketKind_Screen];
    // R_Pass *geo_screen_pass = r_pass_from_kind(d_thread_ctx->arena, &geo_screen_bucket->passes, R_PassKind_Geo3D, 1);
    // R_PassParams_Geo3D *geo_screen_params = geo_screen_pass->params_geo3d;

    /////////////////////////////////////////////////////////////////////////////////////
    // Update passes

    // NOTE(k): we need two update passes (1: update_fn, 2: the rest of update)
    //          the reason is update_fn could modify the node tree which makes transient not working

    RK_Node *root = rk_node_from_handle(&scene->root);

    // Pass 1
    if(scene->update_fn)
    {
      scene->update_fn(scene, &ctx);
    }
    // TODO(k): maybe we don't need use update_fn for individual node, instead we use one single udpate entry for the whole scene
    // RK_Node *node = root;
    // while(node != 0)
    // {
    //   RK_NodeRec rec = rk_node_df_pre(node, 0, 0);
    //   for(RK_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
    //   {
    //     fn->f(node, scene, &ctx);
    //   }
    //   node = rec.next;
    // }

    // Pass 2
    RK_Node *node = root;
    R_Light3D *lights = geo_back_params->lights;
    U64 *light_count = &geo_back_params->light_count;

    HashSlot *material_slots = push_array(rk_frame_arena(), HashSlot, MAX_MATERIALS_PER_PASS);
    R_Material3D *materials = geo_back_params->materials;
    R_PackedTextures *textures = geo_back_params->textures;
    U64 *material_count = &geo_back_params->material_count;

    U64 drawable_count = 0;
    U64 drawable_cap = 1000;
    RK_Node **nodes_to_draw = push_array(scratch.arena, RK_Node*, drawable_cap);
    // TODO(XXX): debug only
    U64 node_count = 0;
    ProfBegin("generic update");
    while(node != 0)
    {
      node_count++;
      AssertAlways(*light_count < MAX_LIGHTS_PER_PASS);
      AssertAlways(*material_count < MAX_MATERIALS_PER_PASS);

      RK_NodeRec rec = rk_node_df_pre(node, 0, 0);
      RK_Node *parent = node->parent;

      // for(RK_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
      // {
      //   fn->f(node, scene, &ctx);
      // }

      ///////////////////////////////////////////////////////////////////////////////////
      // update node hot_t/active_t

      B32 is_hot = rk_key_match(scene->hot_key, node->key);
      B32 is_active = rk_key_match(scene->active_key, node->key);

      // determine animation rate
      F32 hot_rate = rk_state->animation.fast_rate;
      F32 active_rate = rk_state->animation.fast_rate;

      // update animate t
      node->hot_t    += hot_rate * ((F32)is_hot - node->hot_t);
      node->active_t += active_rate * ((F32)is_active - node->active_t);

      ///////////////////////////////////////////////////////////////////////////////////
      // update node transform based on it's type flags

      B32 has_transform = 0;

      if((node->type_flags & RK_NodeTypeFlag_Particle3D) && (node->type_flags & RK_NodeTypeFlag_Node3D))
      {
        MemoryCopy(&node->node3d->transform.position, &node->particle3d->v.x, sizeof(Vec3F32));
        rk_ps_push_particle3d(&node->particle3d->v);
      }

      if((node->type_flags & RK_NodeTypeFlag_Rigidbody3D) && (node->type_flags & RK_NodeTypeFlag_Node3D))
      {
        MemoryCopy(&node->node3d->transform.position, &node->rigidbody3d->v.x, sizeof(Vec3F32));
        MemoryCopy(&node->node3d->transform.rotation, &node->rigidbody3d->v.q, sizeof(QuatF32));
        rk_rs_push_rigidbody3d(&node->rigidbody3d->v);
      }

      if(node->type_flags & RK_NodeTypeFlag_Node2D)
      {
        has_transform = 1;
        MemoryCopy(&node->position, &node->node2d->transform.position, sizeof(Vec2F32));
        MemoryCopy(&node->scale, &node->node2d->transform.scale, sizeof(Vec2F32));
        node->rotation = make_rotate_quat_f32(v3f32(0,0,-1), node->node2d->transform.rotation);
      }

      if(node->type_flags & RK_NodeTypeFlag_Node3D)
      {
        has_transform = 1;
        node->position = node->node3d->transform.position;
        node->scale = node->node3d->transform.scale;
        node->rotation = node->node3d->transform.rotation;
      }

      // Update artifacts in DFS order 
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
        rk_trs_from_xform(&xform, &node->fixed_position, &node->fixed_rotation, &node->fixed_scale);
      }

      if(node->type_flags & RK_NodeTypeFlag_Camera3D)
      {
        if(node->camera3d->is_active) 
        {
          rk_scene_active_camera_set(scene, node);
        }
      }

      if(node->type_flags & RK_NodeTypeFlag_DirectionalLight)
      {
        R_Light3D *light_dst = &lights[(*light_count)++];
        RK_DirectionalLight *light_src = node->directional_light;

        Vec4F32 direction_ws = {0,0,0,0};
        MemoryCopy(&direction_ws, &light_src->direction, sizeof(Vec3F32));
        Vec4F32 direction_vs = transform_4x4f32(view_m, direction_ws);

        Vec4F32 color = {0,0,0, 1};
        MemoryCopy(&color, &light_src->color, sizeof(Vec3F32));

        light_dst->kind = R_Vulkan_Light3DKind_Directional;
        light_dst->direction_ws = direction_ws;
        light_dst->direction_vs = direction_vs;
        light_dst->color = color;
        light_dst->intensity = light_src->intensity;
      }

      if(node->type_flags & RK_NodeTypeFlag_PointLight)
      {
        R_Light3D *light_dst = &lights[(*light_count)++];
        RK_PointLight *light_src = node->point_light;

        Vec4F32 position_ws = {0,0,0,1};
        MemoryCopy(&position_ws, &node->fixed_position, sizeof(Vec3F32));
        Vec4F32 position_vs = transform_4x4f32(view_m, position_ws);

        Vec4F32 color = {0,0,0, 1};
        MemoryCopy(&color, &light_src->color, sizeof(Vec3F32));

        Vec4F32 attenuation = {0};
        MemoryCopy(&attenuation, &light_src->attenuation, sizeof(Vec3F32));

        light_dst->kind = R_Vulkan_Light3DKind_Point;
        light_dst->position_ws = position_ws;
        light_dst->position_vs = position_vs;
        light_dst->range = light_src->range;
        light_dst->color = color;
        light_dst->intensity = light_src->intensity;
        light_dst->attenuation = attenuation;
      }

      if(node->type_flags & RK_NodeTypeFlag_SpotLight)
      {
        R_Light3D *light_dst = &lights[(*light_count)++];
        RK_SpotLight *light_src = node->spot_light;

        Vec4F32 position_ws = {0,0,0,1};
        MemoryCopy(&position_ws, &node->fixed_position, sizeof(Vec3F32));
        Vec4F32 position_vs = transform_4x4f32(view_m, position_ws);

        Vec4F32 direction_ws = {0,0,0,0};
        MemoryCopy(&direction_ws, &light_src->direction, sizeof(Vec3F32));
        direction_ws = transform_4x4f32(node->fixed_xform, direction_ws);
        Vec4F32 direction_vs = transform_4x4f32(view_m, direction_ws);

        Vec4F32 color = {0,0,0, 1};
        MemoryCopy(&color, &light_src->color, sizeof(Vec3F32));

        Vec4F32 attenuation = {0};
        MemoryCopy(&attenuation, &light_src->attenuation, sizeof(Vec3F32));

        light_dst->kind = R_Vulkan_Light3DKind_Spot;
        light_dst->direction_ws = direction_ws;
        light_dst->direction_vs = direction_vs;
        light_dst->position_ws = position_ws;
        light_dst->position_vs = position_vs;
        light_dst->spot_angle = light_src->angle;
        light_dst->range = light_src->range;
        light_dst->color = color;
        light_dst->intensity = light_src->intensity;
        light_dst->attenuation = attenuation;
      }

      // Animation Player progress
      if(node->type_flags & RK_NodeTypeFlag_AnimationPlayer)
      {
        RK_AnimationPlayer *anim_player = node->animation_player;
        // TODO(XXX): test for now, play the first animation
        if(rk_handle_is_zero(anim_player->playback.curr))
        {
          anim_player->playback.curr = anim_player->animations[0]; 
          anim_player->playback.loop = 1;
        }

        // play animation if there is any
        if(!rk_handle_is_zero(anim_player->playback.curr))
        {
          anim_player->playback.pos += rk_state->frame_dt;
          RK_Animation *animation = (RK_Animation*)rk_res_unwrap(rk_res_from_handle(&anim_player->playback.curr));
          if(anim_player->playback.pos > animation->duration_sec && anim_player->playback.loop)
          {
            anim_player->playback.pos = mod_f32(anim_player->playback.pos, animation->duration_sec);
          }
          if(animation)
          {
            F32 pos = anim_player->playback.pos;
            for(U64 i = 0; i < animation->track_count; i++)
            {
              RK_Track *track = &animation->tracks[i];
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

      if(node->type_flags & RK_NodeTypeFlag_Drawable)
      {
        if(drawable_count == drawable_cap)
        {
          RK_Node **new = push_array(scratch.arena, RK_Node*, drawable_cap*2);
          MemoryCopy(new, nodes_to_draw, sizeof(RK_Node*)*drawable_cap);
          drawable_cap*=2;
          nodes_to_draw = new;
        }
        nodes_to_draw[drawable_count++] = node;
      }

      if(node->flags & RK_NodeFlag_Transient)
      {
        RK_NodeBucket *node_bucket = node->owner_bucket;
        SLLStackPush_N(node_bucket->first_to_free_node, node, free_next);
      }

      // pop stacks
      {
        // TODO(k): we may have some stacks here
      }
      node = rec.next;
    }
    ProfEnd();
    AssertAlways(rk_top_node_bucket()->node_count == node_count);

    // sorting if needed
    qsort(nodes_to_draw, drawable_count, sizeof(RK_Node*), rk_node2d_cmp_z_rev);

    // drawing
    ProfBegin("main drawing");
    for(U64 i = 0; i < drawable_count; i++)
    {
      RK_Node *node = nodes_to_draw[i];

      // draw sprite2d
      D_BucketScope(geo2d_bucket) if(node->type_flags & RK_NodeTypeFlag_Sprite2D)
      {
        // TODO(XXX): since we are doing dynamic drawlist, gpu instancing will not work, maybe we should create or reuse the rect pass

        RK_Transform2D *transform2d = &node->node2d->transform;
        RK_Sprite2D *sprite = node->sprite2d;
        RK_Texture2D *tex2d = rk_tex2d_from_handle(&sprite->tex);
        R_Handle tex = tex2d ? tex2d->tex : r_handle_zero();

        /////////////////////////////////////////////////////////////////////////////////
        // draw mesh 2d

        RK_DrawNode *n = 0;
        Mat4x4F32 xform = {0};
        Mat4x4F32 xform_inv = {0};

        switch(sprite->shape)
        {
          case RK_Sprite2DShapeKind_Rect:
          {
            Vec2F32 half_size = scale_2f32(sprite->size.rect, 0.5);
            // NOTE(k): don't add pos here, we are using xform to do translation
            Rng2F32 dst = rk_rect_from_sprite2d(sprite, v2f32(0,0));
            Rng2F32 src = r2f32p(0,0, 1,1);
            n = rk_drawlist_push_rect(rk_frame_arena(), rk_frame_drawlist(), dst, src);
            xform = node->fixed_xform;
            xform_inv = inverse_4x4f32(xform);
          }break;
          case RK_Sprite2DShapeKind_Circle:
          {
            F32 radius = sprite->size.circle.radius;
            n = rk_drawlist_push_circle(rk_frame_arena(), rk_frame_drawlist(), radius, 69);
            xform = make_rotate_4x4f32(v3f32(1,0,0), -0.25);
            xform = mul_4x4f32(node->fixed_xform, xform);
            xform_inv = inverse_4x4f32(xform);
          }break;
          default:{InvalidPath;}break;
        }

        R_Mesh2DInst *inst = d_sprite(n->vertices, n->indices, n->vertices_buffer_offset, n->indices_buffer_offset, n->indice_count, n->topology, n->polygon, 0, tex, 1.);
        inst->key = node->key.u64[0];
        inst->xform = xform;
        inst->xform_inv = xform_inv;
        inst->has_texture = !sprite->omit_texture;
        inst->has_color = sprite->omit_texture;
        inst->color = sprite->color;
        inst->draw_edge = sprite->draw_edge;

        // if(rk_key_match(scene->hot_key, node->key) && (node->flags & RK_NodeFlag_DrawHotEffects))
        // {
        //   RK_DrawNode *n = rk_drawlist_push_rect(rk_frame_arena(), rk_frame_drawlist(), dst, src, v4f32(1,0,0,1.));

        //   R_Mesh2DInst *inst = d_sprite(n->vertices, n->indices, n->vertices_buffer_offset, n->indices_buffer_offset, n->indice_count, R_GeoTopologyKind_Triangles, R_GeoPolygonKind_Line, 0, r_handle_zero(), 3.);
        //   inst->key = node->key.u64[0];
        //   inst->xform = xform;
        //   inst->xform_inv = xform_inv;
        //   inst->has_texture = 0;

        //   // n->color.w = mix_1f32(0., 0.1, node->hot_t);
        // }
      }

      // animated sprite2d
      D_BucketScope(geo2d_bucket) if(node->type_flags & RK_NodeTypeFlag_AnimatedSprite2D)
      {
        RK_Transform2D *transform2d = &node->node2d->transform;
        RK_AnimatedSprite2D *sprite = node->animated_sprite2d;
        RK_SpriteSheet *sheet = (RK_SpriteSheet*)rk_res_unwrap(rk_res_from_handle(&sprite->sheet));

        /////////////////////////////////////////////////////////////////////////////////
        // draw mesh

        // animation
        if(sprite->is_animating)
        {
          sprite->ts_ms += rk_state->frame_dt;
        }

        RK_SpriteSheetTag *tag = 0;
        while(tag == 0)
        {
          RK_SpriteSheetTag *curr_tag = &sheet->tags[sprite->curr_tag];

          if(sprite->ts_ms > curr_tag->duration)
          {
            if(!sprite->loop)
            {
              sprite->curr_tag = sprite->next_tag;
            }
            else
            {
              sprite->ts_ms = fmod(sprite->ts_ms, curr_tag->duration);
            }
          }
          else
          {
            tag = curr_tag;
            break;
          }
        }

        RK_SpriteSheetFrame *frame;
        F32 frame_duration_acc = 0;

        // find the frame
        // TODO(XXX): linear search for now, we could do better
        for(U64 i = tag->from; i <= tag->to; i++)
        {
          frame_duration_acc += sheet->frames[i].duration;
          if(sprite->ts_ms < frame_duration_acc)
          {
            frame = &sheet->frames[i];
            break;
          }
        }
        AssertAlways(frame != 0);

        Rng2F32 dst = {0};
        dst.x1 = sprite->size.x;
        dst.y1 = sprite->size.y;
        Rng2F32 src = {0};
        src.x0 = frame->x / sheet->size.x;
        src.y0 = frame->y / sheet->size.y;
        src.x1 = (frame->x+frame->w) / sheet->size.x;
        src.y1 = (frame->y+frame->h) / sheet->size.y;
        if(sprite->flipped)
        {
          // TODO(XXX): won't work, fix it later
          // Swap(F32, dst.x0, dst.x1);
        }

        RK_DrawNode *n = rk_drawlist_push_rect(rk_frame_arena(), rk_frame_drawlist(), dst, src);
        R_Mesh2DInst *inst = d_sprite(n->vertices, n->indices, n->vertices_buffer_offset, n->indices_buffer_offset, n->indice_count, R_GeoTopologyKind_Triangles, R_GeoPolygonKind_Fill, 0, rk_tex2d_from_handle(&sheet->tex)->tex, 1.);
        inst->key = node->key.u64[0];
        inst->xform = node->fixed_xform;
        inst->xform_inv = inverse_4x4f32(node->fixed_xform);
        inst->has_texture = 1;
        inst->has_color = 0;
      }

      // Draw mesh(3d)
      D_BucketScope(geo3d_back_bucket) if(node->type_flags & RK_NodeTypeFlag_MeshInstance3D)
      {
        RK_MeshInstance3D *mesh_inst3d = node->mesh_inst3d;
        RK_Mesh *mesh = rk_mesh_from_handle(&mesh_inst3d->mesh);
        RK_Skin *skin = rk_skin_from_handle(&mesh_inst3d->skin);
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

        /////////////////////////////////////////////////////////////////////////////////
        // unpack & upload material

        U64 mat_idx = 0; // 0 is the default material
        if(camera->viewport_shading == RK_ViewportShadingKind_Material)
        {
          RK_Material *mat = rk_mat_from_handle(&mesh_inst3d->material_override);
          // load mesh material if no override is provided
          if(mat == 0)
          {
            mat = rk_mat_from_handle(&mesh->material);
          }

          if(mat)
          {
            U64 mat_key = (U64)mat;
            U64 slot_idx = mat_key % MAX_MATERIALS_PER_PASS;
            U64 *ret = 0;
            for(HashNode *n = material_slots->first; n != 0; n = n->hash_next)
            {
              if(n->key == mat_key)
              {
                ret = &n->value.u64;
                break;
              }
            }

            // upload material if no cache is found
            if(ret == 0)
            {
              RK_Material *mat_src = mat;
              R_Material3D *mat_dst = &materials[*material_count];

              // copy src to dst
              // TODO(k): we could set global ambient here based on the settings of scene
              MemoryCopy(mat_dst, &mat_src->v, sizeof(R_Material3D));
              for(U64 kind = 0; kind < R_GeoTexKind_COUNT; kind++)
              {
                RK_Texture2D *tex = rk_tex2d_from_handle(&mat_src->textures[kind]);
                if(tex) textures[*material_count].array[kind] = tex->tex;
              }

              // cache it
              HashNode *hash_node = push_array(rk_frame_arena(), HashNode, 1);
              hash_node->key = mat_key;
              hash_node->value.u64 = *material_count;
              DLLPushBack_NP(material_slots[slot_idx].first, material_slots[slot_idx].last, hash_node, hash_next, hash_prev);
              (*material_count)++;

              ret = &hash_node->value.u64;
            }
            mat_idx = *ret;
          }
        }

        /////////////////////////////////////////////////////////////////////////////////
        // draw mesh

        R_Mesh3DInst *inst = d_mesh(mesh->vertices, mesh->indices, 0,0, mesh->indice_count,
                                    mesh->topology, polygon_mode,
                                    R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                    joint_xforms, joint_count,
                                    mat_idx, 1, 0);

        // fill inst info 
        inst->xform      = node->fixed_xform;
        inst->xform_inv  = inverse_4x4f32(node->fixed_xform);
        inst->omit_light = mesh_inst3d->omit_light || scene->omit_light;
        inst->key        = node->key.u64[0];
        inst->draw_edge  = rk_node_is_active(node) || mesh_inst3d->draw_edge;
        inst->depth_test = !mesh_inst3d->omit_depth_test;
      }

    }
    ProfEnd();
  }

  // NOTE(k): there could be ui elements within node update
  rk_state->sig = ui_signal_from_box(overlay);

  ///////////////////////////////////////////////////////////////////////////////////////
  // Update hot/active key

  scene->hot_key = rk_key_make(rk_state->hot_pixel_key, 0);
  if(rk_state->sig.f & UI_SignalFlag_LeftPressed)
  {
    scene->active_key = scene->hot_key;
    rk_scene_active_node_set(scene, scene->active_key, 1);
  }
  if(rk_state->sig.f & UI_SignalFlag_LeftReleased)
  {
    scene->active_key = rk_key_zero();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // handle cursor (hide/show/wrap)

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

  ///////////////////////////////////////////////////////////////////////////////////////
  // draw gizmo3d

  if(!scene->omit_gizmo3d)
  {
    if(active_node)
    {
      D_BucketScope(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front])
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
        rk_ijk_from_xform(gizmo_xform_tr, &i_hat, &j_hat, &k_hat);

        Vec3F32 eye = camera_node->position;
        Vec3F32 ray_eye_to_mouse_end;
        Vec3F32 ray_eye_to_gizmo = normalize_3f32(sub_3f32(gizmo_origin, eye));
        Vec3F32 ray_eye_to_center;
        {
          // ray_eye_to_mouse_end
          {
            // mouse ndc pos
            F32 mox_ndc = (rk_state->cursor.x / rk_state->window_dim.x) * 2.f - 1.f;
            F32 moy_ndc = (rk_state->cursor.y / rk_state->window_dim.y) * 2.f - 1.f;
            // unproject
            Vec4F32 ray_end = transform_4x4f32(proj_view_inv_m, v4f32(mox_ndc, moy_ndc, 1.f, 1.f));
            ray_end = scale_4f32(ray_end, 1.f/ray_end.w);
            ray_eye_to_mouse_end = xyz_from_v4f32(ray_end);
          }

          // ray_eye_center
          {
            // unproject
            Vec4F32 ray_end = transform_4x4f32(proj_view_inv_m, v4f32(0, 0, 1.f, 1.f));
            ray_end = scale_4f32(ray_end, 1.f/ray_end.w);
            ray_eye_to_center = normalize_3f32(xyz_from_v4f32(ray_end));
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
          case RK_Gizmo3DMode_Translate:
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
                  if(active_node->type_flags & RK_NodeTypeFlag_Particle3D)
                  {
                    MemoryCopy(&active_node->particle3d->v.x, &active_node->node3d->transform.position, sizeof(Vec3F32));
                    // zero out velocity, since we are controlling it
                    MemoryZeroStruct(&active_node->particle3d->v.v);
                  }
                }

                // draw axis to indicate direction
                rk_gizmo3d_line(rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), axis_clr, 3*linew_scale);
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
                rk_gizmo3d_plane_filled(drag_plane_key, origin, axises[(i+2)%3], axis, size, plane_clr, 1);
              }

              // curr axis
              rk_gizmo3d_line(axis_key, gizmo_origin, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis_clr, 6*linew_scale);
              // curr axis tip
              rk_gizmo3d_cone_filled(axis_key, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis, 0.1*scale_t, 0.3*scale_t, axis_clr, 1);
            }

            // draw a ball and circle in the middle
            rk_gizmo3d_sphere_filled(rk_key_zero(), gizmo_origin, axis_length*0.1, axis_length*0.2, 0, v4f32(0,0,0,0.9), 1);
            rk_gizmo3d_circle_lined(rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*0.11, v4f32(1,1,1,0.9), linew_scale*6, 1); // billboard fashion

            // draw a outer circle
            rk_gizmo3d_circle_lined(rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*1.3, v4f32(1,1,1,1), linew_scale*6, 1);

          }break;
          case RK_Gizmo3DMode_Rotation:
          {
            F32 ring_radius = 1*scale_t;
            U64 ring_segments = 69;
            F32 ring_line_width = 9*linew_scale;
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

                rk_gizmo3d_line(rk_key_make(0,0), intersect, gizmo_origin, v4f32(1,0,0,1), 5.f*linew_scale);
                rk_gizmo3d_line(rk_key_make(0,0), drag_data->anchor, gizmo_origin, v4f32(0,1,0,1), 5.f*linew_scale);

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
                  rk_gizmo3d_arc_filled(rk_key_zero(), gizmo_origin, drag_data->anchor, intersect, v4f32(0,0,1,0.6), 1);

                  // draw axis
                  rk_gizmo3d_line(rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length)), axis_clr, 6.f*linew_scale);
                }

                // change active node rotation
                if(turn != 0)
                {
                  QuatF32 quat = make_rotate_quat_f32(axis, turn);
                  active_node->node3d->transform.rotation = mul_quat_f32(quat, drag_data->start_rot);
                }
              }

              // draw rotation circle to indicate which axis is rotating
              rk_gizmo3d_circle_lined(axis_key, gizmo_origin, axis, ring_radius, axis_clr, linew_scale*6, draw_edge);
            }

            // draw a cube in the inner middle
            // rk_drawlist_push_box_filled(rk_frame_arena(), rk_frame_drawlist(), rk_key_zero(), scale_3f32(v3f32(1,1,1), axis_length*0.1), gizmo_origin, axises[1], v4f32(1,1,1,1), 1);
            // draw a white ball in the middle (NOTE(k): it's semi transparency, so we need draw it last)
            // rk_drawlist_push_sphere(rk_frame_arena(), rk_frame_drawlist(), rk_key_zero(), gizmo_origin, ring_radius*0.9, ring_radius*0.9*2, ring_segments, 19, 0, v4f32(0,0,0,0.6), 0, 1);

            // draw a outer circle
            rk_gizmo3d_circle_lined(rk_key_zero(), gizmo_origin, normalize_3f32(sub_3f32(eye, gizmo_origin)), axis_length*1.3, v4f32(0,0,0,0.1), linew_scale*6, 1);
          }break;
          case RK_Gizmo3DMode_Scale:
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
                rk_gizmo3d_line(rk_key_zero(), sub_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), add_3f32(gizmo_origin, scale_3f32(axis, axis_length*9)), axis_clr, 6.f*linew_scale);
                // draw a cube to indicate curr scale pos(projected)
                Vec4F32 box_clr = axis_clr;
                box_clr.w = 1.0f;
                rk_gizmo3d_box_filled(axis_key, add_3f32(gizmo_origin, scale_3f32(axis, dist)), i_hat, j_hat, scale_3f32(v3f32(1,1,1), axis_length*0.1), box_clr, draw_edge);
              }

              // curr axis
              rk_gizmo3d_line(axis_key, gizmo_origin, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), axis_clr, 19*linew_scale);
              // curr axis tip
              Vec4F32 tip_clr = scale_4f32(axis_clr,0.5);
              tip_clr.w = axis_clr.w;
              rk_gizmo3d_box_filled(axis_key, add_3f32(gizmo_origin,scale_3f32(axis, axis_length)), i_hat, j_hat, scale_3f32(v3f32(1,1,1),axis_length*0.19), tip_clr, draw_edge);
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
                rk_gizmo3d_line(rk_key_zero(), gizmo_origin, intersect, v4f32(1,1,1,0.9), 3*linew_scale);

                // scale active_node along all axises
                if(scale != 0)
                {
                  active_node->node3d->transform.scale = scale_3f32(drag_data->start_scale, scale);
                }
              }

              // draw middle cube
              rk_gizmo3d_box_filled(origin_key, gizmo_origin, i_hat, j_hat, scale_3f32(v3f32(1,1,1),axis_length*0.2), clr, draw_edge);
            }
          }break;
          default:{InvalidPath;}break;
        }
      }
    }

    // draw lights gizmos
    D_BucketScope(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front])
    {
      RK_Node *node = rk_node_from_handle(&scene->root);
      while(node != 0)
      {
        RK_NodeRec rec = rk_node_df_pre(node, 0, 0);

        if(node->type_flags & RK_NodeTypeFlag_SpotLight)
        {
          RK_SpotLight *light = node->spot_light;
          Vec3F32 pos = node->fixed_position;
          Vec3F32 dir = mul_quat_f32_v3f32(node->fixed_rotation, light->direction);

          float height = light->range*0.1;
          Vec3F32 origin = add_3f32(pos, scale_3f32(dir, height));
          F32 radius = tanf(light->angle) * height;
          // RK_DrawNode *draw_node = rk_drawlist_push_cone(rk_frame_arena(), rk_frame_drawlist(), rk_key_zero(),
          //     origin, negate_3f32(dir), radius, height, 39, v4f32(1,1,1,0.3), 1, 1);
          // draw_node->polygon = R_GeoPolygonKind_Line;
        }
        node = rec.next;
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // draw ui

  ui_end_build();
  ProfScope("draw ui") D_BucketScope(rk_state->bucket_rect)
  {
    rk_ui_draw();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // do physics

  // particle system
  // TODO(XXX): we could use fixed step to do physics
  // ph_step_ps(&scene->particle3d_system, rk_state->frame_dt);
  // ph_step_rs(&scene->rigidbody3d_system, rk_state->frame_dt);

  ///////////////////////////////////////////////////////////////////////////////////////
  // end of frame

  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_scene();
  rk_pop_handle_seed();

  ///////////////////////////////////////////////////////////////////////////////////////
  // concate draw buckets

  D_Bucket *bucket = d_bucket_make();
  ProfBegin("bucket making");
  D_BucketScope(bucket)
  {
    // NOTE(k): check if there is anything in passes, we don't a empty geo pass (pass is not cheap)
    if(!d_bucket_is_empty(rk_state->bucket_geo[RK_GeoBucketKind_Geo2D]))       {d_sub_bucket(rk_state->bucket_geo[RK_GeoBucketKind_Geo2D], 0);}
    // TODO(XXX): only for test, make it composiable
    d_edge(rk_state->time_in_seconds);
    if(!d_bucket_is_empty(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Back]))  {d_sub_bucket(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Back], 0);}
    if(!d_bucket_is_empty(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front])) {d_sub_bucket(rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front], 0);}
    if(!d_bucket_is_empty(rk_state->bucket_rect))                              {d_sub_bucket(rk_state->bucket_rect, 0);}
    d_crt(0.25, 1.15, rk_state->time_in_seconds);
    d_noise(r2f32p(0,0,0,0), rk_state->time_in_seconds);
  }
  ProfEnd();

  ///////////////////////////////////////////////////////////////////////////////////////
  // submit work

  // build frame drawlist before we submit draw bucket
  ProfScope("draw drawlist")
  {
    rk_drawlist_build(rk_frame_drawlist());
  }

  rk_state->pre_cpu_time_us = os_now_microseconds()-begin_time_us;

  // submit
  ProfScope("submit")
  {
    r_begin_frame();
    r_window_begin_frame(rk_state->os_wnd, rk_state->r_wnd);
    d_submit_bucket(rk_state->os_wnd, rk_state->r_wnd, bucket);
    rk_state->hot_pixel_key = r_window_end_frame(rk_state->r_wnd, rk_state->cursor);
    r_end_frame();
  }
  ///////////////////////////////////////////////////////////////////////////////////////
  // wait if we still have some cpu time left

  // TODO: use cpu sleep instead of spinning cpu here
  U64 frame_time_target_cap_us = (U64)(1000000/target_hz);
  U64 work_us = os_now_microseconds()-begin_time_us;
  rk_state->cpu_time_us = work_us;
  while(work_us < frame_time_target_cap_us)
  {
    work_us = os_now_microseconds()-begin_time_us;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // determine frame time, record it into history

  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  rk_state->frame_time_us_history[rk_state->frame_index%ArrayCount(rk_state->frame_time_us_history)] = frame_time_us;

  ///////////////////////////////////////////////////////////////////////////////////////
  // bump frame time counters

  rk_state->frame_index++;
  rk_state->time_in_seconds += rk_state->frame_dt;
  rk_state->time_in_us += frame_time_us;

  ///////////////////////////////////////////////////////////////////////////////////////
  // end

  scratch_end(scratch);
  return !rk_state->window_should_close;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Dynamic drawing api (in immediate mode fashion)

internal RK_DrawList *
rk_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap)
{
  RK_DrawList *ret = push_array(arena, RK_DrawList, 1);
  ret->vertex_buffer_cap = vertex_buffer_cap;
  ret->indice_buffer_cap = indice_buffer_cap;
  ret->vertices = r_buffer_alloc(R_ResourceKind_Stream, vertex_buffer_cap, 0, 0);
  ret->indices = r_buffer_alloc(R_ResourceKind_Stream, indice_buffer_cap, 0, 0);
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push(Arena *arena, RK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count)
{
  RK_DrawNode *ret = push_array(arena, RK_DrawNode, 1);

  U64 v_buf_size = sizeof(R_Vertex) * vertex_count;
  U64 i_buf_size = sizeof(U32) * indice_count;
  // TODO(XXX): we should use buffer block, we can't just release/realloc a new buffer, since the buffer handle is already handled over to RK_DrawNode
  AssertAlways(v_buf_size + drawlist->vertex_buffer_cmt <= drawlist->vertex_buffer_cap);
  AssertAlways(i_buf_size + drawlist->indice_buffer_cmt <= drawlist->indice_buffer_cap);

  U64 v_buf_offset = drawlist->vertex_buffer_cmt;
  U64 i_buf_offset = drawlist->indice_buffer_cmt;

  drawlist->vertex_buffer_cmt += v_buf_size;
  drawlist->indice_buffer_cmt += i_buf_size;

  // fill info
  ret->vertices_src = vertices_src;
  ret->vertex_count = vertex_count;
  ret->vertices = drawlist->vertices;
  ret->vertices_buffer_offset = v_buf_offset;
  ret->indices_src = indices_src;
  ret->indice_count = indice_count;
  ret->indices = drawlist->indices;
  ret->indices_buffer_offset = i_buf_offset;

  SLLQueuePush(drawlist->first_node, drawlist->last_node, ret);
  drawlist->node_count++;
  return ret;
}

internal void
rk_drawlist_build(RK_DrawList *drawlist)
{
  Temp scratch = scratch_begin(0,0);
  R_Vertex *vertices = (R_Vertex*)push_array(scratch.arena, U8, drawlist->vertex_buffer_cmt);
  U32 *indices = (U32*)push_array(scratch.arena, U8, drawlist->indice_buffer_cmt);

  // collect buffer
  R_Vertex *vertices_dst = vertices;
  U32 *indices_dst = indices;
  for(RK_DrawNode *n = drawlist->first_node; n != 0; n = n->next)  
  {
    MemoryCopy(vertices_dst, n->vertices_src, sizeof(R_Vertex)*n->vertex_count);
    MemoryCopy(indices_dst, n->indices_src, sizeof(U32)*n->indice_count);

    vertices_dst += n->vertex_count; 
    indices_dst += n->indice_count;
  }

  r_buffer_copy(drawlist->vertices, vertices, drawlist->vertex_buffer_cmt);
  r_buffer_copy(drawlist->indices, indices, drawlist->indice_buffer_cmt);
  scratch_end(scratch);
}

internal void
rk_drawlist_reset(RK_DrawList *drawlist)
{
  // clear per-frame data
  drawlist->first_node = 0;
  drawlist->last_node = 0;
  drawlist->node_count = 0;
  drawlist->vertex_buffer_cmt = 0;
  drawlist->indice_buffer_cmt = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Helpers

internal void
rk_trs_from_xform(Mat4x4F32 *m, Vec3F32 *trans, QuatF32 *rot, Vec3F32 *scale)
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
rk_ijk_from_xform(Mat4x4F32 m, Vec3F32 *i, Vec3F32 *j, Vec3F32 *k)
{
  // NOTE(K): don't apply translation, hence 0 for w
  Vec4F32 i_hat = {1,0,0,0};
  Vec4F32 j_hat = {0,1,0,0};
  Vec4F32 k_hat = {0,0,1,0};
  i_hat = transform_4x4f32(m, i_hat);
  j_hat = transform_4x4f32(m, j_hat);
  k_hat = transform_4x4f32(m, k_hat);

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

internal Rng2F32
rk_rect_from_sprite2d(RK_Sprite2D *sprite2d, Vec2F32 pos)
{
  Rng2F32 ret = {0};
  Vec2F32 rect_size = sprite2d->size.rect;
  Vec2F32 half_size = scale_2f32(rect_size, 0.5);
  switch(sprite2d->anchor)
  {
    case RK_Sprite2DAnchorKind_TopLeft:
    {
      ret = (Rng2F32){0,0, rect_size.x, rect_size.y};
    }break;
    case RK_Sprite2DAnchorKind_TopRight:
    {
      ret = (Rng2F32){-rect_size.x, 0,0, rect_size.y};
    }break;
    case RK_Sprite2DAnchorKind_BottomLeft:
    {
      ret = (Rng2F32){0, -rect_size.y, rect_size.x, 0};
    }break;
    case RK_Sprite2DAnchorKind_BottomRight:
    {
      ret = (Rng2F32){-rect_size.x, -rect_size.y, 0,0};
    }break;
    case RK_Sprite2DAnchorKind_Center:
    {
      ret = (Rng2F32){-half_size.x,-half_size.y, half_size.x, half_size.y};
    }break;
    default:{InvalidPath;}break;
  }
  ret.p0 = add_2f32(pos, ret.p0);
  ret.p1 = add_2f32(pos, ret.p1);
  return ret;
}

internal Vec2F32
rk_center_from_sprite2d(RK_Sprite2D *sprite2d, Vec2F32 pos)
{
  Vec2F32 ret = pos;
  Vec2F32 rect_size = sprite2d->size.rect;
  Vec2F32 half_size = scale_2f32(rect_size, 0.5);

  switch(sprite2d->anchor)
  {
    case RK_Sprite2DAnchorKind_TopLeft:
    {
      ret.x += half_size.x;
      ret.y += half_size.y;
    }break;
    case RK_Sprite2DAnchorKind_TopRight:
    {
      ret.x -= half_size.x;
      ret.y += half_size.y;
    }break;
    case RK_Sprite2DAnchorKind_BottomLeft:
    {
      ret.x += half_size.x;
      ret.y -= half_size.y;
    }break;
    case RK_Sprite2DAnchorKind_BottomRight:
    {
      ret.x -= half_size.x;
      ret.y -= half_size.y;
    }break;
    case RK_Sprite2DAnchorKind_Center: {/*noop*/}break;
    default:{InvalidPath;}break;
  }
  return ret;
}

// TODO: revisiting is needed
internal void
rk_sprite2d_equip_string(Arena *arena,
                         RK_Sprite2D *sprite2d,
                         String8 string,
                         F_Tag font,
                         F32 font_size,
                         Vec4F32 font_color,
                         U64 tab_size,
                         F_RasterFlags text_raster_flags)
{
  D_FancyStringNode fancy_string_n = {0};
  fancy_string_n.next = 0;
  fancy_string_n.v.font                    = font;
  fancy_string_n.v.string                  = string;
  fancy_string_n.v.color                   = font_color;
  fancy_string_n.v.size                    = font_size;
  fancy_string_n.v.underline_thickness     = 0;
  fancy_string_n.v.strikethrough_thickness = 0;

  D_FancyStringList fancy_strings = {0};
  fancy_strings.first = &fancy_string_n;
  fancy_strings.last = &fancy_string_n;
  fancy_strings.node_count = 1;

  sprite2d->string            = string;
  sprite2d->font              = font;
  sprite2d->font_size         = font_size;
  sprite2d->font_color        = font_color;
  sprite2d->text_raster_flags = text_raster_flags;
  sprite2d->fancy_run_list    = d_fancy_run_list_from_fancy_string_list(arena, tab_size, text_raster_flags, &fancy_strings);
}

internal int
rk_node2d_cmp_z_rev(const void *a, const void *b)
{
  int ret = 0;
  RK_Node **left = (RK_Node**)a;
  RK_Node **right = (RK_Node**)b;
  if(((*left)->type_flags & RK_NodeTypeFlag_Node2D) &&((*right)->type_flags & RK_NodeTypeFlag_Node2D))
  {
    ret = (*right)->node2d->z_index - (*left)->node2d->z_index;
  }
  return ret;
}

internal int
rk_path_cmp(const void *left_, const void *right_)
{
  String8 left = *(String8*)left_;
  String8 right = *(String8*)right_;
  return rk_no_from_filename(str8_skip_last_slash(left))-rk_no_from_filename(str8_skip_last_slash(right));
}

internal U64
rk_no_from_filename(String8 filename)
{
  U64 ret = 0;
  U64 start = str8_find_needle(filename, 0, str8_lit("_"), 0);
  start++;
  U64 end = str8_find_needle(filename, 0, str8_lit("."), 0);
  String8 no_str = str8_substr(filename, r1u64(start, end));
  ret = u64_from_str8(no_str, 10);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Enum to string

internal String8
rk_string_from_projection_kind(RK_ProjectionKind kind)
{
  String8 ret = {0};
  switch(kind)
  {
    case RK_ProjectionKind_Perspective:  {ret = str8_lit("perspective"); }break;
    case RK_ProjectionKind_Orthographic: {ret = str8_lit("orthographic"); }break;
    default:                             {InvalidPath; }break;
  }
  return ret;
}

internal String8
rk_string_from_viewport_shading_kind(RK_ViewportShadingKind kind)
{

  String8 ret = {0};
  switch(kind)
  {
    case RK_ViewportShadingKind_Solid:     {ret = str8_lit("solid"); }break;
    case RK_ViewportShadingKind_Wireframe: {ret = str8_lit("wireframe"); }break;
    case RK_ViewportShadingKind_Material:  {ret = str8_lit("material"); }break;
    default:                               {InvalidPath; }break;
  }
  return ret;
}

internal String8
rk_string_from_polygon_kind(RK_ViewportShadingKind kind)
{
  String8 ret = {0};
  switch(kind)
  {
    case R_GeoPolygonKind_Line:  {ret = str8_lit("line"); }break;
    case R_GeoPolygonKind_Point: {ret = str8_lit("point"); }break;
    case R_GeoPolygonKind_Fill:  {ret = str8_lit("fill"); }break;
    default:                     {InvalidPath; }break;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Scene creation and destruction

internal RK_Scene *
rk_scene_alloc()
{
  Arena *arena = arena_alloc();
  RK_Scene *ret = push_array(arena, RK_Scene, 1);
  {
    ret->arena           = arena;
    ret->node_bucket     = rk_node_bucket_make(arena, 4096-1);
    ret->res_node_bucket = rk_node_bucket_make(arena, 4096-1);
    ret->res_bucket      = rk_res_bucket_make(arena, 4096-1);
    ret->handle_seed     = (U64)rand();
  }
  return ret;
}

internal void
rk_scene_release(RK_Scene *s)
{
  rk_res_bucket_release(s->res_bucket);
  arena_release(s->arena);
}

internal void
rk_scene_active_camera_set(RK_Scene *s, RK_Node *camera_node)
{
  RK_Node *old_camera = rk_node_from_handle(&s->active_camera);
  if(old_camera != 0 && old_camera != camera_node)
  {
    old_camera->camera3d->is_active = 0;
  }
  s->active_camera = rk_handle_from_node(camera_node);
}

internal void
rk_scene_active_node_set(RK_Scene *s, RK_Key key, B32 only_navigation_root)
{
  RK_Node *node = rk_node_from_key(key);

  if(only_navigation_root)
  {
    for(; node != 0 && !(node->flags & RK_NodeFlag_NavigationRoot); node = node->parent) {}
  }

  if(node || rk_key_match(key, rk_key_zero()))
  {
    s->active_node = rk_handle_from_node(node);
  }
}

/////////////////////////////////
// Gizmo Drawing Functions

internal void
rk_gizmo3d_rect(RK_Key key, Rng2F32 dst, Rng2F32 src, R_GeoPolygonKind polygon, Vec4F32 clr, R_Handle albedo_tex, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_rect(arena, rk_frame_drawlist(), dst, src);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = !r_handle_match(albedo_tex, r_handle_zero());
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = albedo_tex;
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1, 1);

    inst->xform = mat_4x4f32(1.0);
    inst->xform_inv = inst->xform;
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_line(RK_Key key, Vec3F32 start, Vec3F32 end, Vec4F32 clr, F32 line_width)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_line(arena, rk_frame_drawlist(), start, end);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Lines, R_GeoPolygonKind_Line,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, line_width, 1);

    inst->xform = mat_4x4f32(1.0);
    inst->xform_inv = inst->xform;
    inst->key = key.u64[0];
    inst->draw_edge = 0;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_cone(RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, F32 height, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_cone(arena, rk_frame_drawlist(), radius, height, 30);

  // transform
  Mat4x4F32 xform_t = make_translate_4x4f32(origin);
  Mat4x4F32 xform_r;
  {
    Vec3F32 j_hat = v3f32(0,-1,0);
    F32 dot = dot_3f32(j_hat, normal);
    // same direction
    if(dot == 1)
    {
      xform_r = mat_4x4f32_from_quat_f32(make_indentity_quat_f32());
    }
    // reverse direction
    else if(dot == -1)
    {
      xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5));
    }
    else
    {
      Vec3F32 cross = cross_3f32(j_hat, normal);
      F32 turn = turns_from_radians_f32(acosf(dot_3f32(normal, j_hat)));
      xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(cross, turn));
    }
  }
  Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1., 1);

    inst->xform = xform;
    inst->xform_inv = inverse_4x4f32(xform);
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_plane(RK_Key key, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec2F32 size, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_plane(arena, rk_frame_drawlist(), size, 1);

  // transform
  Mat4x4F32 xform_t = make_translate_4x4f32(origin);
  Mat4x4F32 xform_r = mat_4x4f32(1.0f);
  {
    Vec3F32 k_hat = cross_3f32(i_hat, j_hat);
    xform_r.v[0][0] = i_hat.x;
    xform_r.v[0][1] = i_hat.y;
    xform_r.v[0][2] = i_hat.z;

    xform_r.v[1][0] = j_hat.x;
    xform_r.v[1][1] = j_hat.y;
    xform_r.v[1][2] = j_hat.z;

    xform_r.v[2][0] = k_hat.x;
    xform_r.v[2][1] = k_hat.y;
    xform_r.v[2][2] = k_hat.z;
  }
  Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1., 1);

    inst->xform = xform;
    inst->xform_inv = inverse_4x4f32(xform);
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_sphere(RK_Key key, Vec3F32 origin, F32 radius, F32 height, B32 is_hemisphere, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_sphere(arena, rk_frame_drawlist(), radius, height, 10, 10, is_hemisphere);

  // transform
  // NOTE(k): don't need to rotate, it's sphere
  Mat4x4F32 xform = make_translate_4x4f32(origin);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1., 1);

    inst->xform = xform;
    inst->xform_inv = inverse_4x4f32(xform);
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_arc(RK_Key key, Vec3F32 origin, Vec3F32 start, Vec3F32 end, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_arc(arena, rk_frame_drawlist(), origin, start, end, 10, 1);

  // transform
  Mat4x4F32 xform = mat_4x4f32(1.0);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1., 1);

    inst->xform = xform;
    inst->xform_inv = xform;
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_circle_lined(RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, Vec4F32 clr, F32 line_width, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_circle(arena, rk_frame_drawlist(), radius, 30);

  // transform
  Mat4x4F32 xform_t = make_translate_4x4f32(origin);
  Mat4x4F32 xform_r;
  {
    Vec3F32 j_hat = v3f32(0,-1,0);
    Vec3F32 cross = cross_3f32(j_hat, normal);
    F32 turn = turns_from_radians_f32(acosf(dot_3f32(normal, j_hat)));
    xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(cross, turn));
  }
  Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Lines, R_GeoPolygonKind_Line,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, line_width, 1);

    inst->xform = xform;
    inst->xform_inv = inverse_4x4f32(xform);
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

internal void
rk_gizmo3d_box(RK_Key key, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec3F32 size, R_GeoPolygonKind polygon, Vec4F32 clr, B32 draw_edge)
{
  Arena *arena = rk_frame_arena();
  D_Bucket *draw_bucket = rk_state->bucket_geo[RK_GeoBucketKind_Geo3D_Front];
  RK_DrawNode *draw_node = rk_drawlist_push_box(arena, rk_frame_drawlist(), size);

  // transform
  Mat4x4F32 xform_t = make_translate_4x4f32(origin);
  Mat4x4F32 xform_r = mat_4x4f32(1.0f);
  {
    Vec3F32 k_hat = cross_3f32(i_hat, j_hat);
    xform_r.v[0][0] = i_hat.x;
    xform_r.v[0][1] = i_hat.y;
    xform_r.v[0][2] = i_hat.z;

    xform_r.v[1][0] = j_hat.x;
    xform_r.v[1][1] = j_hat.y;
    xform_r.v[1][2] = j_hat.z;

    xform_r.v[2][0] = k_hat.x;
    xform_r.v[2][1] = k_hat.y;
    xform_r.v[2][2] = k_hat.z;
  }
  Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

  // drawing
  D_BucketScope(draw_bucket)
  {
    R_Pass *pass = r_pass_from_kind(d_thread_ctx->arena, &draw_bucket->passes, R_PassKind_Geo3D, 1);
    R_PassParams_Geo3D *pass_params = pass->params_geo3d;

    U64 mat_idx = pass_params->material_count;
    pass_params->materials[mat_idx].diffuse_color = clr;
    pass_params->materials[mat_idx].opacity = 1.0;
    pass_params->materials[mat_idx].has_diffuse_texture = 0;
    pass_params->textures[mat_idx].array[R_GeoTexKind_Diffuse] = r_handle_zero();
    pass_params->material_count++;

    // NOTE(k): mesh group stored as hash map which don't contain the order, that's why we get flicker (we may need a submit ordered group list)
    R_Mesh3DInst *inst = d_mesh(draw_node->vertices, draw_node->indices,
                                draw_node->vertices_buffer_offset, draw_node->indices_buffer_offset, draw_node->indice_count,
                                R_GeoTopologyKind_Triangles, polygon,
                                R_GeoVertexFlag_TexCoord|R_GeoVertexFlag_Normals|R_GeoVertexFlag_RGB,
                                0, 0,
                                mat_idx, 1.0, 1);
    inst->xform = xform;
    inst->xform_inv = inverse_4x4f32(xform);
    inst->key = key.u64[0];
    inst->draw_edge = draw_edge;
    inst->depth_test = 1;
    inst->omit_light = 1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// OS events

////////////////////////////////
// event consumption helpers

internal void
rk_eat_event(OS_EventList *list, OS_Event *evt)
{
  DLLRemove(list->first, list->last, evt);
  list->count--;
}

internal B32
rk_key_press(OS_Modifiers mods, OS_Key key)
{
  B32 result = 0;
  for(OS_Event *evt = rk_state->os_events.first; evt != 0; evt = evt->next)
  {
    if(evt->kind == OS_EventKind_Press && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      rk_eat_event(&rk_state->os_events, evt);
      break;
    }
  }
  return result;
}
internal B32
rk_key_release(OS_Modifiers mods, OS_Key key)
{

  B32 result = 0;
  for(OS_Event *evt = rk_state->os_events.first; evt != 0; evt = evt->next)
  {
    if(evt->kind == OS_EventKind_Release && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      rk_eat_event(&rk_state->os_events, evt);
      break;
    }
  }
  return result;
}
