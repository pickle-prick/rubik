/////////////////////////////////////////////////////////////////////////////////////////
// Constants


#define BD_INITIAL_GOAL 20

// cheating
#define BD_DETERMINED_DICE 0
#define BD_UNLIMITED_ROLL  0
#define BD_NO_NEXT_ROUND   0
#define BD_IGNORE_DICE     0

// rules
#define BD_RULE_CAPTURE       1
#define BD_RULE_JUMP_ONE      1
#define BD_RULE_LUCK_FLIP     0
#define BD_RULE_TICK_TACK_TOE 0
#define BD_RULE_UPPER_HAND    1

// #define BD_FLIP_DURATION_SEC 0.2f

/////////////////////////////////////////////////////////////////////////////////////////
// Type/Enum

typedef U64 BD_EntityFlags;
#define BD_EntityFlag_Null (BD_EntityFlags)(1ull<<0)

typedef U64 BD_DiceFlags;
#define BD_DiceFlag_BlackOnOdd (BD_DiceFlags)(1ull<<0)

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

typedef enum BD_SpriteKind
{
  BD_SpriteKind_White,
  BD_SpriteKind_Black,
  BD_SpriteKind_FaceDefault,
  BD_SpriteKind_FaceTiger,
  BD_SpriteKind_FacePanda,
  BD_SpriteKind_FaceWolf,
  BD_SpriteKind_COUNT,
} BD_SpriteKind;

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

typedef enum BD_CellBinaryKind
{
  BD_CellBinaryKind_Invalid = -1,
  BD_CellBinaryKind_Black,
  BD_CellBinaryKind_White,
  BD_CellBinaryKind_COUNT,
} BD_CellBinaryKind;

typedef enum BD_CellKind
{
  BD_CellKind_Default,
  BD_CellKind_Unity,
  BD_CellKind_Tiger,
  BD_CellKind_Wolf,
  BD_CellKind_COUNT,
} BD_CellKind;

typedef struct BD_Cell BD_Cell;
struct BD_Cell
{
  BD_CellBinaryKind binary;
  BD_CellKind kind;
  U64 base_value;
  U64 base_value_this_round;
  U64 cached_value;

  // position info
  U64 i;
  U64 j;
  U64 index;

  B32 flipped;
  F32 flip_t;
  F32 animation_t;
  B32 animating;
  B32 has_liberties;
  U64 qi;
  B32 captured;
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
  U64 gold;

  B32 is_shopping;
  B32 is_round_end;
  B32 is_failed;

  // resource
  RK_Handle sprites[BD_SpriteKind_COUNT];

  // dice
  B32 dice_rolled;
  BD_DiceFlags dice_flags;
  B32 draw_is_odd;
  U64 draw_count_src;
  U64 draw_count;

  // grid & cell
  Vec2U64 grid_size;
  BD_Cell *cells;
  U64 cell_count;

  // list
  Arena *flip_arena;
  U64 flip_arena_pos;
  BD_CellList flip_list;
  Arena *animation_arena;
  U64 animation_arena_pos;
  BD_CellList animation_list;
};

/////////////////////////////////////////////////////////////////////////////////////////
// maps

internal const String8 bd_string_from_cell_binary[BD_CellBinaryKind_COUNT] =
{
  str8_lit_comp("Black"),
  str8_lit_comp("White"),
};

internal const String8 bd_name_from_cell_kind[BD_CellKind_COUNT] =
{
  str8_lit_comp("Default"),
  str8_lit_comp("Unity"), // double the score of allies
  str8_lit_comp("Tiger"),
  str8_lit_comp("Wolf"),
};

internal const String8 bd_desc_from_cell_kind[BD_CellKind_COUNT] = 
{
  str8_lit_comp("Default"),
  str8_lit_comp("Double score of all allies in 3x3"),
  str8_lit_comp("Turn all enemies (non-tiger) to the same binary"),
  str8_lit_comp("Every unflipped neighbors will be the same binary upon flipped"),
};

internal const BD_SpriteKind bd_sprite_kind_from_cell_kind[BD_CellKind_COUNT] =
{
  BD_SpriteKind_FaceDefault,
  BD_SpriteKind_FacePanda,
  BD_SpriteKind_FaceTiger,
  BD_SpriteKind_FaceWolf,
};

/////////////////////////////////////////////////////////////////////////////////////////
// helpers

#define bd_is_same_binary(left, right) (!left->flipped ? !right->flipped : right->flipped && left->binary == right->binary)

internal void
bd_cell_trigger_animation(BD_Cell *cell)
{
  cell->animation_t = 0.0;
  cell->animating = 1;
}

internal U64
bd_dice_roll()
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;
  U64 ret = rand()%6+1; // 1-6
  return ret;
}

