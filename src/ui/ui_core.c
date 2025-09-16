/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Basic Type Functions

internal U64
ui_hash_from_string(U64 seed, String8 string)
{
  U64 result = XXH3_64bits_withSeed(string.str, string.size, seed);
  return result;
}

internal String8
ui_hash_part_from_key_string(String8 string)
{
  String8 result = string;

  // rjf: look for ### patterns, which can replace the entirety of the part of
  // the string that is hashed.
  U64 hash_replace_signifier_pos = str8_find_needle(string, 0, str8_lit("###"), 0);
  if(hash_replace_signifier_pos < string.size)
  {
    result = str8_skip(string, hash_replace_signifier_pos);
  }

  return result;
}

internal String8
ui_display_part_from_key_string(String8 string)
{
  U64 hash_pos = str8_find_needle(string, 0, str8_lit("##"), 0);
  string.size = hash_pos;
  return string;
}

internal UI_Key
ui_key_zero(void)
{
  UI_Key result = {0};
  return result;
}

internal UI_Key
ui_key_make(U64 v)
{
  UI_Key result = {v};
  return result;
}

internal UI_Key
ui_key_from_string(UI_Key seed_key, String8 string)
{
  UI_Key result = {0};
  if(string.size != 0)
  {
    String8 hash_part = ui_hash_part_from_key_string(string);
    result.u64[0] = ui_hash_from_string(seed_key.u64[0], hash_part);
  }
  return result;
}

internal UI_Key
ui_key_from_stringf(UI_Key seed_key, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Key result = ui_key_from_string(seed_key, string);
  scratch_end(scratch);
  return result;
}

