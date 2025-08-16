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

internal void
ik_ui_draw()
{
  Temp scratch = scratch_begin(0,0);
  F32 box_squish_epsilon = 0.001f;

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
  scratch_end(scratch);
}


/////////////////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////////////////
//~ State accessor/mutator

/////////////////////////////////
//- Init

internal void
ik_init(OS_Handle os_wnd, R_Handle r_wnd)
{
  Arena *arena = arena_alloc();
  ik_state = push_array(arena, IK_State, 1);
  {
    ik_state->arena       = arena;
    ik_state->os_wnd      = os_wnd;
    ik_state->r_wnd       = r_wnd;
    ik_state->dpi         = os_dpi_from_window(os_wnd);
    ik_state->last_dpi    = ik_state->last_dpi;
    ik_state->window_rect = os_client_rect_from_window(os_wnd, 1);
    ik_state->window_dim  = dim_2f32(ik_state->window_rect);

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
  ik_state->cfg_font_tags[IK_FontSlot_Main]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_Code]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_Icons]  = f_tag_from_path(str8_lit("./fonts/icons.ttf"));
  ik_state->cfg_font_tags[IK_FontSlot_Game]   = f_tag_from_path(str8_lit("./fonts/Mplus1Code-Medium.ttf"));

  // Theme 
  MemoryCopy(ik_state->cfg_theme_target.colors, ik_theme_preset_colors__handmade_hero, sizeof(ik_theme_preset_colors__handmade_hero));
  MemoryCopy(ik_state->cfg_theme.colors, ik_theme_preset_colors__handmade_hero, sizeof(ik_theme_preset_colors__handmade_hero));

  //////////////////////////////
  //- k: compute ui palettes from theme
  {
    IK_Theme *current = &ik_state->cfg_theme;
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
  }

  IK_InitStacks(ik_state)
  IK_InitStackNils(ik_state)
}

/////////////////////////////////
//- Frame

