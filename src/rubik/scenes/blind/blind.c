/////////////////////////////////////////////////////////////////////////////////////////
// Constants
//

#define BD_INITIAL_GOAL 500

// cheating
#define BD_DETERMINED_DICE 0
#define BD_UNLIMITED_ROLL  0
#define BD_NO_NEXT_ROUND   0

// rules
#define BD_RULE_CAPTURE    1
#define BD_RULE_JUMP_ONE   1
#define BD_RULE_LUCK_FLIP  0

#define BD_FLIP_DURATION_SEC 0.2f

/////////////////////////////////////////////////////////////////////////////////////////
// Type/Enum

typedef U64 BD_EntityFlags;
#define BD_EntityFlag_Null (BD_EntityFlags)(1ull<<0)

typedef enum BD_RegSlot
{
  BD_RegSlot_COUNT,
} BD_RegSlot;

typedef enum BD_InstrumentKind
{
  BD_InstrumentKind_Beep,
  BD_InstrumentKind_Boop,
  BD_InstrumentKind_Gear,
  BD_InstrumentKind_Radiation,
  BD_InstrumentKind_AirPressor,
  BD_InstrumentKind_Ping,
  BD_InstrumentKind_Echo,
  BD_InstrumentKind_COUNT,
} BD_InstrumentKind;

typedef enum BD_CellNeighborKind
{
  BD_CellNeighborKind_TopLeft,
  BD_CellNeighborKind_Top,
  BD_CellNeighborKind_TopRight,
  BD_CellNeighborKind_Left,
  BD_CellNeighborKind_Right,
  BD_CellNeighborKind_DownLeft,
  BD_CellNeighborKind_Down,
  BD_CellNeighborKind_DownRight,
  BD_CellNeighborKind_COUNT,
} BD_CellNeighborKind;

typedef enum BD_CellKind
{
  BD_CellKind_Invalid = -1,
  BD_CellKind_Black,
  BD_CellKind_White,
  BD_CellKind_COUNT,
} BD_CellKind;

typedef struct BD_Cell BD_Cell;
struct BD_Cell
{
  BD_Cell *flip_next;

  U64 i;
  U64 j;
  U64 index;
  B32 flipped;
  F32 flip_t;
  B32 has_liberties;
  BD_CellKind kind;
};

typedef struct BD_CellNode BD_CellNode;
struct BD_CellNode
{
  BD_CellNode *next;
  BD_CellNode *prev;
  BD_Cell *cell;
};

typedef struct BD_CellList BD_CellList;
struct BD_CellList
{
  BD_CellList *next;
  BD_CellList *prev;

  BD_CellNode *first;
  BD_CellNode *last;
  U64 count;
};

typedef struct BD_State BD_State;
struct BD_State
{
  SY_Instrument *instruments[S5_InstrumentKind_COUNT];

  U64 round;
  U64 goal;
  U64 roll_count;

  // dice
  B32 dice_rolled;
  B32 draw_is_odd;
  U64 draw_count;

  // grid & cell
  Vec2U64 grid_size;
  BD_Cell *cells;
  U64 cell_count;

  // flip list
  BD_Cell *first_cell_to_flip;
  BD_Cell *last_cell_to_flip;
};

/////////////////////////////////////////////////////////////////////////////////////////
// helpers

#define bd_is_same_kind(left, right) (!left->flipped ? !right->flipped : right->flipped && left->kind == right->kind)

internal BD_CellNode *
bd_cell_list_push(Arena *arena, BD_CellList *list)
{
  BD_CellNode *ret = push_array(arena, BD_CellNode, 1);
  DLLPushBack(list->first, list->last, ret);
  list->count++;
  return ret;
}

internal void
bd_neighbors_for_cell(BD_Cell *cell, BD_Cell **out)
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  Vec2U64 grid_size = bd_state->grid_size;
  U64 i = cell->i;
  U64 j = cell->j;
  U64 cell_index = cell->index;

  B32 has_prev_row = j > 0;
  B32 has_next_row = j < grid_size.y-1;
  B32 has_prev_col = i > 0;
  B32 has_next_col = i < grid_size.x-1;

  U64 left_index  = cell_index - 1;
  U64 top_index   = cell_index - grid_size.x;
  U64 right_index = cell_index + 1;
  U64 down_index  = cell_index + grid_size.x;

  if(has_prev_row && has_prev_col) out[BD_CellNeighborKind_TopLeft] = &bd_state->cells[top_index-1];    // top left
  if(has_prev_row)                 out[BD_CellNeighborKind_Top] = &bd_state->cells[top_index];          // top
  if(has_prev_row && has_next_col) out[BD_CellNeighborKind_TopRight] = &bd_state->cells[top_index+1];   // top right
  if(has_prev_col)                 out[BD_CellNeighborKind_Left] = &bd_state->cells[left_index];        // left
  if(has_next_col)                 out[BD_CellNeighborKind_Right] = &bd_state->cells[right_index];      // right
  if(has_next_row && has_prev_col) out[BD_CellNeighborKind_DownLeft] = &bd_state->cells[down_index-1];  // down left
  if(has_next_row)                 out[BD_CellNeighborKind_Down] = &bd_state->cells[down_index];        // down
  if(has_next_row && has_next_col) out[BD_CellNeighborKind_DownRight] = &bd_state->cells[down_index+1]; // down right
}