internal B32
ui_key_match(UI_Key a, UI_Key b)
{
  return a.u64[0] == b.u64[0];
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Event Types Functions

internal UI_EventNode *
ui_event_list_push(Arena *arena, UI_EventList *list, UI_Event *v)
{
  UI_EventNode *n = push_array(arena, UI_EventNode, 1);
  MemoryCopy(&n->v, v, sizeof(UI_Event));
  DLLPushBack(list->first, list->last, n);
  list->count += 1;
  return n;
}

internal void ui_eat_event_node(UI_EventList *list, UI_EventNode *node)
{
  DLLRemove(list->first, list->last, node);
  list->count -= 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Box Type Functions

internal B32
ui_box_is_nil(UI_Box *box)
{
  return box == 0 || box == &ui_nil_box;
}

internal UI_BoxRec
ui_box_rec_df(UI_Box *box, UI_Box *root, U64 sib_member_off, U64 child_member_off)
{
  // Depth first search starting from the current box 
  UI_BoxRec result = {0};
  result.next = &ui_nil_box;
  if(!ui_box_is_nil(*MemberFromOffset(UI_Box **, box, child_member_off)))
  {
    result.next = *MemberFromOffset(UI_Box **, box, child_member_off);
    result.push_count = 1;
  }
  else for(UI_Box *p = box; !ui_box_is_nil(p) && p != root; p = p->parent)
  {
    if(!ui_box_is_nil(*MemberFromOffset(UI_Box **, p, sib_member_off)))
    {
      result.next = *MemberFromOffset(UI_Box **, p, sib_member_off);
      break;
    }
    result.pop_count += 1;
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: State Allocating / Selection

internal UI_State *
ui_state_alloc(void)
{
  Arena *arena = arena_alloc();
  UI_State *ui = push_array(arena, UI_State, 1);
  ui->arena = arena;

  for(U64 i = 0; i < ArrayCount(ui->build_arenas); i++)
  {
    ui->build_arenas[i] = arena_alloc();
  }

  ui->drag_state_arena = arena_alloc();
  ui->box_table_size   = 4096;
  ui->box_table        = push_array(arena, UI_BoxHashSlot, ui->box_table_size);
  ui->anim_slots_count = 4096;
  ui->anim_slots       = push_array(arena, UI_AnimSlot, ui->anim_slots_count);

  UI_InitStackNils(ui);
  return ui;
}

internal UI_Box *
ui_root_from_state(UI_State *state)
{
  return state->root;
}

internal void
ui_select_state(UI_State *s)
{
  ui_state = s;
}

/////////////////////////////////////////////////////////////////////////////////////////
/// Implicit State Accessor/Mutators

internal Arena *
ui_build_arena(void)
{
  Arena *result = ui_state->build_arenas[ui_state->build_index%ArrayCount(ui_state->build_arenas)];
  return result;
}

internal OS_Handle ui_window(void) { return ui_state->os_wnd; }
internal UI_EventList *ui_events(void) { return ui_state->events; }
internal F_Tag ui_icon_font(void) { return ui_state->icon_info.icon_font; }
internal String8 ui_icon_string_from_kind(UI_IconKind icon_kind) { return ui_state->icon_info.icon_kind_text_map[icon_kind]; }
internal Vec2F32 ui_mouse(void) { return ui_state->mouse; }
internal F32 ui_dt(void) { return ui_state->animation_dt; }

//- rjf: event pumping

internal B32
ui_next_event(UI_Event **ev)
{
  UI_EventList *events = ui_state->events;
  UI_EventNode *start_node = events->first;
  if(ev[0] != 0)
  {
    start_node = CastFromMember(UI_EventNode, v, ev[0]);
    start_node = start_node->next;
    ev[0] = 0;
  }
  if(start_node != 0)
  {
    UI_PermissionFlags perms = ui_top_permission_flags();
    for(UI_EventNode *n = start_node; n != 0; n = n->next)
    {
      B32 good = 1;
      if(!(perms & UI_PermissionFlag_ClicksLeft) &&
         (n->v.kind == UI_EventKind_Press ||
          n->v.kind == UI_EventKind_Release) &&
         (n->v.key == OS_Key_LeftMouseButton))
      {
        good = 0;
      }
      if(!(perms & UI_PermissionFlag_ClicksMiddle) &&
         (n->v.kind == UI_EventKind_Press ||
          n->v.kind == UI_EventKind_Release) &&
         (n->v.key == OS_Key_MiddleMouseButton))
      {
        good = 0;
      }
      if(!(perms & UI_PermissionFlag_ClicksRight) &&
         (n->v.kind == UI_EventKind_Press ||
          n->v.kind == UI_EventKind_Release) &&
         (n->v.key == OS_Key_RightMouseButton))
      {
        good = 0;
      }
      if(!(perms & UI_PermissionFlag_ScrollX) && (n->v.kind == UI_EventKind_Scroll) && (n->v.delta_2f32.x != 0 || n->v.modifiers == OS_Modifier_Shift))
      {
        good = 0;
      }
      if(!(perms & UI_PermissionFlag_ScrollY) && (n->v.kind == UI_EventKind_Scroll) && n->v.delta_2f32.y != 0 && n->v.modifiers == 0)
      {
        good = 0;
      }
      if((n->v.kind == UI_EventKind_Press ||
          n->v.kind == UI_EventKind_Release ||
          n->v.kind == UI_EventKind_Navigate ||
          n->v.kind == UI_EventKind_Edit) &&
         (n->v.key != OS_Key_LeftMouseButton &&
          n->v.key != OS_Key_MiddleMouseButton &&
          n->v.key != OS_Key_RightMouseButton))
      {
        if((perms & UI_PermissionFlag_Keyboard) == UI_PermissionFlag_KeyboardSecondary)
        {
          good = !!(n->v.flags & UI_EventFlag_Secondary);
        }
        else if(!(perms & UI_PermissionFlag_Keyboard))
        {
          good = 0;
        }
      }
      else if(!(perms & UI_PermissionFlag_Text) && (n->v.kind == UI_EventKind_Text))
      {
        good = 0;
      }
      if(good)
      {
        ev[0] = &n->v;
        break;
      }
    }
  }
  B32 result = !!ev[0];
  return result;
}

internal void
ui_eat_event(UI_Event *ev)
{
  if(ev != 0)
  {
    UI_EventNode *n = CastFromMember(UI_EventNode, v, ev);
    ui_eat_event_node(ui_state->events, n);
  }
}

//- rjf: event consumption helpers

internal B32
ui_key_press(OS_Modifiers mods, OS_Key key)
{
  B32 result = 0;
  for(UI_Event *evt = 0; ui_next_event(&evt);)
  {
    if(evt->kind == UI_EventKind_Press && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      ui_eat_event(evt);
      break;
    }
  }
  return result;
}

internal B32
ui_key_release(OS_Modifiers mods, OS_Key key)
{
  B32 result = 0;
  for(UI_Event *evt = 0; ui_next_event(&evt);)
  {
    if(evt->kind == UI_EventKind_Release && evt->key == key && evt->modifiers == mods)
    {
      result = 1;
      ui_eat_event(evt);
      break;
    }
  }
  return result;
}

internal B32
ui_text(U32 character)
{
  B32 result = 0;
  Temp scratch = scratch_begin(0, 0);
  String8 character_text = str8_from_32(scratch.arena, str32(&character, 1));
  for(UI_Event *evt = 0; ui_next_event(&evt);)
  {
    if(evt->kind == UI_EventKind_Text && str8_match(character_text, evt->string, 0))
    {
      result = 1;
      ui_eat_event(evt);
      break;
    }
  }
  scratch_end(scratch);
  return result;
}

internal B32
ui_slot_press(UI_EventActionSlot slot)
{
  B32 result = 0;
  for(UI_Event *evt = 0; ui_next_event(&evt);)
  {
    if(evt->kind == UI_EventKind_Press && evt->slot == slot)
    {
      result = 1;
      ui_eat_event(evt);
      break;
    }
  }
  return result;
}

//- rjf: drag data
internal Vec2F32
ui_drag_start_mouse(void)
{
  return ui_state->drag_start_mouse;
}

internal Vec2F32
ui_drag_delta(void)
{
  return sub_2f32(ui_mouse(), ui_state->drag_start_mouse);
}

internal void
ui_store_drag_data(String8 string)
{
  arena_clear(ui_state->drag_state_arena);
  ui_state->drag_state_data = push_str8_copy(ui_state->drag_state_arena, string);
}

internal String8
ui_get_drag_data(U64 min_required_size)
{
  // NOTE: if left size is 0 and it's causing assertion failed
  //       check duplicated ui box keys
  AssertAlways(ui_state->drag_state_data.size >= min_required_size);

  // TODO: don't get it
  // if(ui_state->drag_state_data.size < min_required_size)
  // {
  //   Temp scratch = scratch_begin(0, 0);
  //   String8 str = {push_array(scratch.arena, U8, min_required_size), min_required_size};
  //   ui_store_drag_data(str);
  //   scratch_end(scratch);
  // }
  return ui_state->drag_state_data;
}

internal UI_Box *
ui_box_from_key(UI_Key key)
{
  UI_Box *result = &ui_nil_box;

  if(!ui_key_match(key, ui_key_zero()))
  {
    U64 slot = key.u64[0] % ui_state->box_table_size;
    for(UI_Box *b = ui_state->box_table[slot].hash_first; !ui_box_is_nil(b); b = b->hash_next)
    {
      if(ui_key_match(b->key, key))
      {
        result = b;
        break;
      }
    }
  }
  return result;
}

//- rjf: interaction keys

internal UI_Key
ui_hot_key(void)
{
  return ui_state->hot_box_key;
}

internal UI_Key
ui_active_key(UI_MouseButtonKind button_kind)
{
  return ui_state->active_box_key[button_kind];
}

internal UI_Key
ui_drop_hot_key(void)
{
  return ui_state->drop_hot_box_key;
}

//- rjf: controls over interaction
internal void ui_kill_action(void)
{
  for EachEnumVal(UI_MouseButtonKind, k)
  {
    ui_state->active_box_key[k] = ui_key_zero();
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Top-Level Building API

internal void
ui_begin_build(OS_Handle os_wnd, UI_EventList *events, UI_IconInfo *icon_info, UI_WidgetPaletteInfo *widget_palette_info, UI_AnimationInfo *animation_info, F32 animation_dt)
{
  ProfBeginFunction();
  //- k: reset per-build ui state
  {
    UI_InitStacks(ui_state);
    ui_state->root = &ui_nil_box;
    ui_state->ctx_menu_touched_this_frame = 0;
    ui_state->default_animation_rate = 1 - pow_f32(2, (-50.f * ui_state->animation_dt));
    ui_state->last_build_box_count = ui_state->build_box_count;
    ui_state->build_box_count = 0;
    ui_state->tooltip_open = 0;
    ui_state->tooltip_can_overflow_window = 0;
    ui_state->tooltip_anchor_key = ui_key_zero();
    ui_state->ctx_menu_changed = 0;
  }

  // prune unused animation nodes
  ProfScope("ui prune unused animation nodes")
  {
    for(UI_AnimNode *n = ui_state->lru_anim_node, *next = &ui_nil_anim_node; n != &ui_nil_anim_node && n != 0; n = next)
    {
      next = n->lru_next;
      if(n->last_touched_build_index+2 < ui_state->build_index)
      {
        U64 slot_idx = n->key.u64[0]%ui_state->anim_slots_count;
        UI_AnimSlot *slot = &ui_state->anim_slots[slot_idx];
        DLLRemove_NPZ(&ui_nil_anim_node, slot->first, slot->last, n, slot_next, slot_prev);;
        DLLRemove_NPZ(&ui_nil_anim_node, ui_state->lru_anim_node, ui_state->mru_anim_node, n, lru_next, lru_prev);
        SLLStackPush_N(ui_state->free_anim_node, n, slot_next);
      }
      else
      {
        break;
      }
    }
  }

  //- k: detect mouse-moves
  for(UI_EventNode *n = events->first; n!=0; n = n->next)
  {
    if(n->v.kind == UI_EventKind_MouseMove)
    {
      ui_state->mouse = n->v.pos;
      ui_state->last_time_mousemoved_us = os_now_microseconds();
    }
  }

  if(ui_state->build_index == 0)
  {
    ui_state->mouse = os_mouse_from_window(os_wnd);
  }

  //- k: fill build phase parameters
  {
    ui_state->os_wnd = os_wnd;
    ui_state->animation_dt = animation_dt;
    ui_state->events = events;
    // NOTE(k): User might be dragging something from another application into this window
    ui_state->mouse = (os_window_is_focused(os_wnd) || ui_state->last_time_mousemoved_us+500000 >= os_now_microseconds()) ? os_mouse_from_window(os_wnd) : v2f32(-100,-100);

    // Icon
    MemoryZeroStruct(&ui_state->icon_info);
    ui_state->icon_info.icon_font = icon_info->icon_font;
    for(UI_IconKind icon_kind = UI_IconKind_Null; icon_kind < UI_IconKind_COUNT; icon_kind = (UI_IconKind)(icon_kind+1))
    {
      ui_state->icon_info.icon_kind_text_map[icon_kind] = push_str8_copy(ui_build_arena(), icon_info->icon_kind_text_map[icon_kind]);
    }
    MemoryCopyStruct(&ui_state->widget_palette_info, widget_palette_info);
    MemoryCopyStruct(&ui_state->animation_info, animation_info);
  }

  // next-default-nav-focus keys => current-default-nav-focus-keys
  {
    for(U64 slot = 0; slot < ui_state->box_table_size; slot++)
    {
      for(UI_Box *box = ui_state->box_table[slot].hash_first;
          !ui_box_is_nil(box);
          box = box->hash_next)
      {
        box->default_nav_focus_hot_key = box->default_nav_focus_next_hot_key;
        box->default_nav_focus_active_key = box->default_nav_focus_next_active_key;
      }
    }
  }

  //- k: build top-level root
  {
    Rng2F32 window_rect = os_client_rect_from_window(os_wnd,0);
    Vec2F32 window_rect_size = dim_2f32(window_rect);
    ui_set_next_fixed_width(window_rect_size.x);
    ui_set_next_fixed_height(window_rect_size.y);
    ui_set_next_child_layout_axis(Axis2_X);

    // TODO: move this to a dedicated root box
    ui_set_next_focus_hot(UI_FocusKind_On);
    ui_set_next_focus_active(UI_FocusKind_On);
    ui_set_next_flags(UI_BoxFlag_DefaultFocusNav);

    UI_Box *root = ui_build_box_from_stringf(0, "###%I64x", os_wnd.u64[0]);
    ui_push_parent(root);
    ui_state->root = root;
  }

  // setup parent box for tooltip
  UI_FixedX(ui_state->mouse.x+15.f) UI_FixedY(ui_state->mouse.y+15.f) UI_PrefWidth(ui_children_sum(1.f)) UI_PrefHeight(ui_children_sum(1.f))
  {
    ui_set_next_child_layout_axis(Axis2_Y);
    ui_state->tooltip_root = ui_build_box_from_stringf(0, "###tooltip_%I64x", os_wnd.u64[0]);
  }

  //- rjf: setup parent box for context menu
  ui_state->ctx_menu_open = ui_state->next_ctx_menu_open;
  ui_state->ctx_menu_anchor_key = ui_state->next_ctx_menu_anchor_key;
  {
    UI_Box *anchor_box = ui_box_from_key(ui_state->ctx_menu_anchor_key);
    if(!ui_box_is_nil(anchor_box))
    {
      ui_state->ctx_menu_anchor_box_last_pos = anchor_box->rect.p0;
    }
    Vec2F32 anchor = add_2f32(ui_state->ctx_menu_anchor_box_last_pos, ui_state->ctx_menu_anchor_off);
    UI_FixedX(anchor.x) UI_FixedY(anchor.y) UI_PrefWidth(ui_children_sum(1.f)) UI_PrefHeight(ui_children_sum(1.f))
      UI_Focus(UI_FocusKind_On)
      UI_Squish(0.1f-ui_state->ctx_menu_open_t*0.1f)
      UI_Transparency(1-ui_state->ctx_menu_open_t)
    {
      ui_set_next_child_layout_axis(Axis2_Y);
      ui_state->ctx_menu_root = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                          UI_BoxFlag_SquishAnchored|
                                                          UI_BoxFlag_DrawDropShadow|
                                                          (ui_state->ctx_menu_open*UI_BoxFlag_DefaultFocusNavY),
                                                          "###ctx_menu_%I64x", os_wnd.u64[0]);
    }
  }

  //- rjf: reset hot if we don't have an active widget
  {
    B32 has_active = 0;
    for EachEnumVal(UI_MouseButtonKind, k)
    {
      if(!ui_key_match(ui_state->active_box_key[k], ui_key_zero()))
      {
        has_active = 1;
      }
    }
    if(!has_active)
    {
      ui_state->hot_box_key = ui_key_zero();
    }
  }

  // reset drop-hot key
  {
    ui_state->drop_hot_box_key = ui_key_zero();
  }

  // reset active if active box is disabled
  for EachEnumVal(UI_MouseButtonKind, k)
  {
    if(!ui_key_match(ui_state->active_box_key[k], ui_key_zero()))
    {
      UI_Box *box = ui_box_from_key(ui_state->active_box_key[k]);
      if(!ui_box_is_nil(box) && box->flags & UI_BoxFlag_Disabled)
      {
        ui_state->active_box_key[k] = ui_key_zero();
      }
    }
  }

  // reset active keys if they have been pruned
  for EachEnumVal(UI_MouseButtonKind, k)
  {
    UI_Box *box = ui_box_from_key(ui_state->active_box_key[k]);
    if(ui_box_is_nil(box))
    {
      ui_state->active_box_key[k] = ui_key_zero();
    }
  }

  //- rjf: escape -> close context menu
  if(ui_any_ctx_menu_is_open() && ui_slot_press(UI_EventActionSlot_Cancel))
  {
    ui_ctx_menu_close();
  }
  ProfEnd();
}

internal void
ui_end_build(void)
{
  ProfBeginFunction();

  //- rjf: prune untouched or transient boxes in the cache
  ProfScope("prune ui nodes")
  for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx++)
  {
    for(UI_Box *box = ui_state->box_table[slot_idx].hash_first; !ui_box_is_nil(box); box = box->hash_next)
    {
      Assert(!ui_key_match(box->key, ui_key_zero()));
      if(box->last_touched_build_index < ui_state->build_index)
      {
        DLLRemove_NPZ(&ui_nil_box,
                      ui_state->box_table[slot_idx].hash_first,
                      ui_state->box_table[slot_idx].hash_last,
                      box,
                      hash_next,hash_prev);
        SLLStackPush(ui_state->first_free_box, box);
      }
    }
  }

  //- rjf: layout box tree
  ProfScope("ui layout") for(Axis2 axis = 0; axis < Axis2_COUNT; axis++)
  {
    ui_layout_root(ui_state->root, axis);
  }

  //- rjf: close ctx menu if untouched
  if(!ui_state->ctx_menu_touched_this_frame)
  {
    ui_ctx_menu_close();
  }
  
  //- rjf: stick ctx menu to anchor
  if(ui_state->ctx_menu_touched_this_frame && !ui_state->ctx_menu_changed)
  {
    UI_Box *anchor_box = ui_box_from_key(ui_state->ctx_menu_anchor_key);
    if(!ui_box_is_nil(anchor_box))
    {
      Rng2F32 root_rect = ui_state->ctx_menu_root->rect;
      Vec2F32 pos =
      {
        anchor_box->rect.x0 + ui_state->ctx_menu_anchor_off.x,
        anchor_box->rect.y0 + ui_state->ctx_menu_anchor_off.y,
      };
      Vec2F32 shift = sub_2f32(pos, root_rect.p0);
      Rng2F32 new_root_rect = shift_2f32(root_rect, shift);
      ui_state->ctx_menu_root->fixed_position = new_root_rect.p0;
      ui_state->ctx_menu_root->fixed_size = dim_2f32(new_root_rect);
      ui_state->ctx_menu_root->rect = new_root_rect;
    }
  }
  
  //- rjf: anchor tooltips
  if(!ui_key_match(ui_state->tooltip_anchor_key, ui_key_zero()))
  {
    UI_Box *anchor_box = ui_box_from_key(ui_state->tooltip_anchor_key);
    if(!ui_box_is_nil(anchor_box))
    {
      Vec2F32 dim = dim_2f32(ui_state->tooltip_root->rect);
      ui_state->tooltip_root->fixed_position.x = ui_state->tooltip_root->rect.x0 = anchor_box->rect.x0;
      ui_state->tooltip_root->fixed_position.y = ui_state->tooltip_root->rect.y0 = anchor_box->rect.y1 + anchor_box->font_size*0.5f;
      ui_state->tooltip_root->rect.x1 = ui_state->tooltip_root->rect.x0 + dim.x;
      ui_state->tooltip_root->rect.y1 = ui_state->tooltip_root->rect.y0 + dim.y;
    }
    else
    {
      ui_state->tooltip_root->rect.x0 = 10000;
      ui_state->tooltip_root->rect.y0 = 10000;
    }
  }

  //- rjf: ensure special floating roots are within screen bounds
  UI_Box *floating_roots[] = {ui_state->tooltip_root, ui_state->ctx_menu_root};
  B32 force_contain[] =
  {
    !ui_state->tooltip_can_overflow_window,
    1,
  };
  for(U64 idx = 0; idx < ArrayCount(floating_roots); idx += 1)
  {
    UI_Box *root = floating_roots[idx];
    if(!ui_box_is_nil(root))
    {
      Rng2F32 window_rect = os_client_rect_from_window(ui_state->os_wnd, 0);
      Vec2F32 window_dim = dim_2f32(window_rect);
      Rng2F32 root_rect = root->rect;
      Vec2F32 shift_down =
      {
        -ClampBot(0, root_rect.x1 - window_rect.x1) * (force_contain[idx]),
        -ClampBot(0, root_rect.y1 - window_rect.y1) * (force_contain[idx]),
      };
      Rng2F32 new_root_rect = shift_2f32(root_rect, shift_down);
      Vec2F32 shift_up =
      {
        ClampBot(0, window_rect.x0 - new_root_rect.x0) * (force_contain[idx]),
        ClampBot(0, window_rect.y0 - new_root_rect.y0) * (force_contain[idx]),
      };
      new_root_rect = shift_2f32(new_root_rect, shift_up);
      root->fixed_position = new_root_rect.p0;
      root->fixed_size = dim_2f32(new_root_rect);
      root->rect = new_root_rect;
      for(Axis2 axis = (Axis2)0; axis < Axis2_COUNT; axis = (Axis2)(axis + 1))
      {
        ui_calc_sizes_standalone__in_place_rec(root, axis);
        ui_calc_sizes_upwards_dependent__in_place_rec(root, axis);
        ui_calc_sizes_downwards_dependent__in_place_rec(root, axis);
        ui_layout_enforce_constraints__in_place_rec(root, axis);
        ui_layout_position__in_place_rec(root, axis);
      }
    }
  }

  // Enforce child-rounding
  ProfScope("enforce child-rounding") for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx++)
  {
    for(UI_Box *box = ui_state->box_table[slot_idx].hash_first; !ui_box_is_nil(box); box = box->hash_next)
    {
      if(box->flags & UI_BoxFlag_RoundChildrenByParent && !ui_box_is_nil(box->first) && !ui_box_is_nil(box->last))
      {
        box->first->corner_radii[Corner_00] = box->corner_radii[Corner_00];
        box->first->corner_radii[Corner_10] = box->corner_radii[Corner_10];
        box->last->corner_radii[Corner_01] = box->corner_radii[Corner_01];
        box->last->corner_radii[Corner_11] = box->corner_radii[Corner_11];
      }
    }
  }

  // Animate
  ProfScope("animate")
  {
    // // ln(0.2) means 80%
    // // critical / 0.1f means 0.1 unit time to reach this critical point (80%)
    // F32 critical = log(0.2);
    // // Exponential smoothing
    // // ref: https://lisyarus.github.io/blog/posts/exponential-smoothing.html
    // F32 vast_rate = 1 - exp_f32((critical / 0.04f) * ui_state->animation_dt);
    // F32 fast_rate = 1 - exp_f32((critical / 0.05f) * ui_state->animation_dt);
    // F32 fish_rate = 1 - exp_f32((critical / 0.06f) * ui_state->animation_dt);
    // F32 slow_rate = 1 - exp_f32((critical / 0.07f) * ui_state->animation_dt);
    // F32 slug_rate = 1 - exp_f32((critical / 0.14f) * ui_state->animation_dt);
    // F32 slaf_rate = 1 - exp_f32((critical / 0.28f) * ui_state->animation_dt);

    F32 vast_rate = 1 - pow_f32(2, (-60.f * ui_state->animation_dt));
    F32 fast_rate = 1 - pow_f32(2, (-50.f * ui_state->animation_dt));
    F32 fish_rate = 1 - pow_f32(2, (-40.f * ui_state->animation_dt));
    F32 slow_rate = 1 - pow_f32(2, (-30.f * ui_state->animation_dt));
    F32 slug_rate = 1 - pow_f32(2, (-15.f * ui_state->animation_dt));
    F32 slaf_rate = 1 - pow_f32(2, (-8.f * ui_state->animation_dt));

    for(U64 slot_idx = 0; slot_idx < ui_state->anim_slots_count; slot_idx += 1)
    {
      for(UI_AnimNode *n = ui_state->anim_slots[slot_idx].first;
          n != &ui_nil_anim_node && n != 0;
          n = n->slot_next)
      {
        n->current += (n->params.target - n->current) * n->params.rate;
        // ui_state->is_animating = (ui_state->is_animating || abs_f32(n->params.target - n->current) > n->params.epsilon);
      }
    }

    ui_state->ctx_menu_open_t += ((F32)!!ui_state->ctx_menu_open - ui_state->ctx_menu_open_t) * ui_state->animation_info.menu_animation_rate;
    if(ui_state->ctx_menu_open_t >= 0.99f && ui_state->ctx_menu_open)
    {
      ui_state->ctx_menu_open_t = 1.f;
    }
    ui_state->tooltip_open_t += ((F32)!!ui_state->tooltip_open - ui_state->tooltip_open_t) * ui_state->animation_info.tooltip_animation_rate;

    for(U64 slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx++)
    {
      for(UI_Box *b = ui_state->box_table[slot_idx].hash_first; !ui_box_is_nil(b); b = b->hash_next)
      {
        B32 is_hot      = ui_key_match(b->key, ui_state->hot_box_key);
        B32 is_active   = ui_key_match(b->key, ui_state->active_box_key[UI_MouseButtonKind_Left]);
        B32 is_disabled = !!(b->flags & UI_BoxFlag_Disabled) && (b->first_disabled_build_index+2 < ui_state->build_index || b->first_touched_build_index == b->first_disabled_build_index);

        B32 is_focus_hot    = !!(b->flags & UI_BoxFlag_FocusHot) && !(b->flags & UI_BoxFlag_FocusHotDisabled);
        B32 is_focus_active = !!(b->flags & UI_BoxFlag_FocusActive) && !(b->flags & UI_BoxFlag_FocusActiveDisabled);

        // determine rates
        F32 hot_rate      = ui_state->animation_info.hot_animation_rate;
        F32 active_rate   = ui_state->animation_info.active_animation_rate;
        F32 disabled_rate = slow_rate;
        F32 focus_rate    = ui_state->animation_info.focus_animation_rate;

        // Animate interaction transition states
        b->hot_t          += hot_rate * ((F32)is_hot - b->hot_t);
        b->active_t       += active_rate * ((F32)is_active - b->active_t);
        b->disabled_t     += disabled_rate * ((F32)is_disabled - b->disabled_t);
        b->focus_hot_t    += focus_rate * ((F32)is_focus_hot - b->focus_hot_t);
        b->focus_active_t += focus_rate * ((F32)is_focus_active - b->focus_active_t);

        //- k: animate position
        {
          b->fixed_position_animated.x += fast_rate * (b->fixed_position.x - b->fixed_position_animated.x);
          b->fixed_position_animated.y += fast_rate * (b->fixed_position.y - b->fixed_position_animated.y);
          if(abs_f32(b->fixed_position.x - b->fixed_position_animated.x) < 1)
          {
            b->fixed_position_animated.x = b->fixed_position.x;
          }
          if(abs_f32(b->fixed_position.y - b->fixed_position_animated.y) < 1)
          {
            b->fixed_position_animated.y = b->fixed_position.y;
          }
        }

        // k: clamp view
        if(b->flags & UI_BoxFlag_ViewClamp)
        {
          Vec2F32 max_view_off_target =
          {
            ClampBot(0, b->view_bounds.x - b->fixed_size.x),
            ClampBot(0, b->view_bounds.y - b->fixed_size.y),
          };
          if(b->flags & UI_BoxFlag_ViewClampX) { b->view_off_target.x = Clamp(0, b->view_off_target.x, max_view_off_target.x); }
          if(b->flags & UI_BoxFlag_ViewClampY) { b->view_off_target.y = Clamp(0, b->view_off_target.y, max_view_off_target.y); }
        }

        //- k: animate view offset
        {
          b->view_off.x += fish_rate * (b->view_off_target.x - b->view_off.x);
          b->view_off.y += fish_rate * (b->view_off_target.y - b->view_off.y);
          if(abs_f32(b->view_off.x-b->view_off_target.x) < 2)
          {
            b->view_off.x = b->view_off_target.x;
          }
          if(abs_f32(b->view_off.y-b->view_off_target.y) < 2)
          {
            b->view_off.y = b->view_off_target.y;
          }
        }
      }
    }
  }

  //- rjf: fall-through interact with context menu
  if(ui_state->ctx_menu_open)
  {
    ui_signal_from_box(ui_state->ctx_menu_root);
  }

  //- rjf: close ctx menu if unconsumed clicks
  {
    for(UI_Event *evt = 0; ui_next_event(&evt);)
    {
      if(evt->kind == UI_EventKind_Press &&
         (evt->key == OS_Key_LeftMouseButton || evt->key == OS_Key_RightMouseButton))
      {
        ui_ctx_menu_close();
      }
    }
  }

  // k: hover cursor
  ProfScope("hover_cursor")
  {
    UI_Box *hot = ui_box_from_key(ui_state->hot_box_key);
    UI_Box *active = ui_box_from_key(ui_state->active_box_key[UI_MouseButtonKind_Left]);
    UI_Box *box = ui_box_is_nil(active) ? hot : active;
    OS_Cursor cursor = box->hover_cursor;
    if(box->flags & UI_BoxFlag_Disabled && box->flags & UI_BoxFlag_Clickable)
    {
      cursor = OS_Cursor_Disabled;
    }
    if(os_window_is_focused(ui_state->os_wnd) || !ui_box_is_nil(active))
    {
      os_set_cursor(cursor);
      ui_state->grab_cursor = !ui_box_is_nil(box);
    }
  }

  // clear (next to use) ui_state build arena
  ui_state->build_index++;
  arena_clear(ui_build_arena());
  ProfEnd();
}

internal void 
ui_calc_sizes_standalone__in_place_rec(UI_Box *root, Axis2 axis)
{
  switch(root->pref_size[axis].kind)
  {
    default:{}break;
    case UI_SizeKind_Pixels:
    {
      root->fixed_size.v[axis] = root->pref_size[axis].value;
    }break;
    case UI_SizeKind_TextContent:
    {
      F32 padding = root->pref_size[axis].value;
      // F32 text_size = root->display_string_runs.dim.x;
      F32 text_size = root->display_string_runs.dim.v[axis];
      root->fixed_size.v[axis] = padding + text_size + root->text_padding*2.f;
    }break;
  }

  // Recurse
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    ui_calc_sizes_standalone__in_place_rec(child, axis);
  }
}

internal void
ui_calc_sizes_upwards_dependent__in_place_rec(UI_Box *root, Axis2 axis)
{
  switch(root->pref_size[axis].kind)
  {
    default:{}break;
    case UI_SizeKind_ParentPct: 
    {
      UI_Box *fixed_parent = &ui_nil_box;
      // Find a parent that has a fixed size
      for(UI_Box *p = root->parent; !ui_box_is_nil(p); p = p->parent)
      {
        if(p->flags & UI_BoxFlag_FixedWidth<<axis||
            p->pref_size[axis].kind == UI_SizeKind_Pixels||
            p->pref_size[axis].kind == UI_SizeKind_TextContent||
            p->pref_size[axis].kind == UI_SizeKind_ParentPct)
        {
          fixed_parent = p;
          break;
        }
      }
      F32 size = fixed_parent->fixed_size.v[axis] * root->pref_size[axis].value;
      root->fixed_size.v[axis] = size;
    }break;
  };

  // Recurse
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    ui_calc_sizes_upwards_dependent__in_place_rec(child, axis);
  }
}

internal void
ui_calc_sizes_downwards_dependent__in_place_rec(UI_Box *root, Axis2 axis)
{
  // Recurse first
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    ui_calc_sizes_downwards_dependent__in_place_rec(child, axis);
  }

  switch(root->pref_size[axis].kind)
  {
    default:{}break;
    case UI_SizeKind_ChildrenSum: 
    {
      F32 sum = 0;
      for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
      {
        if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
        {
          if(axis == root->child_layout_axis)
          {
            sum += child->fixed_size.v[axis];
          }
          else
          {
            sum = Max(sum, child->fixed_size.v[axis]);
          }
        } 
        else
        {
          sum = Max(sum, child->fixed_size.v[axis]);
        }
      }
      root->fixed_size.v[axis] = sum;
    }break;
  }
}

