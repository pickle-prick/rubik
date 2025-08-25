#define IK_StackTopImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.top->v;

#define IK_StackBottomImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.bottom_val;

#define IK_StackPushImpl(state, name_upper, name_lower, type, new_value) \
IK_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(ik_state->arena, IK_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define IK_StackPopImpl(state, name_upper, name_lower) \
IK_##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
SLLStackPop(state->name_lower##_stack.top);\
SLLStackPush(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define IK_StackSetNextImpl(state, name_upper, name_lower, type, new_value) \
IK_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(ik_state->arena, IK_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;

#include "generated/ink.meta.c"

#define M_1 200

internal void
ik_ui_draw()
{
  Temp scratch = scratch_begin(0,0);
  F32 box_squish_epsilon = 0.001f;
  d_push_viewport(ik_state->window_dim);

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

    // push transparency
    if(box->transparency != 0)
    {
      d_push_transparency(box->transparency);
    }

    // push squish
    if(box->squish > box_squish_epsilon)
    {
      Vec2F32 box_dim = dim_2f32(box->rect);
      Vec2F32 anchor_off = {0};
      if(box->flags & UI_BoxFlag_SquishAnchored)
      {
        anchor_off.x = box_dim.x/2.0;
      }
      else
      {
        anchor_off.y = -box_dim.y/8.0;
      }

      Mat3x3F32 box2origin_xform = make_translate_3x3f32(v2f32(-box->rect.x0 - box_dim.x/2 + anchor_off.x, -box->rect.y0 + anchor_off.y));
      Mat3x3F32 scale_xform = make_scale_3x3f32(v2f32(1-box->squish, 1-box->squish));
      Mat3x3F32 origin2box_xform = make_translate_3x3f32(v2f32(box->rect.x0 + box_dim.x/2 - anchor_off.x, box->rect.y0 - anchor_off.y));
      Mat3x3F32 xform = mul_3x3f32(origin2box_xform, mul_3x3f32(scale_xform, box2origin_xform));
      // TODO: we may need to tranpose this
      d_push_xform2d(xform);
      d_push_tex2d_sample_kind(R_Tex2DSampleKind_Linear);
    }

    // draw drop_shadw
    if(box->flags & UI_BoxFlag_DrawDropShadow)
    {
      Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
      Vec4F32 drop_shadow_color = ik_rgba_from_theme_color(IK_ThemeColor_DropShadow);
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
          Vec4F32 color = ik_rgba_from_theme_color(IK_ThemeColor_Hover);
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
          Vec4F32 shadow_color = ik_rgba_from_theme_color(IK_ThemeColor_Hover);
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

    // draw image
    if(box->flags & UI_BoxFlag_DrawImage)
    {
      R_Rect2DInst *inst = d_img(box->rect, box->src, box->albedo_tex, box->albedo_clr, 0., 0., 0.);
      inst->white_texture_override = box->albedo_white_texture_override ? 1.0 : 0.0;
    }

    // draw string
    if(box->flags & UI_BoxFlag_DrawText)
    {
      // TODO(k): handle font color
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

    // push clip
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

    // call custom draw callback
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
            Vec4F32 color = ik_rgba_from_theme_color(IK_ThemeColor_Hover);
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
          Vec4F32 color = ik_rgba_from_theme_color(IK_ThemeColor_Focus);
          color.w *= 0.2f*b->focus_hot_t;
          R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 0.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: draw focus border
        if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
        {
          Vec4F32 color = ik_rgba_from_theme_color(IK_ThemeColor_Focus);
          color.w *= b->focus_active_t;
          R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: disabled overlay
        if(b->disabled_t >= 0.005f)
        {
          Vec4F32 color = ik_rgba_from_theme_color(IK_ThemeColor_DisabledOverlay);
          color.w *= b->disabled_t;
          R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1);
          MemoryCopyArray(inst->corner_radii, b->corner_radii);
        }

        // rjf: pop squish
        if(b->squish > box_squish_epsilon)
        {
          d_pop_xform2d();
          d_pop_tex2d_sample_kind();
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
  d_pop_viewport();
  scratch_end(scratch);
}

/////////////////////////////////
//~ Basic Type Functions

internal U64
ik_hash_from_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal String8
ik_hash_part_from_key_string(String8 string)
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
ik_display_part_from_key_string(String8 string)
{
  U64 hash_pos = str8_find_needle(string, 0, str8_lit("##"), 0);
  string.size = hash_pos;
  return string;
}

/////////////////////////////////
//~ Key

internal IK_Key
ik_key_from_string(IK_Key seed_key, String8 string)
{
  IK_Key result = {0};
  if(string.size != 0)
  {
    String8 hash_part = ik_hash_part_from_key_string(string);
    result.u64[0] = ik_hash_from_string(seed_key.u64[0], hash_part);
  }
  return result;
}

internal IK_Key
ik_key_from_stringf(IK_Key seed_key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);

  IK_Key result = {0};
  result = ik_key_from_string(seed_key, string);
  scratch_end(scratch);
  return result;
}

internal B32
ik_key_match(IK_Key a, IK_Key b)
{
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

internal IK_Key
ik_key_make(U64 a, U64 b)
{
  IK_Key result = {a,b};
  return result;
}

internal IK_Key
ik_key_zero()
{
  return (IK_Key){0}; 
}

internal IK_Key
ik_key_new()
{
  // TODO: we could use generation index here
  IK_Key ret = ik_key_make(os_now_microseconds(), 0);
  return ret;
}

/////////////////////////////////
//~ State accessor/mutator

//- init

internal void
ik_init(OS_Handle os_wnd, R_Handle r_wnd)
{
  Arena *arena = arena_alloc();
  ik_state = push_array(arena, IK_State, 1);
  {
    ik_state->arena = arena;
    ik_state->os_wnd = os_wnd;
    ik_state->r_wnd = r_wnd;
    ik_state->dpi = os_dpi_from_window(os_wnd);
    ik_state->last_dpi = ik_state->last_dpi;
    ik_state->window_rect = os_client_rect_from_window(os_wnd, 1);
    ik_state->window_dim = dim_2f32(ik_state->window_rect);
    ik_state->drag_state_arena = arena_alloc();
    ik_state->box_scratch_arena = arena_alloc();
    ik_state->tool = IK_ToolKind_Selection;

    // frame arena
    for(U64 i = 0; i < ArrayCount(ik_state->frame_arenas); i++)
    {
      ik_state->frame_arenas[i] = arena_alloc(.reserve_size = MB(128), .commit_size = MB(64));
    }
    // frame drawlists
    for(U64 i = 0; i < ArrayCount(ik_state->drawlists); i++)
    {
      // TODO(k): some device offers 256MB memory which is both cpu visiable and device local
      ik_state->drawlists[i] = ik_drawlist_alloc(arena, MB(16), MB(16));
    }
  }

  // Settings
  for EachEnumVal(IK_SettingCode, code)
  {
    ik_state->setting_vals[code] = ik_setting_code_default_val_table[code];
  }
  ik_state->setting_vals[IK_SettingCode_MainFontSize].s32 = ik_state->setting_vals[IK_SettingCode_MainFontSize].s32 * (ik_state->last_dpi/96.f);
  ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32 = ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32 * (ik_state->last_dpi/96.f);
  ik_state->setting_vals[IK_SettingCode_MainFontSize].s32 = ClampBot(ik_state->setting_vals[IK_SettingCode_MainFontSize].s32, ik_setting_code_default_val_table[IK_SettingCode_MainFontSize].s32);
  ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32 = ClampBot(ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32, ik_setting_code_default_val_table[IK_SettingCode_CodeFontSize].s32);

  // Fonts
  ik_state->cfg_font_tags[IK_FontSlot_Main]         = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_Code]         = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_Icons]        = f_tag_from_path(str8_lit("./fonts/icons.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_ToolbarIcons] = f_tag_from_path(str8_lit("./fonts/toolbar_icons.ttf"));

  // Theme 
  MemoryCopy(ik_state->cfg_theme_target.colors, ik_theme_preset_colors__handmade_hero, sizeof(ik_theme_preset_colors__handmade_hero));
  MemoryCopy(ik_state->cfg_theme.colors, ik_theme_preset_colors__handmade_hero, sizeof(ik_theme_preset_colors__handmade_hero));

  //////////////////////////////
  //- k: compute palettes from theme
  {
    IK_Theme *current = &ik_state->cfg_theme;

    // ui palette
    for EachEnumVal(IK_PaletteCode, code)
    {
      ik_state->cfg_ui_debug_palettes[code].null       = v4f32(1, 0, 1, 1);
      ik_state->cfg_ui_debug_palettes[code].cursor     = current->colors[IK_ThemeColor_Cursor];
      ik_state->cfg_ui_debug_palettes[code].selection  = current->colors[IK_ThemeColor_SelectionOverlay];
    }
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Base].background              = current->colors[IK_ThemeColor_BaseBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Base].text                    = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Base].text_weak               = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Base].border                  = current->colors[IK_ThemeColor_BaseBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_MenuBar].background           = current->colors[IK_ThemeColor_MenuBarBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_MenuBar].text                 = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_MenuBar].text_weak            = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_MenuBar].border               = current->colors[IK_ThemeColor_MenuBarBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Floating].background          = current->colors[IK_ThemeColor_FloatingBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Floating].text                = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Floating].text_weak           = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Floating].border              = current->colors[IK_ThemeColor_FloatingBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ImplicitButton].background    = current->colors[IK_ThemeColor_ImplicitButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ImplicitButton].text          = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ImplicitButton].text_weak     = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ImplicitButton].border        = current->colors[IK_ThemeColor_ImplicitButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PlainButton].background       = current->colors[IK_ThemeColor_PlainButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PlainButton].text             = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PlainButton].text_weak        = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PlainButton].border           = current->colors[IK_ThemeColor_PlainButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PositivePopButton].background = current->colors[IK_ThemeColor_PositivePopButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PositivePopButton].text       = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PositivePopButton].text_weak  = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_PositivePopButton].border     = current->colors[IK_ThemeColor_PositivePopButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NegativePopButton].background = current->colors[IK_ThemeColor_NegativePopButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NegativePopButton].text       = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NegativePopButton].text_weak  = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NegativePopButton].border     = current->colors[IK_ThemeColor_NegativePopButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NeutralPopButton].background  = current->colors[IK_ThemeColor_NeutralPopButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NeutralPopButton].text        = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NeutralPopButton].text_weak   = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_NeutralPopButton].border      = current->colors[IK_ThemeColor_NeutralPopButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ScrollBarButton].background   = current->colors[IK_ThemeColor_ScrollBarButtonBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ScrollBarButton].text         = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ScrollBarButton].text_weak    = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_ScrollBarButton].border       = current->colors[IK_ThemeColor_ScrollBarButtonBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Tab].background               = current->colors[IK_ThemeColor_TabBackground];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Tab].text                     = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Tab].text_weak                = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_Tab].border                   = current->colors[IK_ThemeColor_TabBorder];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_TabInactive].background       = current->colors[IK_ThemeColor_TabBackgroundInactive];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_TabInactive].text             = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_TabInactive].text_weak        = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_TabInactive].border           = current->colors[IK_ThemeColor_TabBorderInactive];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_DropSiteOverlay].background   = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_DropSiteOverlay].text         = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_DropSiteOverlay].text_weak    = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_ui_debug_palettes[IK_PaletteCode_DropSiteOverlay].border       = current->colors[IK_ThemeColor_DropSiteOverlay];

    // main palette
    for EachEnumVal(IK_PaletteCode, code)
    {
      ik_state->cfg_main_palettes[code].null       = v4f32(1, 0, 1, 1);
      ik_state->cfg_main_palettes[code].cursor     = current->colors[IK_ThemeColor_Cursor];
      ik_state->cfg_main_palettes[code].selection  = current->colors[IK_ThemeColor_SelectionOverlay];
    }
    ik_state->cfg_main_palettes[IK_PaletteCode_Base].background              = current->colors[IK_ThemeColor_BaseBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_Base].text                    = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_Base].text_weak               = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_Base].border                  = current->colors[IK_ThemeColor_BaseBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_MenuBar].background           = current->colors[IK_ThemeColor_MenuBarBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_MenuBar].text                 = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_MenuBar].text_weak            = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_MenuBar].border               = current->colors[IK_ThemeColor_MenuBarBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_Floating].background          = current->colors[IK_ThemeColor_FloatingBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_Floating].text                = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_Floating].text_weak           = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_Floating].border              = current->colors[IK_ThemeColor_FloatingBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_ImplicitButton].background    = current->colors[IK_ThemeColor_ImplicitButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_ImplicitButton].text          = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_ImplicitButton].text_weak     = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_ImplicitButton].border        = current->colors[IK_ThemeColor_ImplicitButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_PlainButton].background       = current->colors[IK_ThemeColor_PlainButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_PlainButton].text             = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_PlainButton].text_weak        = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_PlainButton].border           = current->colors[IK_ThemeColor_PlainButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_PositivePopButton].background = current->colors[IK_ThemeColor_PositivePopButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_PositivePopButton].text       = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_PositivePopButton].text_weak  = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_PositivePopButton].border     = current->colors[IK_ThemeColor_PositivePopButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_NegativePopButton].background = current->colors[IK_ThemeColor_NegativePopButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_NegativePopButton].text       = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_NegativePopButton].text_weak  = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_NegativePopButton].border     = current->colors[IK_ThemeColor_NegativePopButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_NeutralPopButton].background  = current->colors[IK_ThemeColor_NeutralPopButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_NeutralPopButton].text        = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_NeutralPopButton].text_weak   = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_NeutralPopButton].border      = current->colors[IK_ThemeColor_NeutralPopButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_ScrollBarButton].background   = current->colors[IK_ThemeColor_ScrollBarButtonBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_ScrollBarButton].text         = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_ScrollBarButton].text_weak    = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_ScrollBarButton].border       = current->colors[IK_ThemeColor_ScrollBarButtonBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_Tab].background               = current->colors[IK_ThemeColor_TabBackground];
    ik_state->cfg_main_palettes[IK_PaletteCode_Tab].text                     = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_Tab].text_weak                = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_Tab].border                   = current->colors[IK_ThemeColor_TabBorder];
    ik_state->cfg_main_palettes[IK_PaletteCode_TabInactive].background       = current->colors[IK_ThemeColor_TabBackgroundInactive];
    ik_state->cfg_main_palettes[IK_PaletteCode_TabInactive].text             = current->colors[IK_ThemeColor_Text];
    ik_state->cfg_main_palettes[IK_PaletteCode_TabInactive].text_weak        = current->colors[IK_ThemeColor_TextWeak];
    ik_state->cfg_main_palettes[IK_PaletteCode_TabInactive].border           = current->colors[IK_ThemeColor_TabBorderInactive];
    ik_state->cfg_main_palettes[IK_PaletteCode_DropSiteOverlay].background   = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_main_palettes[IK_PaletteCode_DropSiteOverlay].text         = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_main_palettes[IK_PaletteCode_DropSiteOverlay].text_weak    = current->colors[IK_ThemeColor_DropSiteOverlay];
    ik_state->cfg_main_palettes[IK_PaletteCode_DropSiteOverlay].border       = current->colors[IK_ThemeColor_DropSiteOverlay];
  }

  IK_InitStacks(ik_state)
  IK_InitStackNils(ik_state)
}