internal BD_Cell *
bd_cell_next(BD_Cell *cell, BD_CellNeighborKind next_kind)
{
  // TODO: this is just wastful, simplify it later 
  BD_Cell *ret = 0;
  if(cell)
  {
    BD_Cell *neighbors[BD_CellNeighborKind_COUNT] = {0};
    bd_neighbors_for_cell(cell, neighbors);
    ret = neighbors[next_kind];
  }
  return ret;
}

internal void
bd_flip_list_push(BD_Cell *cell, BD_CellKind kind)
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;
  if(bd_state->first_cell_to_flip == 0) cell->flip_t = 1.0;
  SLLQueuePush_N(bd_state->first_cell_to_flip, bd_state->last_cell_to_flip, cell, flip_next);
  cell->kind = kind;
}

internal void
bd_cell_flood_fill(Arena *arena, B32 *grid, BD_CellList *list, BD_Cell *cell)
{
  // push if not pushed
  if(!grid[cell->index]) 
  {
    grid[cell->index] = 1;

    // collect neighbors
    BD_Cell *neighbors[8] = {0};
    bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
    BD_Cell *direct_neighbors[4] = {0};
    direct_neighbors[0] = neighbors[BD_CellNeighborKind_Left];
    direct_neighbors[1] = neighbors[BD_CellNeighborKind_Top];
    direct_neighbors[2] = neighbors[BD_CellNeighborKind_Right];
    direct_neighbors[3] = neighbors[BD_CellNeighborKind_Down];

    // push to list if provided
    if(list)
    {
      BD_CellNode *node = bd_cell_list_push(arena, list);
      node->cell = cell;
    }

    for(U64 i = 0; i < 4; i++)
    {
      if(direct_neighbors[i])
      {
        BD_Cell *n = direct_neighbors[i];
        B32 same_kind = !cell->flipped ? !n->flipped : n->flipped && cell->kind == n->kind;
        if(same_kind)
        {
          bd_cell_flood_fill(arena, grid, list, n);
        }
      }
    }
  }
}

internal BD_CellList
bd_group_boundary(Arena *arena, BD_CellList *group)
{
  BD_CellList ret = {0};

  for(BD_CellNode *node = group->first; node != 0; node = node->next)
  {
    if(node->cell)
    {
      BD_Cell *cell = node->cell;
      // collect neighbors
      BD_Cell *neighbors[8] = {0};
      bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
      BD_Cell *direct_neighbors[4] = {0};
      direct_neighbors[0] = neighbors[BD_CellNeighborKind_Left];
      direct_neighbors[1] = neighbors[BD_CellNeighborKind_Top];
      direct_neighbors[2] = neighbors[BD_CellNeighborKind_Right];
      direct_neighbors[3] = neighbors[BD_CellNeighborKind_Down];

      for(U64 i = 0; i < 4; i++)
      {
        BD_Cell *n = direct_neighbors[i];
        B32 push = n == 0 || !bd_is_same_kind(cell, n);
        if(push)
        {
          BD_CellNode *node = bd_cell_list_push(arena, &ret);
          node->cell = n;
        }
      }
    }
  }
  return ret;
}