internal BD_CellNode *
bd_cell_list_push(Arena *arena, BD_CellList *list, BD_Cell *cell)
{
  BD_CellNode *ret = push_array(arena, BD_CellNode, 1);
  ret->cell = cell;
  DLLPushBack(list->first, list->last, ret);
  list->count++;
  return ret;
}

internal void
bd_cell_list_remove(BD_CellList *list, BD_CellNode *cn)
{
  DLLRemove(list->first, list->last, cn);
  list->count--;
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
  // TODO: this is just not necessary, simplify it later 
  BD_Cell *ret = 0;
  if(cell)
  {
    BD_Cell *neighbors[BD_CellNeighborKind_COUNT] = {0};
    bd_neighbors_for_cell(cell, neighbors);
    ret = neighbors[next_kind];
  }
  return ret;
}

// TODO: cheange the name
internal BD_CellBinaryKind
bd_draw_binary(BD_Cell *cell)
{
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;
  BD_CellBinaryKind ret = BD_CellBinaryKind_Invalid;

  // collect neighbors
  BD_Cell *neighbors[8] = {0};
  bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);

  // wolf
  U64 black_wolves_count = 0;
  U64 white_wolves_count = 0;
  for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
  {
    BD_Cell *n = neighbors[i];
    if(n && n->flipped && n->kind == BD_CellKind_Wolf)
    {
      if(n->binary == BD_CellBinaryKind_Black)
      {
        black_wolves_count++;
      }
      else
      {
        white_wolves_count++;
      }
    }
  }
  if(black_wolves_count+white_wolves_count != 0)
  {
    if(black_wolves_count > white_wolves_count)
    {
      ret = BD_CellBinaryKind_Black;
    }
    else
    {
      ret = BD_CellBinaryKind_White;
    }
  }

  if(ret == BD_CellBinaryKind_Invalid)
    ret = bd_state->draw_count_src==1 ? BD_CellBinaryKind_Black : rand()%BD_CellBinaryKind_COUNT;

  return ret;
}


internal void
bd_cell_update_qi(BD_Cell *cell)
{
  if(cell->flipped)
  {
    // collect neighbors
    BD_Cell *neighbors[8] = {0};
    bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
    BD_Cell *direct_neighbors[4] = {0};
    direct_neighbors[0] = neighbors[BD_CellNeighborKind_Left];
    direct_neighbors[1] = neighbors[BD_CellNeighborKind_Top];
    direct_neighbors[2] = neighbors[BD_CellNeighborKind_Right];
    direct_neighbors[3] = neighbors[BD_CellNeighborKind_Down];

    U64 qi = 0;
    for(U64 i = 0; i < 4; i++)
    {
      if(direct_neighbors[i])
      {
        BD_Cell *n = direct_neighbors[i];
        B32 same_binary = bd_is_same_binary(cell, n);
        if(same_binary) qi++;
      }
    }
    cell->qi = qi;
  }
}

internal void
bd_cell_flood_fill(Arena *arena, B32 *grid, BD_CellList *list, BD_Cell *cell)
{
  ProfBeginFunction();

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

    // push to linear list if provided, representing result with grid may be not enough
    if(list)
    {
      bd_cell_list_push(arena, list, cell);
    }

    // recursivly check all direct neighbors
    for(U64 i = 0; i < 4; i++)
    {
      BD_Cell *n = direct_neighbors[i];
      if(n && bd_is_same_binary(n, cell) && !grid[n->index])
      {
        bd_cell_flood_fill(arena, grid, list, n);
      }
    }
  }
  ProfEnd();
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
        B32 push = n == 0 || !bd_is_same_binary(cell, n);
        if(push)
        {
          bd_cell_list_push(arena, &ret, n);
        }
      }
    }
  }
  return ret;
}