internal void
ui_layout_enforce_constraints__in_place_rec(UI_Box *root, Axis2 axis)
{
  Temp scratch = scratch_begin(0, 0);

  // NOTE(rjf): The "layout axis" is the direction in which children
  // of some node are intended to be laid out.

  //- rjf: fixup children sizes (if we're solving along the *non-layout* axis)
  if(axis != root->child_layout_axis && !(root->flags & (UI_BoxFlag_AllowOverflowX << axis)))
  {
    F32 allowed_size = root->fixed_size.v[axis];
    for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
      if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
      {
        F32 child_size = child->fixed_size.v[axis];
        F32 violation = child_size - allowed_size;
        F32 max_fixup = child_size;
        F32 fixup = Clamp(0, violation, max_fixup);
        if(fixup > 0)
        {
          child->fixed_size.v[axis] -= fixup;
        }
      }
    }
  }

  //- rjf: fixup children sizes (in the direction of the layout axis)
  if(axis == root->child_layout_axis && !(root->flags & (UI_BoxFlag_AllowOverflowX << axis)))
  {
    // rjf: figure out total allowed size & total size
    F32 total_allowed_size = root->fixed_size.v[axis];
    F32 total_size = 0;
    F32 total_weighted_size = 0;
    for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
      if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
      {
        total_size += child->fixed_size.v[axis];
        total_weighted_size += child->fixed_size.v[axis] * (1-child->pref_size[axis].strictness);
      }
    }

    // rjf: if we have a violation, we need to subtract some amount from all children
    F32 violation = total_size - total_allowed_size;
    if(violation > 0)
    {
      // rjf: figure out how much we can take in totality
      F32 child_fixup_sum = 0;
      F32 *child_fixups = push_array(scratch.arena, F32, root->child_count);
      {
        U64 child_idx = 0;
        for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next, child_idx += 1)
        {
          if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
          {
            F32 fixup_size_this_child = child->fixed_size.v[axis] * (1-child->pref_size[axis].strictness);
            fixup_size_this_child = ClampBot(0, fixup_size_this_child);
            child_fixups[child_idx] = fixup_size_this_child;
            child_fixup_sum += fixup_size_this_child;
          }
        }
      }

      // rjf: fixup child sizes
      {
        U64 child_idx = 0;
        for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next, child_idx += 1)
        {
          if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
          {
            F32 fixup_pct = (violation / total_weighted_size);
            fixup_pct = Clamp(0, fixup_pct, 1);
            child->fixed_size.v[axis] -= child_fixups[child_idx] * fixup_pct;
          }
        }
      }
    }
  }

  //- rjf: fixup upwards-relative sizes
  if(root->flags & (UI_BoxFlag_AllowOverflowX << axis))
  {
    for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
    {
      if(child->pref_size[axis].kind == UI_SizeKind_ParentPct)
      {
        child->fixed_size.v[axis] = root->fixed_size.v[axis] * child->pref_size[axis].value;
      }
    }
  }

  //- rjf: enforce clamps
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    child->fixed_size.v[axis] = Max(child->fixed_size.v[axis], child->min_size.v[axis]);
  }

  //- rjf: recurse
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    ui_layout_enforce_constraints__in_place_rec(child, axis);
  }

  scratch_end(scratch);
}

  internal void