internal B32
bd_cell_has_liberties(BD_Cell *cell, B32 *block_grid, B32 *whitelist_grid, B32 is_root)
{
  B32 ret = whitelist_grid[cell->index];
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  // collect neighbors
  BD_Cell *neighbors[8] = {0};
  bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
  BD_Cell *direct_neighbors[4] = {0};
  direct_neighbors[0] = neighbors[BD_CellNeighborKind_Left];
  direct_neighbors[1] = neighbors[BD_CellNeighborKind_Top];
  direct_neighbors[2] = neighbors[BD_CellNeighborKind_Right];
  direct_neighbors[3] = neighbors[BD_CellNeighborKind_Down];

  ///////////////////////////////////////////////////////////////////////////////////////
  // fast path (top and left are expected visited)

  if(is_root && ret == 0)
  {
    for(U64 i = 0; i < 2 && ret == 0; i++)
    {
      if(!direct_neighbors[i]) continue;
      BD_Cell *n = direct_neighbors[i];
      B32 same_kind = n->flipped && n->kind == cell->kind;

      // top or left has liberties
      if(same_kind)
      {
        ret = n->has_liberties;
      }
    }
  }
  // check if any unflipped neighbor
  if(ret == 0) for(U64 i = 0; i < 4 && ret == 0; i++)
  {
    if(!direct_neighbors[i]) continue;
    BD_Cell *n = direct_neighbors[i];
    if(!n->flipped)
    {
      ret = 1;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // fast path didn't worked? recursivly check neighbors with the same kind

  if(ret == 0) for(U64 i = 0; i < 4 && ret == 0; i++)
  {
    if(!direct_neighbors[i]) continue;
    BD_Cell *n = direct_neighbors[i];
    B32 blocked = block_grid[n->index];
    B32 same_kind = n->flipped && n->kind == cell->kind;
    if(!blocked && same_kind)
    {
      block_grid[cell->index] = 1;
      ret = bd_cell_has_liberties(n, block_grid, whitelist_grid, 0);
      block_grid[cell->index] = 0;
    }
  }

  cell->has_liberties = ret;
  return ret;
}

internal void
bd_cell_flip(BD_Cell *cell, BD_CellKind kind)
{
  ProfBeginFunction();
  AssertAlways(!cell->flipped);

  Temp scratch = scratch_begin(0,0);
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  cell->kind = kind;
  cell->flipped = 1;

  // collect neighbors
  BD_Cell *neighbors[8] = {0};
  bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);

  ///////////////////////////////////////////////////////////////////////////////////////
  // generic rules (capture, ...)

  ////////////////////////////////
  // capture (mimic go)

#if BD_RULE_CAPTURE
  B32 pruned = 0;
  {
    B32 *prune_list = push_array(scratch.arena, B32, bd_state->cell_count);
    B32 *block_grid = push_array(scratch.arena, B32, bd_state->cell_count);
    B32 *whitelist_grid = push_array(scratch.arena, B32, bd_state->cell_count);

    for(U64 pass_index = 0; pass_index < 2; pass_index++)
    {
      // first pass with whitelist
      if(pass_index == 0)
      {
        bd_cell_flood_fill(scratch.arena, whitelist_grid, 0, cell);
      }

      // second pass without whitelist
      if(pass_index == 1)
      {
        MemoryZero(block_grid, sizeof(B32)*bd_state->cell_count);
        MemoryZero(prune_list, sizeof(B32)*bd_state->cell_count);
        MemoryZero(whitelist_grid, sizeof(B32)*bd_state->cell_count);
      }

      for(U64 i = 0; i < bd_state->cell_count; i++)
      {
        BD_Cell *c = &bd_state->cells[i];
        if(c->flipped)
        {
          B32 has_liberties = bd_cell_has_liberties(c, block_grid, whitelist_grid, 1);
          if(!has_liberties)
          {
            prune_list[c->index] = 1;
            pruned = 1;
          }
        }
      }

      // prune dead cell
      for(U64 i = 0; i < bd_state->cell_count; i++)
      {
        if(prune_list[i])
        {
          // TODO(k): decide what would happed to dead cell (unflip or turn)
          bd_state->cells[i].kind = (bd_state->cells[i].kind+1)%BD_CellKind_COUNT;
          // bd_state->cells[i].flipped = 0;
        }
      }
    }
  }

  if(pruned)
  {
    sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 1, 1.0);
    sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0.11, 0.1, 12, 1.0);
  }
#endif

  ///////////////////////////////////////////////////////////////////////////////////////
  // composite rules

  ////////////////////////////////
  // jump one

#if BD_RULE_JUMP_ONE
  for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
  {
    BD_Cell *n = neighbors[i];
    B32 is_direct = (i == BD_CellNeighborKind_Top)   ||
                    (i == BD_CellNeighborKind_Right) ||
                    (i == BD_CellNeighborKind_Down)  ||
                    (i == BD_CellNeighborKind_Left);

    if(n && is_direct && !n->flipped)
    {
      BD_Cell *nn = bd_cell_next(n, i);
      if(nn && nn->flipped && nn->kind == cell->kind)
      {
        bd_flip_list_push(n, cell->kind);
      }
    }
  }
#endif

  ////////////////////////////////
  // lucky flip