// TODO(k): this would causing infinite or unnecessary long recusivly loop
// NOTE(k): we are expecting this function be called in grid order
internal B32
bd_cell_has_liberties(BD_Cell *cell, B32 *block_grid, B32 *whitelist_grid, B32 is_root)
{
  ProfBeginFunction();
  // NOTE: any cell in whitelist has liberties (we don't check yet, so we could handle upper hand situaition)
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

  // not cached? => use fast path first
  if(is_root && ret == 0)
  {
    for(U64 i = 0; i < 2 && ret == 0; i++)
    {
      if(direct_neighbors[i])
      {
        BD_Cell *n = direct_neighbors[i];
        B32 same_kind = n->flipped && n->binary == cell->binary;

        // top or left has liberties
        if(same_kind)
        {
          ret = n->has_liberties;
        }
      }
    }
  }

  // top and left has no liberties => check if any unflipped neighbor
  if(ret == 0) for(U64 i = 0; i < 4 && ret == 0; i++)
  {
    BD_Cell *n = direct_neighbors[i];
    if(n && !n->flipped)
    {
      ret = 1;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // fast path didn't worked? recursivly check neighbors with the same binary

  if(ret == 0) for(U64 i = 0; i < 4 && ret == 0; i++)
  {
    if(!direct_neighbors[i]) continue;
    BD_Cell *n = direct_neighbors[i];
    B32 blocked = block_grid[n->index];
    B32 same_binary = n->flipped && n->binary == cell->binary;
    if(!blocked && same_binary)
    {
      block_grid[cell->index] = 1;
      ret = bd_cell_has_liberties(n, block_grid, whitelist_grid, 0);
      block_grid[cell->index] = 0;
    }
  }

  cell->has_liberties = ret;
  ProfEnd();
  return ret;
}

internal U64
bd_value_from_cell(BD_Cell *cell)
{
  U64 ret = cell->base_value_this_round;

  BD_Cell *neighbors[8] = {0};
  bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);
  for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
  {
    BD_Cell *n = neighbors[i];
    B32 is_direct = (i == BD_CellNeighborKind_Top)   || (i == BD_CellNeighborKind_Right) || (i == BD_CellNeighborKind_Down)  || (i == BD_CellNeighborKind_Left);
    if(n && n->flipped)
    {
      B32 same_binary = bd_is_same_binary(cell, n);
      switch(n->kind)
      {
        case BD_CellKind_Unity:
        {
          if(same_binary) ret*=2;
        }break;
        case BD_CellKind_Default:
        case BD_CellKind_Tiger:
        case BD_CellKind_Wolf:{}break;
        default:{InvalidPath;}break;
      }
    }
  }
  return ret;
}

// TODO(k): revisit is needed, performance issue in some situation
internal void
bd_cell_flip(BD_Cell *cell, BD_CellBinaryKind binary, B32 passive, B32 turned)
{
  ProfBeginFunction();

  // collect neighbors
  BD_Cell *neighbors[BD_CellNeighborKind_COUNT] = {0};
  bd_neighbors_for_cell(cell, (BD_Cell**)neighbors);

  Temp scratch = scratch_begin(0,0);
  RK_Scene *scene = rk_top_scene();
  BD_State *bd_state = scene->custom_data;

  bd_cell_list_push(bd_state->flip_arena, &bd_state->flip_list, cell);

  cell->binary = binary;
  cell->flipped = 1;

  ///////////////////////////////////////////////////////////////////////////////////////
  // generic rules (capture, ...)

  ////////////////////////////////
  // capture (mimic go)

#if BD_RULE_CAPTURE
  B32 pruned = 0;
  ProfBegin("capture");
  {
    B32 *prune_list = push_array(scratch.arena, B32, bd_state->cell_count);
    B32 *block_grid = push_array(scratch.arena, B32, bd_state->cell_count);
    B32 *whitelist_grid = push_array(scratch.arena, B32, bd_state->cell_count);

    for(U64 pass_index = 0; pass_index < 2; pass_index++)
    {
      // first pass with whitelist (upper hand)
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

      // add all flipped cell without any liberty to prune list
      for(U64 i = 0; i < bd_state->cell_count; i++)
      {
        BD_Cell *c = &bd_state->cells[i];
        if(c->flipped)
        {
          // TODO(BUG): infinite loop when flip the last cell in the grid
          B32 has_liberties = bd_cell_has_liberties(c, block_grid, whitelist_grid, 1);
          if(!has_liberties)
          {
            c->captured = 1;
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
          bd_state->cells[i].binary = (bd_state->cells[i].binary+1)%BD_CellBinaryKind_COUNT;
          bd_cell_trigger_animation(&bd_state->cells[i]);
        }
      }
    }
  }

  if(pruned)
  {
    sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 1, 1.0);
    sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0.11, 0.1, 12, 1.0);
  }
  ProfEnd();
#endif

  ///////////////////////////////////////////////////////////////////////////////////////
  // triggering dice rules

  ////////////////////////////////
  // upper hand

#if BD_RULE_UPPER_HAND
  if(!turned & !passive)
  {
    bd_cell_update_qi(cell);
    for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
    {
      BD_Cell *n = neighbors[i];
      B32 is_direct = (i == BD_CellNeighborKind_Top)   || (i == BD_CellNeighborKind_Right) || (i == BD_CellNeighborKind_Down)  || (i == BD_CellNeighborKind_Left);
      if(n && is_direct && n->flipped)
      {
        B32 same_binary = bd_is_same_binary(cell, n);
        if(!same_binary && cell->qi >= n->qi)
        {
          bd_cell_flip(n, cell->binary, 1, 1);
        }
      }
    }
  }
#endif

  ////////////////////////////////
  // jump one

#if BD_RULE_JUMP_ONE
  for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++) ProfScope("jump one")
  {
    BD_Cell *n = neighbors[i];
    B32 is_direct = (i == BD_CellNeighborKind_Top)   ||
                    (i == BD_CellNeighborKind_Right) ||
                    (i == BD_CellNeighborKind_Down)  ||
                    (i == BD_CellNeighborKind_Left);

    if(n && is_direct && !n->flipped)
    {
      BD_Cell *nn = bd_cell_next(n, i);
      if(nn && nn->flipped && nn->binary == cell->binary)
      {
        bd_cell_flip(n, cell->binary, 0, 0);
      }
    }
  }