//- key

internal IK_Key
ik_hot_key(void)
{
  return ik_state->hot_box_key;
}

internal IK_Key
ik_active_key(IK_MouseButtonKind button_kind)
{
  return ik_state->active_box_key[button_kind];
}

internal IK_Key
ik_drop_hot_key(void)
{
  NotImplemented;
}

//- interaction

internal void
ik_kill_action(void)
{
  for EachEnumVal(IK_MouseButtonKind, k)
  {
    ik_state->active_box_key[k] = ik_key_zero();
  }
}

//- frame

internal B32
ik_frame(void)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  /////////////////////////////////
  // do per-frame resets

  ik_drawlist_reset(ik_frame_drawlist());
  arena_clear(ik_frame_arena());

  /////////////////////////////////
  // remake drawing buckets every frame

  d_begin_frame();
  ik_state->bucket_rect = d_bucket_make();

  /////////////////////////////////
  // get events from os
  
  OS_EventList os_events = os_get_events(ik_frame_arena(), 0);
  {
    ik_state->last_window_rect   = ik_state->window_rect;
    ik_state->last_window_dim    = dim_2f32(ik_state->last_window_rect);
    ik_state->window_rect        = os_client_rect_from_window(ik_state->os_wnd, 0);
    ik_state->window_res_changed = ik_state->window_rect.x0 != ik_state->last_window_rect.x0 || ik_state->window_rect.x1 != ik_state->last_window_rect.x1 || ik_state->window_rect.y0 != ik_state->last_window_rect.y0 || ik_state->window_rect.y1 != ik_state->last_window_rect.y1;
    ik_state->window_dim         = dim_2f32(ik_state->window_rect);
    ik_state->last_mouse         = ik_state->mouse;
    {
      Vec2F32 mouse = os_window_is_focused(ik_state->os_wnd) ? os_mouse_from_window(ik_state->os_wnd) : v2f32(-100,-100);
      if(mouse.x >= 0 && mouse.x <= ik_state->window_dim.x &&
         mouse.y >= 0 && mouse.y <= ik_state->window_dim.y)
      {
        ik_state->mouse = mouse;
      }
    }
    ik_state->mouse_delta       = sub_2f32(ik_state->mouse, ik_state->last_mouse);
    ik_state->last_dpi           = ik_state->dpi;
    ik_state->dpi                = os_dpi_from_window(ik_state->os_wnd);

    // animation
    ik_state->animation.vast_rate = 1 - pow_f32(2, (-60.f * ui_state->animation_dt));
    ik_state->animation.fast_rate = 1 - pow_f32(2, (-50.f * ui_state->animation_dt));
    ik_state->animation.fish_rate = 1 - pow_f32(2, (-40.f * ui_state->animation_dt));
    ik_state->animation.slow_rate = 1 - pow_f32(2, (-30.f * ui_state->animation_dt));
    ik_state->animation.slug_rate = 1 - pow_f32(2, (-15.f * ui_state->animation_dt));
    ik_state->animation.slaf_rate = 1 - pow_f32(2, (-8.f  * ui_state->animation_dt));
  }

  /////////////////////////////////
  // calculate avg length in us of last many frames

  U64 frame_time_history_avg_us = 0;
  ProfScope("calculate avg length in us of last many frames")
  {
    U64 num_frames_in_history = Min(ArrayCount(ik_state->frame_time_us_history), ik_state->frame_index);
    U64 frame_time_history_sum_us = 0;
    if(num_frames_in_history > 0)
    {
      for(U64 i = 0; i < num_frames_in_history; i++)
      {
        frame_time_history_sum_us += ik_state->frame_time_us_history[i];
      }
      frame_time_history_avg_us = frame_time_history_sum_us/num_frames_in_history;
    }
  }

  /////////////////////////////////
  // pick target hz

  // pick among a number of sensible targets to snap to, given how well we've been performing
  F32 target_hz = os_get_gfx_info()->default_refresh_rate;
  if(ik_state->frame_index > 32)
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

  ik_state->frame_dt = 1.f/target_hz;

  // begin measuring actual per-frame work
  U64 begin_time_us = os_now_microseconds();

  /////////////////////////////////
  //~ Build UI

  ////////////////////////////////
  //- build event list for UI

  UI_EventList ui_events = {0};
  for(OS_Event *os_evt = os_events.first; os_evt != 0; os_evt = os_evt->next)
  {
    UI_Event ui_evt = {0};

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

    // k
    if(ui_evt.key == OS_Key_Up && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = UI_EventDeltaUnit_Line;
      ui_evt.delta_2s32 = v2s32(0,-1);
    }

    // k
    if(ui_evt.key == OS_Key_Down && ui_evt.kind == UI_EventKind_Press)
    {
      ui_evt.kind       = UI_EventKind_Navigate;
      ui_evt.flags      = 0;
      ui_evt.delta_unit = UI_EventDeltaUnit_Line;
      ui_evt.delta_2s32 = v2s32(0,1);
    }

    UI_EventNode *evt_node = ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt);

    // TODO(k): we may want to handle this somewhere else
    if(os_evt->kind == OS_EventKind_WindowClose) {ik_state->window_should_close = 1;}
  }

  ////////////////////////////////
  //- begin build UI

  {
    // gather font info
    F_Tag main_font = ik_font_from_slot(IK_FontSlot_Main);
    F32 main_font_size = ik_font_size_from_slot(IK_FontSlot_Main);
    F_Tag icon_font = ik_font_from_slot(IK_FontSlot_Icons);

    // build icon info
    UI_IconInfo icon_info = {0};
    {
      icon_info.icon_font = icon_font;
      icon_info.icon_kind_text_map[UI_IconKind_RightArrow]  = ik_icon_kind_text_table[IK_IconKind_RightScroll];
      icon_info.icon_kind_text_map[UI_IconKind_DownArrow]   = ik_icon_kind_text_table[IK_IconKind_DownScroll];
      icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]   = ik_icon_kind_text_table[IK_IconKind_LeftScroll];
      icon_info.icon_kind_text_map[UI_IconKind_UpArrow]     = ik_icon_kind_text_table[IK_IconKind_UpScroll];
      icon_info.icon_kind_text_map[UI_IconKind_RightCaret]  = ik_icon_kind_text_table[IK_IconKind_RightCaret];
      icon_info.icon_kind_text_map[UI_IconKind_DownCaret]   = ik_icon_kind_text_table[IK_IconKind_DownCaret];
      icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]   = ik_icon_kind_text_table[IK_IconKind_LeftCaret];
      icon_info.icon_kind_text_map[UI_IconKind_UpCaret]     = ik_icon_kind_text_table[IK_IconKind_UpCaret];
      icon_info.icon_kind_text_map[UI_IconKind_CheckHollow] = ik_icon_kind_text_table[IK_IconKind_CheckHollow];
      icon_info.icon_kind_text_map[UI_IconKind_CheckFilled] = ik_icon_kind_text_table[IK_IconKind_CheckFilled];
    }

    UI_WidgetPaletteInfo widget_palette_info = {0};
    {
      widget_palette_info.tooltip_palette   = ik_ui_palette_from_code(IK_PaletteCode_Floating);
      widget_palette_info.ctx_menu_palette  = ik_ui_palette_from_code(IK_PaletteCode_Floating);
      widget_palette_info.scrollbar_palette = ik_ui_palette_from_code(IK_PaletteCode_ScrollBarButton);
    }

    // build animation info
    UI_AnimationInfo animation_info = {0};
    {
      animation_info.hot_animation_rate     = ik_state->animation.fast_rate;
      animation_info.active_animation_rate  = ik_state->animation.fast_rate;
      animation_info.focus_animation_rate   = 1.f;
      animation_info.tooltip_animation_rate = ik_state->animation.fast_rate;
      animation_info.menu_animation_rate    = ik_state->animation.fast_rate;
      animation_info.scroll_animation_rate  = ik_state->animation.fast_rate;
    }

    // begin & push initial stack values
    ui_begin_build(ik_state->os_wnd, &ui_events, &icon_info, &widget_palette_info, &animation_info, ik_state->frame_dt);

    ui_push_font(main_font);
    ui_push_font_size(main_font_size);
    // ui_push_text_raster_flags(...);
    ui_push_text_padding(main_font_size*0.2f);
    ui_push_pref_width(ui_em(20.f, 1.f));
    ui_push_pref_height(ui_em(1.35f, 1.f));
    ui_push_palette(ik_ui_palette_from_code(IK_PaletteCode_Base));
  }

  ////////////////////////////////
  //~ Begin build ink
  //  TODO(k): we may want to move on top

  IK_Frame *frame = ik_state->active_frame;

  // font
  F_Tag main_font = ik_font_from_slot(IK_FontSlot_Main);
  F32 main_font_size = ik_font_size_from_slot(IK_FontSlot_Main);
  // F_Tag icon_font = ik_font_from_slot(IK_FontSlot_Icons);

  // widget palette
  IK_WidgetPaletteInfo widget_palette_info = {0};
  {
    widget_palette_info.tooltip_palette   = ik_palette_from_code(IK_PaletteCode_Floating);
    widget_palette_info.ctx_menu_palette  = ik_palette_from_code(IK_PaletteCode_Floating);
    widget_palette_info.scrollbar_palette = ik_palette_from_code(IK_PaletteCode_ScrollBarButton);
  }
  ik_state->widget_palette_info = widget_palette_info;

  ik_push_frame(frame);
  ik_push_parent(frame->root);
  ik_push_font(main_font);
  ik_push_font_size(main_font_size);
  ik_push_text_padding(main_font_size*0.6f);
  ik_push_palette(ik_palette_from_code(IK_PaletteCode_Base));

  ////////////////////////////////
  //~ Unpack camera

  IK_Camera *camera = &frame->camera;
  // NOTE(k): since we are not using view_mat, it's a indentity matrix, so proj_mat == proj_view_mat
  ik_state->proj_mat = make_orthographic_vulkan_4x4f32(camera->rect.x0, camera->rect.x1, camera->rect.y1, camera->rect.y0, camera->zn, camera->zf);
  ik_state->proj_mat_inv = inverse_4x4f32(ik_state->proj_mat);
  ik_state->mouse_in_world = ik_mouse_in_world(ik_state->proj_mat_inv);
  Vec2F32 camera_rect_dim = dim_2f32(camera->rect);
  ik_state->world_to_screen_ratio = (Vec2F32){camera_rect_dim.x/ik_state->window_dim.x, camera_rect_dim.y/ik_state->window_dim.y};

  ////////////////////////////////
  //~ Build Debug UI

  ik_ui_stats();
  ik_ui_toolbar();
  ik_ui_inspector();
  ik_ui_selection();

  // NOTE(k): deprecated, we just use ui_events
  ////////////////////////////////
  //~ Rebuild os_events from the remaining ui_events

  // OS_EventList os_events = {0};
  // for(UI_EventNode *n = ui_state->events->first, *next = 0; n != 0; n = n->next)
  // {
  //   os_event_list_push(ik_frame_arena(), &os_events, &n->src);
  // }
  // ik_state->os_events = os_events;

  ik_state->events = &ui_events; // NOTE(k): ui_events is stored on the stack

  ////////////////////////////////
  //~ Main scene building

  // unpack ctx
  // TODO(k): bug, won't return the right result 
  // B32 middle_is_down = os_key_is_down(OS_Key_MiddleMouseButton);
  // B32 middle_is_down = 0;
  B32 cursor_override = 0;
  OS_Cursor next_cursor = 0;

  ////////////////////////////////
  //- camera controls

  {
    B32 is_zooming = 0;

    ////////////////////////////////
    //- window resized

    if(ik_state->window_res_changed)
    {
      Vec2F32 camera_dim = dim_2f32(camera->target_rect);
      F32 area = camera_dim.x*camera_dim.y;
      F32 ratio = ik_state->window_dim.x/ik_state->window_dim.y;
      F32 y = sqrt_f32(area/ratio);
      F32 x = ratio*y;
      camera->target_rect.y1 = camera->target_rect.y0 + y;
      camera->target_rect.x1 = camera->target_rect.x0 + x;
    }

    ////////////////////////////////
    //- events related to camera

    for(UI_EventNode *n = ik_state->events->first, *next = 0; n != 0; n = next)
    {
      B32 taken = 0;
      next = n->next;
      UI_Event *evt = &n->v;

      ////////////////////////////////
      //- zoom

      if(evt->kind == UI_EventKind_Scroll && evt->modifiers == OS_Modifier_Ctrl)
      {
        is_zooming = 1;
        F32 delta = evt->delta_2f32.y;
        // get normalized rect
        Rng2F32 rect = camera->rect;
        Vec2F32 rect_center = center_2f32(camera->rect);
        Vec2F32 shift = {-rect_center.x, -rect_center.y};
        Vec2F32 shift_inv = rect_center;
        rect = shift_2f32(rect, shift);

        F32 zoom_step = mix_1f32(camera->min_zoom_step, camera->max_zoom_step, camera->zoom_t);
        F32 scale = 1 + delta*zoom_step;
        rect.x0 *= scale;
        rect.x1 *= scale;
        rect.y0 *= scale;
        rect.y1 *= scale;
        rect = shift_2f32(rect, shift_inv);

        // anchor mouse pos in world
        Mat4x4F32 proj_mat_after = make_orthographic_vulkan_4x4f32(rect.x0, rect.x1, rect.y1, rect.y0, camera->zn, camera->zf);
        Mat4x4F32 proj_mat_inv_after = inverse_4x4f32(proj_mat_after);
        Vec2F32 mouse_in_world_after = ik_mouse_in_world(proj_mat_inv_after);
        Vec2F32 world_delta = sub_2f32(ik_state->mouse_in_world, mouse_in_world_after);
        rect = shift_2f32(rect, world_delta);
        camera->target_rect = rect;

        taken = 1;
      }

      ////////////////////////////////
      //- pan

      // TODO(k): support middle mouse dragging

      // pan started
      if((ik_tool() == IK_ToolKind_Hand) && evt->kind == UI_EventKind_Press && evt->key == OS_Key_LeftMouseButton)
      {
        camera->drag_start_mouse = ik_state->mouse;
        Vec2F32 rect_dim = dim_2f32(camera->rect);
        camera->drag_start_rect = camera->target_rect;
        camera->dragging = 1;
        taken = 1;
      }

      // pan ended
      // NOTE(k): we can't check if space is released or not
      // since pressing space will repeat the keydown and keyup
      if(camera->dragging && evt->kind == UI_EventKind_Release && evt->key == OS_Key_LeftMouseButton)
      {
        taken = 1;
        camera->dragging = 0;
      }

      if(taken)
      {
        ui_eat_event_node(ik_state->events, n);
      }
    }

    // TODO(Next): not pretty
    camera->dragging = ik_tool() == IK_ToolKind_Hand ? camera->dragging : 0;

    // apply pan
    if(camera->dragging)
    {
      Vec2F32 delta = sub_2f32(camera->drag_start_mouse, ik_state->mouse);
      Vec2F32 camera_rect_dim = dim_2f32(camera->rect);
      Vec2F32 mouse_scale = v2f32(camera_rect_dim.x/ik_state->window_dim.x, camera_rect_dim.y/ik_state->window_dim.y);
      delta.x *= mouse_scale.x;
      delta.y *= mouse_scale.y;

      Rng2F32 rect = shift_2f32(camera->drag_start_rect, delta);
      camera->target_rect = rect;
      camera->rect = rect;
    }

    // camera animation
    camera->rect.x0 += ik_state->animation.fast_rate * (camera->target_rect.x0-camera->rect.x0);
    camera->rect.x1 += ik_state->animation.fast_rate * (camera->target_rect.x1-camera->rect.x1);
    camera->rect.y0 += ik_state->animation.fast_rate * (camera->target_rect.y0-camera->rect.y0);
    camera->rect.y1 += ik_state->animation.fast_rate * (camera->target_rect.y1-camera->rect.y1);
    camera->zoom_t  += ik_state->animation.slow_rate * ((F32)is_zooming-camera->zoom_t);
  }

  ////////////////////////////////
  //- box interaction

  typedef struct IK_BoxDrag IK_BoxDrag;
  struct IK_BoxDrag
  {
    Vec2F32 drag_start_position; 
    Vec2F32 drag_start_mouse_in_world; 
  };

  {
    IK_Box *roots[2] = {frame->root, frame->blank};
    for(U64 i = 0; i < ArrayCount(roots); i++)
    {
      IK_Box *root = roots[i];
      IK_Box *box = root;
      while(box != 0)
      {
        IK_BoxRec rec = ik_box_rec_df_post(box, root);
        IK_Box *parent = box->parent;
        IK_Signal sig = ik_signal_from_box(box);

        B32 is_hot = ik_key_match(box->key, ik_state->hot_box_key);
        B32 is_active = ik_key_match(box->key, ik_state->active_box_key[IK_MouseButtonKind_Left]);
        B32 is_focus_hot = ik_key_match(box->key, ik_state->focus_hot_box_key);
        B32 is_focus_active = ik_key_match(box->key, ik_state->focus_active_box_key);
        B32 is_disabled = box->flags&IK_BoxFlag_Disabled;

        if(box->flags & IK_BoxFlag_FitViewport)
        {
          box->position = camera->rect.p0;
          box->rect_size = dim_2f32(camera->rect);
          // TODO: maybe reset scale, rotation?
        }

        // update xform (in TRS order)
        Vec2F32 rect_size_half = scale_2f32(box->rect_size, 0.5);
        Vec3F32 center = {box->position.x+rect_size_half.x, box->position.y+rect_size_half.y, 0.0};
        Mat4x4F32 fixed_xform = mat_4x4f32(1.0);
        Mat4x4F32 scale_mat = make_scale_4x4f32(v3f32(box->scale.x, box->scale.y, 1.0));
        // TODO: maybe just use euler rotation, quat may be too slow
        Mat4x4F32 rotation_mat = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(v3f32(0,0,-1), box->rotation));
        Mat4x4F32 translation_mat = make_translate_4x4f32(center);
        fixed_xform = mul_4x4f32(scale_mat, fixed_xform);
        fixed_xform = mul_4x4f32(rotation_mat, fixed_xform);
        fixed_xform = mul_4x4f32(translation_mat, fixed_xform);
        if(parent)
        {
          fixed_xform = mul_4x4f32(parent->fixed_xform, fixed_xform);
        }

        // update rect
        Vec4F32 p0 = {-rect_size_half.x, -rect_size_half.y, 0.0, 1.0};
        Vec4F32 p1 = {rect_size_half.x, rect_size_half.y, 0.0, 1.0};
        p0 = transform_4x4f32(fixed_xform, p0);
        p1 = transform_4x4f32(fixed_xform, p1);
        Rng2F32 fixed_rect = {0};
        fixed_rect.x0 = p0.x;
        fixed_rect.y0 = p0.y;
        fixed_rect.x1 = p1.x;
        fixed_rect.y1 = p1.y;
        // TODO: don't keep changing this, only at the start of ratio-fixed resizing
        Vec2F32 fixed_rect_dim = dim_2f32(fixed_rect);
        box->ratio = fixed_rect_dim.x/fixed_rect_dim.y;

        // update artifacts
        // TODO(k): we don't really need xform here
        box->fixed_xform = fixed_xform;
        box->fixed_rect = fixed_rect;
        box->fixed_size = dim_2f32(box->fixed_rect);
        box->sig = sig;

        if((box->flags&IK_BoxFlag_DrawText) && (box->flags&IK_BoxFlag_HasDisplayString))
        {
          Temp scratch = scratch_begin(0,0);
          IK_ColorCode text_color_code = (box->flags & IK_BoxFlag_DrawTextWeak ? IK_ColorCode_TextWeak : IK_ColorCode_Text);
          // push line runs
          char *by = "\n";
          String8List lines = str8_split(ik_frame_arena(), box->string, (U8*)by, 1, StringSplitFlag_KeepEmpties);
          D_FancyRunList *line_fruns = push_array(ik_frame_arena(), D_FancyRunList, lines.node_count);

          U64 line_index = 0;
          for(String8Node *n = lines.first; n != 0; n = n->next, line_index++)
          {
            String8 line = n->string;

            D_FancyStringList fancy_strings = {0};

            D_FancyString fstr = {0};
            fstr.font = box->font;
            fstr.string = line;
            fstr.color = box->palette->colors[text_color_code];
            fstr.size = M_1;
            fstr.underline_thickness = 0;
            fstr.strikethrough_thickness = 0;
            d_fancy_string_list_push(scratch.arena, &fancy_strings, &fstr);

            line_fruns[line_index] = d_fancy_run_list_from_fancy_string_list(ik_frame_arena(), box->tab_size, box->text_raster_flags, &fancy_strings);
            scratch_end(scratch);
          }

          // fill
          box->display_lines = lines;
          box->display_line_fruns = line_fruns;
        }

        // dragging
        // TODO(Next): bug here for blank root, since it has key 0
        if((sig.f&IK_SignalFlag_LeftDragging) && box->flags&IK_BoxFlag_DragToPosition && !ik_key_match(box->key, ik_key_zero()) && (!is_focus_active))
        {
          if(sig.f & IK_SignalFlag_Pressed)
          {
            IK_BoxDrag drag = {box->position, ik_state->mouse_in_world};
            ik_store_drag_struct(&drag);
          }
          else
          {
            IK_BoxDrag drag = *ik_get_drag_struct(IK_BoxDrag);
            Vec2F32 delta = sub_2f32(ik_state->mouse_in_world, drag.drag_start_mouse_in_world);
            Vec2F32 position = add_2f32(drag.drag_start_position, delta);
            box->position = position;
          }
        }

        // animation (hot_t, ...)
        box->hot_t += ik_state->animation.fast_rate * ((F32)is_hot-box->hot_t);
        box->active_t += ik_state->animation.fast_rate * ((F32)is_active-box->active_t);
        box->focus_hot_t += ik_state->animation.fast_rate * ((F32)is_focus_hot-box->focus_hot_t);
        box->focus_active_t += ik_state->animation.fast_rate * ((F32)is_focus_active-box->focus_active_t);
        box->disabled_t += ik_state->animation.fast_rate * ((F32)is_disabled-box->disabled_t);


        F32 epsilon = 1e-2;
        box->hot_t = abs_f32(is_hot-box->hot_t) < epsilon ? (F32)is_hot : box->hot_t;
        box->active_t = abs_f32(is_hot-box->active_t) < epsilon ? (F32)is_hot : box->active_t;
        box->focus_hot_t = abs_f32(is_hot-box->focus_hot_t) < epsilon ? (F32)is_hot : box->focus_hot_t;
        box->focus_active_t = abs_f32(is_hot-box->focus_active_t) < epsilon ? (F32)is_hot : box->focus_active_t;
        box->disabled_t = abs_f32(is_hot-box->disabled_t) < epsilon ? (F32)is_hot : box->disabled_t;

        // custom update
        if(box->custom_update)
        {
          box->custom_update(box);
        }

        box = rec.next;
      }
    }

    /////////////////////////////////
    //~ Blank box update and interaction

    IK_Box *blank = frame->blank;

    /////////////////////////////////
    //~ Scrolled on blank -> mouve viewport up or down

    if(blank->sig.scroll.y != 0)
    {
      F32 scroll_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "camera_scroll_t"), 1, .reset = 0, .rate = ik_state->animation.slow_rate);
      F32 y_pct = mix_1f32(0.01, 0.04, scroll_t);
      F32 delta_y = blank->sig.scroll.y;
      F32 step_px = ik_state->window_dim.y*y_pct*delta_y;
      F32 step_world = step_px * ik_state->world_to_screen_ratio.y;

      camera->target_rect.y0 += step_world;
      camera->target_rect.y1 += step_world;
    }

    /////////////////////////////////
    //~ Mouse pressed on blank -> unset focus_hot & focus_active

    if(blank->sig.f&IK_SignalFlag_LeftPressed)
    {
      ik_state->focus_hot_box_key = ik_key_zero();
      ik_state->focus_active_box_key = ik_key_zero();
    }

    /////////////////////////////////
    //~ Pruned box is hot/active/focus_hot, reset it

    if(!ik_key_match(ik_state->hot_box_key, ik_key_zero()))
    {
      IK_Box *box = ik_box_from_key(ik_state->hot_box_key);
      if(!box) ik_state->hot_box_key = ik_key_zero();
    }
    for(U64 i = 0; i < IK_MouseButtonKind_COUNT; i++)
    {
      if(!ik_key_match(ik_state->active_box_key[i], ik_key_zero()))
      {
        IK_Box *box = ik_box_from_key(ik_state->active_box_key[i]);
        if(!box) ik_state->active_box_key[i] = ik_key_zero();
      }
    }
    if(!ik_key_match(ik_state->focus_hot_box_key, ik_key_zero()))
    {
      IK_Box *box = ik_box_from_key(ik_state->focus_hot_box_key);
      if(!box) ik_state->focus_hot_box_key = ik_key_zero();
    }

    /////////////////////////////////
    //~ Deletion

    if(!ik_key_match(ik_state->focus_hot_box_key, ik_key_zero()) && ui_key_press(0, OS_Key_Delete))
    {
      IK_Box *box = ik_box_from_key(ik_state->focus_hot_box_key);
      if(box)
      {
        ik_box_release(box);
      }
    }

    /////////////////////////////////
    //~ Hot keys

    // TODO(Next): this one is competing with focus active text input
    if(os_window_is_focused(ik_state->os_wnd) && ik_key_match(ik_state->focus_active_box_key, ik_key_zero()))
    {
      if(ui_key_press(0, OS_Key_H))
      {
        ik_state->tool = IK_ToolKind_Hand;
      }
      if(ui_key_press(0, OS_Key_S))
      {
        ik_state->tool = IK_ToolKind_Selection;
      }
      if(ui_key_press(0, OS_Key_R))
      {
        ik_state->tool = IK_ToolKind_Rectangle;
      }
      if(ui_key_press(0, OS_Key_D))
      {
        ik_state->tool = IK_ToolKind_Draw;
      }
      if(ui_key_press(0, OS_Key_I))
      {
        ik_state->tool = IK_ToolKind_InsertImage;
      }
      if(ui_key_press(0, OS_Key_E))
      {
        ik_state->tool = IK_ToolKind_Eraser;
      }
    }

    /////////////////////////////////
    //~ Edit string (Double clicked on the blank)

    if(blank->sig.f&IK_SignalFlag_LeftDoubleClicked)
    {
      IK_Box *box = ik_text(str8_lit(""));
      // TODO(Next): not ideal, fix it later
      box->draw_frame_index = ik_state->frame_index+1;
      ik_state->focus_hot_box_key = box->key;
      ik_state->focus_active_box_key = box->key;
    }

    /////////////////////////////////
    //~ Paste image

    if(ui_key_press(OS_Modifier_Ctrl, OS_Key_V))
    {
      Temp scratch = scratch_begin(0,0);

      // try pasting text first, then image
      B32 content_is_text = 0;
      B32 content_is_image = 0;
      String8 content = os_get_clipboard_text(scratch.arena);
      content_is_text = content.size > 0;
      if(!content_is_text)
      {
        content = os_get_clipboard_image(scratch.arena);
        content_is_image = content.size > 0;
      }

      // paste text
      if(content_is_text)
      {
        // TODO(k): to be implemented
      }

      // paste image
      if(content_is_image)
      {
        int x = 0;
        int y = 0;
        int z = 0;
        U8 *data = stbi_load_from_memory(content.str, content.size, &x, &y, &z, 4);
        if(data)
        {
          // TODO: cache tex2d
          R_Handle tex_handle = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Linear, v2s32(x, y), R_Tex2DFormat_RGBA8, data);

          F32 default_screen_width = Min(ik_state->window_dim.x * 0.35, x);
          F32 ratio = (F32)x/y;
          F32 width_in_world = default_screen_width * ik_state->world_to_screen_ratio.x;
          F32 height_in_world = width_in_world / ratio;

          IK_Box *box = ik_image(IK_BoxFlag_DrawBorder, ik_state->mouse_in_world, v2f32(width_in_world, height_in_world), tex_handle, v2f32((F32)x, (F32)y));
          box->disabled_t = 1.0;
          // TODO(Next): not ideal, fix it later
          box->draw_frame_index = ik_state->frame_index+1;
        }
        stbi_image_free(data);
      }

      scratch_end(scratch);
    }

    /////////////////////////////////
    //~ Hover cursor

    if(ik_tool() == IK_ToolKind_Selection)
    {
      IK_Box *hot = ik_box_from_key(ik_state->hot_box_key);
      IK_Box *active = ik_box_from_key(ik_state->active_box_key[IK_MouseButtonKind_Left]);
      IK_Box *box = active == 0 ? hot : active;
      if(box)
      {
        OS_Cursor cursor = box->hover_cursor;
        if(os_window_is_focused(ik_state->os_wnd) || active != 0)
        {
          // NOTE: ui_end_build will override cursor, so we have to override it again
          // TODO: still not ideal, find a better way
          next_cursor = cursor;
          cursor_override = 1;
        }
      }
    }
    if(ik_tool() == IK_ToolKind_Hand)
    {
      next_cursor = OS_Cursor_HandPoint;
      cursor_override = 1;
    }
  }

  /////////////////////////////////
  //~ UI drawing

  ui_end_build();
  D_BucketScope(ik_state->bucket_rect)
  {
    ik_ui_draw();
  }

  // override cursor
  if(!ui_state->grab_cursor && cursor_override)
  {
    os_set_cursor(next_cursor);
  }

  /////////////////////////////////
  //~ Main scene drawing

  // start geo2d pass
  ik_state->bucket_geo2d = d_bucket_make();

  // recursive drawing
  D_BucketScope(ik_state->bucket_geo2d)
  {
    // unpack camera settings
    Vec2F32 viewport = dim_2f32(camera->rect);
    Mat3x3F32 xform2d = make_translate_3x3f32(negate_2f32(camera->rect.p0));

    d_push_viewport(viewport);
    // TODO(Next): what heck? should it be column major?
    d_push_xform2d(transpose_3x3f32(xform2d));

    IK_Box *box = frame->root;
    while(box != 0)
    {
      IK_BoxRec rec = ik_box_rec_df_pre(box, frame->root);

      if(ik_state->frame_index >= box->draw_frame_index)
      {
        Rng2F32 dst = box->fixed_rect;
        dst.p1 = mix_2f32(v2f32(0,0), dst.p1, 1.0-box->disabled_t);

        // draw border
        if(box->flags & IK_BoxFlag_DrawBorder)
        {
          Vec4F32 border_clr = box->palette->colors[IK_ColorCode_Border];
          F32 border_thickness = 2.0 * ik_state->world_to_screen_ratio.x;
          R_Rect2DInst *inst = d_rect(pad_2f32(dst, 0.0f), border_clr, 0, border_thickness, border_thickness);
        }

        // draw rect
        if(box->flags & IK_BoxFlag_DrawBackground)
        {
          d_rect(dst, box->color, 0, 0, 0);
        }
        
        // draw image
        if(box->flags & IK_BoxFlag_DrawImage)
        {
          Rng2F32 src = {0,0, box->albedo_tex_size.x, box->albedo_tex_size.y};
          // TODO(Next): why there is a white edge
          d_img(dst, src, box->albedo_tex, v4f32(1,1,1,1), 0, 0, 0);
        }

        if(box->custom_draw)
        {
          box->custom_draw(box, box->custom_draw_user_data);
        }
      }

      box = rec.next;
    }

    d_pop_viewport();
    d_pop_xform2d();
  }

  /////////////////////////////////
  //~ Update hot/active key

  // scene->hot_key = ik_key_make(ik_state->hot_pixel_key, 0);
  // if(ik_state->sig.f & UI_SignalFlag_LeftPressed)
  // {
  //   scene->active_key = scene->hot_key;
  //   ik_scene_active_node_set(scene, scene->active_key, 1);
  // }
  // if(ik_state->sig.f & UI_SignalFlag_LeftReleased)
  // {
  //   scene->active_key = ik_key_zero();
  // }

  /////////////////////////////////
  //~ Handle cursor (hide/show/wrap)

  // ProfScope("handle cursor")
  // {
  //   if(camera->hide_cursor && (!ik_state->cursor_hidden))
  //   {
  //     os_hide_cursor(ik_state->os_wnd);
  //     ik_state->cursor_hidden = 1;
  //   }
  //   if(!camera->hide_cursor && ik_state->cursor_hidden)
  //   {
  //     os_show_cursor(ik_state->os_wnd);
  //     ik_state->cursor_hidden = 0;
  //   }
  //   if(camera->lock_cursor)
  //   {
  //     Vec2F32 cursor = center_2f32(ik_state->window_rect);
  //     os_wrap_cursor(ik_state->os_wnd, cursor.x, cursor.y);
  //     ik_state->cursor = cursor;
  //   }
  // }

  /////////////////////////////////
  //~ End of frame

  // end drag/drop if needed (no code handled drop)
  if(ik_state->drag_drop_state == IK_DragDropState_Dropping)
  {
    ik_state->drag_drop_state = IK_DragDropState_Null;
  }

  /////////////////////////////////
  //~ Submit work

  // TODO: maybe we don't need dynamic drawlist
  // Build frame drawlist before we submit draw bucket
  ProfScope("draw drawlist")
  {
    ik_drawlist_build(ik_frame_drawlist());
  }

  ik_state->pre_cpu_time_us = os_now_microseconds()-begin_time_us;

  // Submit
  ProfScope("submit")
  {
    r_begin_frame();
    r_window_begin_frame(ik_state->os_wnd, ik_state->r_wnd);
    if(!d_bucket_is_empty(ik_state->bucket_geo2d))
    {
      d_submit_bucket(ik_state->os_wnd, ik_state->r_wnd, ik_state->bucket_geo2d);
    }
    if(!d_bucket_is_empty(ik_state->bucket_rect))
    {
      d_submit_bucket(ik_state->os_wnd, ik_state->r_wnd, ik_state->bucket_rect);
    }
    ik_state->hot_pixel_key = r_window_end_frame(ik_state->r_wnd, ik_state->mouse);
    r_end_frame();
  }

  /////////////////////////////////
  //~ Wait if we still have some cpu time left

  local_persist B32 frame_missed = 0;
  local_persist U64 exiting_frame_index = 0;

  // TODO: use cpu sleep instead of spinning cpu here
  U64 frame_time_target_cap_us = (U64)(1000000/target_hz);
  U64 woik_us = os_now_microseconds()-begin_time_us;
  ik_state->cpu_time_us = woik_us;
  if(woik_us < frame_time_target_cap_us)
  {
    ProfScope("wait frame target cap")
    {
      while(woik_us < frame_time_target_cap_us)
      {
        // TODO: check if os supports ms granular sleep
        if(1)
        {
          os_sleep_milliseconds((frame_time_target_cap_us-woik_us)/1000);
        }
        woik_us = os_now_microseconds()-begin_time_us;
      }
    }
  }
  else
  {
    // TODO(k): missed frame rate!
    // TODO(k): proper logging
    fprintf(stderr, "missed frame, over %06.2f ms from %06.2f ms\n", (woik_us-frame_time_target_cap_us)/1000.0, frame_time_target_cap_us/1000.0);
    // TODO(k): not sure why spall why not flushing if we stopped to early, maybe there is a limit for flushing

    // tag it
    ProfBegin("MISSED FRAME");
    ProfEnd();
  }

  if(frame_missed && ik_state->frame_index > exiting_frame_index)
  {
    ik_state->window_should_close = 1;
  }

  /////////////////////////////////
  //~ Determine frame time, record it into history

  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  ik_state->frame_time_us_history[ik_state->frame_index%ArrayCount(ik_state->frame_time_us_history)] = frame_time_us;

  /////////////////////////////////
  //~ Bump frame time counters

  ik_state->frame_index++;
  ik_state->time_in_seconds += ik_state->frame_dt;
  ik_state->time_in_us += frame_time_us;

  ////////////////////////////////
  //~ Pop frame ctx

  ik_pop_frame();
  ik_pop_parent();
  ik_pop_font();
  ik_pop_font_size();
  ik_pop_text_padding();
  ik_pop_palette();

  /////////////////////////////////
  //~ End

  scratch_end(scratch);
  ProfEnd();
  return !ik_state->window_should_close;
}

