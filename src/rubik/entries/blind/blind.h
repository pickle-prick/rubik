#ifndef BLIND_H
#define BLIND_H

/////////////////////////////////
//~ Cell

typedef struct BD_Cell BD_Cell;
struct BD_Cell
{
};

/////////////////////////////////
//~ Table

typedef struct BD_Table BD_Table;
struct BD_Table
{
  BD_Cell *cells;
  Vec2U64 size;
};

/////////////////////////////////
//~ State

typedef struct BD_State BD_State;
struct BD_State
{
  Arena *arena;

  DR_Bucket *bucket_main;
  DR_Bucket *bucket_ui;
  BD_Table table;
};

/////////////////////////////////
//~ Globals

global BD_State *bd_state = 0;

/////////////////////////////////
//~ Drawing

internal void bd_draw_ui(void);
internal void bd_draw_table(void);

/////////////////////////////////
//~ Entry Calls

RK_FRAME_ENTRY_UPDATE(bd_update);
internal RK_FrameEntry * bd_entry_alloc();

#endif