#if BD_RULE_LUCK_FLIP
  {
    U64 max_luck_flip = 8;
    U64 luck_flip = 0;
    for(U64 i = 0; i < BD_CellNeighborKind_COUNT && luck_flip < max_luck_flip; i++)
    {
      BD_Cell *n = neighbors[i];
      if(n && !n->flipped)
      {
        // 10%
        F32 pct = ((F32)rand()/RAND_MAX);
        B32 auto_flip = pct <= 0.1;
        if(auto_flip)
        {
          bd_flip_list_push(n, cell->kind);
          luck_flip++;
        }
      }
    }
  }
#endif
  scratch_end(scratch);
  ProfEnd();
}

internal void
bd_round_begin(void)
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  // TODO(k): we could use some curve here, initial slow
  bd_state->goal = BD_INITIAL_GOAL*bd_state->round;
  bd_state->roll_count = 6;
  bd_state->dice_rolled = 0;
  bd_state->draw_count = 0;
  bd_state->first_cell_to_flip = 0;
  bd_state->last_cell_to_flip = 0;

  // reset deck
  U64 cell_count = bd_state->grid_size.x*bd_state->grid_size.y;
  for(U64 i = 0; i < cell_count; i++)
  {
    BD_Cell *cell = &bd_state->cells[i];
    cell->flipped = 0;
    cell->kind = BD_CellKind_Invalid;
    cell->flip_t = 0.0;
  }
}