ui_layout_position__in_place_rec(UI_Box *root, Axis2 axis)
{
  F32 layout_position = 0;

  //- rjf: lay out children
  F32 bounds = 0;
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    // rjf: grab original position
    F32 original_position = Min(child->rect.p0.v[axis], child->rect.p1.v[axis]);

    // rjf: calculate fixed position & size
    if(!(child->flags & (UI_BoxFlag_FloatingX<<axis)))
    {
      child->fixed_position.v[axis] = layout_position;
      if(root->child_layout_axis == axis)
      {
        layout_position += child->fixed_size.v[axis];
        bounds += child->fixed_size.v[axis];
      }
      else
      {
        bounds = Max(bounds, child->fixed_size.v[axis]);
      }
    }

    // rjf: determine final rect for child, given fixed_position & size
    if(child->flags & (UI_BoxFlag_AnimatePosX<<axis))
    {
      if(child->first_touched_build_index == child->last_touched_build_index)
      {
        child->fixed_position_animated = child->fixed_position;
      }
      child->rect.p0.v[axis] = root->rect.p0.v[axis] + child->fixed_position_animated.v[axis] - !(child->flags&(UI_BoxFlag_SkipViewOffX<<axis))*floor_f32(root->view_off.v[axis]);
    }
    else
    {
      child->rect.p0.v[axis] = root->rect.p0.v[axis] + child->fixed_position.v[axis] - !(child->flags&(UI_BoxFlag_SkipViewOffX<<axis))*floor_f32(root->view_off.v[axis]);
    }
    child->rect.p1.v[axis] = child->rect.p0.v[axis] + child->fixed_size.v[axis];
    child->rect.p0.x = floor_f32(child->rect.p0.x);
    child->rect.p0.y = floor_f32(child->rect.p0.y);
    child->rect.p1.x = floor_f32(child->rect.p1.x);
    child->rect.p1.y = floor_f32(child->rect.p1.y);

    // rjf: grab new position
    F32 new_position = Min(child->rect.p0.v[axis], child->rect.p1.v[axis]);

    // rjf: store position delta
    child->position_delta.v[axis] = new_position - original_position;
  }

  //- rjf: store view bounds
  {
    root->view_bounds.v[axis] = bounds;
  }

  //- rjf: recurse
  for(UI_Box *child = root->first; !ui_box_is_nil(child); child = child->next)
  {
    ui_layout_position__in_place_rec(child, axis);
  }
}

