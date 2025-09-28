#ifndef BLIND_H
#define BLIND_H

/////////////////////////////////
//~ State

typedef struct BD_State BD_State;
struct BD_State
{
  Arena *arena;

  DR_Bucket *bucket_main;
};

/////////////////////////////////
//~ Globals

global BD_State *bd_state = 0;

/////////////////////////////////
//~ Entry Calls

RK_FRAME_ENTRY_UPDATE(bd_update);
internal RK_FrameEntry * bd_entry_alloc();

#endif