internal void
bd_round_end(B32 next)
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  if(next)
  {
    bd_state->round++;
  }
  // no next, end game
  // TODO: proper ending handling
  else
  {
    bd_state->round = 1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// update & setup & default

RK_SCENE_UPDATE(bd_update)
{
  Temp scratch = scratch_begin(0,0);
  BD_State *bd_state = scene->custom_data;
  RK_NodeBucket *nobd_bucket = scene->node_bucket;
  RK_Node **nodes = 0;
  for(U64 slot_index = 0; slot_index < nobd_bucket->hash_table_size; slot_index++)
  {
    for(RK_Node *node = nobd_bucket->hash_table[slot_index].first;
        node != 0;
        node = node->hash_next)
    {
      darray_push(rk_frame_arena(), nodes, node);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // accumulators

  U64 score = 0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // process flip list

  B32 flipping = bd_state->first_cell_to_flip != 0;
#if 1
  F32 budget = rk_state->frame_dt;
  for(BD_Cell *cell = bd_state->first_cell_to_flip, *next = 0;
      cell != 0 && budget > 0.0;
      cell = next)
  {
    BD_Cell *next = cell->flip_next;
    F32 remain = (1.0-cell->flip_t)*BD_FLIP_DURATION_SEC;

    F32 t = 0;
    if(remain >= budget)
    {
      t = budget/BD_FLIP_DURATION_SEC;
      budget = 0;
    }
    else
    {
      t = remain/BD_FLIP_DURATION_SEC;
      budget -= remain;
    }

    cell->flip_t += t;
    cell->flip_t = Clamp(0.0, cell->flip_t, 1.0);
    if(abs_f32(cell->flip_t-1.0f) < 0.001f) cell->flip_t = 1.0;

    B32 flipped = cell->flip_t == 1.0;
    AssertAlways(!cell->flipped);
    if(flipped)
    {
      bd_cell_flip(cell, cell->kind);
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 3, 1.0);
      SLLQueuePop_N(bd_state->first_cell_to_flip, bd_state->last_cell_to_flip, flip_next);
    }
  }
#else
  for(BD_Cell *cell = bd_state->first_cell_to_flip, *next = 0;
      cell != 0;
      cell = next)
  {
    cell->flip_t += rk_state->animation.vast_rate * (1.0f - cell->flip_t);
    if(abs_f32(cell->flip_t-1.0f) < 0.001) cell->flip_t = 1.0;
    cell->flip_t = Clamp(0.0, cell->flip_t, 1.0);
    B32 flipped = cell->flip_t == 1.0;
    if(flipped)
    {
      bd_cell_flip(cell, cell->kind);
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 1, 1.0);
      SLLQueuePop_N(bd_state->first_cell_to_flip, bd_state->last_cell_to_flip, flip_next);
      break;
    }
  }
#endif

  ///////////////////////////////////////////////////////////////////////////////////////
  // game ui

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *overlay_container = 0;
    UI_Rect(rk_state->window_rect)
    {
      overlay_container = ui_build_box_from_stringf(0, "###game_overlay");
    }
    ui_push_parent(overlay_container);

    Vec2F32 window_dim_h = scale_2f32(rk_state->window_dim, 0.5);
    Vec2F32 window_dim_hh = scale_2f32(rk_state->window_dim, 0.25);

    /////////////////////////////////////////////////////////////////////////////////////
    // grid (deck)

    Vec2F32 grid_dim = {1300,1300};
    Vec2F32 grid_dim_h = scale_2f32(grid_dim, 0.5);
    Vec2F32 cell_dim = {grid_dim.x/(F32)bd_state->grid_size.x, grid_dim.y/(F32)bd_state->grid_size.y};

    UI_Box *grid_container = 0;
    {
      // center the grid
      Rng2F32 rect = {0,0,grid_dim.x,grid_dim.y};
      rect = shift_2f32(rect, v2f32(window_dim_h.x-grid_dim_h.x, window_dim_h.y-grid_dim_h.y));

      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
        UI_ChildLayoutAxis(Axis2_Y)
      grid_container = ui_build_box_from_stringf(0, "##grid_container");
    }

    Vec2U64 grid_size = bd_state->grid_size;
    // slots
    UI_Parent(grid_container)
    {
      for(U64 j = 0; j < grid_size.y; j++) 
      {
        UI_Box *row_container;
        UI_PrefWidth(ui_pct(1.0,0.0))
          UI_PrefHeight(ui_pct(1.0,0.0))
          UI_ChildLayoutAxis(Axis2_X)
          row_container = ui_build_box_from_stringf(0, "row_%I64u", j);

        for(U64 i = 0; i < grid_size.y; i++) UI_Parent(row_container)
        {
          BD_Cell *cell = &bd_state->cells[j*grid_size.x+i];
          UI_Box *cell_container;
          UI_PrefWidth(ui_pct(1.0,0.0))
            UI_PrefHeight(ui_pct(1.0,0.0))
            UI_Flags(UI_BoxFlag_DrawBorder)
          {
            cell_container = ui_build_box_from_stringf(0, "cell_%I64u:%I64u", i,j);
          }

          UI_Parent(cell_container) ProfScope("draw cell")
          {
            if(!cell->flipped)
            {
              UI_Box *b;
              UI_HeightFill
                UI_WidthFill
                UI_Column
                UI_Padding(ui_em(0.35,1.0))
                UI_Row
                UI_Padding(ui_em(0.35,1.0))
                UI_Flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects)
                UI_Palette(ui_build_palette(ui_top_palette(), .background = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)))
                UI_CornerRadius(2.0)
                UI_TextAlignment(UI_TextAlign_Center)
                b = ui_build_box_from_stringf(0, "btn");
              UI_Signal sig = ui_signal_from_box(b);

              B32 can_flip = bd_state->dice_rolled && bd_state->draw_count>0;
              if(((sig.f&UI_SignalFlag_LeftClicked) || (sig.f&UI_SignalFlag_RightClicked)) && !flipping)
              {
                if(can_flip)
                {
#if BD_DETERMINED_DICE
                  if(sig.f & UI_SignalFlag_LeftClicked)
                  {
                    bd_flip_list_push(cell, BD_CellKind_Black);
                  }
                  else
                  {
                    bd_flip_list_push(cell, BD_CellKind_White);
                  }
#else
                  BD_CellKind kind = bd_state->draw_is_odd ? BD_CellKind_Black : rand()%BD_CellKind_COUNT;
                  bd_flip_list_push(cell, kind);
#endif

                  bd_state->draw_count--;
                  if(bd_state->draw_count == 0)
                  {
                    bd_state->dice_rolled = 0;
                  }
                }
                else
                {
                  sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 12, 1.0);
                  sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0.11, 0.1, 1, 1.0);
                }
              }
            }
            else
            {
              UI_Box *b;
              String8 display = cell->kind == BD_CellKind_Black ? str8_lit("x") : str8_lit("O");
              UI_HeightFill
                UI_WidthFill
                UI_Column
                UI_Padding(ui_em(0.5,1.0))
                UI_Row
                UI_Padding(ui_em(0.5,1.0))
                UI_Flags(UI_BoxFlag_DrawText)
                UI_Font(ui_icon_font())
                // UI_FontSize(ui_top_font_size()*1.1)
                UI_FontSize(cell_dim.y*0.5)
                UI_TextAlignment(UI_TextAlign_Center)
                b = ui_build_box_from_string(0, display);
            }
          }
        }
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Dice

    Vec2F32 dice_size = {100,100};
    UI_Box *dice_container;
    {
      UI_Flags(UI_BoxFlag_Floating|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
        UI_FixedX(grid_container->rect.x1+30) UI_FixedY(grid_container->rect.y1-dice_size.y)
        UI_PrefWidth(ui_px(dice_size.x,0.0)) UI_PrefHeight(ui_px(dice_size.y,0.0))
        UI_CornerRadius(3.0)
        dice_container = ui_build_box_from_stringf(0, "dice_container");

      UI_BoxFlags flags = UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_Clickable|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawActiveEffects;
      String8 display = {0};
      if(bd_state->dice_rolled)
      {
        flags |= UI_BoxFlag_DrawText;
        display = push_str8f(rk_frame_arena(), "%I64u", bd_state->draw_count);
      }
      UI_Parent(dice_container)
        UI_WidthFill UI_HeightFill
        UI_Column UI_Padding(ui_em(0.3,1.0))
        UI_Row UI_Padding(ui_em(0.3,1.0))
        UI_CornerRadius(3.0)
        UI_Flags(flags)
        UI_FontSize(ui_top_font_size()*2.5)
        UI_Palette(ui_build_palette(ui_top_palette(), .background = rk_rgba_from_theme_color(RK_ThemeColor_HighlightOverlay)))
        UI_TextAlignment(UI_TextAlign_Center)
      {
        UI_Box *dice = ui_build_box_from_stringf(0, "%S##dice", display);
        UI_Signal sig = ui_signal_from_box(dice);

        if(ui_clicked(sig))
        {
          if(bd_state->roll_count > 0)
          {
            bd_state->dice_rolled = 1;
            bd_state->draw_count = rand()%6 + 1; // 1-6
            bd_state->draw_is_odd = (bd_state->draw_count%2) != 0;
            sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 12, 1.0);
#if !BD_UNLIMITED_ROLL
            bd_state->roll_count--;
#endif
          }
          else
          {
            sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 12, 1.0);
            sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0.11, 0.1, 1, 1.0);
          }
        }
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // compute scores

    U64 cell_count = bd_state->cell_count;

    U64 territory = 0;
    // collect territory
    {
      B32 *visit_grid = push_array(scratch.arena, B32, cell_count);
      BD_CellList *first_group = 0;
      BD_CellList *last_group = 0;
      U64 group_count = 0;
      for(U64 index = 0; index < cell_count; index++)
      {
        BD_Cell *c = &bd_state->cells[index];
        if(visit_grid[index] || c->flipped) continue;

        BD_CellList *group = push_array(scratch.arena, BD_CellList, 1);
        bd_cell_flood_fill(scratch.arena, visit_grid, group, c);
        SLLQueuePush(first_group, last_group, group);
        group_count++;
      }

      for(BD_CellList *group = first_group;
          group != 0;
          group = group->next)
      {
        BD_CellList boundaries = bd_group_boundary(scratch.arena, group);
        B32 is_black_territory = 0;
        for(BD_CellNode *node = boundaries.first;
            node != 0;
            node = node->next)
        {
          BD_Cell *c = node->cell;
          if(c)
          {
            if(c->kind == BD_CellKind_Black) is_black_territory = 1;
            if(c->kind == BD_CellKind_White)
            {
              is_black_territory = 0;
              break;
            }
          }
        }

        if(is_black_territory)
        {
          territory += group->count;
        }
      }
      // printf("territory: %lu\n", territory);
    }

    U64 black_count = 0;
    U64 white_count = 0;
    U64 qi = 0;
    for(U64 cell_index = 0; cell_index < cell_count; cell_index++)
    {
      BD_Cell *cell = &bd_state->cells[cell_index];
      if(!cell->flipped) continue;
      if(cell->kind == BD_CellKind_Black)
      {
        black_count++;
        // collect neighbors
        BD_Cell *neighbors[8] = {0};
        bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
        BD_Cell *direct_neighbors[4] = {0};
        direct_neighbors[0] = neighbors[BD_CellNeighborKind_Left];
        direct_neighbors[1] = neighbors[BD_CellNeighborKind_Top];
        direct_neighbors[2] = neighbors[BD_CellNeighborKind_Right];
        direct_neighbors[3] = neighbors[BD_CellNeighborKind_Down];

        for(U64 i = 0; i < 4; i++)
        {
          if(direct_neighbors[i])
          {
            BD_Cell *n = direct_neighbors[i];
            B32 same_kind = bd_is_same_kind(cell, n);
            if(same_kind) qi++;
          }
        }
      }
      else if(cell->kind == BD_CellKind_White)
      {
        white_count++;
      }
    }

    score = territory * qi;

    /////////////////////////////////////////////////////////////////////////////////////
    // score panel

    UI_Parent(overlay_container)
    {
      UI_Box *score_panel;
      UI_FixedX(10) UI_FixedY(10)
        UI_PrefHeight(ui_children_sum(0.0))
        UI_ChildLayoutAxis(Axis2_Y)
        score_panel = ui_build_box_from_stringf(0, "score_panel");

      UI_Parent(score_panel)
        UI_PrefHeight(ui_children_sum(0.0))
        UI_Flags(UI_BoxFlag_DrawBorder)
        UI_PrefHeight(ui_em(3,0.0))
        UI_TextPadding(10.0)
        UI_FontSize(ui_top_font_size()*2.5)
      {
        UI_Row
        {
          ui_labelf("Round");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", bd_state->round);
        }

        UI_Row
        {
          ui_labelf("Goal");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", bd_state->goal);
        }

        UI_Row
        {
          ui_labelf("Score");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", score);
        }

        UI_Row
        {
          ui_labelf("Territory");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", territory);
        }

        UI_Row
        {
          ui_labelf("Qi");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", qi);
        }

        UI_Row
        {
          ui_labelf("Rolls");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64d", bd_state->roll_count);
        }

        UI_Row
        {
          ui_labelf("Black");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", black_count);
        }

        UI_Row
        {
          ui_labelf("White");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", white_count);
        }
        // rk_capped_labelf(submarine->pulse_cd_t, "pulse_cd_t: %.2f", submarine->pulse_cd_t);
      }
    }

    ui_pop_parent();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // next round or end game