#endif

  ////////////////////////////////
  // lucky flip

#if BD_RULE_LUCK_FLIP
  {
    U64 max_luck_flip = 1;
    U64 luck_flip = 0;
    B32 can_luck_flip = cell->binary == BD_CellBinaryKind_Black;

    // check if all neighbors are unflipped
    for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
    {
      BD_Cell *n = neighbors[i];
      if(n && n->flipped && n->binary == BD_CellBinaryKind_White)
      {
        can_luck_flip = 0;
      }
    }

    if(can_luck_flip) for(U64 i = 0; i < BD_CellNeighborKind_COUNT && luck_flip < max_luck_flip; i++)
    {
      BD_Cell *n = neighbors[i];
      if(n && !n->flipped)
      {
        // 10%
        F32 pct = ((F32)rand()/RAND_MAX);
        B32 auto_flip = pct <= 0.1;
        if(auto_flip)
        {
          bd_cell_flip(n, cell->binary, 0, 0);
          luck_flip++;
        }
      }
    }
  }
#endif

  ////////////////////////////////
  // tick tack toe

#if BD_RULE_TICK_TACK_TOE

  // TODO
#endif

  ///////////////////////////////////////////////////////////////////////////////////////
  // triggering cell rule

  switch(cell->kind)
  {
    case BD_CellKind_Tiger:
    {
      for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
      {
        BD_Cell *n = neighbors[i];
        if(n && !n->flipped)
        {
          bd_cell_flip(n, cell->binary, 1,0);
        }
      }
    }break;
    default:{}break;
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // trigger animation

  switch(cell->kind)
  {
    case BD_CellKind_Unity:
    {
      for(U64 i = 0; i < BD_CellNeighborKind_COUNT; i++)
      {
        BD_Cell *n = neighbors[i];
        if(n && bd_is_same_binary(cell, n))
        {
          bd_cell_list_push(bd_state->animation_arena, &bd_state->animation_list, n);
        }
      }
    }break;
    default: {/*noop*/}break;
  }

  scratch_end(scratch);
  ProfEnd();
}