internal void
ui_layout_root(UI_Box *root, Axis2 axis)
{
  ProfBeginFunction();
  ui_calc_sizes_standalone__in_place_rec(root, axis);
  ui_calc_sizes_upwards_dependent__in_place_rec(root, axis);
  ui_calc_sizes_downwards_dependent__in_place_rec(root, axis);
  ui_layout_enforce_constraints__in_place_rec(root, axis);
  ui_layout_position__in_place_rec(root, axis);
  ProfEnd();
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Box Tree Building API

//- rjf: box node construction
internal UI_Box *
ui_build_box_from_key(UI_BoxFlags flags, UI_Key key)
{
  ProfBeginFunction();
  ui_state->build_box_count += 1;

  // Grab active parent
  UI_Box *parent = ui_top_parent();
  UI_BoxFlags last_flags = 0;
  // Try to get box from cache
  UI_Box *box = ui_box_from_key(key);
  U32 box_first_frame = ui_box_is_nil(box);
  last_flags = box->flags;

  // Zero key on duplicate
  if(!(box_first_frame) && box->last_touched_build_index == ui_state->build_index)
  {
    box = &ui_nil_box;
    key = ui_key_zero();
    box_first_frame = 1;
  }

  // Gather info from box
  U32 box_is_transient = ui_key_match(key, ui_key_zero());

  // Allocate box if it doesn't yet exist
  if(box_first_frame)
  {
    box = box_is_transient ? 0 : ui_state->first_free_box;

    if(!ui_box_is_nil(box))
    {
      SLLStackPop(ui_state->first_free_box);
    }
    else
    {
      box = push_array_no_zero(box_is_transient ? ui_build_arena(): ui_state->arena, UI_Box, 1);
    }
    MemoryZeroStruct(box);
  }

  // Zero out per-frame state
  {
    box->first = box->last = box->next = box->prev = box->parent = &ui_nil_box;
    box->flags = 0;
    box->child_count = 0;
    MemoryZeroArray(box->pref_size);
    MemoryZeroStruct(&box->draw_bucket);
    box->hover_cursor = OS_Cursor_Pointer;
  }

  // Hook into persistent state table
  if(box_first_frame && !box_is_transient)
  {
    U64 slot = key.u64[0] % ui_state->box_table_size;
    DLLInsert_NPZ(&ui_nil_box, ui_state->box_table[slot].hash_first,
                  ui_state->box_table[slot].hash_last,
                  ui_state->box_table[slot].hash_last,
                  box, hash_next, hash_prev);
  }

  // Hook into per-frame tree structure
  if(!ui_box_is_nil(parent))
  {
    DLLPushBack_NPZ(&ui_nil_box, parent->first, parent->last, box, next, prev);
    parent->child_count+=1;
    AssertAlways(parent->child_count < 10000);
    box->parent = parent;
  }

  // Fill box info
  {
    box->key = key;
    box->flags = flags|ui_state->flags_stack.top->v;
    box->last_touched_build_index = ui_state->build_index;
    if(box_first_frame)
    {
      box->first_touched_build_index = ui_state->build_index;
      box->disabled_t = (F32)!!(box->flags & UI_BoxFlag_Disabled);
    }
    if(box->flags & UI_BoxFlag_Disabled && (!(last_flags & UI_BoxFlag_Disabled) || box_first_frame))
    {
      box->first_disabled_build_index = ui_state->build_index;
    }
    if(ui_state->fixed_x_stack.top != &ui_state->fixed_x_nil_stack_top)
    {
      box->flags |= UI_BoxFlag_FloatingX;
      box->fixed_position.x = ui_state->fixed_x_stack.top->v;
    }
    if(ui_state->fixed_y_stack.top != &ui_state->fixed_y_nil_stack_top)
    {
      box->flags |= UI_BoxFlag_FloatingY;
      box->fixed_position.y = ui_state->fixed_y_stack.top->v;
    }
    if(ui_state->fixed_width_stack.top != &ui_state->fixed_width_nil_stack_top)
    {
      box->flags |= UI_BoxFlag_FixedWidth;
      box->fixed_size.x = ui_state->fixed_width_stack.top->v;
    }
    else
    {
      box->pref_size[Axis2_X] = ui_state->pref_width_stack.top->v;
    }
    if(ui_state->fixed_height_stack.top != &ui_state->fixed_height_nil_stack_top)
    {
      box->flags |= UI_BoxFlag_FixedHeight;
      box->fixed_size.y = ui_state->fixed_height_stack.top->v;
    }
    else
    {
      box->pref_size[Axis2_Y] = ui_state->pref_height_stack.top->v;
    }

    box->min_size.v[Axis2_X] = ui_state->min_width_stack.top->v;
    box->min_size.v[Axis2_Y] = ui_state->min_height_stack.top->v;

    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
    B32 is_auto_focus_hot    = ui_is_key_auto_focus_hot(key);
    if(is_auto_focus_hot)    { ui_set_next_focus_hot(UI_FocusKind_On); }
    if(is_auto_focus_active) { ui_set_next_focus_active(UI_FocusKind_On); }

    box->flags |= UI_BoxFlag_FocusHot * (ui_state->focus_hot_stack.top->v == UI_FocusKind_On);
    box->flags |= UI_BoxFlag_FocusActive * (ui_state->focus_active_stack.top->v == UI_FocusKind_On);
    if(box->flags & UI_BoxFlag_FocusHot && !ui_is_focus_hot())       { box->flags |= UI_BoxFlag_FocusHotDisabled; }
    if(box->flags & UI_BoxFlag_FocusActive && !ui_is_focus_active()) { box->flags |= UI_BoxFlag_FocusActiveDisabled; }

    box->text_align              = ui_state->text_alignment_stack.top->v;
    box->child_layout_axis       = ui_state->child_layout_axis_stack.top->v;
    box->font                    = ui_state->font_stack.top->v;
    box->font_size               = ui_state->font_size_stack.top->v;
    box->tab_size                = ui_state->tab_size_stack.top->v;
    box->palette                 = ui_state->palette_stack.top->v;
    box->text_raster_flags       = ui_state->text_raster_flags_stack.top->v;
    box->corner_radii[Corner_00] = ui_state->corner_radius_00_stack.top->v;
    box->corner_radii[Corner_01] = ui_state->corner_radius_01_stack.top->v;
    box->corner_radii[Corner_10] = ui_state->corner_radius_10_stack.top->v;
    box->corner_radii[Corner_11] = ui_state->corner_radius_11_stack.top->v;
    box->transparency            = ui_state->transparency_stack.top->v;
    box->squish                  = ui_state->squish_stack.top->v;
    box->text_padding            = ui_state->text_padding_stack.top->v;
    box->hover_cursor            = ui_state->hover_cursor_stack.top->v;
    box->custom_draw             = 0;
  }

  UI_AutoPopStacks(ui_state)
  ProfEnd();
  return box;
}

internal UI_Key
ui_active_seed_key(void)
{
  UI_Box *keyed_ancestor = &ui_nil_box;
  for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
  {
    if(!ui_key_match(ui_key_zero(), p->key))
    {
      keyed_ancestor = p;
      break;
    }
  }
  return keyed_ancestor->key;
}

internal UI_Box *
ui_build_box_from_string(UI_BoxFlags flags, String8 string)
{
  ProfBeginFunction();
  // Grab active parent
  UI_Box *parent = ui_top_parent();

  UI_Key key = ui_key_from_string(ui_active_seed_key(), string);

  UI_Box *box = ui_build_box_from_key(flags, key);
  // TODO: maybe we don't need this field
  box->indentifier = push_str8_copy(ui_build_arena(), string);
  if(box->flags & UI_BoxFlag_DrawText)
  {
    ui_box_equip_display_string(box, string);
  }
  ProfEnd();
  return box;
}

internal UI_Box *
ui_build_box_from_stringf(UI_BoxFlags flags, char *fmt, ...)
{
  Temp scratch = scratch_begin(0,0);
  va_list args;
  va_start(args, fmt);
  String8 string = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  UI_Box *box = ui_build_box_from_string(flags, string);
  scratch_end(scratch);
  return box;
}

//- rjf: box node equipment
internal inline void
ui_box_equip_display_string(UI_Box *box, String8 string)
{
  ProfBeginFunction();
  box->string = push_str8_copy(ui_build_arena(), string);
  box->flags |= UI_BoxFlag_HasDisplayString;
  UI_ColorCode text_color_code = (box->flags & UI_BoxFlag_DrawTextWeak ? UI_ColorCode_TextWeak : UI_ColorCode_Text);

  if(box->flags & UI_BoxFlag_DrawText)
  {
    String8 display_string = ui_box_display_string(box);
    D_FancyStringNode fancy_string_n = {0};
    fancy_string_n.next = 0;
    fancy_string_n.v.font = box->font;
    fancy_string_n.v.string = display_string;
    fancy_string_n.v.color = box->palette->colors[text_color_code];
    fancy_string_n.v.size = box->font_size;
    fancy_string_n.v.underline_thickness     = 0;
    fancy_string_n.v.strikethrough_thickness = 0;

    D_FancyStringList fancy_strings = {0};
    fancy_strings.first = &fancy_string_n;
    fancy_strings.last = &fancy_string_n;
    fancy_strings.node_count = 1;
    box->display_string_runs = d_fancy_run_list_from_fancy_string_list(ui_build_arena(), box->tab_size, box->text_raster_flags, &fancy_strings);
  }
  ProfEnd();
}

internal Vec2F32
ui_box_text_position(UI_Box *box)
{
  Vec2F32 result = {0};

  F_Tag font = box->font;
  F32 font_size = box->font_size;
  F_Metrics font_metrics = f_metrics_from_tag_size(font, font_size);
  result.y = floor_f32((box->rect.p0.y+box->rect.p1.y)/2.0f);
  result.y += font_metrics.ascent/2.0f;
  result.y -= font_metrics.descent/2.0f;

  switch(box->text_align)
  {
    default:
    case UI_TextAlign_Left:
    {
      result.x = box->rect.p0.x + box->text_padding;
    }break;
    case UI_TextAlign_Center:
    {
      Vec2F32 text_dim = box->display_string_runs.dim;
      result.x = (box->rect.p0.x + box->rect.p1.x)/2.0f - text_dim.x/2.0f;
      result.x = ClampBot(result.x, box->rect.x0 + box->text_padding);
    }break;
    case UI_TextAlign_Right:
    {
      Vec2F32 text_dim = box->display_string_runs.dim;
      result.x = box->rect.p1.x - box->text_padding - text_dim.x;
      result.x = ClampBot(result.x, box->rect.x0 + box->text_padding);
    }break;
  }
  return result;
}

internal void
ui_box_equip_draw_bucket(UI_Box *box, D_Bucket *bucket)
{
  box->flags |= UI_BoxFlag_DrawBucket;
  if(box->draw_bucket != 0)
  {
    D_BucketScope(box->draw_bucket) d_sub_bucket(bucket, 1);
  }
  else
  {
    box->draw_bucket = bucket;
  }
}

internal void
ui_box_equip_custom_draw(UI_Box *box, UI_BoxCustomDrawFunctionType *custom_draw, void *user_data)
{
  box->custom_draw = custom_draw;
  box->custom_draw_user_data = user_data;
}

internal U64
ui_box_char_pos_from_xy(UI_Box *box, Vec2F32 xy)
{
  F_Tag font = box->font;
  F32 font_size = box->font_size;
  String8 line = ui_box_display_string(box);
  U64 result = f_char_pos_from_tag_size_string_p(font, font_size, 0, box->tab_size, line, xy.x - ui_box_text_position(box).x);
  return result;
}

  internal String8
ui_box_display_string(UI_Box *box)
{
  String8 result = box->string;
  if(!(box->flags & UI_BoxFlag_DisableIDString))
  {
    result = ui_display_part_from_key_string(result);
  }
  return result;
}

internal B32
ui_is_key_auto_focus_hot(UI_Key key)
{
  B32 result = 0;
  if(!ui_key_match(ui_key_zero(), key))
  {
    for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
    {
      if(p->flags & UI_BoxFlag_FocusHot &&
          ((!(p->flags & UI_BoxFlag_FocusHotDisabled) && ui_key_match(key, p->default_nav_focus_hot_key)) ||
           ui_key_match(key, p->default_nav_focus_active_key)))
      {
        result = 1;
        break;
      }
    }
  }
  return result;
}

internal B32
ui_is_key_auto_focus_active(UI_Key key)
{
  B32 result = 0;
  if(!ui_key_match(ui_key_zero(), key))
  {
    for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
    {
      if(p->flags & UI_BoxFlag_FocusActive && ui_key_match(key, p->default_nav_focus_active_key))
      {
        result = 1;
        break;
      }
    }
  }
  return result;
}

internal void
ui_set_auto_focus_hot_key(UI_Key key)
{
  for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
  {
    if(p->flags & UI_BoxFlag_DefaultFocusNav)
    {
      p->default_nav_focus_next_hot_key = key;
      break;
    }
  }
}

internal void
ui_set_auto_focus_active_key(UI_Key key)
{
  for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
  {
    if(p->flags & UI_BoxFlag_DefaultFocusNav)
    {
      p->default_nav_focus_next_active_key = key;
      break;
    }
  }
}

//- rjf: palette forming

internal UI_Palette *
ui_build_palette_(UI_Palette *base, UI_Palette *overrides)
{
  UI_Palette *palette = push_array(ui_build_arena(), UI_Palette, 1);
  if(base != 0)
  {
    MemoryCopyStruct(palette, base);
  }
  for EachEnumVal(UI_ColorCode, code)
  {
    if(overrides->colors[code].x != 0 ||
       overrides->colors[code].y != 0 ||
       overrides->colors[code].z != 0 ||
       overrides->colors[code].w != 0)
    {
      palette->colors[code] = overrides->colors[code];
    }
  }
  return palette;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: focus tree coloring

internal B32
ui_is_focus_hot(void)
{
  B32 result = (ui_state->focus_hot_stack.top->v == UI_FocusKind_On);
  if(result)
  {
    for(UI_FocusHotNode *n = ui_state->focus_hot_stack.top; n != 0; n = n->next)
    {
      if(n->v == UI_FocusKind_Root)
      {
        break;
      }
      if(n->v == UI_FocusKind_Off)
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

internal B32
ui_is_focus_active(void)
{
  B32 result = (ui_state->focus_active_stack.top->v == UI_FocusKind_On);
  if(result)
  {
    for(UI_FocusHotNode *n = ui_state->focus_hot_stack.top; n != 0; n = n->next)
    {
      if(n->v == UI_FocusKind_Root)
      {
        break;
      }
      if(n->v == UI_FocusKind_Off)
      {
        result = 0;
        break;
      }
    }
  }
  return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: User Interaction

internal UI_Signal
ui_signal_from_box(UI_Box *box)
{
  ProfBeginFunction();
  UI_Signal sig = {0};
  sig.box = box;
  sig.event_flags |= os_get_modifiers();
  B32 is_focus_hot = box->flags & UI_BoxFlag_FocusHot && !(box->flags & UI_BoxFlag_FocusHotDisabled);

  //////////////////////////////
  //- rjf: calculate possibly-clipped box rectangle
  //
  Rng2F32 rect = box->rect;
  for(UI_Box *b = box->parent; !ui_box_is_nil(b); b = b->parent)
  {
    if(b->flags & UI_BoxFlag_Clip)
    {
      rect = intersect_2f32(rect, b->rect);
    }
  }

  //////////////////////////////
  //- rjf: determine if we're under the context menu or not
  //
  B32 ctx_menu_is_ancestor = 0;
  ProfScope("check context menu ancestor")
  {
    for(UI_Box *parent = box; !ui_box_is_nil(parent); parent = parent->parent)
    {
      if(parent == ui_state->ctx_menu_root)
      {
        ctx_menu_is_ancestor = 1;
        break;
      }
    }
  }
  
  //////////////////////////////
  //- rjf: calculate blacklist rectangles
  //
  Rng2F32 blacklist_rect = {0};
  if(!ctx_menu_is_ancestor && ui_state->ctx_menu_open)
  {
    blacklist_rect = ui_state->ctx_menu_root->rect;
  }

  //////////////////////////////
  //- rjf: process events related to this box
  //
  for(UI_EventNode *n = ui_state->events->first, *next=0; n != 0; n = next)
  {
    B32 taken = 0;
    next = n->next;
    UI_Event *evt = &n->v;

    //- k: unpack event
    Vec2F32 evt_mouse = evt->pos;
    B32 evt_mouse_in_bounds = !contains_2f32(blacklist_rect, evt_mouse) && contains_2f32(rect, evt_mouse);
    UI_MouseButtonKind evt_mouse_button_kind = 
      evt->key == OS_Key_LeftMouseButton   ? UI_MouseButtonKind_Left   :
      evt->key == OS_Key_RightMouseButton  ? UI_MouseButtonKind_Right  :
      evt->key == OS_Key_MiddleMouseButton ? UI_MouseButtonKind_Middle :
      UI_MouseButtonKind_Left;
    B32 evt_key_is_mouse =
      evt->key == OS_Key_LeftMouseButton  || 
      evt->key == OS_Key_RightMouseButton ||
      evt->key == OS_Key_MiddleMouseButton;
    sig.event_flags |= evt->modifiers;

    //- k: mouse pressed in box -> set hot/active, mark signal accordingly
    if(box->flags & UI_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Press &&
       evt_mouse_in_bounds &&
       evt_key_is_mouse)
    {
      ui_state->hot_box_key = box->key;
      ui_state->active_box_key[evt_mouse_button_kind] = box->key;
      sig.f |= (UI_SignalFlag_LeftPressed << evt_mouse_button_kind);

      ui_state->drag_start_mouse = evt->pos;

      if(ui_key_match(box->key, ui_state->press_key_history[evt_mouse_button_kind][0]) &&
         evt->timestamp_us-ui_state->press_timestamp_history_us[evt_mouse_button_kind][0] <= 1000000*os_get_gfx_info()->double_click_time)
      {
        sig.f |= (UI_SignalFlag_LeftDoubleClicked<<evt_mouse_button_kind);
      }

      // TODO: handle tripled clicking
      MemoryCopy(&ui_state->press_key_history[evt_mouse_button_kind][1],
                 &ui_state->press_key_history[evt_mouse_button_kind][0],
                 sizeof(ui_state->press_key_history[evt_mouse_button_kind][0]) * (ArrayCount(ui_state->press_key_history)-1));
      MemoryCopy(&ui_state->press_timestamp_history_us[evt_mouse_button_kind][1],
                 &ui_state->press_timestamp_history_us[evt_mouse_button_kind][0],
                 sizeof(ui_state->press_timestamp_history_us[evt_mouse_button_kind][0]) * (ArrayCount(ui_state->press_timestamp_history_us)-1));
      ui_state->press_key_history[evt_mouse_button_kind][0] = box->key;
      ui_state->press_timestamp_history_us[evt_mouse_button_kind][0] = evt->timestamp_us;

      taken = 1;
    }

    //- k: mouse released in active box -> unset active
    if(box->flags & UI_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Release &&
       ui_key_match(box->key, ui_state->active_box_key[evt_mouse_button_kind]) &&
       evt_mouse_in_bounds && evt_key_is_mouse)
    {
      ui_state->hot_box_key = box->key;
      ui_state->active_box_key[evt_mouse_button_kind] = ui_key_zero();
      sig.f |= (UI_SignalFlag_LeftReleased << evt_mouse_button_kind);
      sig.f |= (UI_SignalFlag_LeftClicked << evt_mouse_button_kind);

      taken = 1;
    }

    // TODO: handle ancestors which are covering this box

    //- k: mouse released outside of active box -> unset hot/active
    if(box->flags & UI_BoxFlag_MouseClickable &&
       evt->kind == UI_EventKind_Release &&
       ui_key_match(box->key, ui_state->active_box_key[evt_mouse_button_kind]) &&
       !evt_mouse_in_bounds &&
       evt_key_is_mouse)
    {
      ui_state->hot_box_key = ui_key_zero();
      ui_state->active_box_key[evt_mouse_button_kind] = ui_key_zero();
      sig.f |= (UI_SignalFlag_LeftReleased << evt_mouse_button_kind);

      taken = 1;
    }

    //- k: (input)focus is hot & keyboard click -> mark signal
    if((box->flags & UI_BoxFlag_KeyboardClickable) &&
        is_focus_hot &&
        evt->kind == UI_EventKind_Press &&
        evt->slot == UI_EventActionSlot_Accept)
    {
      sig.f |= UI_SignalFlag_KeyboardPressed;
      taken = 1;
    }

    //- k: scroll
    if(box->flags & UI_BoxFlag_Scroll &&
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

    //- k: taken -> eat event
    if(taken)
    {
      ui_eat_event(evt);
    }
  }

  //////////////////////////////
  //- rjf: mouse is over this box's rect -> always mark mouse-over

  {
    if(contains_2f32(rect, ui_state->mouse) &&
       !contains_2f32(blacklist_rect, ui_state->mouse))
    {
      sig.f |= UI_SignalFlag_MouseOver;
    }
  }

  //////////////////////////////
  //- rjf: mouse is over this box's rect, no other hot key? -> set hot key, mark hovering

  if(box->flags & UI_BoxFlag_MouseClickable &&
     contains_2f32(rect, ui_state->mouse) &&
     !contains_2f32(blacklist_rect, ui_state->mouse) &&
     (ui_key_match(ui_state->hot_box_key, ui_key_zero()) || ui_key_match(ui_state->hot_box_key, box->key)) &&
     (ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Left], ui_key_zero()) || ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Left], box->key)) &&
     (ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Middle], ui_key_zero()) || ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Middle], box->key)) &&
     (ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Right], ui_key_zero()) || ui_key_match(ui_state->active_box_key[UI_MouseButtonKind_Right], box->key)))
  {
    ui_state->hot_box_key = box->key;
    sig.f |= UI_SignalFlag_Hovering;
  }

  //////////////////////////////
  //- rjf: mouse is over this box's rect, drop site, no other drop hot key? -> set drop hot key
  //
  {
    if(box->flags & UI_BoxFlag_DropSite &&
       contains_2f32(rect, ui_state->mouse) &&
       !contains_2f32(blacklist_rect, ui_state->mouse) &&
       (ui_key_match(ui_state->drop_hot_box_key, ui_key_zero()) || ui_key_match(ui_state->drop_hot_box_key, box->key)))
    {
      ui_state->drop_hot_box_key = box->key;
    }
  }

  //////////////////////////////
  //- rjf: mouse is not over this box's rect, but this is the drop hot key? -> zero drop hot key
  //
  {
    if(box->flags & UI_BoxFlag_DropSite &&
       (!contains_2f32(rect, ui_state->mouse) ||
        contains_2f32(blacklist_rect, ui_state->mouse)) &&
       ui_key_match(ui_state->drop_hot_box_key, box->key))
    {
      ui_state->drop_hot_box_key = ui_key_zero();
    }
  }

  //////////////////////////////
  //- rjf: active -> dragging

  if(box->flags & UI_BoxFlag_MouseClickable)
  {
    for EachEnumVal(UI_MouseButtonKind, k)
    {
      if(ui_key_match(ui_state->active_box_key[k], box->key) || sig.f & (UI_SignalFlag_LeftPressed<<k))
      {
        sig.f |= (UI_SignalFlag_LeftDragging<<k);
      }
    }
  }

  //////////////////////////////
  //- rjf: mouse is not over this box, hot key is the box? -> unset hot key

  if(!contains_2f32(rect, ui_state->mouse) && ui_key_match(ui_state->hot_box_key, box->key))
  {
    ui_state->hot_box_key = ui_key_zero();
  }

  //////////////////////////////
  //- rjf: get default nav ancestor

  UI_Box *default_nav_parent = &ui_nil_box;
  for(UI_Box *p = ui_top_parent(); !ui_box_is_nil(p); p = p->parent)
  {
    if(p->flags & UI_BoxFlag_DefaultFocusNav)
    {
      default_nav_parent = p;
      break;
    }
  }

  //////////////////////////////
  //- rjf: clicking in default nav -> set navigation state to this box

  if(box->flags & UI_BoxFlag_ClickToFocus && sig.f&UI_SignalFlag_Pressed && !ui_box_is_nil(default_nav_parent))
  {
    default_nav_parent->default_nav_focus_next_hot_key = box->key;
    if(!ui_key_match(default_nav_parent->default_nav_focus_active_key, box->key))
    {
      default_nav_parent->default_nav_focus_next_active_key = ui_key_zero();
    }
  }

  ProfEnd();
  return sig;
}