#if 1
  U64 score_u64 = 0;
  if(score > 0) score_u64 = (U64)score;
  if(!flipping)
  {
#if !BD_NO_NEXT_ROUND
    if(bd_state->goal <= score_u64)
    {
      bd_round_end(1);
      bd_round_begin();
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.2, 6, 1.0);
    }
    else if(bd_state->roll_count == 0 && bd_state->draw_count == 0)
    {
      bd_round_end(0);
      bd_round_begin();
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0, 0.2, 12, 1.0);
    }
#endif
  }
#endif
  scratch_end(scratch);
}

RK_SCENE_SETUP(bd_setup)
{
  BD_State *bd_state = scene->custom_data;
  for(U64 i = 0; i < BD_InstrumentKind_COUNT; i++)
  {
    SY_Instrument **dst = &bd_state->instruments[i];
    SY_Instrument *src = 0;
    switch(i)
    {
      case BD_InstrumentKind_Beep:
      {

        src = sy_instrument_alloc(str8_lit("beep"));
        src->env.attack_time  = 0.005f;
        src->env.decay_time   = 0.05f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.8f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 880.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 1.0;
        }
      }break;
      case BD_InstrumentKind_Boop:
      {
        src = sy_instrument_alloc(str8_lit("boop"));
        src->env.attack_time  = 0.005f;
        src->env.decay_time   = 0.05f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.8f;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 440.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 1.0;
        }
      }break;
      case BD_InstrumentKind_Gear:
      {
        src = sy_instrument_alloc(str8_lit("gear"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.10f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.0f;

        // Main gear clunk body (low-frequency thump)
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 80.0;
          osc->kind = SY_OSC_Kind_Square;
        }

        // Add high-frequency metallic overtone
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 1000.0;
          osc->kind = SY_OSC_Kind_Saw;
          osc->amp = 1.0f;
        }

        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 2000.0;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0f;
        }

        // Add some white noise for texture (if supported)
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->kind = SY_OSC_Kind_NoiseWhite;
          osc->amp = 0.3f;
        }
      } break;
      case BD_InstrumentKind_Radiation:
      {
        src = sy_instrument_alloc(str8_lit("radiation"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.001f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 3.0f;
        src->env.sustain_amp  = 0.1f;

        {
          // Main burst - high-pitched noise
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0; // Assume 0.0 or special flag means white noise
          osc->kind = SY_OSC_Kind_NoiseBrown;
          osc->amp = 1.0;
        }
      }break;
      case BD_InstrumentKind_AirPressor:
      {
#if 1
        src = sy_instrument_alloc(str8_lit("air_pressor"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.8f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 0.1f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0;
          osc->kind = SY_OSC_Kind_NoiseWhite;
          osc->amp = 1.0;
        }
#else
        src  = sy_instrument_alloc(str8_lit("air_pressor"));
        src->env.attack_time  = 0.001f;
        src->env.decay_time   = 0.0f;
        src->env.release_time = 0.0f;
        src->env.start_amp    = 1.0f;
        src->env.sustain_amp  = 1.0f;
        {

          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 0.0;
          osc->kind = SY_OSC_Kind_NoiseBrown;
          osc->amp = 1.0;
        }
#endif
      }break;
      case BD_InstrumentKind_Ping:
      {
        // ping: sonar-like clean sine tone with subtle fade out
        src = sy_instrument_alloc(str8_lit("ping"));
        src->env.attack_time = 0.01f;
        src->env.decay_time = 0.3f;
        src->env.release_time = 0.2f;
        src->env.start_amp = 3.0;
        src->env.sustain_amp = 0.0;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 1500;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0;
        }
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 440.0;
          osc->kind = SY_OSC_Kind_Square;
          osc->amp = 0.2f;
        }
      }break;
      case BD_InstrumentKind_Echo:
      {
        // echo: softer, lower sine wave to simulate sonar reflection
        src = sy_instrument_alloc(str8_lit("echo"));
        src->env.attack_time = 0.005f;
        src->env.decay_time = 0.6f;
        src->env.release_time = 0.3f;
        src->env.start_amp = 1.6;
        src->env.sustain_amp = 0.0;
        {
          SY_InstrumentOSCNode *osc = sy_instrument_push_osc(src);
          osc->base_hz = 880.0f*0.75f;
          osc->kind = SY_OSC_Kind_Sine;
          osc->amp = 1.0;
        }
      }break;
      default:{InvalidPath;}break;
    }
    *dst = src;
  }

  bd_state->grid_size = (Vec2U64){9,9};
  U64 cell_count = bd_state->grid_size.x*bd_state->grid_size.y;
  bd_state->cells = push_array(scene->arena, BD_Cell, cell_count);
  bd_state->cell_count = cell_count;

  for(U64 cell_index = 0;
      cell_index < bd_state->grid_size.x*bd_state->grid_size.y;
      cell_index++)
  {
    U64 i = cell_index % bd_state->grid_size.x;
    U64 j = cell_index / bd_state->grid_size.x;
    bd_state->cells[cell_index].i = i;
    bd_state->cells[cell_index].j = j;
    bd_state->cells[cell_index].index = cell_index;
  }

  // start the first round
  bd_state->round = 1;
  bd_round_begin();
}