internal void
bd_cell_swap_pos(BD_Cell *a, BD_Cell *b)
{
  Swap(BD_Cell, *a, *b);
  // swap postion info again
  Swap(U64, a->i, b->i);
  Swap(U64, a->j, b->j);
  Swap(U64, a->index, b->index);
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

  // reset deck
  U64 cell_count = bd_state->grid_size.x*bd_state->grid_size.y;
  for(U64 i = 0; i < cell_count; i++)
  {
    BD_Cell *cell = &bd_state->cells[i];
    cell->binary = BD_CellBinaryKind_Invalid;
    cell->base_value_this_round = cell->base_value;
    cell->cached_value = 0;
    cell->flipped = 0;
    cell->flip_t = 0.0;
    cell->animation_t = 0.0;
    cell->animating = 0;
    cell->has_liberties = 0;
    cell->qi = 0;
    cell->captured = 0;
  }

  // shuffle deck
  for(U64 i = 0; i < cell_count; i++)
  {
    BD_Cell *src = &bd_state->cells[i];
    BD_Cell *dst = &bd_state->cells[rand()%bd_state->cell_count];
    bd_cell_swap_pos(src, dst);
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

  B32 is_flipping = bd_state->flip_list.first != 0;
#if 1
  for(BD_CellNode *cn = bd_state->flip_list.first, *next = 0;
      cn != 0;
      cn = next)
  {
    BD_CellNode *cn_next = cn->next;
    BD_Cell *cell = cn->cell;

    if(cell->flip_t == 0.0)
    {
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 12, 1.0);
    }

    cell->flip_t += rk_state->animation.vast_rate * (cell->flipped-cell->flip_t);
    F32 diff = abs_f32(cell->flip_t-cell->flipped);
    if(diff < 0.01) cell->flip_t = 1.0;
    cell->animation_t = cell->flip_t;

    if(cell->flip_t == 1.0)
    {
      bd_cell_list_remove(&bd_state->flip_list, cn);
    }

    break;
  }

  // clear flip arena
  if(bd_state->flip_list.count == 0)
  {
    arena_pop_to(bd_state->flip_arena, bd_state->flip_arena_pos);
  }
#endif

  ///////////////////////////////////////////////////////////////////////////////////////
  // animation

  B32 is_animating = 0;

  ////////////////////////////////
  // process animation list 

  for(BD_CellNode *cn = bd_state->animation_list.first, *next = 0; cn != 0; cn = next)
  {
    next = cn->next;
    B32 animated = 0;
    if(cn->cell->animating == 0)
    {
      // start cell animation if not already
      if(cn->cell->animation_t == 0.0)
      {
        cn->cell->animating = 1;
        sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 1, 1.0);
      }
      // animation for this cell is done
      else
      {
        animated = 1;
      }
    }

    if(!animated)
    {
      break;
    }
    else
    {
      // remove current cell from animation list
      bd_cell_list_remove(&bd_state->animation_list, cn);
      if(next)
      {
        next->cell->animating = 0;
        next->cell->animation_t = 0.0;
      }
    }
  }

  // reset animation arena if it's empty
  if(bd_state->animation_list.count == 0)
  {
    arena_pop_to(bd_state->animation_arena, bd_state->animation_arena_pos);
  }

  ////////////////////////////////
  // animating cell

  for(U64 i = 0; i < bd_state->cell_count; i++)
  {
    BD_Cell *cell = &bd_state->cells[i];
    if(cell->flipped)
    {
      if(cell->animating)
      {
        if(!is_animating) is_animating = 1;
        cell->animation_t += rk_state->animation.vast_rate * (cell->animating-cell->animation_t);
        F32 diff = abs_f32(cell->animation_t-cell->animating);
        if(diff < 0.01)
        {
          cell->animating = 0;
        }
      }
      else
      {
        cell->animation_t += rk_state->animation.slow_rate * (cell->animating-cell->animation_t);
        F32 diff = abs_f32(cell->animation_t-cell->animating);
        if(diff < 0.08)
        {
          cell->animation_t = 0.0;
        }
      }
    }
  }

  ////////////////////////////////
  // animating dice
  // TODO

  ///////////////////////////////////////////////////////////////////////////////////////
  // game ui

  B32 can_play = !bd_state->is_round_end && !bd_state->is_shopping && !is_flipping && !is_animating;

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *overlay_container = 0;
    UI_Rect(rk_state->window_rect)
    {
      overlay_container = ui_build_box_from_stringf(0, "##game_overlay");
    }
    ui_push_parent(overlay_container);

    Vec2F32 window_dim_h = scale_2f32(rk_state->window_dim, 0.5);
    Vec2F32 window_dim_hh = scale_2f32(rk_state->window_dim, 0.25);

    /////////////////////////////////////////////////////////////////////////////////////
    // Failure summary

    F32 summary_open_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "summary_open_t"), bd_state->is_failed, .reset = 0, .rate = rk_state->animation.slow_rate);
    if(bd_state->is_failed && !is_animating && !is_flipping)
    {
      // TODO(k): don't use hard-coded px values
      Vec2F32 dim = {1700, 1400};
      dim = mix_2f32(scale_2f32(dim, 0.0), dim, summary_open_t);
      Vec2F32 dim_h = scale_2f32(dim, 0.5);
      Rng2F32 rect = {0,0,dim.x,dim.y};
      rect = shift_2f32(rect, v2f32(window_dim_h.x-dim_h.x, window_dim_h.y-dim_h.y));
      UI_Box *container;
      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable)
        UI_ChildLayoutAxis(Axis2_Y)
        UI_Transparency(0.1)
        container = ui_build_box_from_stringf(0, "##failed_summary");

      UI_Parent(container)
      UI_PrefWidth(ui_pct(1.0,0.0))
      {
        UI_PrefHeight(ui_pct(1.0,0.0))
          UI_Flags(UI_BoxFlag_DrawText)
          UI_TextAlignment(UI_TextAlign_Center)
          UI_FontSize(ui_top_font_size()*3)
          ui_build_box_from_stringf(0, "You will do better next time");

        if(ui_clicked(ui_buttonf("Restart")))
        {
          bd_round_end(0);
          bd_round_begin();
          sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.2, 6, 1.0);
          bd_state->is_failed = 0;
          bd_state->is_round_end = 0;
        }
      }

      ui_signal_from_box(container);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Shopping ui

    F32 shop_open_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "shop_open_t"), bd_state->is_shopping, .reset = 0, .rate = rk_state->animation.slow_rate);
    if((bd_state->is_shopping || shop_open_t > 0.0) && !is_animating && !is_flipping)
    {
      // TODO(k): don't use hard-coded px values
      Vec2F32 dim = {1700, 1400};
      dim = mix_2f32(scale_2f32(dim, 0.0), dim, shop_open_t);
      Vec2F32 dim_h = scale_2f32(dim, 0.5);
      Rng2F32 rect = {0,0,dim.x,dim.y};
      rect = shift_2f32(rect, v2f32(window_dim_h.x-dim_h.x, window_dim_h.y-dim_h.y));
      UI_Box *container;
      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_Clickable)
        UI_ChildLayoutAxis(Axis2_Y)
        UI_Transparency(mix_1f32(1.0, 0.1, shop_open_t))
        container = ui_build_box_from_stringf(0, "##shopping");

      UI_Parent(container)
      UI_PrefWidth(ui_pct(1.0,0.0))
      {
        UI_PrefHeight(ui_pct(1.0,0.0))
          UI_Flags(UI_BoxFlag_DrawText)
          UI_TextAlignment(UI_TextAlign_Center)
          UI_FontSize(ui_top_font_size()*3)
          ui_build_box_from_stringf(0, "Shop");

        if(bd_state->is_shopping && ui_clicked(ui_buttonf("Next")))
        {
          bd_round_end(1);
          bd_round_begin();
          sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Beep], 0, 0.2, 6, 1.0);
          bd_state->is_shopping = 0;
          bd_state->is_round_end = 0;
        }
      }

      ui_signal_from_box(container);
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // main grid (deck)

    UI_Box *grid_container = 0;
    {
      // TODO(k): don't use hard-coded px values
      Vec2F32 grid_dim = {1300,1300};
      Vec2F32 grid_dim_h = scale_2f32(grid_dim, 0.5);
      Vec2F32 cell_dim = {grid_dim.x/(F32)bd_state->grid_size.x, grid_dim.y/(F32)bd_state->grid_size.y};

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
              if(!cell->flipped || cell->flip_t == 0.0)
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

#if BD_IGNORE_DICE
                B32 can_flip = !cell->flipped;
#else
                B32 can_flip = bd_state->dice_rolled && bd_state->draw_count>0 && can_play;
#endif
                if(((sig.f&UI_SignalFlag_LeftClicked) || (sig.f&UI_SignalFlag_RightClicked)))
                {
                  if(can_flip)
                  {
#if BD_DETERMINED_DICE
                    if(sig.f & UI_SignalFlag_LeftClicked)
                    {
                      bd_cell_flip(cell, BD_CellBinaryKind_Black, 0, 0);
                    }
                    else
                    {
                      bd_cell_flip(cell, BD_CellBinaryKind_White, 0, 0);
                    }
#else
                    BD_CellBinaryKind binary = bd_draw_binary(cell);
                    bd_cell_flip(cell, binary, 0, 0);
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
                // draw score at the top left
                {
                  Vec2F32 size = {cell_dim.x*0.2, cell_dim.y*0.2};
                  size.y = mix_1f32(size.y, size.y*2.0f, cell->animation_t);
                  F32 font_size = ui_top_font_size();
                  font_size = mix_1f32(font_size, font_size*2.0f, cell->animation_t);
                  Vec4F32 background_clr = rk_rgba_from_theme_color(RK_ThemeColor_TextWeak);
                  Vec4F32 text_clr = rk_rgba_from_theme_color(RK_ThemeColor_BaseBackground);
                  background_clr.w = cell->flip_t;
                  text_clr.w = cell->flip_t;
                  UI_Flags(UI_BoxFlag_DrawText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_Floating)
                    UI_PrefHeight(ui_px(size.y,0.0))
                    UI_PrefWidth(ui_text_dim(1.0, 0.0))
                    UI_Palette(ui_build_palette(ui_top_palette(), .background = background_clr, .text = text_clr))
                    UI_FontSize(font_size)
                    ui_build_box_from_stringf(0, "%I64u##cell_value", cell->cached_value);
                }

                UI_Box *b;
                String8 display = bd_name_from_cell_kind[cell->kind];
                RK_Texture2D *tex2d = cell->binary == BD_CellBinaryKind_Black ? rk_tex2d_from_handle(&bd_state->sprites[BD_SpriteKind_Black]) : rk_tex2d_from_handle(&bd_state->sprites[BD_SpriteKind_White]);
                UI_BoxFlags flags = UI_BoxFlag_Clickable|UI_BoxFlag_DrawImage|UI_BoxFlag_DrawOverlay|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow;
                Vec4F32 overlay_clr = rk_rgba_from_theme_color(RK_ThemeColor_TextWeak);
                overlay_clr.w = mix_1f32(0, 0.3, cell->animation_t);
                Vec4F32 text_clr = cell->binary == BD_CellBinaryKind_Black ? rk_rgba_from_theme_color(RK_ThemeColor_TextPositive) : rk_rgba_from_theme_color(RK_ThemeColor_TextNeutral);
                text_clr.w = cell->flip_t;
                // F32 font_size = mix_1f32(cell_dim.y*0.5, cell_dim.y*0.9, cell->animation_t);
                if(cell->captured) flags |= UI_BoxFlag_DrawOverlay;

                UI_HeightFill
                  UI_WidthFill
                  UI_Column
                  UI_Padding(ui_em(0.5,1.0))
                  UI_Row
                  UI_Padding(ui_em(0.5,1.0))
                  UI_Flags(flags)
                  // UI_Font(ui_icon_font())
                  // UI_FontSize(ui_top_font_size()*1.1)
                  UI_TextAlignment(UI_TextAlign_Center)
                  UI_Palette(ui_build_palette(ui_top_palette(), .overlay = overlay_clr, .text = text_clr))
                  b = ui_build_box_from_stringf(0, "%S##%I64u", display, cell->index);
                b->albedo_tex = tex2d->tex;
                b->albedo_clr = v4f32(0.9, 0.9, 0.9, 1.0*cell->flip_t);
                b->src = r2f32p(0,0, tex2d->size.x, tex2d->size.y);
                UI_Signal sig = ui_signal_from_box(b);

                // draw sprite from cell kind
                {
                  RK_Handle tex2d_handle = bd_state->sprites[bd_sprite_kind_from_cell_kind[cell->kind]];
                  // RK_Handle tex2d_handle = bd_state->sprites[BD_SpriteKind_FaceTiger];
                  RK_Texture2D *tex2d = rk_tex2d_from_handle(&tex2d_handle);
                  UI_BoxFlags flags = UI_BoxFlag_Clickable|UI_BoxFlag_DrawImage|UI_BoxFlag_DrawDropShadow;
                  UI_Box *icon_box;
                  Vec4F32 clr = cell->binary == BD_CellBinaryKind_Black ? v4f32(1.0,1.0,1.0,0.1) : v4f32(0,0,0,1);
                  clr.w *= cell->flip_t;
                  F32 padding = mix_1f32(0.5, 0, cell->animation_t);
                  UI_Parent(b)
                    UI_HeightFill
                    UI_WidthFill
                    UI_Column
                    UI_Padding(ui_em(padding,1.0))
                    UI_Row
                    UI_Padding(ui_em(padding,1.0))
                    UI_Flags(flags)
                    UI_Palette(ui_build_palette(ui_top_palette(), .overlay = overlay_clr))
                    icon_box = ui_build_box_from_stringf(0, "icon");
                  icon_box->albedo_tex = tex2d->tex;
                  icon_box->albedo_white_texture_override = 1;
                  icon_box->albedo_clr = clr;
                  icon_box->src = r2f32p(0,0, tex2d->size.x, tex2d->size.y);
                }

                if(ui_hovering(sig))
                {
                  // TODO: draw a selection box

                  UI_Tooltip
                  {
                    UI_Box *container;
                    UI_PrefWidth(ui_em(10, 0))
                      UI_PrefHeight(ui_children_sum(0.0))
                      UI_ChildLayoutAxis(Axis2_Y)
                      container = ui_build_box_from_stringf(0, "container");

                    UI_Parent(container)
                      UI_PrefWidth(ui_pct(1.0,0.0))
                    {
                      ui_labelf("%I64u-%I64u", cell->i, cell->j);
                      UI_Row
                      {
        
                        ui_labelf("kind");
                        ui_spacer(ui_pct(1.0,0.0));
                        ui_label(bd_name_from_cell_kind[cell->kind]);
                      }
                      UI_Row
                      {
        
                        ui_labelf("value");
                        ui_spacer(ui_pct(1.0,0.0));
                        ui_labelf("%I64u", cell->cached_value);
                      }
                      UI_Row
                      {
        
                        ui_labelf("qi");
                        ui_spacer(ui_pct(1.0,0.0));
                        ui_labelf("%I64u", cell->qi);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // Dice

#if !BD_IGNORE_DICE
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
            bd_state->draw_count = bd_dice_roll();
            bd_state->draw_count_src = bd_state->draw_count;
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
#endif

    /////////////////////////////////////////////////////////////////////////////////////
    // compute acc & score

    ////////////////////////////////
    // acc

    U64 territory = 0;
    U64 black_count = 0;
    U64 white_count = 0;
    U64 captures = 0;
    U64 black_value_sum = 0;
    U64 white_value_sum = 0;

    U64 cell_count = bd_state->cell_count;

    ////////////////////////////////
    // compute cell value

    for(U64 i = 0; i < cell_count; i++)
    {
      BD_Cell *cell = &bd_state->cells[i];
      U64 value = bd_value_from_cell(cell);
      cell->cached_value = value;
      if(cell->binary == BD_CellBinaryKind_Black) black_value_sum += value;
      if(cell->binary == BD_CellBinaryKind_White) white_value_sum += value;
    }

    ////////////////////////////////
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
            if(c->binary == BD_CellBinaryKind_Black) is_black_territory = 1;
            if(c->binary == BD_CellBinaryKind_White)
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
    }

    ////////////////////////////////
    // black/white count & captures

    for(U64 cell_index = 0; cell_index < cell_count; cell_index++)
    {
      BD_Cell *cell = &bd_state->cells[cell_index];
      if(!cell->flipped) continue;
      if(cell->binary == BD_CellBinaryKind_Black)
      {
        captures += cell->captured;
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
          }
        }
      }
      else if(cell->binary == BD_CellBinaryKind_White)
      {
        white_count++;
      }
    }

    ////////////////////////////////
    // cell qi

    for(U64 cell_index = 0; cell_index < cell_count; cell_index++)
    {
      BD_Cell *cell = &bd_state->cells[cell_index];
      bd_cell_update_qi(cell);
    }

    // compute score
    // score = territory * qi;
    // score = territory + black_count*2 + captures;
    // score = black_count*2 + captures;
    score = black_value_sum;

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
          ui_labelf("Gold");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", bd_state->gold);
        }

        UI_Row
        {
          ui_labelf("Territory");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", territory);
        }

        UI_Row
        {
          ui_labelf("Captures");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64u", captures);
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

        UI_Row
        {
          ui_labelf("Rolls");
          ui_spacer(ui_pct(1.0,0.0));
          ui_labelf("%I64d", bd_state->roll_count);
        }

        // rk_capped_labelf(submarine->pulse_cd_t, "pulse_cd_t: %.2f", submarine->pulse_cd_t);
      }
    }

    ui_pop_parent();
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // next round or end game

  U64 score_u64 = 0;
  if(score > 0) score_u64 = (U64)score;

#if !BD_NO_NEXT_ROUND
  if(!bd_state->is_round_end)
  {
    if(bd_state->goal <= score_u64)
    {
      bd_state->is_shopping = 1;
      bd_state->is_round_end = 1;
    }
    // game failture
    else if(bd_state->roll_count == 0 && bd_state->draw_count == 0)
    {
      bd_state->is_round_end = 1;
      bd_state->is_failed = 1;
      sy_instrument_play(bd_state->instruments[S5_InstrumentKind_Boop], 0, 0.2, 12, 1.0);
    }
  }
#endif

  scratch_end(scratch);
}

RK_SCENE_SETUP(bd_setup)
{
  BD_State *bd_state = scene->custom_data;
  bd_state->flip_arena = arena_alloc();
  bd_state->flip_arena_pos = arena_pos(bd_state->flip_arena);
  bd_state->animation_arena = arena_alloc();
  bd_state->animation_arena_pos = arena_pos(bd_state->animation_arena);

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
    bd_state->cells[cell_index].base_value = 1;
  }

  // spawn some cell
  {
    U64 cell_index = 0;
    for(U64 i = 0; i < 10 && cell_index < bd_state->cell_count; i++,cell_index++)
    {
      bd_state->cells[cell_index].kind = BD_CellKind_Unity;
    }
    for(U64 i = 0; i < 3 && cell_index < bd_state->cell_count; i++,cell_index++)
    {
      bd_state->cells[cell_index].kind = BD_CellKind_Tiger;
    }
    for(U64 i = 0; i < 3 && cell_index < bd_state->cell_count; i++,cell_index++)
    {
      bd_state->cells[cell_index].kind = BD_CellKind_Wolf;
    }
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

  bd_state->sprites[BD_SpriteKind_White]       = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/white.png"), 0, rk_key_zero());
  bd_state->sprites[BD_SpriteKind_Black]       = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/black.png"), 0, rk_key_zero());
  bd_state->sprites[BD_SpriteKind_FaceDefault] = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/default.png"), 0, rk_key_zero());
  bd_state->sprites[BD_SpriteKind_FaceTiger]   = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/tiger.png"), 0, rk_key_zero());
  bd_state->sprites[BD_SpriteKind_FacePanda]   = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/panda.png"), 0, rk_key_zero());
  bd_state->sprites[BD_SpriteKind_FaceWolf]   = rk_tex2d_from_path(str8_lit("./src/rubik/scenes/blind/wolf.png"), 0, rk_key_zero());

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
      game_camera_node->camera3d->orthographic.top = viewport_world.y0;
      game_camera_node->camera3d->orthographic.bottom = viewport_world.y1;
      game_camera_node->camera3d->orthographic.left= viewport_world.x0;
      game_camera_node->camera3d->orthographic.right = viewport_world.x1;
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

  // TODO(k): call it somewhere else
  bd_setup(ret);

  // pop ctx
  rk_pop_scene();
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  return ret;
}