////////////////////////////////
//~ rjf: Animation Cache Interaction API

internal F32
ui_anim_(UI_Key key, UI_AnimParams *params)
{
  // rjf: get animation cache node
  UI_AnimNode *node = &ui_nil_anim_node;
  if(ui_state != 0)
  {
    U64 slot_idx = key.u64[0]%ui_state->anim_slots_count;
    UI_AnimSlot *slot = &ui_state->anim_slots[slot_idx];
    for(UI_AnimNode *n = slot->first; n != &ui_nil_anim_node && n != 0; n = n->slot_next)
    {
      if(ui_key_match(n->key, key))
      {
        node = n;
        break;
      }
    }
    if(node == &ui_nil_anim_node)
    {
      node = ui_state->free_anim_node;
      if(node != 0)
      {
        SLLStackPop_N(ui_state->free_anim_node, slot_next);
      }
      else
      {
        node = push_array(ui_state->arena, UI_AnimNode, 1);
      }
      node->first_touched_build_index = ui_state->build_index;
      node->key = key;
      MemoryCopyStruct(&node->params, params);
      node->current = params->initial;
      DLLPushBack_NPZ(&ui_nil_anim_node, slot->first, slot->last, node, slot_next, slot_prev);
    }
    else
    {
      DLLRemove_NPZ(&ui_nil_anim_node, ui_state->lru_anim_node, ui_state->mru_anim_node, node, lru_next, lru_prev);
    }
  }
  
  // rjf: touch node & update parameters - grab current
  if(node != &ui_nil_anim_node)
  {
    node->last_touched_build_index = ui_state->build_index;
    DLLPushBack_NPZ(&ui_nil_anim_node, ui_state->lru_anim_node, ui_state->mru_anim_node, node, lru_next, lru_prev);
    if(params->reset)
    {
      node->current = params->initial;
    }
    MemoryCopyStruct(&node->params, params);
    if(node->params.epsilon == 0)
    {
      node->params.epsilon = 0.005f;
    }
    if(node->params.rate == 1 || abs_f32(node->current - node->params.target) < abs_f32(node->params.epsilon))
    {
      node->current = node->params.target;
    }
  }
  return node->current;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Sizes

internal UI_Size
ui_size(UI_SizeKind kind, F32 value, F32 strictness)
{
  UI_Size size = {kind, value, strictness};
  return size;
}

////////////////////////////////
//~ rjf: Scroll Point Type Functions

internal UI_ScrollPt
ui_scroll_pt(S64 idx, F32 off)
{
  UI_ScrollPt pt = {idx, off};
  return pt;
}

internal void
ui_scroll_pt_target_idx(UI_ScrollPt *v, S64 idx)
{
  v->off = mod_f32(v->off, 1.f) + (F32)(v->idx+(S64)v->off - idx);
  v->idx = idx;
}

internal void
ui_scroll_pt_clamp_idx(UI_ScrollPt *v, Rng1S64 range)
{
  if(v->idx < range.min || range.max < v->idx)
  {
    S64 clamped = range.min;
    ui_scroll_pt_target_idx(v, clamped);
  }
}

////////////////////////////////
//~ rjf: Text Operation Functions
internal UI_TxtOp
ui_single_line_txt_op_from_event(Arena *arena, UI_Event *event, String8 string, TxtPt cursor, TxtPt mark)
{
  TxtPt next_cursor = cursor;
  TxtPt next_mark = mark;
  TxtRng range = {0};
  String8 replace = {0};
  String8 copy = {0};
  UI_TxtOpFlags flags = 0;
  Vec2S32 delta = event->delta_2s32;
  // ???
  Vec2S32 original_delta = delta;

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

internal String8
ui_push_string_replace_range(Arena *arena, String8 string, Rng1S64 col_range, String8 replace)
{
  //- rjf: convert to offset range
  Rng1U64 range =
  {
    (U64)(col_range.min-1),
    (U64)(col_range.max-1),
  };

  //- rjf: clamp range
  if(range.min > string.size)
  {
    range.min = 0;
  }
  if(range.max > string.size)
  {
    range.max = string.size;
  }

  //- rjf: calculate new size
  U64 old_size = string.size;
  U64 new_size = old_size - (range.max - range.min) + replace.size;

  //- rjf: push+fill new string storage
  U8 *push_base = push_array(arena, U8, new_size);
  {
    MemoryCopy(push_base+0, string.str, range.min);
    MemoryCopy(push_base+range.min+replace.size, string.str+range.max, string.size-range.max);
    if(replace.str != 0)
    {
      MemoryCopy(push_base+range.min, replace.str, replace.size);
    }
  }

  String8 result = str8(push_base, new_size);
  return result;
}

////////////////////////////////
//~ rjf: Stacks

//- rjf: helpers

internal Rng2F32
ui_push_rect(Rng2F32 rect)
{
  Rng2F32 replaced = {0};
  Vec2F32 size = dim_2f32(rect);
  replaced.x0 = ui_push_fixed_x(rect.x0);
  replaced.y0 = ui_push_fixed_y(rect.y0);
  replaced.x1 = replaced.x0 + ui_push_fixed_width(size.x);
  replaced.y1 = replaced.y0 + ui_push_fixed_height(size.y);
  return replaced;
}

internal Rng2F32
ui_pop_rect(void)
{
  Rng2F32 popped = {0};
  popped.x0 = ui_pop_fixed_x();
  popped.y0 = ui_pop_fixed_y();
  popped.x1 = popped.x0 + ui_pop_fixed_width();
  popped.y1 = popped.y0 + ui_pop_fixed_height();
  return popped;
}

internal void
ui_set_next_rect(Rng2F32 rect)
{
  Vec2F32 size = dim_2f32(rect);
  ui_set_next_fixed_x(rect.x0);
  ui_set_next_fixed_y(rect.y0);
  ui_set_next_fixed_width(size.x);
  ui_set_next_fixed_height(size.y);
}

internal UI_Size
ui_push_pref_size(Axis2 axis, UI_Size v)
{
  UI_Size result = zero_struct;
  switch(axis)
  {
    default: break;
    case Axis2_X: {result = ui_push_pref_width(v);}break;
    case Axis2_Y: {result = ui_push_pref_height(v);}break;
  }
  return result;
}

internal UI_Size
ui_pop_pref_size(Axis2 axis)
{
  UI_Size result = zero_struct;
  switch(axis)
  {
    default: break;
    case Axis2_X: {result = ui_pop_pref_width();}break;
    case Axis2_Y: {result = ui_pop_pref_height();}break;
  }
  return result;
}

internal UI_Size
ui_set_next_pref_size(Axis2 axis, UI_Size v)
{
  return (axis == Axis2_X ? ui_set_next_pref_width : ui_set_next_pref_height)(v);
}

internal void
ui_push_corner_radius(F32 v)
{
  ui_push_corner_radius_00(v);
  ui_push_corner_radius_01(v);
  ui_push_corner_radius_10(v);
  ui_push_corner_radius_11(v);
}

internal void
ui_pop_corner_radius(void)
{
  ui_pop_corner_radius_00();
  ui_pop_corner_radius_01();
  ui_pop_corner_radius_10();
  ui_pop_corner_radius_11();
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ rjf: Generated Code

#define UI_StackTopImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.top->v;

#define UI_StackBottomImpl(state, name_upper, name_lower) \
return state->name_lower##_stack.bottom_val;

#define UI_StackPushImpl(state, name_upper, name_lower, type, new_value) \
UI_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(ui_build_arena(), UI_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define UI_StackPopImpl(state, name_upper, name_lower) \
UI_##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
SLLStackPop(state->name_lower##_stack.top);\
SLLStackPush(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define UI_StackSetNextImpl(state, name_upper, name_lower, type, new_value) \
UI_##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
else {node = push_array(ui_build_arena(), UI_##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
SLLStackPush(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;

#include "generated/ui.meta.c"