internal Arena *
ik_frame_arena()
{
  return ik_state->frame_arenas[ik_state->frame_index % ArrayCount(ik_state->frame_arenas)];
}

internal IK_DrawList *
ik_frame_drawlist()
{
  return ik_state->drawlists[ik_state->frame_index % ArrayCount(ik_state->drawlists)];
}

// editor

internal IK_ToolKind
ik_tool(void)
{
  IK_ToolKind ret = ik_state->tool;
  B32 space_is_down = os_key_is_down(OS_Key_Space);
  if(space_is_down && ik_key_match(ik_state->focus_active_box_key, ik_key_zero())) ret = IK_ToolKind_Hand;
  return ret;
}

internal IK_ToolKind ik_tool();

//- colors
internal Vec4F32
ik_rgba_from_theme_color(IK_ThemeColor color)
{
  return ik_state->cfg_theme.colors[color];
}

//- code -> palette
internal IK_Palette *
ik_palette_from_code(IK_PaletteCode code)
{
  IK_Palette *ret = &ik_state->cfg_main_palettes[code];
  return ret;
}

//- code -> ui palette
internal UI_Palette *
ik_ui_palette_from_code(IK_PaletteCode code)
{
  UI_Palette *ret = &ik_state->cfg_ui_debug_palettes[code];
  return ret;
}

