/////////////////////////////////
//~ Drawing

internal void
bd_draw_ui()
{
  // NotImplemented;
}

internal void
bd_draw_table()
{
  // unpack table settings
  BD_Table *table = &bd_state->table;
  Vec2F32 table_size_px = scale_2f32(rk_state->window_dim, 0.6);
  Vec2F32 cell_size_px = {table_size_px.x/table->size.x, table_size_px.y/table->size.y};

  Vec2F32 window_center = center_2f32(rk_state->window_rect);
  Mat3x3F32 xform2d = make_translate_3x3f32(sub_2f32(window_center, scale_2f32(table_size_px, 0.5)));
  dr_push_xform2d(transpose_3x3f32(xform2d));

  for(U64 j = 0; j < table->size.y; j++)
  {
    for(U64 i = 0; i < table->size.x; i++)
    {
      U64 cell_idx = j*table->size.x+i;
      BD_Cell *cell = &table->cells[cell_idx];
      RK_Key key = rk_key_from_stringf(rk_key_zero(), "cell_%I64u_%I64u", i, j);
      Vec2F32 p0 = {i*cell_size_px.x, j*cell_size_px.y};
      Vec2F32 p1 = {(i+1)*cell_size_px.x, (j+1)*cell_size_px.y};
      Rng2F32 dst = {.p0 = p0, .p1 = p1};
      dst = pad_2f32(dst, -2);
      Vec4F32 clr = v4f32(1,0,0,1);
      if(rk_key_match(rk_state->pixel_hot_key, key))
      {
        clr.w = 0.3;
        dr_rect_keyed(pad_2f32(dst, 4), v4f32(1,1,1,0.1), 1,0,0, rk_2f32_from_key(key));
      }

      dr_rect_keyed(dst, clr, 0,0,0, rk_2f32_from_key(key));
    }
  }

  dr_pop_xform2d();
}

/////////////////////////////////
//~ Entry Calls

RK_FRAME_ENTRY_UPDATE(bd_update)
{
  ////////////////////////////////
  //~ Frame Begin

  bd_state->bucket_main = dr_bucket_make();
  bd_state->bucket_ui = dr_bucket_make();

  ////////////////////////////////
  //~ Events

  B32 window_should_close = 0;
  for(OS_Event *os_evt = rk_state->os_events.first; os_evt != 0; os_evt = os_evt->next)
  {
    if(os_evt->kind == OS_EventKind_WindowClose) {window_should_close = 1;}
  }

  ////////////////////////////////
  //~ What?

  ////////////////////////////////
  //~ Drawing

  DR_BucketScope(bd_state->bucket_main)
  {
    dr_push_viewport(rk_state->window_dim);
    bd_draw_table();
    dr_pop_viewport();
  }

  DR_BucketScope(bd_state->bucket_ui)
  {
    dr_push_viewport(rk_state->window_dim);
    bd_draw_ui();
    dr_pop_viewport();
  }

  ////////////////////////////////
  // Submit

  DR_Bucket **buckets = push_array(rk_frame_arena(), DR_Bucket*, 2);
  buckets[0] = bd_state->bucket_main;
  buckets[1] = bd_state->bucket_ui;
  *return_buckets = buckets;
  *return_bucket_count = 2;

  return window_should_close;
}

internal RK_FrameEntry * 
bd_entry_alloc()
{
  Arena *arena = arena_alloc();
  bd_state = push_array(arena, BD_State, 1);
  bd_state->arena = arena;
  bd_state->table.size = (Vec2U64){9,9};
  bd_state->table.cells = push_array(arena, BD_Cell, bd_state->table.size.x*bd_state->table.size.y);

  // fill+return entry
  RK_FrameEntry *entry = push_array(arena, RK_FrameEntry, 1);
  entry->update = bd_update;
  entry->name = str8_lit("blind");
  return entry;
}