internal B32
ik_frame(void)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(0,0);

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Do per-frame resets

  ik_drawlist_reset(ik_frame_drawlist());
  arena_clear(ik_frame_arena());

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Remake drawing buckets every frame

  d_begin_frame();
  ik_state->bucket_rect = d_bucket_make();

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Get events from os
  
  {
    ik_state->os_events          = os_get_events(ik_frame_arena(), 0);
    ik_state->last_window_rect   = ik_state->window_rect;
    ik_state->last_window_dim    = dim_2f32(ik_state->last_window_rect);
    ik_state->window_res_changed = ik_state->window_rect.x0 == ik_state->last_window_rect.x0 && ik_state->window_rect.x1 == ik_state->last_window_rect.x1 && ik_state->window_rect.y0 == ik_state->last_window_rect.y0 && ik_state->window_rect.y1 == ik_state->last_window_rect.y1;
    ik_state->window_rect        = os_client_rect_from_window(ik_state->os_wnd, 0);
    ik_state->window_dim         = dim_2f32(ik_state->window_rect);
    ik_state->last_cursor        = ik_state->cursor;
    {
      Vec2F32 cursor = os_window_is_focused(ik_state->os_wnd) ? os_mouse_from_window(ik_state->os_wnd) : v2f32(-100,-100);
      if(cursor.x >= 0 && cursor.x <= ik_state->window_dim.x &&
         cursor.y >= 0 && cursor.y <= ik_state->window_dim.y)
      {
        ik_state->cursor = cursor;
      }
    }
    ik_state->cursor_delta       = sub_2f32(ik_state->cursor, ik_state->last_cursor);
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

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Calculate avg length in us of last many frames

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

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Pick target hz

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

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Build UI

  ////////////////////////////////
  //- Build event list for UI

  UI_EventList ui_events = {0};
  for(OS_Event *os_evt = ik_state->os_events.first; os_evt != 0; os_evt = os_evt->next)
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

    ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt);

    // TODO(k): we may want to handle this somewhere else
    if(os_evt->kind == OS_EventKind_WindowClose) {ik_state->window_should_close = 1;}
  }

  ////////////////////////////////
  //- Begin build UI

  {
    // Gather font info
    F_Tag main_font = ik_font_from_slot(IK_FontSlot_Main);
    F32 main_font_size = ik_font_size_from_slot(IK_FontSlot_Main);
    F_Tag icon_font = ik_font_from_slot(IK_FontSlot_Icons);

    // Build icon info
    UI_IconInfo icon_info = {0};
    {
      icon_info.icon_font = icon_font;
      icon_info.icon_kind_text_map[UI_IconKind_RightArrow]     = ik_icon_kind_text_table[IK_IconKind_RightScroll];
      icon_info.icon_kind_text_map[UI_IconKind_DownArrow]      = ik_icon_kind_text_table[IK_IconKind_DownScroll];
      icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]      = ik_icon_kind_text_table[IK_IconKind_LeftScroll];
      icon_info.icon_kind_text_map[UI_IconKind_UpArrow]        = ik_icon_kind_text_table[IK_IconKind_UpScroll];
      icon_info.icon_kind_text_map[UI_IconKind_RightCaret]     = ik_icon_kind_text_table[IK_IconKind_RightCaret];
      icon_info.icon_kind_text_map[UI_IconKind_DownCaret]      = ik_icon_kind_text_table[IK_IconKind_DownCaret];
      icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]      = ik_icon_kind_text_table[IK_IconKind_LeftCaret];
      icon_info.icon_kind_text_map[UI_IconKind_UpCaret]        = ik_icon_kind_text_table[IK_IconKind_UpCaret];
      icon_info.icon_kind_text_map[UI_IconKind_CheckHollow]    = ik_icon_kind_text_table[IK_IconKind_CheckHollow];
      icon_info.icon_kind_text_map[UI_IconKind_CheckFilled]    = ik_icon_kind_text_table[IK_IconKind_CheckFilled];
    }

    UI_WidgetPaletteInfo widget_palette_info = {0};
    {
      widget_palette_info.tooltip_palette   = ik_palette_from_code(IK_PaletteCode_Floating);
      widget_palette_info.ctx_menu_palette  = ik_palette_from_code(IK_PaletteCode_Floating);
      widget_palette_info.scrollbar_palette = ik_palette_from_code(IK_PaletteCode_ScrollBarButton);
    }

    // Build animation info
    UI_AnimationInfo animation_info = {0};
    {
      animation_info.hot_animation_rate      = ik_state->animation.fast_rate;
      animation_info.active_animation_rate   = ik_state->animation.fast_rate;
      animation_info.focus_animation_rate    = 1.f;
      animation_info.tooltip_animation_rate  = ik_state->animation.fast_rate;
      animation_info.menu_animation_rate     = ik_state->animation.fast_rate;
      animation_info.scroll_animation_rate   = ik_state->animation.fast_rate;
    }

    // Begin & push initial stack values
    ui_begin_build(ik_state->os_wnd, &ui_events, &icon_info, &widget_palette_info, &animation_info, ik_state->frame_dt);

    ui_push_font(main_font);
    ui_push_font_size(main_font_size);
    // ui_push_text_raster_flags(...);
    ui_push_text_padding(main_font_size*0.2f);
    ui_push_pref_width(ui_em(20.f, 1.f));
    ui_push_pref_height(ui_em(1.35f, 1.f));
    ui_push_palette(ik_palette_from_code(IK_PaletteCode_Base));
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Build Debug UI

  {
    ik_ui_stats();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Main scene building

  // Unpack ctx
  IK_Frame *frame = ik_state->active_frame;
  IK_Camera *camera = &frame->camera;

  ////////////////////////////////
  // Camera

  // Camera zoom/pan
  {
    B32 space_is_down = os_key_is_down(OS_Key_Space);
    for(OS_Event *os_evt = ik_state->os_events.first, *next = 0; os_evt != 0;)
    {
      next = os_evt->next;
      B32 taken = 0;

      // Zoom
      if(!space_is_down && os_evt->kind == OS_EventKind_Scroll && os_evt->modifiers == OS_Modifier_Ctrl)
      {
        F32 delta = os_evt->delta.y;

        Mat4x4F32 proj_mat = make_orthographic_vulkan_4x4f32(camera->rect.x0, camera->rect.x1, camera->rect.y1, camera->rect.y0, camera->zn, camera->zf);

        // Get normalized rect
        Rng2F32 rect = camera->rect;
        Vec2F32 rect_center = center_2f32(camera->rect);
        Vec2F32 shift = {-rect_center.x, -rect_center.y};
        Vec2F32 shift_inv = rect_center;
        rect = shift_2f32(rect, shift);

        F32 scale = 1 + delta*0.1;
        rect.x0 *= scale;
        rect.x1 *= scale;
        rect.y0 *= scale;
        rect.y1 *= scale;
        rect = shift_2f32(rect, shift_inv);
        camera->target_rect = rect;

        // TODO: anchor mouse pos in world

        // TODO: handle ratio changed caused by window resizing
        taken = 1;
      }

      // Pan
      if(space_is_down && os_evt->kind == OS_EventKind_Press && os_evt->key == OS_Key_LeftMouseButton)
      {
        camera->mouse_drag_start = ik_state->cursor;
        camera->rect_drag_start = camera->target_rect;
        camera->dragging = 1;
        taken = 1;
      }
      if(os_evt->kind == OS_EventKind_Release && (os_evt->key == OS_Key_LeftMouseButton || os_evt->key == OS_Key_Space))
      {
        taken = 1;
        camera->dragging = 0;
      }

      if(taken)
      {
        ik_eat_event(&ik_state->os_events, os_evt);
      }

      os_evt = next;
    }

    if(camera->dragging)
    {
      // Vec2F32 mouse_delta = sub_2f32(ik_state->cursor, camera->mouse_drag_start);
      Vec2F32 mouse_delta = sub_2f32(camera->mouse_drag_start, ik_state->cursor);
      // TODO: scale it based on the zoom level
      Rng2F32 rect = shift_2f32(camera->rect_drag_start, mouse_delta);
      camera->target_rect = rect;
    }
  }

  // Camera animation
  camera->rect.x0 += ik_state->animation.fast_rate * (camera->target_rect.x0-camera->rect.x0);
  camera->rect.x1 += ik_state->animation.fast_rate * (camera->target_rect.x1-camera->rect.x1);
  camera->rect.y0 += ik_state->animation.fast_rate * (camera->target_rect.y0-camera->rect.y0);
  camera->rect.y1 += ik_state->animation.fast_rate * (camera->target_rect.y1-camera->rect.y1);

  ////////////////////////////////
  //~ Box interaction

  IK_Frame(frame) IK_Parent(frame->root)
  {
    IK_Box *box = frame->root;
    while(box != 0)
    {
      IK_BoxRec rec = ik_box_rec_df_post(box, frame->root);

      /////////////////////////////////
      //~ Process events related to this box

      for(OS_Event *os_evt = ik_state->os_events.first, *next = 0; os_evt != 0;)
      {
        next = os_evt->next;
        B32 taken = 0;

        /////////////////////////////////
        //- Mouse over this box -> set hot

        IK_Key hot_pixel_key = ik_key_make(ik_state->hot_pixel_key, 0);
        if(ik_key_match(hot_pixel_key, box->key))
        {
          ik_state->hot_box_key = box->key;
        }

        /////////////////////////////////
        //- Mouse pressed in box -> set hot/active, mark signal accordingly

        if(os_evt->kind == OS_EventKind_Press && ik_key_match(ik_state->hot_box_key, box->key))
        {
          ik_state->active_box_key = box->key;
          taken = 1;
        }

        /////////////////////////////////
        //- Mouse release in active box -> unset hot/active

        /////////////////////////////////
        //- Mouse release outside of active box -> unset hot/active

        if(taken)
        {
          ik_eat_event(&ik_state->os_events, os_evt);
        }

        os_evt = next;
      }

      box = rec.next;
    }

    /////////////////////////////////
    //~ TODO: Create new rect

    /////////////////////////////////
    //~ Hover cursor

    {
      IK_Box *hot = ik_box_from_key(ik_state->hot_box_key);
      IK_Box *active = ik_box_from_key(ik_state->active_box_key);
      IK_Box *box = active == 0 ? hot : active;
      if(box)
      {
        OS_Cursor cursor = box->hover_cursor;
        if(os_window_is_focused(ik_state->os_wnd) || active != 0)
        {
          // TODO: will be override by ui_end_build
          os_set_cursor(cursor);
        }
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Main scene drawing

  // Unpack camera settings
  Rng2F32 viewport = ik_state->window_rect;
  Mat4x4F32 view_mat = mat_4x4f32(1.0);
  Mat4x4F32 proj_mat = make_orthographic_vulkan_4x4f32(camera->rect.x0, camera->rect.x1, camera->rect.y1, camera->rect.y0, camera->zn, camera->zf);

  // Start geo2d pass
  ik_state->bucket_geo2d = d_bucket_make();

  // Recursive drawing
  D_BucketScope(ik_state->bucket_geo2d)
  {
    d_geo2d_begin(viewport, view_mat, proj_mat);
    IK_Box *box = frame->root;
    while(box != 0)
    {
      IK_BoxRec rec = ik_box_rec_df_post(box, frame->root);

      // draw active indicator
      if(ik_key_match(box->key, ik_state->active_box_key))
      {
        Rng2F32 dst = box->rect;
        // TODO: change padding size based the area of the rect
        dst = pad_2f32(dst, 3);
        Rng2F32 src = {0,0,1,1};
        IK_DrawNode *dn = ik_drawlist_push_rect(ik_frame_arena(), ik_frame_drawlist(), dst, src);
        Mat4x4F32 xform = mat_4x4f32(1.0);
        Mat4x4F32 xform_inv = mat_4x4f32(1.0);

        R_Mesh2DInst *inst = d_sprite(dn->vertices, dn->indices, dn->vertices_buffer_offset, dn->indices_buffer_offset, dn->indice_count, dn->topology, dn->polygon, 0, r_handle_zero(), 1.);
        inst->key = box->key.u64[0];
        inst->xform = xform;
        inst->xform_inv = xform_inv;
        inst->has_texture = 0;
        inst->has_color = 1;
        inst->color = v4f32(1,0,0,1);
        // TODO: draw_edge is not working for now
        inst->draw_edge = 1;
      }

      // draw rect
      if(box->flags & IK_BoxFlag_DrawRect)
      {
        Rng2F32 dst = box->rect;
        Rng2F32 src = {0,0,1,1};
        IK_DrawNode *dn = ik_drawlist_push_rect(ik_frame_arena(), ik_frame_drawlist(), dst, src);
        Mat4x4F32 xform = mat_4x4f32(1.0);
        Mat4x4F32 xform_inv = mat_4x4f32(1.0);

        R_Mesh2DInst *inst = d_sprite(dn->vertices, dn->indices, dn->vertices_buffer_offset, dn->indices_buffer_offset, dn->indice_count, dn->topology, dn->polygon, 0, r_handle_zero(), 1.);
        inst->key = box->key.u64[0];
        inst->xform = xform;
        inst->xform_inv = xform_inv;
        inst->has_texture = 0;
        inst->has_color = 1;
        inst->color = box->color;
        inst->draw_edge = 1;
      }
      
      // TODO: draw image

      box = rec.next;
    }
  }

  // NOTE(k): there could be ui elements within node update
  // ik_state->sig = ui_signal_from_box(overlay);

  ///////////////////////////////////////////////////////////////////////////////////////
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

  ///////////////////////////////////////////////////////////////////////////////////////
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

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ Draw UI

  ui_end_build();
  D_BucketScope(ik_state->bucket_rect)
  {
    ik_ui_draw();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  //~ End of frame

  // end drag/drop if needed (no code handled drop)
  if(ik_state->drag_drop_state == IK_DragDropState_Dropping)
  {
    ik_state->drag_drop_state = IK_DragDropState_Null;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // Submit work

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
    ik_state->hot_pixel_key = r_window_end_frame(ik_state->r_wnd, ik_state->cursor);
    r_end_frame();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // Wait if we still have some cpu time left

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

  ///////////////////////////////////////////////////////////////////////////////////////
  // determine frame time, record it into history

  U64 end_time_us = os_now_microseconds();
  U64 frame_time_us = end_time_us-begin_time_us;
  ik_state->frame_time_us_history[ik_state->frame_index%ArrayCount(ik_state->frame_time_us_history)] = frame_time_us;

  ///////////////////////////////////////////////////////////////////////////////////////
  // bump frame time counters

  ik_state->frame_index++;
  ik_state->time_in_seconds += ik_state->frame_dt;
  ik_state->time_in_us += frame_time_us;

  ///////////////////////////////////////////////////////////////////////////////////////
  // end

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

/////////////////////////////////
//- Color, Fonts, Config

//- colors
internal Vec4F32
ik_rgba_from_theme_color(IK_ThemeColor color)
{
  return ik_state->cfg_theme.colors[color];
}

//- code -> palette
internal UI_Palette *
ik_palette_from_code(IK_PaletteCode code)
{
  UI_Palette *result = &ik_state->cfg_ui_debug_palettes[code];
  return result;
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

// box table lookup
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

/////////////////////////////////
//~ OS event consumption helpers

internal void
ik_eat_event(OS_EventList *list, OS_Event *evt)
{
  DLLRemove(list->first, list->last, evt);
  list->count--;
}

internal B32
ik_key_press(OS_Modifiers mods, OS_Key key)
{
  ProfBeginFunction();
  B32 result = 0;
  for(OS_Event *evt = ik_state->os_events.first; evt != 0; evt = evt->next)
  {
    if(evt->kind == OS_EventKind_Press && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      ik_eat_event(&ik_state->os_events, evt);
      break;
    }
  }
  ProfEnd();
  return result;
}

internal B32
ik_key_release(OS_Modifiers mods, OS_Key key)
{
  ProfBeginFunction();
  B32 result = 0;
  for(OS_Event *evt = ik_state->os_events.first; evt != 0; evt = evt->next)
  {
    if(evt->kind == OS_EventKind_Release && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      ik_eat_event(&ik_state->os_events, evt);
      break;
    }
  }
  ProfEnd();
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Dynamic drawing (in immediate mode fashion)

/////////////////////////////////
//- Basic building API

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
//- High level building API 

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

/////////////////////////////////////////////////////////////////////////////////////////
//~ Box Type Functions

internal IK_BoxRec
ik_box_rec_df(IK_Box *box, IK_Box *root, U64 sib_member_off, U64 child_member_off)
{
  // Depth first search starting from the current box 
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

/////////////////////////////////////////////////////////////////////////////////////////
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

  // Create root
  ik_push_frame(ret);
  IK_Box *root = ik_build_box_from_stringf(0, "##root");
  ik_pop_frame();

  // TODO: testing
  IK_Frame(ret) IK_Parent(root)
  {
    IK_Box *box = ik_build_box_from_stringf(0, "##demo_rect");
    box->flags |= IK_BoxFlag_DrawRect;
    box->rect = r2f32p(300, 300, 600, 600);
    box->color = v4f32(1,1,0,1.0);
    box->hover_cursor = OS_Cursor_HandPoint;
  }

  /////////////////////////////////
  // Fill default settings

  // camera
  ret->camera.rect = ik_state->window_rect;
  ret->camera.target_rect = ret->camera.rect;
  ret->camera.zn = -0.1;
  ret->camera.zf = 1000000.0;

  ret->root = root;
  return ret;
}

internal void
ik_frame_release(IK_Frame *frame)
{
  SLLStackPush(ik_state->first_free_frame, frame);
}

/////////////////////////////////////////////////////////////////////////////////////////
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

  // Grab active parent
  IK_Box *parent = ik_top_parent();
  if(parent)
  {
    // Insert to tree
    DLLPushBack(parent->first, parent->last, ret);
    parent->children_count++;
  }

  // Fill box info
  ret->key = key;
  ret->flags = flags;
  ret->frame = top_frame;

  // Hook into lookup table
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
  // Grab active parent
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

/////////////////////////////////
//- box node equipment

internal String8
ik_box_equip_display_string(IK_Box *box, String8 string)
{
  // box->string = push_str8_copy(ui_build_arena(), string);
  // box->flags |= IK_BoxFlag_HasDisplayString;
  // IK_ColorCode text_color_code = (box->flags & IK_BoxFlag_DrawTextWeak ? IK_ColorCode_TextWeak : IK_ColorCode_Text);

  // if(box->flags & IK_BoxFlag_DrawText)
  // {
  //   String8 display_string = ui_box_display_string(box);
  //   D_FancyStringNode fancy_string_n = {0};
  //   fancy_string_n.next = 0;
  //   fancy_string_n.v.font                    = box->font;
  //   fancy_string_n.v.string                  = display_string;
  //   fancy_string_n.v.color                   = box->palette->colors[text_color_code];
  //   fancy_string_n.v.size                    = box->font_size;
  //   fancy_string_n.v.underline_thickness     = 0;
  //   fancy_string_n.v.strikethrough_thickness = 0;

  //   D_FancyStringList fancy_strings = {0};
  //   fancy_strings.first = &fancy_string_n;
  //   fancy_strings.last = &fancy_string_n;
  //   fancy_strings.node_count = 1;
  //   box->display_string_runs = d_fancy_run_list_from_fancy_string_list(ui_build_arena(), box->tab_size, box->text_raster_flags, &fancy_strings);
  // }
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ UI Widget

internal void ik_ui_stats(void)
{
  UI_Box *container = 0;
  UI_Rect(ik_state->window_rect)
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
        ui_labelf("mouse");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.2f, %.2f", ui_state->mouse.x, ui_state->mouse.y);
      }
      UI_Row
      {
        ui_labelf("window");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.2f, %.2f", ik_state->window_dim.x, ik_state->window_dim.y);
      }
      UI_Row
      {
        ui_labelf("drag start mouse");
        ui_spacer(ui_pct(1.0, 0.0));
        ui_labelf("%.2f, %.2f", ui_state->drag_start_mouse.x, ui_state->drag_start_mouse.y);
      }
    }
  }

}