//- fonts/sizes
internal F_Tag
ik_font_from_slot(IK_FontSlot slot)
{
  return ik_state->cfg_font_tags[slot];
}

internal F32
ik_font_size_from_slot(IK_FontSlot slot)
{
  F32 result = 0;
  F32 dpi = ik_state->dpi;
  if(dpi != ik_state->last_dpi)
  {
    F32 old_dpi = ik_state->last_dpi;
    F32 new_dpi = dpi;
    ik_state->last_dpi = dpi;
    S32 *pt_sizes[] =
    {
      &ik_state->setting_vals[IK_SettingCode_MainFontSize].s32,
      &ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32,
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
    case IK_FontSlot_Code:
    {
      result = (F32)ik_state->setting_vals[IK_SettingCode_CodeFontSize].s32;
    }break;
    default:
    case IK_FontSlot_Main:
    case IK_FontSlot_Icons:
    {
      result = (F32)ik_state->setting_vals[IK_SettingCode_MainFontSize].s32;
    }break;
  }
  return result;
}

//- box lookup
internal IK_Box *
ik_box_from_key(IK_Key key)
{
  IK_Box *ret = 0;
  if(!ik_key_match(key, ik_key_zero()))
  {
    IK_Frame *frame = ik_top_frame();
    U64 slot = key.u64[0]%frame->box_table_size;
    for(IK_Box *b = frame->box_table[slot].hash_first; b != 0; b = b->hash_next)
    {
      if(ik_key_match(b->key, key))
      {
        ret = b;
        break;
      }
    }
  }
  return ret;
}

//- drag data

internal Vec2F32
ik_drag_start_mouse(void)
{
  return ik_state->drag_start_mouse;
}

internal Vec2F32
ik_drag_delta(void)
{
  return sub_2f32(ik_state->mouse, ik_state->drag_start_mouse);
}

internal void
ik_store_drag_data(String8 string)
{
  arena_clear(ik_state->drag_state_arena);
  ik_state->drag_state_data = push_str8_copy(ik_state->drag_state_arena, string);
}

internal String8
ik_get_drag_data(U64 min_required_size)
{
  // NOTE: if left size is 0 and it's causing assertion failed
  //       check duplicated ui box keys
  AssertAlways(ik_state->drag_state_data.size >= min_required_size);

  // TODO: don't get it, what fuck is this
  // if(ik_state->drag_state_data.size < min_required_size)
  // {
  //     Temp scratch = scratch_begin(0, 0);
  //     String8 str = {push_array(scratch.arena, U8, min_required_size), min_required_size};
  //     ik_store_drag_data(str);
  //     scratch_end(scratch);
  // }
  return ik_state->drag_state_data;
}

/////////////////////////////////
//~ OS event consumption helpers

// TODO(k): for now, we just reuse the ui eat consumption helpers, since we are using the same event list

/////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

/////////////////////////////////
//- basic building API

internal IK_DrawList *
ik_drawlist_alloc(Arena *arena, U64 vertex_buffer_cap, U64 indice_buffer_cap)
{
  ProfBeginFunction();
  IK_DrawList *ret = push_array(arena, IK_DrawList, 1);
  ret->vertex_buffer_cap = vertex_buffer_cap;
  ret->indice_buffer_cap = indice_buffer_cap;
  ret->vertices = r_buffer_alloc(R_ResourceKind_Stream, vertex_buffer_cap, 0, 0);
  ret->indices = r_buffer_alloc(R_ResourceKind_Stream, indice_buffer_cap, 0, 0);
  ProfEnd();
  return ret;
}

internal IK_DrawNode *
ik_drawlist_push(Arena *arena, IK_DrawList *drawlist, R_Vertex *vertices_src, U64 vertex_count, U32 *indices_src, U64 indice_count)
{
  IK_DrawNode *ret = push_array(arena, IK_DrawNode, 1);

  U64 v_buf_size = sizeof(R_Vertex) * vertex_count;
  U64 i_buf_size = sizeof(U32) * indice_count;
  // TODO(XXX): we should use buffer block, we can't just release/realloc a new buffer, since the buffer handle is already handled over to IK_DrawNode
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
ik_drawlist_build(IK_DrawList *drawlist)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);
  R_Vertex *vertices = (R_Vertex*)push_array(scratch.arena, U8, drawlist->vertex_buffer_cmt);
  U32 *indices = (U32*)push_array(scratch.arena, U8, drawlist->indice_buffer_cmt);

  // collect buffer
  R_Vertex *vertices_dst = vertices;
  U32 *indices_dst = indices;
  for(IK_DrawNode *n = drawlist->first_node; n != 0; n = n->next)  
  {
    MemoryCopy(vertices_dst, n->vertices_src, sizeof(R_Vertex)*n->vertex_count);
    MemoryCopy(indices_dst, n->indices_src, sizeof(U32)*n->indice_count);

    vertices_dst += n->vertex_count; 
    indices_dst += n->indice_count;
  }

  r_buffer_copy(drawlist->vertices, vertices, drawlist->vertex_buffer_cmt);
  r_buffer_copy(drawlist->indices, indices, drawlist->indice_buffer_cmt);
  scratch_end(scratch);
  ProfEnd();
}

internal void
ik_drawlist_reset(IK_DrawList *drawlist)
{
  // clear per-frame data
  drawlist->first_node = 0;
  drawlist->last_node = 0;
  drawlist->node_count = 0;
  drawlist->vertex_buffer_cmt = 0;
  drawlist->indice_buffer_cmt = 0;
}

/////////////////////////////////
//- high level building API 

