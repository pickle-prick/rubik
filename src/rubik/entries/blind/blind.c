/////////////////////////////////
//~ Entry Calls

RK_FRAME_ENTRY_UPDATE(bd_update)
{
  ////////////////////////////////
  //~ Frame Begin

  bd_state->bucket_main = dr_bucket_make();

  printf("s\n");

  // TODO
  // submit bucket

  return 0;
}

internal RK_FrameEntry * 
bd_entry_alloc()
{
  Arena *arena = arena_alloc();
  BD_State *bd_state = push_array(arena, BD_State, 1);
  bd_state->arena = arena;
  RK_FrameEntry *entry = push_array(arena, RK_FrameEntry, 1);
  entry->update = bd_update;
  entry->name = str8_lit("blind");
  return entry;
}