RK_SCENE_DEFAULT(bd_default)
{
  RK_Scene *ret = rk_scene_alloc();
  ret->name            = str8_lit("blind");
  ret->save_path       = str8_lit("./src/rubik/scenes/blind/default.tscn");
  ret->setup_fn_name   = str8_lit("bd_setup");
  ret->default_fn_name = str8_lit("bd_default");
  ret->update_fn_name  = str8_lit("bd_update");

  // push ctx
  rk_push_scene(ret);
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  ///////////////////////////////////////////////////////////////////////////////////////
  // scene settings

  ret->omit_grid = 1;
  ret->omit_gizmo3d = 1;
  ret->omit_light = 1;

  // scene data
  BD_State *bd_state = rk_scene_push_custom_data(ret, BD_State);

  // 2d viewport
  // Rng2F32 viewport_screen = {0,0,600,600};
  // Vec2F32 viewport_screen_dim = dim_2f32(viewport_screen);
  Rng2F32 viewport_world = rk_state->window_rect;

  // root node
  RK_Node *root = rk_build_node2d_from_stringf(0,0, "root");

  ///////////////////////////////////////////////////////////////////////////////////////
  // load resources

  ///////////////////////////////////////////////////////////////////////////////////////
  // build node tree

  RK_Parent_Scope(root)
  {
    // create the orthographic camera 
    RK_Node *game_camera_node = rk_build_camera3d_from_stringf(0, 0, "camera2d");
    {
      // game_camera_node->camera3d->viewport = viewport_screen;
      game_camera_node->camera3d->projection = RK_ProjectionKind_Orthographic;
      game_camera_node->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
      game_camera_node->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
      game_camera_node->camera3d->hide_cursor = 0;
      game_camera_node->camera3d->lock_cursor = 0;
      game_camera_node->camera3d->is_active = 1;
      game_camera_node->camera3d->zn = -0.1;
      game_camera_node->camera3d->zf = 1000; // support 1000 layers
      game_camera_node->camera3d->orthographic.top    = viewport_world.y0;
      game_camera_node->camera3d->orthographic.bottom = viewport_world.y1;
      game_camera_node->camera3d->orthographic.left   = viewport_world.x0;
      game_camera_node->camera3d->orthographic.right  = viewport_world.x1;
      game_camera_node->node3d->transform.position = v3f32(0,0,0);
      game_camera_node->custom_flags = S5_EntityFlag_GameCamera;

      S5_Entity *entity = rk_node_push_custom_data(game_camera_node, S5_Entity);
      S5_GameCamera *camera = &entity->game_camera;
      camera->viewport_world = viewport_world;
      camera->viewport_world_target = viewport_world;
    }
    ret->active_camera = rk_handle_from_node(game_camera_node);
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // end

  // TODO(k): maybe set it somewhere else
  ret->setup_fn = bd_setup;
  ret->update_fn = bd_update;
  ret->default_fn = bd_default;
  ret->root = rk_handle_from_node(root);

  rk_push_scene(ret);
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  // TODO(k): call it somewhere else
  bd_setup(ret);

  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  return ret;
}