internal IK_DrawNode *
ik_drawlist_push_rect(Arena *arena, IK_DrawList *drawlist, Rng2F32 dst, Rng2F32 src)
{
  R_Vertex *vertices = push_array(arena, R_Vertex, 4);
  U64 vertex_count = 4;
  U32 indice_count = 6;
  U32 *indices = push_array(arena, U32, 6);

  Vec4F32 color_zero = v4f32(0,0,0,0);
  // top left 0
  vertices[0].col = color_zero;
  vertices[0].nor = v3f32(0,0,-1);
  vertices[0].pos = v3f32(dst.x0, dst.y0, 0);
  vertices[0].tex = v2f32(src.x0, src.y0);

  // bottom left 1
  vertices[1].col = color_zero;
  vertices[1].nor = v3f32(0,0,-1);
  vertices[1].pos = v3f32(dst.x0, dst.y1, 0);
  vertices[1].tex = v2f32(src.x0, src.y1);

  // top right 2
  vertices[2].col = color_zero;
  vertices[2].nor = v3f32(0,0,-1);
  vertices[2].pos = v3f32(dst.x1, dst.y0, 0);
  vertices[2].tex = v2f32(src.x1, src.y0);

  // bottom right 3
  vertices[3].col = color_zero;
  vertices[3].nor = v3f32(0,0,-1);
  vertices[3].pos = v3f32(dst.x1, dst.y1, 0);
  vertices[3].tex = v2f32(src.x1, src.y1);

  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;

  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;

  IK_DrawNode *ret = ik_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

/////////////////////////////////
//~ String Block Functions

internal String8
ik_push_str8_copy(String8 src)
{
#define MIN_CAP 512
  String8 ret = {0};
  IK_Frame *frame = ik_top_frame();
  Arena *arena = frame->arena;

  U64 required_bytes = Max(MIN_CAP, src.size+1); // add one for null terminator
  IK_StringBlock *block = 0;
  for(IK_StringBlock *b = frame->first_free_string_block; b != 0; b = b->free_next)
  {
    S64 remain_bytes = (S64)b->cap_bytes - (S64)required_bytes;
    F32 tolerance_pct = 0.25;
    B32 fit = remain_bytes > 0 && remain_bytes < (b->cap_bytes*tolerance_pct);
    if(fit)
    {
      block = b;
      DLLRemove_NP(frame->first_free_string_block, frame->last_free_string_block, b, free_next, free_prev);
      break;
    }
  }

  // didn't find a block -> alloc a new block
  if(block == 0)
  {
    block = push_array(arena, IK_StringBlock, 1);
    // TODO: we can get away with no-zero
    block->p = push_array_fat_sized(arena, required_bytes, block);
    block->cap_bytes = required_bytes;
    block->frame = frame;
  }

  ret.str = block->p;
  ret.size = src.size;
  MemoryCopy(ret.str, src.str, src.size);
  ret.str[ret.size] = 0;
  return ret;
#undef MIN_CAP
}


internal void
ik_string_block_release(String8 string)
{
  IK_StringBlock *block = ptr_from_fat(string.str);
  IK_Frame *frame = block->frame;
  DLLPushFront_NP(frame->first_free_string_block, frame->last_free_string_block, block, free_next, free_prev);
}

/////////////////////////////////
//~ Box Type Functions

internal IK_BoxRec
ik_box_rec_df(IK_Box *box, IK_Box *root, U64 sib_member_off, U64 child_member_off)
{
  // depth first search starting from the current box 
  IK_BoxRec result = {0};
  result.next = 0;
  if((*MemberFromOffset(IK_Box **, box, child_member_off)) != 0)
  {
    result.next = *MemberFromOffset(IK_Box **, box, child_member_off);
    result.push_count = 1;
  }
  else for(IK_Box *p = box; p != 0 && p != root; p = p->parent)
  {
    if((*MemberFromOffset(IK_Box **, p, sib_member_off)) != 0)
    {
      result.next = *MemberFromOffset(IK_Box **, p, sib_member_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

/////////////////////////////////
//~ Frame Building API

internal IK_Frame *
ik_frame_alloc()
{
  IK_Frame *ret = ik_state->first_free_frame;
  if(ret)
  {
    SLLStackPop(ik_state->first_free_frame);
    Arena *arena = ret->arena;
    U64 arena_clear_pos = ret->arena_clear_pos;
    arena_pop_to(arena, arena_clear_pos);
    MemoryZeroStruct(ret);
    ret->arena = arena;
    ret->arena_clear_pos = arena_clear_pos;
  }
  else
  {
    Arena *arena = arena_alloc();
    ret = push_array(arena, IK_Frame, 1);
    ret->arena = arena;
    ret->arena_clear_pos = arena_pos(arena);
  }

  Arena *arena = ret->arena;
  ret->box_table_size = 1024;
  ret->box_table = push_array(arena, IK_BoxHashSlot, ret->box_table_size);

  /////////////////////////////////
  //~ Fill default settings

  // camera
  ret->camera.rect = ik_state->window_rect;
  ret->camera.target_rect = ret->camera.rect;
  ret->camera.zn = -0.1;
  ret->camera.zf = 1000000.0;
  ret->camera.min_zoom_step = 0.05;
  ret->camera.max_zoom_step = 0.35;

  // create root box
  ik_push_frame(ret);
  IK_Box *root = ik_build_box_from_stringf(0, "##root");
  // TODO(k): we want to use key zero to make it mouse passtive, not sure if it will work well
  IK_Box *blank = ik_build_box_from_key(IK_BoxFlag_MouseClickable|IK_BoxFlag_FitViewport|IK_BoxFlag_Scroll, ik_key_zero());
  ik_pop_frame();
  ret->root = root;
  ret->blank = blank;

  // TODO: testing
  IK_Frame(ret) IK_Parent(root)
  {
    {
      IK_Box *box = ik_build_box_from_stringf(0, "##demo_rect_1");
      box->flags |= IK_BoxFlag_DrawBackground|IK_BoxFlag_MouseClickable|IK_BoxFlag_ClickToFocus|IK_BoxFlag_DragToPosition|IK_BoxFlag_DragToResize;
      box->position = v2f32(0,0);
      box->rect_size = v2f32(300, 300);
      box->ratio = 1.f;
      box->color = v4f32(1,1,0,1.0);
      box->hover_cursor = OS_Cursor_UpDownLeftRight;
    }
    {
      IK_Box *box = ik_build_box_from_stringf(0, "##demo_rect_2");
      box->flags |= IK_BoxFlag_DrawBackground|IK_BoxFlag_MouseClickable|IK_BoxFlag_ClickToFocus|IK_BoxFlag_DragToPosition|IK_BoxFlag_DragToResize;
      box->position = v2f32(600,600);
      box->rect_size = v2f32(300, 300);
      box->ratio = 1.f;
      box->color = v4f32(0,1,0,1.0);
      box->hover_cursor = OS_Cursor_UpDownLeftRight;
    }
  }

  return ret;
}

internal void
ik_frame_release(IK_Frame *frame)
{
  SLLStackPush(ik_state->first_free_frame, frame);
}

/////////////////////////////////
//~ Box Tree Building API

/////////////////////////////////
//- box node construction

internal IK_Box *
ik_build_box_from_key(IK_BoxFlags flags, IK_Key key)
{
  ProfBeginFunction();

  IK_Frame *top_frame = ik_top_frame();
  AssertAlways(top_frame);
  Arena *arena = top_frame->arena;

  IK_Box *ret = top_frame->first_free_box;
  if(ret != 0)
  {
    SLLStackPop(top_frame->first_free_box);
  }
  else
  {
    ret = push_array_no_zero(arena, IK_Box, 1);
  }
  MemoryZeroStruct(ret);

  // grab active parent
  IK_Box *parent = ik_top_parent();
  if(parent)
  {
    // insert to tree
    DLLPushBack(parent->first, parent->last, ret);
    parent->children_count++;
  }

  // fill box info
  ret->key = key;
  ret->flags = flags;
  ret->parent = parent;
  ret->frame = top_frame;
  ret->scale = v2f32(1.0, 1.0);
  ret->font = ik_top_font();
  ret->font_size = ik_top_font_size();
  ret->tab_size = ik_top_tab_size();
  ret->text_raster_flags = ik_top_text_raster_flags();
  ret->palette = ik_top_palette();
  ret->transparency = ik_top_transparency();
  ret->text_padding = ik_top_text_padding();
  ret->hover_cursor = ik_top_hover_cursor();

  // hook into lookup table
  U64 slot_index = key.u64[0]%top_frame->box_table_size;
  IK_BoxHashSlot *slot = &top_frame->box_table[slot_index];
  DLLInsert_NPZ(0, slot->hash_first, slot->hash_last, slot->hash_last, ret, hash_next, hash_prev);

  ProfEnd();
  return ret;
}

internal IK_Key
ik_active_seed_key(void)
{
  IK_Key ret = ik_key_zero();
  for(IK_Box *p = ik_top_parent(); p != 0; p = p->parent)
  {
    if(!ik_key_match(ik_key_zero(), p->key))
    {
      ret = p->key;
      break;
    }
  }
  return ret;
}

internal IK_Box *
ik_build_box_from_string(IK_BoxFlags flags, String8 string)
{
  ProfBeginFunction();
  // grab active parent
  IK_Box *parent = ik_top_parent();

  IK_Key key = ik_key_from_string(ik_active_seed_key(), string);

  IK_Box *box = ik_build_box_from_key(flags, key);
  if(box->flags & IK_BoxFlag_DrawText)
  {
    ik_box_equip_display_string(box, string);
  }
  ProfEnd();
  return box;
}

internal IK_Box *
ik_build_box_from_stringf(IK_BoxFlags flags, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  IK_Box *box = ik_build_box_from_string(flags, string);
  scratch_end(scratch);
  return box;
}

//- box node destruction
internal void
ik_box_release(IK_Box *box)
{
  IK_Frame *frame = box->frame;
  IK_Box *parent = box->parent;

  // recursively remove children first
  for(IK_Box *c = box->first, *next = 0; c != 0; c = next)
  {
    next = c->next;
    ik_box_release(c);
  }

  if(parent)
  {
    DLLRemove(parent->first, parent->last, box);
    parent->children_count--;
  }

  // remove from lookup table
  U64 slot_index = box->key.u64[0]%frame->box_table_size;
  IK_BoxHashSlot *slot = &frame->box_table[slot_index];
  DLLRemove_NP(slot->hash_first, slot->hash_last, box, hash_next, hash_prev);

  // push to free list stack
  SLLStackPush(frame->first_free_box, box);
}

/////////////////////////////////
//- box node equipment


internal void
ik_box_equip_name(IK_Box *box, String8 name)
{
  String8 string = push_str8_copy_static(name, box->_name);
  box->name = string;
}

internal String8
ik_box_equip_display_string(IK_Box *box, String8 display_string)
{
#if 1
  String8 ret = {0};
  if(box->string.size != 0)
  {
    ik_string_block_release(box->string);
  }
  box->string = ik_push_str8_copy(display_string);
  box->flags |= IK_BoxFlag_HasDisplayString;
  return ret;
#else
  // Testing, don't use this, will causing memory leak
  String8 ret = {0};
  box->string = push_str8_copy(ik_state->arena, display_string);
  box->flags |= IK_BoxFlag_HasDisplayString;
  return ret;
#endif
}

internal void
ik_box_equip_custom_draw(IK_Box *box, IK_BoxCustomDrawFunctionType *custom_draw, void *user_data)
{
  box->custom_draw = custom_draw;
  box->custom_draw_user_data = user_data;
}

/////////////////////////////////
//~ High Level Box Building

typedef struct IK_EditBoxTextRect IK_EditBoxTextRect;
struct IK_EditBoxTextRect
{
  Rng2F32 parent_rect;
  Rng2F32 dst;
  Rng2F32 src;
  R_Handle tex;
  Vec4F32 color;
  B32 highlight;
};

typedef struct IK_EditBoxDrawData IK_EditBoxDrawData;
struct IK_EditBoxDrawData
{
  Rng2F32 mark_rect;
  Rng2F32 cursor_rect;
  IK_EditBoxTextRect *text_rects;
};

IK_BOX_CUSTOM_UPDATE(ik_text_update)
{ 
  B32 is_focus_hot = ik_key_match(box->key, ik_state->focus_hot_box_key);
  B32 is_focus_active = ik_key_match(box->key, ik_state->focus_active_box_key);

  // data buffer
  IK_EditBoxDrawData *draw_data = push_array(ik_frame_arena(), IK_EditBoxDrawData, 1);
  box->custom_draw_user_data = draw_data;

  TxtPt *cursor = &box->cursor;
  TxtPt *mark = &box->mark;

  ////////////////////////////////
  // Take navigation actions for editing

  if(is_focus_active)
  {
    Temp scratch = scratch_begin(0,0);
    UI_EventList *events = ik_state->events;
    for(UI_EventNode *n = events->first; n != 0; n = n->next)
    {
      UI_Event *evt = &n->v;

      if(evt->key == OS_Key_Return && evt->kind == UI_EventKind_Press)
      {
        evt->kind = UI_EventKind_Text;
        evt->string = str8_lit("\n");
      }

      // don't consume anything that doesn't fit a single|multi-line's operations
      if((evt->kind != UI_EventKind_Edit && evt->kind != UI_EventKind_Navigate && evt->kind != UI_EventKind_Text)) { continue; }

      // map this action to an TxtOp
      UI_TxtOp op = ik_multi_line_txt_op_from_event(scratch.arena, &n->v, box->string, *cursor, *mark);

      // perform replace range
      if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
      {
        String8 new_string = ui_push_string_replace_range(scratch.arena, box->string, r1s64(op.range.min.column, op.range.max.column), op.replace);
        ik_box_equip_display_string(box, new_string);
      }

      // commit op's changed cursor & mark to caller-provided state
      *cursor = op.cursor;
      *mark = op.mark;

      // consume event
      ui_eat_event(&n->v);
    }
    scratch_end(scratch);
  }

  ////////////////////////////////
  // rendering

  TxtPt mouse_pt = {1, 1};
  Vec2F32 text_bounds = {0};
  {
    F32 font_size_scale = box->font_size / (F32)M_1;
    F32 best_mouse_offset = inf32();
    F32 advance_x = 0;
    F32 advance_y = 0;
    F32 text_padding_x = box->text_padding*font_size_scale;
    F32 x = box->fixed_rect.p0.x + text_padding_x;
    F32 y = box->fixed_rect.p0.y;

    Rng2F32 mark_rect ={x, y, x, box->fixed_rect.y1};
    Rng2F32 cursor_rect ={x, y, x, box->fixed_rect.y1};
    TxtRng selection_rng = is_focus_active ? txt_rng(*cursor, *mark) : (TxtRng){0};

    U64 c_index = 0;
    for(U64 line_index = 0; line_index < box->display_lines.node_count; line_index++)
    {
      // one line
      D_FancyRunList *fruns = &box->display_line_fruns[line_index];
      for(D_FancyRunNode *n = fruns->first; n != 0; n = n->next)
      {
        D_FancyRun run = n->v;
        Vec2F32 line_run_dim = scale_2f32(run.run.dim, font_size_scale);
        text_bounds.x = Max(text_bounds.x, line_run_dim.x+text_padding_x*2);
        text_bounds.y += line_run_dim.y;

        F_Piece *piece_first = run.run.pieces.v;
        F_Piece *piece_opl = run.run.pieces.v + n->v.run.pieces.count;

        F32 line_x = x;
        F32 line_y = y + run.run.ascent*font_size_scale;;
        Rng2F32 line_cursor = {x,y,x,y+line_run_dim.y};
        // Rng2F32 line_rect = {x, y, x+line_run_dim.x, y+line_run_dim.y};
        B32 mouse_in_line_bounds = ik_state->mouse_in_world.y > y && ik_state->mouse_in_world.y < (y+line_run_dim.y);

        for(F_Piece *piece = piece_first; piece < piece_opl; piece++, c_index++)
        {
          F32 this_advance_x = piece->advance*font_size_scale;
          // NOTE(k): piece rect already computed x offset
          Rng2F32 dst = r2f32p(piece->rect.x0*font_size_scale + line_x,
                               piece->rect.y0*font_size_scale + line_y,
                               piece->rect.x1*font_size_scale + line_x,
                               piece->rect.y1*font_size_scale + line_y);
          Rng2F32 src = r2f32p(piece->subrect.x0, piece->subrect.y0, piece->subrect.x1, piece->subrect.y1);
          // TODO(k): src wil be all zeros in gcc release build but not with clang, wtf!!!
          AssertAlways((src.x0 + src.x1 + src.y0 + src.y1) != 0);
          Vec2F32 size = dim_2f32(dst);
          AssertAlways(!r_handle_match(piece->texture, r_handle_zero()));

          // issue draw
          // TODO(Next): Space will have 0 extent, what heck, fix this?
          // if(size.x > 0 && size.y > 0)
          // TODO(Next): how we handle space
          B32 highlight = (c_index+1) >= selection_rng.min.column && (c_index+1) < selection_rng.max.column;
          Rng2F32 parent_rect = {x+advance_x, y, x+advance_x+piece->advance*font_size_scale, y+line_run_dim.y};
          IK_EditBoxTextRect text_rect = {parent_rect, dst, src, piece->texture, run.color, highlight};
          darray_push(ik_frame_arena(), draw_data->text_rects, text_rect);

          // TODO(Next): we have to handle \n, not pretty, fix it later
          if(mark && mark->column == (c_index+1))     mark_rect = line_cursor;
          if(cursor && cursor->column == (c_index+1)) cursor_rect = line_cursor;

          // update mouse_pt if it's closer
          // TODO:(Next): this won't work on empty line
          if(mouse_in_line_bounds)
          {
            F32 dist_to_mouse = length_2f32(sub_2f32(ik_state->mouse_in_world, center_2f32(parent_rect)));
            if(dist_to_mouse < best_mouse_offset)
            {
              // TODO(k): if we are handling unicode, we want the decode_size_offset too
              best_mouse_offset = dist_to_mouse;
              mouse_pt.column = abs_f32(ik_state->mouse_in_world.x-dst.x0) < abs_f32(ik_state->mouse_in_world.x-dst.x0-this_advance_x) ? c_index+1 : c_index+2;
            }
          }

          advance_x += this_advance_x;
          line_cursor.x1 += this_advance_x;
          line_cursor.x0 += this_advance_x;
        }

        // TODO(Next): we have to handle \n, not pretty, fix it later
        if(mark && mark->column == (c_index+1))     mark_rect = line_cursor;
        if(cursor && cursor->column == (c_index+1)) cursor_rect = line_cursor;

        // mouse is on empty line => mark mouse_pt (since we don't have any pieces to snap mouse)
        if(mouse_in_line_bounds && run.run.pieces.count == 0)
        {
          mouse_pt.column = c_index+1;
        }

        advance_x = 0;
        // TODO(Next): font metrics has line_gap, we should be considering that
        advance_y += line_run_dim.y;
        y += line_run_dim.y;
      }

      // has next line => inc c_index for \n
      if(line_index < (box->display_lines.node_count-1)) c_index++;
    }

    // TODO(Next): fix it
    if(is_focus_active)
    {
      draw_data->mark_rect = mark_rect;
      draw_data->cursor_rect = cursor_rect;
    }
  }

  ////////////////////////////////
  // Update box rect base on text bounds

  // TODO(Next): we shoudl query font ascent+descend+padding
  // TODO(Next): for acurracy, we should use a empty run
  box->rect_size.x = Max(text_bounds.x, box->font_size+box->text_padding*2);
  box->rect_size.y = Max(text_bounds.y, box->font_size*1.15);

  ////////////////////////////////
  // interaction

  IK_Signal sig = box->sig;

  // not focus active => doubleclicked or keyboard
  if(!is_focus_active && sig.f&(IK_SignalFlag_DoubleClicked|IK_SignalFlag_KeyboardPressed))
  {
    ik_state->focus_hot_box_key = box->key;
    ik_state->focus_active_box_key = box->key;

    // select all text
    mark->line = 1;
    mark->column = 1;
    cursor->line = 1;
    cursor->column = box->string.size+1;
    ik_kill_action();
  }

  // dragging => move cursor
  if(is_focus_active && ik_dragging(sig))
  {
    if(ik_pressed(sig))
    {
      *mark = mouse_pt;
    }
    *cursor = mouse_pt;
  }

  // focus active and double clicked => select all
  if(is_focus_active && sig.f&IK_SignalFlag_DoubleClicked)
  {
    *mark = txt_pt(1,1);
    *cursor = txt_pt(1, box->string.size+1);
    ik_kill_action();
  }

  box->hover_cursor = is_focus_active ? OS_Cursor_IBar : OS_Cursor_Pointer;
}

IK_BOX_CUSTOM_DRAW(ik_text_draw)
{
  IK_EditBoxDrawData *draw_data = box->custom_draw_user_data;

  // draw text rects
  for(U64 i = 0; i < darray_size(draw_data->text_rects); i++)
  {
    IK_EditBoxTextRect *text_rect = &draw_data->text_rects[i];
    d_img(text_rect->dst, text_rect->src, text_rect->tex, text_rect->color, 0,0,0);

    if(text_rect->highlight)
    {
      d_rect(text_rect->parent_rect, v4f32(0,0,1,0.2), 0, 0, 0);
    }
  }

  // TODO(Next): fix this
  // draw mark & cursor
  F32 line_height = draw_data->cursor_rect.y1-draw_data->cursor_rect.y0;
  F32 cursor_thickness = line_height*0.05;
  Rng2F32 cursor_rect = draw_data->cursor_rect;
  Rng2F32 mark_rect = draw_data->mark_rect;
  cursor_rect.x0 -= cursor_thickness;
  cursor_rect.x1 += cursor_thickness;
  mark_rect.x0 -= cursor_thickness;
  mark_rect.x1 += cursor_thickness;
  d_rect(cursor_rect, v4f32(0,0,1,0.5), cursor_thickness, 0, cursor_thickness*0.2);
  d_rect(mark_rect, v4f32(0,1,0,0.5), cursor_thickness, 0, cursor_thickness*0.2);
}

internal IK_Box *
ik_text(String8 string)
{
  IK_Box *box;

  F32 font_size = ik_top_font_size();
  F32 font_size_in_world = font_size * ik_state->world_to_screen_ratio.x * 2;

  IK_Key key = ik_key_new();
  box = ik_build_box_from_key(0, key);
  box->flags |= IK_BoxFlag_MouseClickable|IK_BoxFlag_ClickToFocus|IK_BoxFlag_FixedRatio|IK_BoxFlag_DrawText|IK_BoxFlag_DragToScaleFontSize|IK_BoxFlag_DragToPosition;
  box->position = ik_state->mouse_in_world;
  box->rect_size = v2f32(font_size_in_world*3, font_size_in_world);
  box->color = v4f32(1,1,0,1.0);
  box->hover_cursor = OS_Cursor_UpDownLeftRight;
  box->font_size = font_size_in_world;
  box->albedo_tex = r_handle_zero();
  box->ratio = 0;
  box->disabled_t = 1.0;
  box->custom_draw = ik_text_draw;
  box->custom_update = ik_text_update;
  box->cursor = txt_pt(1, string.size+1);
  box->mark = box->cursor;
  ik_box_equip_name(box, str8_lit("text"));

  if(string.size > 0)
  {
    ik_box_equip_display_string(box, string);
  }
  return box;
}

internal IK_Box *
ik_image(IK_BoxFlags flags, Vec2F32 pos, Vec2F32 rect_size, R_Handle tex, Vec2F32 tex_size)
{
  IK_Box *box;
  box = ik_build_box_from_stringf(0, "##image_%I64u", os_now_microseconds());
  box->flags = flags|IK_BoxFlag_DrawImage|IK_BoxFlag_MouseClickable|IK_BoxFlag_ClickToFocus|IK_BoxFlag_FixedRatio|IK_BoxFlag_DragToPosition|IK_BoxFlag_DragToResize;
  box->position = pos;
  box->rect_size = rect_size;
  box->hover_cursor = OS_Cursor_UpDownLeftRight;
  box->albedo_tex = tex;
  box->albedo_tex_size = tex_size;
  box->ratio = rect_size.x/rect_size.y;
  ik_box_equip_name(box, str8_lit("image"));
  return box;
}

/////////////////////////////////
//~ User interaction

internal IK_Signal
ik_signal_from_box(IK_Box *box)
{
  IK_Signal sig = {0};
  sig.box = box;
  sig.event_flags |= os_get_modifiers();

  /////////////////////////////////
  //~ Process events related to this box

  for(UI_EventNode *n = ik_state->events->first, *next = 0; n != 0; n = next)
  {
    B32 taken = 0;
    next = n->next;
    UI_Event *evt = &n->v;

    //- unpack event
    // Vec2F32 evt_mouse = evt->pos;
    Vec2F32 evt_mouse = ik_state->mouse_in_world;
    B32 evt_mouse_in_bounds = contains_2f32(box->fixed_rect, evt_mouse);
    IK_MouseButtonKind evt_mouse_button_kind = 
      evt->key == OS_Key_LeftMouseButton   ? IK_MouseButtonKind_Left   :
      evt->key == OS_Key_RightMouseButton  ? IK_MouseButtonKind_Right  :
      evt->key == OS_Key_MiddleMouseButton ? IK_MouseButtonKind_Middle :
      IK_MouseButtonKind_Left;
    B32 evt_key_is_mouse =
      evt->key == OS_Key_LeftMouseButton  || 
      evt->key == OS_Key_RightMouseButton ||
      evt->key == OS_Key_MiddleMouseButton;
    sig.event_flags |= evt->modifiers;

    //- mouse pressed in box -> set hot/active, mark signal accordingly
    if(box->flags & IK_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Press &&
       evt_mouse_in_bounds &&
       evt_key_is_mouse)
    {
      ik_state->hot_box_key = box->key;
      ik_state->active_box_key[evt_mouse_button_kind] = box->key;
      sig.f |= (IK_SignalFlag_LeftPressed << evt_mouse_button_kind);
      ik_state->drag_start_mouse = evt->pos;

      if(ik_key_match(box->key, ik_state->press_key_history[evt_mouse_button_kind][0]) &&
         evt->timestamp_us-ik_state->press_timestamp_history_us[evt_mouse_button_kind][0] <= 1000000*os_get_gfx_info()->double_click_time)
      {
        sig.f |= (IK_SignalFlag_LeftDoubleClicked<<evt_mouse_button_kind);
      }

      // TODO: handle tripled clicking
      MemoryCopy(&ik_state->press_key_history[evt_mouse_button_kind][1],
                 &ik_state->press_key_history[evt_mouse_button_kind][0],
                 sizeof(ik_state->press_key_history[evt_mouse_button_kind][0]) * (ArrayCount(ik_state->press_key_history)-1));
      MemoryCopy(&ik_state->press_timestamp_history_us[evt_mouse_button_kind][1],
                 &ik_state->press_timestamp_history_us[evt_mouse_button_kind][0],
                 sizeof(ik_state->press_timestamp_history_us[evt_mouse_button_kind][0]) * (ArrayCount(ik_state->press_timestamp_history_us)-1));
      ik_state->press_key_history[evt_mouse_button_kind][0] = box->key;
      ik_state->press_timestamp_history_us[evt_mouse_button_kind][0] = evt->timestamp_us;

      taken = 1;
    }

    //- mouse released in active box -> unset active
    if(box->flags & IK_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Release &&
       ik_key_match(box->key, ik_state->active_box_key[evt_mouse_button_kind]) &&
       evt_mouse_in_bounds &&
       evt_key_is_mouse)
    {
      ik_state->hot_box_key = box->key;
      ik_state->active_box_key[evt_mouse_button_kind] = ik_key_zero();
      sig.f |= (IK_SignalFlag_LeftReleased << evt_mouse_button_kind);
      sig.f |= (IK_SignalFlag_LeftClicked << evt_mouse_button_kind);
      taken = 1;
    }

    //- mouse released outside of active box -> unset hot & active
    if(box->flags & IK_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Release &&
       ik_key_match(box->key, ik_state->active_box_key[evt_mouse_button_kind]) &&
       !evt_mouse_in_bounds &&
       evt_key_is_mouse)
    {
      ik_state->hot_box_key = ik_key_zero();
      ik_state->active_box_key[evt_mouse_button_kind] = ik_key_zero();
      sig.f |= (IK_SignalFlag_LeftReleased << evt_mouse_button_kind);
      taken = 1;
    }

    //- scroll
    if(box->flags & IK_BoxFlag_Scroll &&
       evt->kind == UI_EventKind_Scroll &&
       evt->modifiers != OS_Modifier_Ctrl &&
       evt_mouse_in_bounds)
    {
      Vec2F32 delta = evt->delta_2f32;
      if(evt->modifiers & OS_Modifier_Shift)
      {
        Swap(F32, delta.x, delta.y);
      }
      Vec2S16 delta16 = v2s16((S16)(delta.x/30.f), (S16)(delta.y/30.f));
      if(delta.x > 0 && delta16.x == 0) { delta16.x = +1; }
      if(delta.x < 0 && delta16.x == 0) { delta16.x = -1; }
      if(delta.y > 0 && delta16.y == 0) { delta16.y = +1; }
      if(delta.y < 0 && delta16.y == 0) { delta16.y = -1; }
      sig.scroll.x += delta16.x;
      sig.scroll.y += delta16.y;
      taken = 1;
    }

    if(taken)
    {
      ui_eat_event_node(ik_state->events, n);
    }
  }

  B32 mouse_in_this_rect = contains_2f32(box->fixed_rect, ik_state->mouse_in_world);

  //////////////////////////////
  //~ Mouse is over this box's rect -> always mark mouse-over

  if(mouse_in_this_rect)
  {
    sig.f |= IK_SignalFlag_MouseOver;
  }

  //////////////////////////////
  //~ Mouse is over this box's rect, no other hot key? -> set hot key, mark hovering

  if(box->flags & IK_BoxFlag_MouseClickable &&
     mouse_in_this_rect &&
     (ik_key_match(ik_state->hot_box_key, ik_key_zero()) || ik_key_match(ik_state->hot_box_key, box->key)) &&
     (ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Left], ik_key_zero()) || ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Left], box->key)) &&
     (ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Middle], ik_key_zero()) || ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Middle], box->key)) &&
     (ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Right], ik_key_zero()) || ik_key_match(ik_state->active_box_key[IK_MouseButtonKind_Right], box->key)))
  {
    ik_state->hot_box_key = box->key;
    sig.f |= IK_SignalFlag_Hovering;
  }

  //////////////////////////////
  // TODO: Mouse is over this box's rect, drop site, no other drop hot key? -> set drop hot key

  // {
  //   if(box->flags & UI_BoxFlag_DropSite &&
  //      contains_2f32(rect, ui_state->mouse) &&
  //      (ui_key_match(ui_state->drop_hot_box_key, ui_key_zero()) || ui_key_match(ui_state->drop_hot_box_key, box->key)))
  //   {
  //     ui_state->drop_hot_box_key = box->key;
  //   }
  // }

  //////////////////////////////
  // TODO: Mouse is not over this box's rect, but this is the drop hot key? -> zero drop hot key

  // {
  //   if(box->flags & UI_BoxFlag_DropSite &&
  //      !contains_2f32(rect, ui_state->mouse) &&
  //      ui_key_match(ui_state->drop_hot_box_key, box->key))
  //   {
  //     ui_state->drop_hot_box_key = ui_key_zero();
  //   }
  // }

  //////////////////////////////
  //~ Active -> dragging

  if(box->flags & IK_BoxFlag_MouseClickable)
  {
    for EachEnumVal(IK_MouseButtonKind, k)
    {
      if(ik_key_match(ik_state->active_box_key[k], box->key) || sig.f & (IK_SignalFlag_LeftPressed<<k))
      {
        sig.f |= (IK_SignalFlag_LeftDragging<<k);
      }
    }
  }

  //////////////////////////////
  //~ Mouse is not over this box, hot key is the box? -> unset hot key

  if(!mouse_in_this_rect && ik_key_match(ik_state->hot_box_key, box->key))
  {
    ik_state->hot_box_key = ik_key_zero();
  }

  //////////////////////////////
  //~ Clicked on this box and set focus_hot_key

  if(box->flags & IK_BoxFlag_ClickToFocus && sig.f&IK_SignalFlag_Pressed)
  {
    ik_state->focus_hot_box_key = box->key;
  }

  return sig;
}

