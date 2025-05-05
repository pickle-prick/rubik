typedef enum StoneKind
{
  StoneKind_White,
  StoneKind_Black,
  StoneKind_COUNT,
} StoneKind;

typedef struct Stone Stone;
struct Slot;
struct Stone
{
  B32 is_revealed;
  StoneKind kind;
  struct Slot *slot;

  // update artifacts
  U64 update_idx;
  B32 has_liberties;
};

typedef struct Slot Slot;
struct Slot
{
  Stone *stone;
  U64 row;
  U64 col;
};

typedef struct Table Table;
struct Table
{
  Vec2U64 size;
  Slot *slots;
  Stone *stones;

  U64 update_idx;

  U64 hand_count;

  Rng2F32 pane_rect;
  B32 pane_show;
};

internal Table *
table_alloc(Arena *arena, Vec2U64 size)
{
  Table *ret = push_array(arena, Table ,1);
  ret->size = size;
  U64 slot_count = size.x * size.y;
  ret->slots = push_array(arena, Slot, slot_count);
  ret->stones = push_array(arena, Stone, slot_count);

  ret->pane_rect = rk_state->window_rect;
  {
    ret->pane_rect.x0 += 900;
    ret->pane_rect.x1 -= 900;
    ret->pane_rect.y0 += 600;
    ret->pane_rect.y1 -= 600;
    ret->pane_show = 1;
  }

  return ret;
}

internal void
table_init(Table *table)
{
  table->update_idx = 0;
  table->hand_count = 0;

  U64 slot_count = table->size.x * table->size.y; 

  for(U64 i = 0; i < slot_count; i++)
  {
    Stone *stone = &table->stones[i];
    stone->kind = (rand() % 2) == 1;
    stone->is_revealed = 0;
    stone->slot = &table->slots[i];
    stone->update_idx = 0;
    stone->has_liberties = 0;

    table->slots[i].stone = stone;
    table->slots[i].row = i / table->size.x;
    table->slots[i].col = i % table->size.x;
  }
}

internal void 
update_stone(Stone *stone, Table *table)
{
  if(stone->is_revealed && table->update_idx >= stone->update_idx)
  {
    // NOTE(k): avoid circular update
    stone->update_idx = table->update_idx+1;
    stone->has_liberties = 0;

    U64 row = stone->slot->row;
    U64 col = stone->slot->col;

    U64 col_count = table->size.x;
    U64 row_count = table->size.y;

    B32 has_liberties = 0;

    Slot *neighbors[4];
    U64 neighbor_count = 0;
    // top
    if(row > 0)
    {
      neighbors[neighbor_count++] = &table->slots[col_count*(row-1) + col];
    }
    // bottom
    if(row < row_count-1)
    {
      neighbors[neighbor_count++] = &table->slots[col_count*(row+1) + col];
    }
    // left
    if(col > 0)
    {
      neighbors[neighbor_count++] = &table->slots[col_count*row + col - 1];
    }
    // right
    if(col < col_count-1)
    {
      neighbors[neighbor_count++] = &table->slots[col_count*row + col + 1];
    }

    Stone *routers[4];
    U64 router_count = 0;
    for(U64 i = 0; i < neighbor_count; i++)
    {
      Slot *slot = neighbors[i];
      if(slot->stone)
      {
        if(slot->stone->is_revealed)
        {
          if(slot->stone->kind == stone->kind)
          {
            routers[router_count++] = slot->stone;
          }
        }
        else
        {
          has_liberties = 1;
          break;
        }
      }
      else
      {
        // no stone
        has_liberties = 1;
        break;
      }
    }

    if(!has_liberties && router_count > 0)
    {
      for(U64 i = 0; i < router_count; i++)
      {
        Stone *router = routers[i];
        update_stone(router, table);
        if(router->has_liberties)
        {
          has_liberties = router->has_liberties;
          break;
        }
      }
    }


    if(!has_liberties)
    {
      // stone->slot->stone = 0;
      // stone->slot = 0;
      stone->kind = !stone->kind;
      has_liberties = 1;
    }

    stone->has_liberties = has_liberties;
  }
}