/////////////////////////////////
//~ UI Widget

internal void
ik_ui_stats(void)
{
  IK_Frame *frame = ik_top_frame();
  IK_Camera *camera = &frame->camera;

  UI_Box *container = 0;
  F32 width = 800;
  Rng2F32 rect = {ik_state->window_dim.x-800, 0, ik_state->window_dim.x, ik_state->window_dim.y};
  UI_Rect(rect)
    UI_ChildLayoutAxis(Axis2_Y)
    UI_Flags(UI_BoxFlag_Floating)
  {
    container = ui_build_box_from_stringf(0, "###stats_container");
  }

  UI_Box *body;
  UI_Parent(container)
  {
    ui_spacer(ui_em(0.5, 0.0));

    UI_WidthFill
      UI_PrefHeight(ui_children_sum(0.0))
      UI_Row
      UI_Padding(ui_em(0.5, 0.0))
      UI_PrefHeight(ui_pct(1.0, 0.0))
      UI_PrefHeight(ui_children_sum(0.0))
      UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBackground)
      UI_Transparency(0.4)
      UI_ChildLayoutAxis(Axis2_Y)
      UI_CornerRadius(1.0)
      body = ui_build_box_from_stringf(0, "###body");
  }

  UI_Parent(body)
    UI_TextAlignment(UI_TextAlign_Left)
    UI_TextPadding(9)
    UI_PrefWidth(ui_pct(1.0, 0.0))
  {
    // collect some values
    U64 last_frame_index = ik_state->frame_index > 0 ? ik_state->frame_index-1 : 0;
    U64 last_frame_us = ik_state->frame_time_us_history[last_frame_index%ArrayCount(ik_state->frame_time_us_history)];

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
      ui_labelf("%.3fms", (F32)ik_state->pre_cpu_time_us/1000.0);
    }
    UI_Row
    {
      ui_labelf("cpu time");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%.3fms", (F32)ik_state->cpu_time_us/1000.0);
    }
    UI_Row
    {
      ui_labelf("fps");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%.2f", 1.0 / (last_frame_us/1000000.0));
    }
    ui_divider(ui_em(0.1, 0.0));
    UI_Row
    {
      ui_labelf("ik_hot_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%lu", ik_state->hot_box_key.u64[0]);
    }
    UI_Row
    {
      ui_labelf("ik_active_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%lu", ik_state->active_box_key[IK_MouseButtonKind_Left].u64[0]);
    }
    UI_Row
    {
      ui_labelf("ik_focus_hot_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%lu", ik_state->focus_hot_box_key.u64[0]);
    }
    UI_Row
    {
      ui_labelf("ik_focus_active_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%lu", ik_state->focus_active_box_key.u64[0]);
    }
    UI_Row
    {
      ui_labelf("ik_pixel_hot_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%I64u", ik_state->hot_pixel_key);
    }
    UI_Row
    {
      ui_labelf("ik_mouse");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%f %f", ik_state->mouse.x, ik_state->mouse.y);
    }
    UI_Row
    {
      ui_labelf("ik_drag_start_mouse");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%f %f", ik_state->drag_start_mouse.x, ik_state->drag_start_mouse.y);
    }
    UI_Row
    {
      ui_labelf("ik_camera_rect");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%f %f %f %f", camera->rect.x0, camera->rect.y0, camera->rect.y0, camera->rect.y1);
    }
    UI_Row
    {
      ui_labelf("window");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%.2f, %.2f", ik_state->window_dim.x, ik_state->window_dim.y);
    }
    ui_divider(ui_em(0.1, 0.0));
    UI_Row
    {
      ui_labelf("ui_hot_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%lu", ui_state->hot_box_key.u64[0]);
    }
    UI_Row
    {
      ui_labelf("hot_pixel_key");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%I64u", ik_state->hot_pixel_key);
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
      ui_labelf("drag start mouse");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%.2f, %.2f", ui_state->drag_start_mouse.x, ui_state->drag_start_mouse.y);
    }
  }
}

internal void
ik_ui_toolbar(void)
{
  UI_Box *container;
  Rng2F32 rect = {0, 0, ik_state->window_dim.x, ui_top_font_size()*10};
  UI_Rect(rect)
    UI_Flags(UI_BoxFlag_Floating)
    container = ui_build_box_from_stringf(0, "##toolbar_container");

  F32 cell_width = ui_top_font_size()*2.5;
  UI_Box *inner;
  UI_Parent(container)
    UI_HeightFill
    UI_WidthFill
    UI_Column
    UI_Padding(ui_pct(1.0, 0.0))
    UI_Row
    UI_Padding(ui_pct(1.0, 0.0))
    UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBackground)
    UI_PrefWidth(ui_px(cell_width*IK_ToolKind_COUNT, 1.0))
    UI_PrefHeight(ui_px(cell_width, 1.0))
    UI_ChildLayoutAxis(Axis2_X)
    inner = ui_build_box_from_stringf(0, "##inner");

  UI_Parent(inner)
    UI_Font(ik_font_from_slot(IK_FontSlot_ToolbarIcons))
  {
    String8 strs[IK_ToolKind_COUNT] =
    {
      str8_lit("H"),
      str8_lit("S"),
      str8_lit("R"),
      str8_lit("D"),
      str8_lit("I"),
      str8_lit("E"),
    };

    for(U64 i = 0; i < IK_ToolKind_COUNT; i++)
    {
      UI_Box *b;

      UI_BoxFlags flags = UI_BoxFlag_DrawText|UI_BoxFlag_MouseClickable|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawBorder;
      B32 is_active = ik_tool() == i;
      if(is_active)
      {
        ui_set_next_palette(ui_build_palette(ui_top_palette(),
                                             .border = ik_rgba_from_theme_color(IK_ThemeColor_BaseBackground),
                                             .background = ik_rgba_from_theme_color(IK_ThemeColor_Breakpoint)));
        ui_set_next_font_size(ui_top_font_size()*1.3);
        flags |= UI_BoxFlag_DrawDropShadow;
      }

      UI_PrefWidth(ui_px(cell_width, 1.0))
        UI_PrefHeight(ui_px(cell_width, 1.0))
        UI_Flags(flags)
        UI_TextAlignment(UI_TextAlign_Center)
        b = ui_build_box_from_string(0, strs[i]);
      UI_Signal sig = ui_signal_from_box(b);

      if(ui_pressed(sig))
      {
        ik_state->tool = i;
      }
    }
  }
}

internal void
ik_ui_selection(void)
{
  ProfBeginFunction();
  IK_Frame *frame = ik_top_frame();
  IK_Camera *camera = &frame->camera;

  IK_Box *box = ik_box_from_key(ik_state->focus_hot_box_key);
  if(box && ik_tool() == IK_ToolKind_Selection)
  {
    Rng2F32 camera_rect = camera->rect;
    Rng2F32 fixed_rect = box->fixed_rect;
    Vec4F32 p0 = {fixed_rect.x0, fixed_rect.y0, 0, 1.0};
    Vec4F32 p1 = {fixed_rect.x1, fixed_rect.y1, 0, 1.0};
    p0 = transform_4x4f32(ik_state->proj_mat, p0);
    p1 = transform_4x4f32(ik_state->proj_mat, p1);
    p0.x = ((p0.x+1.0)/2.0) * ik_state->window_dim.x;
    p1.x = ((p1.x+1.0)/2.0) * ik_state->window_dim.x;
    p0.y = ((p0.y+1.0)/2.0) * ik_state->window_dim.y;
    p1.y = ((p1.y+1.0)/2.0) * ik_state->window_dim.y;
    Rng2F32 rect = r2f32p(p0.x, p0.y, p1.x, p1.y);
    F32 padding_px = mix_1f32(0, ui_top_font_size()*0.5, box->focus_hot_t);
    rect = pad_2f32(rect, padding_px);
    Vec2F32 rect_dim = dim_2f32(rect);

    UI_Box *container;
    UI_Rect(rect)
      // UI_Transparency(0.1)
      // UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
      container = ui_build_box_from_stringf(0, "##selection_box");

    // TODO: dragging won't trigger this
    B32 is_hot = contains_2f32(rect, ui_state->mouse);
    F32 hot_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "selection_hot_t"), is_hot, .reset = 0, .rate = ik_state->animation.slow_rate);

    Vec4F32 base_clr = ik_rgba_from_theme_color(IK_ThemeColor_BaseBorder);
    Vec4F32 hot_clr = ik_rgba_from_theme_color(IK_ThemeColor_MenuBarBorder);
    Vec4F32 clr = mix_4f32(base_clr, hot_clr, hot_t);
    clr.w = 1.0;
    Vec4F32 background_clr = ik_rgba_from_theme_color(IK_ThemeColor_BaseBackground);

    UI_Parent(container)
      UI_Palette(ui_build_palette(ui_top_palette(), .border = clr, .background = background_clr))
    {
      /////////////////////////////////
      //~ Corners

      // TODO(Next): rename to corner_length
      F32 corner_size = mix_1f32(0, 30, box->focus_hot_t);
      UI_BoxFlags flags = UI_BoxFlag_DrawBackground|UI_BoxFlag_MouseClickable|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_Floating;

      typedef struct IK_BoxResizeDrag IK_BoxResizeDrag;
      struct IK_BoxResizeDrag
      {
        Vec2F32 drag_start_position;
        Vec2F32 drag_start_rect_size;
        Vec2F32 drag_start_mouse_in_world;
      };

      // top left
      {
        // anchor
        UI_Box *anchor;
        UI_Rect(r2f32p(0,0, padding_px, padding_px))
          UI_Flags(flags|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideLeft)
          UI_HoverCursor(OS_Cursor_UpLeft)
          anchor = ui_build_box_from_stringf(0, "##top_left -> anchor");
        UI_Signal sig1 = ui_signal_from_box(anchor);

        // => right
        UI_Box *right;
        UI_Rect(r2f32p(padding_px,0, padding_px+corner_size, padding_px))
          UI_Flags(flags|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideRight)
          UI_HoverCursor(OS_Cursor_UpLeft)
          right = ui_build_box_from_stringf(0, "##top_left -> right");
        UI_Signal sig2 = ui_signal_from_box(right);

        // => down
        UI_Box *down;
        UI_Rect(r2f32p(0,padding_px, padding_px, padding_px+corner_size))
          UI_Flags(flags|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideRight)
          UI_HoverCursor(OS_Cursor_UpLeft)
          down = ui_build_box_from_stringf(0, "##top_left -> down");
        UI_Signal sig3 = ui_signal_from_box(down);

        // dragging
        UI_Signal sigs[3] = {sig1, sig2, sig3};
        for(U64 i = 0; i < 3; i++)
        {
          UI_Signal sig = sigs[i];
          if(sig.f&UI_SignalFlag_LeftDragging)
          {
            if(sig.f&UI_SignalFlag_LeftPressed)
            {
              IK_BoxResizeDrag drag = {box->position, box->rect_size, ik_state->mouse_in_world};
              ui_store_drag_struct(&drag);
            }
            else
            {
              IK_BoxResizeDrag drag = *ui_get_drag_struct(IK_BoxResizeDrag);
              Vec2F32 delta = sub_2f32(ik_state->mouse_in_world, drag.drag_start_mouse_in_world);

              // TODO: not working, bug here
              B32 keep_ratio = (box->flags&IK_BoxFlag_FixedRatio) || (sig.event_flags == OS_Modifier_Shift);
              if(keep_ratio)
              {
                F32 sum = delta.x + delta.y;
                F32 y = sum / (box->ratio+1.0);
                F32 x = sum - y;
                delta.x = x;
                delta.y = y;
              }

              Vec2F32 new_rect_size = add_2f32(drag.drag_start_rect_size, scale_2f32(delta, -1));
              if(box->flags & IK_BoxFlag_DragToResize)
              {
                box->rect_size = new_rect_size;
                box->position = add_2f32(drag.drag_start_position, delta);
              }
              if(box->flags & IK_BoxFlag_DragToScaleFontSize)
              {
                box->font_size *= (new_rect_size.x/box->rect_size.x);
                box->position = add_2f32(drag.drag_start_position, delta);
              }
            }
            break;
          }
        }
      }

      // bottom right
      {
        // anchor
        UI_Box *anchor;
        UI_Rect(r2f32p(rect_dim.x-padding_px,rect_dim.y-padding_px, rect_dim.x, rect_dim.y))
          UI_Flags(flags|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideRight)
          UI_HoverCursor(OS_Cursor_DownRight)
          anchor = ui_build_box_from_stringf(0, "##bottom_right -> anchor");
        UI_Signal sig1 = ui_signal_from_box(anchor);

        // => left
        UI_Box *left;
        UI_Rect(r2f32p(rect_dim.x-corner_size-padding_px, rect_dim.y-padding_px, rect_dim.x-padding_px, rect_dim.y))
          UI_Flags(flags|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideBottom|UI_BoxFlag_DrawSideTop)
          UI_HoverCursor(OS_Cursor_DownRight)
          left = ui_build_box_from_stringf(0, "##bottom_right -> left");
        UI_Signal sig2 = ui_signal_from_box(left);

        // => up
        UI_Box *up;
        UI_Rect(r2f32p(rect_dim.x-padding_px, rect_dim.y-padding_px-corner_size, rect_dim.x, rect_dim.y-padding_px))
          UI_Flags(flags|UI_BoxFlag_DrawSideTop|UI_BoxFlag_DrawSideLeft|UI_BoxFlag_DrawSideRight)
          UI_HoverCursor(OS_Cursor_DownRight)
          up = ui_build_box_from_stringf(0, "##bottom_right -> up");
        UI_Signal sig3 = ui_signal_from_box(up);

        // dragging
        UI_Signal sigs[3] = {sig1, sig2, sig3};
        for(U64 i = 0; i < 3; i++)
        {
          UI_Signal sig = sigs[i];
          if(sig.f&UI_SignalFlag_LeftDragging)
          {
            if(sig.f&UI_SignalFlag_LeftPressed)
            {
              IK_BoxResizeDrag drag = {box->position, box->rect_size, ik_state->mouse_in_world};
              ui_store_drag_struct(&drag);
            }
            else
            {
              IK_BoxResizeDrag drag = *ui_get_drag_struct(IK_BoxResizeDrag);
              Vec2F32 delta = sub_2f32(ik_state->mouse_in_world, drag.drag_start_mouse_in_world);
              // TODO(Next): not working, bug here
              B32 keep_ratio = (box->flags&IK_BoxFlag_FixedRatio) || (sig.event_flags == OS_Modifier_Shift);
              if(keep_ratio)
              {
                F32 sum = delta.x + delta.y;
                F32 y = sum / (box->ratio+1.0);
                F32 x = sum - y;
                delta.x = x;
                delta.y = y;
              }

              Vec2F32 new_rect_size = add_2f32(drag.drag_start_rect_size, delta);
              if(box->flags & IK_BoxFlag_DragToResize)
              {
                box->rect_size = new_rect_size;
              }
              if(box->flags & IK_BoxFlag_DragToScaleFontSize)
              {
                box->font_size *= (new_rect_size.x/box->rect_size.x);
              }
            }
            break;
          }
        }
      }

      /////////////////////////////////
      //~ Border

      {
        UI_BoxFlags flags = UI_BoxFlag_DrawBorder|UI_BoxFlag_Floating;
        Rng2F32 border_rect = {0,0, rect_dim.x, rect_dim.y};
        UI_Rect(pad_2f32(border_rect, -padding_px/2.0))
          UI_Flags(flags)
          ui_build_box_from_stringf(0, "##border");
      }
    }
  }
  ProfEnd();
}