RK_NODE_CUSTOM_UPDATE(table_update)
{
  Table *table = node->custom_data;
  U64 slot_count = table->size.x * table->size.y;

  B32 updated = 0;
  B32 second_pass = 0;

  // TODO: just use ui layer to draw for now, we don't have 2d font node yet
  {
    UI_Box *pane = rk_ui_pane_begin(&table->pane_rect, &table->pane_show, str8_lit("FAKE 2D"));
    ui_spacer(ui_em(0.215, 0.f));

    U64 w = table->size.x;
    U64 h = table->size.y;

    // font
    ui_push_font_size(ui_top_font_size()*2.3);
    ui_push_pref_height(ui_em(1.35f, 1.f));

    UI_Font(ui_icon_font()) for(U64 j = 0; j < h; j++)
    {
      UI_PrefWidth(ui_pct(1.f, 0.f))UI_Row for(U64 i = 0; i < w; i++)
      {
        Slot *slot = &table->slots[j*w + i];
        Stone *stone = slot->stone;
        if(stone)
        {
          ui_set_next_text_alignment(UI_TextAlign_Center);
          String8 display = !stone->is_revealed ? str8_lit("") : stone->kind == StoneKind_Black ? str8_lit("x") : str8_lit("O");
          if(table->hand_count > 0)
          {
            UI_Signal sig = ui_buttonf("%S###%d_%d", display, j, i);
            if(ui_clicked(sig))
            {
              stone->is_revealed = !stone->is_revealed;
              stone->update_idx++;
              updated = 1;
            }
            if(stone->is_revealed && sig.f & UI_SignalFlag_RightClicked)
            {
              stone->kind = !stone->kind;
              stone->update_idx++;
              updated = 1;
            }
          }
          else
          {
            ui_set_next_flags(UI_BoxFlag_DrawBorder);
            ui_labelf("%S###%d_%d", display, j, i);
          }
        }
        else
        {
          UI_Palette(ui_state->widget_palette_info.scrollbar_palette)
          {
            ui_buttonf("");
          }
        }
      }
    }

    ui_spacer(ui_em(0.315, 0.f));

    UI_Row
    {
      ui_labelf("HANDS: %I64d", table->hand_count);
      if(table->hand_count == 0)
      {
        if(ui_clicked(ui_buttonf("GO")))
        {
          U64 hands = rand() % 30 + 1;
          table->hand_count += hands;
        }
      }
    }

    // count scores
    U64 counts[StoneKind_COUNT] = {0};
    for(U64 slot_idx = 0; slot_idx < slot_count; slot_idx++)
    {
      Slot *slot = &table->slots[slot_idx];
      if(slot->stone && slot->stone->is_revealed)
      {
        counts[slot->stone->kind]++;
      }
    }

    S64 score = (S64)counts[StoneKind_White] - (S64)counts[StoneKind_Black];
    UI_Row
    {
      ui_labelf("W: %I64d", counts[StoneKind_White]);
      ui_labelf("B: %I64d", counts[StoneKind_Black]);
      ui_labelf("T: %I64d", score);
    }

    UI_Row
    {
      if(ui_clicked(ui_buttonf("RESET")))
      {
        table_init(table);
      }
    }

    ui_pop_font_size();
    ui_pop_pref_width();
    ui_pop_pref_height();

    rk_ui_pane_end();
  }

  if(updated)
  {
    table->hand_count--;
  }

  if(updated)
  {
    // check if any stone is captured
    // NOTE(k): we need two passes, ignored the trigger stone in first pass
    for(U64 i = 0; i < 2; i++)
    {
      for(U64 slot_idx = 0; slot_idx < slot_count; slot_idx++)
      {
        Slot *slot = &table->slots[slot_idx];
        if(slot->stone)
        {
          update_stone(slot->stone, table);
        }
      }
      table->update_idx++;
    }
  }
}

internal RK_Scene *
rk_scene_entry__2()
{
  RK_Scene *ret = rk_scene_alloc(str8_lit("prob1"), str8_lit("./src/rubik/scenes/2/default.rscn"));
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);

  RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
  RK_Parent_Scope(root)
  {
    // create the editor camera
    RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "main_camera");
    {
      main_camera->camera3d->projection = RK_ProjectionKind_Perspective;
      main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
      main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
      main_camera->camera3d->hide_cursor = 0;
      main_camera->camera3d->lock_cursor = 0;
      main_camera->camera3d->is_active = 1;
      main_camera->camera3d->zn = 0.1;
      main_camera->camera3d->zf = 200.f;
      main_camera->camera3d->perspective.fov = 0.25f;
      rk_node_push_fn(main_camera, editor_camera_fn);
      main_camera->node3d->transform.position = v3f32(0,-3,0);
    }
    ret->active_camera = rk_handle_from_node(main_camera);

    // table
    RK_Node *table = rk_build_node3d_from_stringf(0,0,"table");
    rk_node_push_fn(table, table_update);
    table->node3d->transform.position.y = -10;

    Arena *arena = ret->node_bucket->arena_ref;
    Table *table_data = table_alloc(arena, (Vec2U64){9,9});
    table_init(table_data);
    table->custom_data = table_data;
  }

  ret->root = rk_handle_from_node(root);
  rk_pop_node_bucket();
  rk_pop_res_bucket();
  return ret;
}