internal void
ik_ui_inspector(void)
{
  ProfBeginFunction();
  IK_Box *b = ik_box_from_key(ik_state->focus_hot_box_key);
  B32 open = b != 0;
  F32 open_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "inspector_open_t"), open, .reset = 0, .rate = ik_state->animation.fast_rate);

  if(b)
  {
    UI_Box *container;
    F32 width_px = ui_top_font_size() * 20;
    Rng2F32 rect = {0, 0, width_px, ik_state->window_dim.y};
    rect.x1 = mix_1f32(rect.x0, rect.x1, open_t);
    rect.y1 = mix_1f32(rect.y0, rect.y1, open_t);
    UI_Rect(rect)
      UI_Flags(UI_BoxFlag_Floating)
      container = ui_build_box_from_stringf(0, "inspector_container");

    UI_Box *body;
    UI_Parent(container)
      UI_WidthFill
      UI_HeightFill
      UI_Column
      UI_Padding(ui_pct(1.0, 0.0))
      UI_Row
      UI_Padding(ui_em(1.0, 0.0))
      UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBackground)
      UI_PrefHeight(ui_children_sum(1.0))
      UI_Transparency(0.2)
      body = ui_build_box_from_stringf(0, "body");

    UI_Box *inner;
    UI_Parent(body)
      UI_WidthFill
      UI_HeightFill
      UI_Column
      UI_Padding(ui_em(0.2, 0.0))
      UI_Row
      UI_Padding(ui_em(0.2, 0.0))
      UI_ChildLayoutAxis(Axis2_Y)
      UI_PrefHeight(ui_children_sum(1.0))
      // UI_Flags(UI_BoxFlag_Clip)
      inner = ui_build_box_from_stringf(0, "inner");

    UI_Parent(inner)
    {
      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("name");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%s", b->name);
      }

      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("key");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%I64u", b->key.u64[0]);
      }

      ui_divider(ui_em(0.5, 0.0));

      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("position");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%.2f %.2f", b->position.x, b->position.y);
      }

      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("scale");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%3.2f %3.2f", b->scale.x, b->scale.y);
      }

      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("size");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%.2f %.2f", b->rect_size.x, b->rect_size.y);
      }

      UI_WidthFill
      UI_Row
      {
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("ratio");
        ui_spacer(ui_pct(1.0, 0.0));
        UI_PrefWidth(ui_text_dim(1, 1.0))
          ui_labelf("%.2f", b->ratio);
      }

      ui_divider(ui_em(0.5, 0.0));

      if(b->flags & IK_BoxFlag_HasDisplayString)
      {
        UI_WidthFill
        UI_Row
        {
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("string_size");
          ui_spacer(ui_pct(1.0, 0.0));
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("%I64u", b->string.size);
        }

        UI_WidthFill
        UI_Row
        {
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("font_size");
          ui_spacer(ui_pct(1.0, 0.0));
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("%I64u", b->font_size);
        }

        UI_WidthFill
        UI_Row
        {
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("text_padding");
          ui_spacer(ui_pct(1.0, 0.0));
          UI_PrefWidth(ui_text_dim(1, 1.0))
            ui_labelf("%.2f", b->text_padding);
        }

        ui_divider(ui_em(0.5, 0.0));
      }
    }
  }
  ProfEnd();
}

internal UI_Signal
ik_ui_checkbox(B32 *b)
{
  UI_Signal result;
  ui_set_next_child_layout_axis(Axis2_X);
  UI_Box *container_box = ui_build_box_from_string(0, str8((U8*)&b, sizeof(b)));

  UI_Parent(container_box)
  {
    ui_spacer(ui_pct(1,0));

    UI_Size size = ui_em(1.0, 1);
    UI_PrefWidth(size) UI_PrefHeight(size)
    {
      ui_set_next_hover_cursor(OS_Cursor_HandPoint);
      UI_Box *outer = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
          UI_BoxFlag_DrawBorder|
          UI_BoxFlag_DrawBackground|
          UI_BoxFlag_DrawHotEffects|
          UI_BoxFlag_DrawActiveEffects,
          "###outer");
      result = ui_signal_from_box(outer);
      F32 active_t = outer->active_t;
      F32 hot_t = outer->hot_t;

      Rng2F32 inner_rect = {0};
      inner_rect.x1 = outer->fixed_size.x;
      inner_rect.y1 = outer->fixed_size.y;
      F32 padding = 0;
      padding = padding + (*b != 0)*outer->fixed_size.x*0.175;
      // padding = padding + mix_1f32(0, -outer->fixed_size.x*0.025, hot_t);
      UI_Parent(outer) UI_Rect(pad_2f32(inner_rect, -(padding))) UI_Palette(ui_state->widget_palette_info.scrollbar_palette)
      {
        UI_Box *inner = ui_build_box_from_stringf(UI_BoxFlag_DrawHotEffects|
            UI_BoxFlag_DrawActiveEffects|
            UI_BoxFlag_DrawBorder|
            UI_BoxFlag_DrawBackground|
            UI_BoxFlag_DrawDropShadow, "###inner");
        inner->hot_t = Clamp(0, hot_t+*b, 1);
        inner->active_t = active_t;
      }
    }

    ui_spacer(ui_pct(1,0));
  }
  if(ui_clicked(result))
  {
    *b = !*b;
  }
  return result;
}

/////////////////////////////////
//~ Text Operation Functions

internal UI_TxtOp
ik_multi_line_txt_op_from_event(Arena *arena, UI_Event *event, String8 string, TxtPt cursor, TxtPt mark)
{
  TxtPt next_cursor = cursor;
  TxtPt next_mark = mark;
  TxtRng range = {0};
  String8 replace = {0};
  String8 copy = {0};
  UI_TxtOpFlags flags = 0;
  Vec2S32 delta = event->delta_2s32;

  // Resolve high-lelvel delta into byte delta, based on unit
  switch(event->delta_unit) 
  {
    default:{}break;
    case UI_EventDeltaUnit_Char:
    {
      // TODO(k): this should account for multi-byte characters in UTF-8... for now, just assume ASCII now
      // no-op
    }break;
    case UI_EventDeltaUnit_Word:
    {
      NotImplemented;
    }break;
    case UI_EventDeltaUnit_Line:
    {
      char *first = (char*)string.str;
      char *opl = (char*)(string.str+string.size);
      char *curr = (char*)(string.str+cursor.column);
      char *c = curr;

      // skip to current line's head
      for(; c > first && *(c-1) != '\n'; c--);
      U64 curr_line_column = curr-c;

      // TODO(Next): finish it
      if(delta.y < 0)
      {
        U64 linebreak_count = 0;
        for(; c >= first && linebreak_count < 2; c--)
        {
          if(*c == '\n') linebreak_count++;
        }
        if(linebreak_count > 0) delta.x = (c-curr+curr_line_column+1);
      }
      else
      {
        // TODO(Next)
      }
    }break;
    case UI_EventDeltaUnit_Whole:
    case UI_EventDeltaUnit_Page: 
    {
      U64 first_nonwhitespace_column = 1;
      for(U64 idx = 0; idx < string.size; idx++)
      {
        if(!char_is_space(string.str[idx]))
        {
          first_nonwhitespace_column = idx + 1;
          break;
        }
      }
      S64 home_dest_column = (first_nonwhitespace_column == cursor.column) ? 1 : first_nonwhitespace_column;
      delta.x = (delta.x > 0) ? ((S64)string.size+1 - cursor.column) : (home_dest_column - cursor.column);
    }break;
  }

  // Form next cursor
  if(txt_pt_match(cursor,mark))
  {
    next_cursor.column += delta.x;
  }

  if(!(event->flags & UI_EventFlag_KeepMark))
  {
    next_mark = next_cursor;
  }

  // Copying
  if(event->flags & UI_EventFlag_Copy)
  {
    copy = str8_substr(string, r1u64(cursor.column-1, mark.column-1));
    flags |= UI_TxtOpFlag_Copy;
  }

  // Pasting
  if(event->flags & UI_EventFlag_Paste)
  {
    range = txt_rng(cursor, mark);
    replace = os_get_clipboard_text(arena);
    next_cursor = next_mark = txt_pt(cursor.line, cursor.column+replace.size);
  }

  // Deletion
  if(event->flags & UI_EventFlag_Delete)
  {
    TxtPt new_pos = txt_pt_min(next_cursor, next_mark);
    range = txt_rng(next_cursor, next_mark);
    replace = str8_lit("");
    next_cursor = next_mark = new_pos;
  }

  // Insertion
  if(event->string.size != 0)
  {
    range = txt_rng(cursor, mark);
    replace = push_str8_copy(arena, event->string);
    next_cursor = next_mark = txt_pt(range.min.line, range.min.column+event->string.size);
  }

  //- rjf: determine if this event should be taken, based on bounds of cursor
  {
    if(next_cursor.column > string.size + 1 || 1 > next_cursor.column || event->delta_2s32.y != 0)
    {
      flags |= UI_TxtOpFlag_Invalid;
    }
    next_cursor.column = Clamp(1, next_cursor.column, string.size + replace.size + 1);
    next_mark.column   = Clamp(1, next_mark.column, string.size + replace.size + 1);
  }

  // Build+fill
  UI_TxtOp op = {0};
  op.flags = flags;
  op.replace = replace;
  op.copy = copy;
  op.range = range;
  op.cursor = next_cursor;
  op.mark = next_mark;
  return op;
}

internal Rng1U64
ik_replace_range_from_txtrng(String8 string, TxtRng txt_rng)
{
  char *first = (char*)string.str;
  char *opl = first+string.size;
  char *c = first;

  U64 min = 0;
  U64 max = 0;
  U64 line_index = 1;
  U64 column_index = 1;

  // skip to the min line
  for(; c < opl && line_index < txt_rng.min.line; c++)
  {
    if(*c == '\n')
    {
      line_index++;
      column_index = 1;
    }
    else
    {
      column_index++;
    }
  }

  // skip to the min column
  for(; c < opl && column_index < txt_rng.min.column; c++, column_index++)
  {
    AssertAlways(*c != '\n');
  }
  min = c - first;

  // skip to the last line
  for(; c < opl && line_index < txt_rng.max.line; c++)
  {
    if(*c == '\n')
    {
      line_index++;
      column_index = 1;
    }
    else
    {
      column_index++;
    }
  }

  // skip to the max column
  for(; c < opl && column_index < txt_rng.max.column; c++, column_index++)
  {
    AssertAlways(*c != '\n');
  }
  max = c - first;

  Rng1U64 ret = {min,max};
  return ret;
}

/////////////////////////////////
//~ Helpers

internal Vec2F32
ik_screen_pos_in_world(Mat4x4F32 proj_view_mat_inv, Vec2F32 pos)
{
  // mouse ndc pos
  F32 mox_ndc = (ik_state->mouse.x / ik_state->window_dim.x) * 2.f - 1.f;
  F32 moy_ndc = (ik_state->mouse.y / ik_state->window_dim.y) * 2.f - 1.f;
  Vec4F32 mouse_in_world_4 = transform_4x4f32(proj_view_mat_inv, v4f32(mox_ndc, moy_ndc, 1., 1.));
  Vec2F32 ret = v2f32(mouse_in_world_4.x, mouse_in_world_4.y);
  return ret;
}
