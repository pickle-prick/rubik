/////////////////////////////////////////////////////////////////////////////////////////
// Constants

/////////////////////////////////////////////////////////////////////////////////////////
// Type/Enum

typedef enum DE_RegSlot
{
  DE_RegSlot_Storage,
  DE_RegSlot_Deck,
  DE_RegSlot_COUNT,
} DE_RegSlot;

typedef enum DE_InstrumentKind
{
  DE_InstrumentKind_Beep,
  DE_InstrumentKind_Boop,
  DE_InstrumentKind_Gear,
  DE_InstrumentKind_Radiation,
  DE_InstrumentKind_AirPressor,
  DE_InstrumentKind_Ping,
  DE_InstrumentKind_Echo,
  DE_InstrumentKind_COUNT,
} DE_InstrumentKind;

typedef U64 DE_EntityFlags;
#define DE_EntityFlag_Null (DE_EntityFlags)(1ull<<0)

typedef enum DE_OperatorKind
{
  DE_OperatorKind_Add,
  DE_OperatorKind_Minus,
  DE_OperatorKind_Mul,
  DE_OperatorKind_Pow,
  DE_OperatorKind_COUNT,
} DE_OperatorKind;

typedef enum DE_DiceKind
{
  DE_DiceKind_Basic,
  DE_DiceKind_RereollOnOdd,
  DE_DiceKind_RereollOnEven,
  DE_DiceKind_GrowOnOdd,
  DE_DiceKind_GrowOnEven,
  DE_DiceKind_Binary,
  DE_DiceKind_COUNT,
} DE_DiceKind;

internal const String8 de_name_from_operator_kind[DE_OperatorKind_COUNT] =
{
  str8_lit_comp("add"),
  str8_lit_comp("minus"),
  str8_lit_comp("mul"),
  str8_lit_comp("pow"),
};

internal const String8 de_name_from_dice_kind[DE_DiceKind_COUNT] =
{
  str8_lit_comp("Basic"),
  str8_lit_comp("RereollOnOdd"),
  str8_lit_comp("RereollOnEven"),
  str8_lit_comp("GorwOnOdd"),
  str8_lit_comp("GorwOnEven"),
  str8_lit_comp("Binary"),
};

internal const String8 de_desc_from_dice_kind[DE_DiceKind_COUNT] =
{
  str8_lit_comp("Basic"),
  str8_lit_comp("RereollOnOdd"),
  str8_lit_comp("RereollOnEven"),
  str8_lit_comp("GorwOnOdd"),
  str8_lit_comp("GorwOnEven"),
  str8_lit_comp("Binary"),
};

typedef struct DE_Dice DE_Dice;
struct DE_Dice
{
  // for free-list
  DE_Dice *next;

  DE_DiceKind kind;
  Vec2U64 base_range;
  F32 scalar;
  U8 desc[512];
  DE_OperatorKind operator;

  union
  {
    struct
    {
      U64 acc;
      U64 step;
    } odd_grow;
  };
};

typedef struct DE_StorageSlot DE_StorageSlot;
struct DE_StorageSlot
{
  DE_StorageSlot *next;

  DE_Dice *dice;
};

typedef struct DE_StockSlot DE_StockSlot;
struct DE_StockSlot
{
  DE_StockSlot *next;

  U64 token;
  DE_Dice *dice;
};

typedef struct DE_DeckSlot DE_DeckSlot;
struct DE_DeckSlot
{
  DE_DeckSlot *next;

  DE_Dice *dice;
  U64 value;
  B32 can_roll;
  U64 roll_count;
};

typedef struct DE_State DE_State;
struct DE_State
{
  U64 round;
  U64 goal;
  U64 token;

  B32 reset;
  B32 shopping;

  SY_Instrument *instruments[S5_InstrumentKind_COUNT];

  // deck
  DE_DeckSlot deck_slots[6];

  // stock
  DE_StockSlot stock_slots[6];

  // storage
  DE_StorageSlot storage_slots[6];

  DE_Dice *first_free_dice;
};

/////////////////////////////////////////////////////////////////////////////////////////
// helpers

internal DE_Dice *
de_dice_alloc(void)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *s = scene->custom_data;

  DE_Dice *ret = s->first_free_dice;
  if(ret != 0)
  {
    SLLStackPop(s->first_free_dice);
  }
  else
  {
    ret = push_array_no_zero(scene->arena, DE_Dice, 1);
  }
  MemoryZeroStruct(ret);
  return ret;
}

internal void
de_dice_release(DE_Dice *dice)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *s = scene->custom_data;
  SLLStackPush(s->first_free_dice, dice);
}

internal U64
de_value_from_slot(DE_DeckSlot *slot, DE_DeckSlot *left, DE_DeckSlot *right)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *de_state = scene->custom_data;
  U64 ret = 0;

  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, slot->roll_count, 1.0);

  B32 reroll = 0;
  if(slot->dice)
  {
    DE_Dice *dice = slot->dice;
    // NOTE(k): dice is supposed to contain consecutive numbers
    switch(dice->kind)
    {
      case DE_DiceKind_Basic:
      {
        U64 range = dice->base_range.y - dice->base_range.x;
        U64 off = rand()%range;
        ret = dice->base_range.x+off;
        ret *= dice->scalar;
        reroll = 0;
      }break;
      case DE_DiceKind_RereollOnOdd:
      {
        U64 range = dice->base_range.y - dice->base_range.x;
        U64 off = rand()%range;
        ret = dice->base_range.x+off;
        if(ret%2 != 0)
        {
          reroll = 1;
          for EachElement(slot_index, de_state->deck_slots)
          {
            DE_DeckSlot *slot = &de_state->deck_slots[slot_index];
            if(slot->dice && slot->dice->kind == DE_DiceKind_GrowOnOdd)
            {
              slot->dice->scalar += 0.5;
            }
          }
        }
        ret *= dice->scalar + dice->scalar*2*slot->value;
      }break;
      case DE_DiceKind_RereollOnEven:
      {
        U64 range = dice->base_range.y - dice->base_range.x;
        U64 off = rand()%range;
        ret = dice->base_range.x+off;
        if(ret%2 == 0)
        {
          reroll = 1;
          for EachElement(slot_index, de_state->deck_slots)
          {
            DE_DeckSlot *slot = &de_state->deck_slots[slot_index];
            if(slot->dice && slot->dice->kind == DE_DiceKind_GrowOnEven)
            {
              slot->dice->scalar += 0.5;
            }
          }
        }
        ret *= dice->scalar + dice->scalar*2*slot->value;
      }break;
      case DE_DiceKind_GrowOnOdd:
      case DE_DiceKind_GrowOnEven:
      {
        U64 range = dice->base_range.y - dice->base_range.x;
        U64 off = rand()%range;
        ret = dice->base_range.x+off;
        ret *= dice->scalar;
        reroll = 0;
      }break;
      case DE_DiceKind_Binary:
      {
        U64 on = rand()%2;
        if(on && left && left->dice)
        {
          left->can_roll = 1;
          ret = left->value;
        }
        if(!on && right && right->dice)
        {
          right->can_roll = 1;
          ret = right->value;
        }
      }break;
      default:{InvalidPath;}break;
    }
  }
  slot->roll_count++;
  slot->can_roll = reroll;
  ret += slot->value;
  return ret;
}

internal UI_Box *
de_ui_dice_info(DE_Dice *dice)
{
  UI_Box *container;
  UI_ChildLayoutAxis(Axis2_Y)
    container = ui_build_box_from_stringf(0, "##dice_info_container");

  UI_Parent(container)
  {
    // header
    UI_PrefWidth(ui_pct(1.0, 0.0))
      UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow)
      ui_label(de_name_from_dice_kind[dice->kind]);

    // operator
    ui_set_next_pref_width(ui_pct(1.0,0.0));
    UI_Row
    {
      ui_labelf("operator");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%S", de_name_from_operator_kind[dice->operator]);
    }

    // range
    ui_set_next_pref_width(ui_pct(1.0,0.0));
    UI_Row
    {
      ui_labelf("range");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("[%u, %u]", dice->base_range.x, dice->base_range.y);
    }

    // scalar
    ui_set_next_pref_width(ui_pct(1.0,0.0));
    UI_Row
    {
      ui_labelf("scalar");
      ui_spacer(ui_pct(1.0, 0.0));
      ui_labelf("%f", dice->scalar);
    }

    UI_PrefWidth(ui_pct(1.0, 0.0))
      ui_label_multiline(300, str8_cstring((char*)dice->desc));
  }
  return container;
}

internal void de_deck_reset(void)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *de_state = scene->custom_data;

  // reset deck
  for EachElement(slot_index, de_state->deck_slots)
  {
    DE_DeckSlot *slot = &de_state->deck_slots[slot_index];
    if(slot->dice)
    {
      slot->can_roll = 1;
      slot->value = 0;
      slot->roll_count = 0;
    }
  }
}

internal void de_stock_reset(void)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *de_state = scene->custom_data;

  // zero-out old stock dices
  for(U64 i = 0; i < ArrayCount(de_state->stock_slots); i++)
  {
    DE_StockSlot *slot = &de_state->stock_slots[i];
    if(slot->dice)
    {
      de_dice_release(slot->dice);
      slot->dice = 0;
    }
  }

  // create random dices for store
  for(U64 i = 0; i < ArrayCount(de_state->stock_slots); i++)
  {
    DE_StockSlot *slot = &de_state->stock_slots[i];
    DE_Dice *dice = de_dice_alloc();
    DE_DiceKind kind = rand()%DE_DiceKind_COUNT;
    dice->kind = kind;
    dice->base_range = (Vec2U64){1,6};
    // dice->scalar = (U64[6]){1,2,3,4,5,6}[rand()%6];
    dice->scalar = 1;
    slot->dice = dice;
  }
}

internal void de_state_reset(void)
{
  RK_Scene *scene = rk_top_scene();
  DE_State *de_state = scene->custom_data;

  de_state->reset = 0;
  de_state->round = 0;
  de_state->goal = 12;
  de_state->token = 0;
  de_state->shopping = 0;

  // reset deck
  de_deck_reset();

  // reset storage
  for EachElement(slot_index, de_state->storage_slots)
  {
    DE_StorageSlot *slot = &de_state->storage_slots[slot_index];
    if(slot->dice)
    {
      de_dice_release(slot->dice);
      slot->dice = 0;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// update & setup & default

RK_SCENE_UPDATE(de_update)
{
  DE_State *de_state = scene->custom_data;
  RK_NodeBucket *node_bucket = scene->node_bucket;
  RK_Node **nodes = 0;
  for(U64 slot_index = 0; slot_index < node_bucket->hash_table_size; slot_index++)
  {
    for(RK_Node *node = node_bucket->hash_table[slot_index].first;
        node != 0;
        node = node->hash_next)
    {
      darray_push(rk_frame_arena(), nodes, node);
    }
  }

  if(de_state->reset) de_state_reset();

  U64 goal = de_state->goal;
  U64 round = de_state->round;
  U64 token = de_state->token;
  U64 sum = 0;

  ///////////////////////////////////////////////////////////////////////////////////////
  // dice update

  ///////////////////////////////////////////////////////////////////////////////////////
  // game ui

  D_BucketScope(rk_state->bucket_rect)
  {
    UI_Box *container = 0;
    UI_Rect(rk_state->window_rect)
    {
      container = ui_build_box_from_stringf(0, "###game_overlay");
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // compute sum

    for(U64 i = 0; i < ArrayCount(de_state->deck_slots); i++)
    {
      DE_DeckSlot *slot = &de_state->deck_slots[i];
      if(slot->dice)
      {
        sum += slot->value;
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // reset stock items

    B32 reached = sum>=goal;
    // end of this round, compute token & reset stock
    if(reached && !de_state->shopping)
    {
      sy_instrument_play(de_state->instruments[S5_InstrumentKind_AirPressor], 0, 0.1, 0, 1.0);
      ui_anim(ui_key_from_stringf(ui_key_zero(), "shopping_t"), 1.0, .reset = 1, .rate = rk_state->animation.slow_rate);
      de_state->shopping = 1;

      // compute token
      token += 6;

      de_stock_reset();
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // shop & upgrades ui

    if(reached) UI_Parent(container)
    {
      F32 shopping_t = ui_anim(ui_key_from_stringf(ui_key_zero(), "shopping_t"), 1.0, .reset = 0);

      Vec2F32 window_dim_hh = scale_2f32(rk_state->window_dim, 0.25);
      Rng2F32 rect = rk_state->window_rect;
      rect.x0 += window_dim_hh.x;
      rect.x1 -= window_dim_hh.x;
      rect.y0 += window_dim_hh.y;
      rect.y1 -= window_dim_hh.y;
      UI_Box *stock_container;
      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
        UI_ChildLayoutAxis(Axis2_Y)
        UI_Transparency(mix_1f32(0.9, 0.05, shopping_t))
        UI_Squish(0.3f-shopping_t*0.3)
        stock_container = ui_build_box_from_stringf(0, "stock");

      UI_Parent(stock_container)
      {
        // 2x3
        // rows
        for(U64 j = 0; j < 2; j++)
        {
          UI_Box *row;
            UI_ChildLayoutAxis(Axis2_X)
            UI_HeightFill
            UI_WidthFill
          row = ui_build_box_from_stringf(0, "row_%I64u", j);

          // cols
          UI_Parent(row) for(U64 i = 0 ; i < 3; i++)
          {
            U64 slot_index = 3*j+i;
            DE_StockSlot *slot = &de_state->stock_slots[slot_index];

            UI_Box *slot_container;
            UI_WidthFill
              UI_HeightFill
              UI_ChildLayoutAxis(Axis2_Y)
              UI_Flags(UI_BoxFlag_DrawBorder)
            {
              slot_container = ui_build_box_from_stringf(0, "##stock_slot_%I64u", slot_index);
            }

            if(slot->dice)
            {
              UI_Signal sig;
              UI_Parent(slot_container)
                UI_HeightFill
                UI_WidthFill
                UI_Column
                UI_Padding(ui_em(1.5,1.0))
                UI_Row
                UI_Padding(ui_em(1.5,1.0))
                UI_Flags(UI_BoxFlag_DrawDropShadow)
                UI_TextAlignment(UI_TextAlign_Center)
              {
                sig = ui_buttonf("%S##stock_slot_dice_%I64u", de_name_from_dice_kind[slot->dice->kind], slot_index);
              }

              if(ui_hovering(sig)) UI_Tooltip
              {
                ui_set_next_pref_width(ui_px(300, 0));
                ui_set_next_pref_height(ui_px(300, 0));
                de_ui_dice_info(slot->dice);
              }

              if(ui_clicked(sig))
              {
                // find a empty slot to store dice
                U64 empty_slot_index = ArrayCount(de_state->storage_slots);
                for(U64 i = 0; i < ArrayCount(de_state->storage_slots); i++)
                {
                  DE_StorageSlot *slot = &de_state->storage_slots[i];
                  if(!slot->dice)
                  {
                    empty_slot_index = i;
                    break;
                  }
                }

                U64 required_token = 3;
                B32 has_enough_token = token >= required_token;

                if(!has_enough_token)
                {
                  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 3, 1.0);
                  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Boop], 0.11, 0.1, 3, 1.0);
                }
                else if(empty_slot_index < ArrayCount(de_state->storage_slots))
                {
                  de_state->storage_slots[empty_slot_index].dice = slot->dice;
                  slot->dice = 0;
                  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 0, 1.0);
                  token -= required_token;
                }
              }
            }
          }
        }

        // extra
        {
          UI_Box *container;
            UI_PrefHeight(ui_px(300, 0.0))
            UI_PrefWidth(ui_pct(1.0, 0.0))
            UI_Flags(UI_BoxFlag_DrawDropShadow)
            UI_ChildLayoutAxis(Axis2_X)
            container = ui_build_box_from_stringf(0, "buttons_container");
          UI_Parent(container)
          {
            ui_spacer(ui_pct(1.0, 0.0));

            // reset
            UI_PrefHeight(ui_pct(1.0, 0.0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_FontSize(ui_top_font_size()*2.5)
            if(ui_clicked(ui_buttonf("Reroll")))
            {
              sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 8, 1.0);
              de_stock_reset();
            }

            UI_PrefHeight(ui_pct(1.0, 0.0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_FontSize(ui_top_font_size()*2.5)
            if(ui_clicked(ui_buttonf("DONE")))
            {
              round++;
              goal*=1.5;
              de_state->shopping = 0;

              // round is over & stock is closed, reset deck
              de_deck_reset();
            }
          }
        }
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // deck

    B32 can_play_deck = !de_state->shopping;

    UI_Parent(container)
    {
      UI_Box *deck_container;
      Vec2F32 window_dim_hh = scale_2f32(rk_state->window_dim, 0.25);
      Rng2F32 rect = rk_state->window_rect;
      rect.x0 += window_dim_hh.x;
      rect.x1 -= window_dim_hh.x;
      rect.y0 += window_dim_hh.y;
      rect.y1 -= window_dim_hh.y*1.75;
      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_Floating)
        UI_ChildLayoutAxis(Axis2_Y)
        deck_container = ui_build_box_from_stringf(0, "storage");

      UI_Parent(deck_container)
      {
        /////////////////////////////////////////////////////////////////////////////////
        // first row showing slot value

        UI_PrefWidth(ui_pct(1.0,0.0))
        UI_PrefHeight(ui_pct(1.0,0.0))
          ui_row_begin();

        for(U64 i = 0; i < ArrayCount(de_state->deck_slots); i++)
        {
          DE_DeckSlot *slot = &de_state->deck_slots[i];
          UI_Box *slot_container;
          UI_PrefWidth(ui_pct(1.0, 0.0))
            UI_PrefHeight(ui_pct(1.0, 0.0))
            UI_Flags((slot->dice==0?UI_BoxFlag_DrawBorder:0))
            slot_container = ui_build_box_from_stringf(0, "###slot_%I64u", i);

          if(slot->dice)
          {
            DE_Dice *dice = slot->dice;
            UI_Parent(slot_container)
              UI_PrefWidth(ui_pct(1.0, 0.0))
              UI_PrefHeight(ui_pct(1.0, 0.0))
              UI_TextAlignment(UI_TextAlign_Center)
              UI_Flags(UI_BoxFlag_DrawBackground)
              UI_FontSize(200.0)
              ui_labelf("%I64u", slot->value);
          }
        }
        ui_row_end();

        /////////////////////////////////////////////////////////////////////////////////
        // second row showing slot dice

        UI_PrefWidth(ui_pct(1.0,0.0))
        UI_PrefHeight(ui_pct(1.0,0.0))
          ui_row_begin();
        for(U64 i = 0; i < ArrayCount(de_state->deck_slots); i++)
        {
          DE_DeckSlot *slot = &de_state->deck_slots[i];
          U64 left_index = i == 0 ? ArrayCount(de_state->deck_slots)-1 : i-1;
          U64 right_index = (i+1)%ArrayCount(de_state->deck_slots);
          DE_DeckSlot *left_slot = &de_state->deck_slots[left_index];
          DE_DeckSlot *right_slot = &de_state->deck_slots[right_index];
          UI_Box *card_container;
          UI_PrefWidth(ui_pct(1.0, 0.0))
            UI_PrefHeight(ui_pct(1.0, 0.0))
            UI_Flags((slot->dice==0?UI_BoxFlag_DrawBorder:0))
            card_container = ui_build_box_from_stringf(0, "###slot_%I64u", i);

          if(slot->dice)
          {
            DE_Dice *dice = slot->dice;

            if(slot->can_roll)
            {

              UI_Signal sig;
              UI_Parent(card_container)
                // UI_Squish(0.03f - card_hover_t*0.03f)
                UI_PrefWidth(ui_pct(1.0, 0.0))
                UI_PrefHeight(ui_pct(1.0, 0.0))
                UI_TextAlignment(UI_TextAlign_Center)
                UI_Flags(UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DropSite)
              {
                sig = ui_buttonf("%S##deck_slot_dice_%I64u", de_name_from_dice_kind[slot->dice->kind], i);
              }

              UI_Box *b = sig.box;

              if(ui_hovering(sig)) UI_Tooltip
              {
                ui_set_next_pref_width(ui_px(300, 0));
                ui_set_next_pref_height(ui_px(300, 0));
                de_ui_dice_info(slot->dice);
              }

              if(ui_clicked(sig) && can_play_deck)
              {
                slot->value = de_value_from_slot(slot, left_slot, right_slot);
              }

              if(ui_dragging(sig) && !rk_drag_is_active() && length_2f32(ui_drag_delta()) > 10.0)
              {
                rk_drag_begin(DE_RegSlot_Deck, (void*)slot);
              }

              B32 drag_target_is_good = rk_drag_is_active() &&
                                        ui_key_match(ui_state->drop_hot_box_key, b->key) &&
                                        (rk_state->drag_drop_slot == DE_RegSlot_Storage || rk_state->drag_drop_slot == DE_RegSlot_Deck);
              if(drag_target_is_good)
              {
                if(rk_state->drag_drop_state == RK_DragDropState_Dragging)
                {
                  D_Bucket *bucket = d_bucket_make();
                  D_BucketScope(bucket)
                  {
                    Vec4F32 color = v4f32(1,1,1,0.1);
                    Rng2F32 rect = pad_2f32(b->rect, -10.0);
                    R_Rect2DInst *inst = d_rect(rect, color, 0, 0, 1.0);
                    ui_box_equip_draw_bucket(b, bucket);
                  }
                }

                if(rk_drag_drop())
                {
                  if(rk_state->drag_drop_slot == DE_RegSlot_Storage)
                  {
                    DE_StorageSlot *src = rk_state->drag_drop_src;
                    Swap(DE_Dice*, slot->dice, src->dice);
                    sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 1, 1.0);
                  }
                  else if(rk_state->drag_drop_slot == DE_RegSlot_Deck && rk_state->drag_drop_src != slot)
                  {
                    DE_DeckSlot *src = rk_state->drag_drop_src;
                    Swap(DE_Dice*, slot->dice, src->dice);
                    sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 2, 1.0);
                  }
                }
              }
            }
            else
            {
              UI_Parent(card_container)
                UI_PrefWidth(ui_pct(1.0, 0.0))
                UI_PrefHeight(ui_pct(1.0, 0.0))
                UI_TextAlignment(UI_TextAlign_Center)
                UI_Flags(UI_BoxFlag_DrawBackground|UI_BoxFlag_DropSite)
                ui_labelf("%I64u##dice_%I64u", slot->value, i);
            }
          }
        }
        ui_row_end();
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // score panel

    UI_Parent(container)
    {
      UI_PrefHeight(ui_children_sum(0.0))
      UI_Column
      UI_Flags(UI_BoxFlag_DrawBorder)
      UI_PrefHeight(ui_em(3,0.0))
      UI_FontSize(ui_top_font_size()*2.5)
      {
        ui_labelf("round: %I64u", round);
        ui_labelf("goal: %I64u", goal);
        ui_labelf("sum: %I64u", sum);
        ui_labelf("token: %I64u", token);
        if(ui_clicked(ui_buttonf("reset_state")))
        {
          de_state->reset = 1;
        }
        if(ui_clicked(ui_buttonf("clear_storage")))
        {

          for EachElement(slot_index, de_state->storage_slots)
          {
            DE_StorageSlot *slot = &de_state->storage_slots[slot_index];
            if(slot->dice)
            {
              de_dice_release(slot->dice);
              slot->dice = 0;
            }
          }
        }
        // rk_capped_labelf(submarine->pulse_cd_t, "pulse_cd_t: %.2f", submarine->pulse_cd_t);
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // storage

    UI_Parent(container)
    {
      UI_Box *storage_container;
      Vec2F32 window_dim_hh = scale_2f32(rk_state->window_dim, 0.25);
      Rng2F32 rect = rk_state->window_rect;
      rect.x0 += window_dim_hh.x;
      rect.x1 -= window_dim_hh.x;
      rect.y0 += window_dim_hh.y*3.25;
      rect.y1 -= window_dim_hh.y*0.25;
      UI_Rect(rect)
      UI_Flags(UI_BoxFlag_Floating|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground)
      storage_container = ui_build_box_from_stringf(0, "storage");

      UI_Parent(storage_container)
      {
        for(U64 slot_index = 0; slot_index < ArrayCount(de_state->storage_slots); slot_index++)
        {
          DE_StorageSlot *slot = &de_state->storage_slots[slot_index];

          UI_Box *slot_container;
          UI_PrefWidth(ui_pct(1.0,0.0))
            UI_PrefHeight(ui_pct(1.0,0.0))
            UI_TextAlignment(UI_TextAlign_Center)
            UI_Flags(UI_BoxFlag_DrawBorder)
          {
            slot_container = ui_build_box_from_stringf(0, "##storage_slot_%I64u", slot_index);
          }

          if(slot->dice)
          {
            UI_Box *b;
            UI_Parent(slot_container)
              UI_WidthFill
              UI_HeightFill
              UI_Column
              UI_Padding(ui_em(1.0,1.0))
              UI_Row
              UI_Padding(ui_em(1.0,1.0))
              UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DropSite|UI_BoxFlag_DrawHotEffects|UI_BoxFlag_Clickable|UI_BoxFlag_DrawActiveEffects|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_DrawText)
              UI_TextAlignment(UI_TextAlign_Center)
            {
              b = ui_build_box_from_stringf(0, "%S##storage_dice_%I64u", de_name_from_dice_kind[slot->dice->kind], slot_index);
            }
            UI_Signal sig = ui_signal_from_box(b);
            B32 is_active = ui_key_match(ui_active_key(UI_MouseButtonKind_Left), b->key);

            if(ui_dragging(sig) && !rk_drag_is_active() && length_2f32(ui_drag_delta()) > 10.f)
            {
              rk_drag_begin(DE_RegSlot_Storage, (void*)slot);
            }

            if(!ui_dragging(sig) && ui_hovering(sig)) UI_Tooltip
            {
              ui_set_next_pref_width(ui_px(300, 0));
              ui_set_next_pref_height(ui_px(300, 0));
              de_ui_dice_info(slot->dice);
            }

            if(rk_drag_is_active() && is_active)
            {
              UI_Tooltip
              {
                ui_set_next_pref_width(ui_px(b->fixed_size.x, 0.0));
                ui_set_next_pref_height(ui_px(b->fixed_size.y, 0.0));
                de_ui_dice_info(slot->dice);
              }

              {
                D_Bucket *bucket = d_bucket_make();
                D_BucketScope(bucket)
                {
                  Vec4F32 color = v4f32(1,1,0,0.3);
                  color.w *= b->active_t;
                  Rng2F32 rect = pad_2f32(b->rect, -10.0);
                  R_Rect2DInst *inst = d_rect(rect, color, 0, 0, 1.0);
                  ui_box_equip_draw_bucket(b, bucket);
                }
              }
            }

            B32 drag_target_is_good = rk_drag_is_active() &&
                                      ui_key_match(ui_state->drop_hot_box_key, b->key) &&
                                      (rk_state->drag_drop_slot == DE_RegSlot_Deck || rk_state->drag_drop_slot == DE_RegSlot_Storage);
            if(drag_target_is_good)
            {
              if(rk_state->drag_drop_state == RK_DragDropState_Dragging)
              {
                D_Bucket *bucket = d_bucket_make();
                D_BucketScope(bucket)
                {
                  Vec4F32 color = v4f32(1,1,1,0.1);
                  Rng2F32 rect = pad_2f32(b->rect, -10.0);
                  R_Rect2DInst *inst = d_rect(rect, color, 0, 0, 1.0);
                  ui_box_equip_draw_bucket(b, bucket);
                }
              }

              if(rk_drag_drop())
              {
                if(rk_state->drag_drop_slot == DE_RegSlot_Deck)
                {
                  DE_DeckSlot *src = rk_state->drag_drop_src;
                  Swap(DE_Dice*, slot->dice, src->dice);
                  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 1, 1.0);
                }
                else if(rk_state->drag_drop_slot == DE_RegSlot_Storage && rk_state->drag_drop_src != slot)
                {
                  DE_DeckSlot *src = rk_state->drag_drop_src;
                  Swap(DE_Dice*, slot->dice, src->dice);
                  sy_instrument_play(de_state->instruments[S5_InstrumentKind_Beep], 0, 0.1, 2, 1.0);
                }
              }
            }
          }
        }
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // trash

    UI_Parent(container)
    {
      Rng2F32 rect = rk_state->window_rect;
      rect.x0 = rk_state->window_dim.x-500;

      UI_Box *b;
      UI_Rect(rect)
        UI_Flags(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawOverlay|UI_BoxFlag_DropSite)
      {
        b = ui_build_box_from_stringf(0, "trash");
        ui_signal_from_box(b);
      }

      B32 drag_target_is_good = rk_drag_is_active() &&
                                ui_key_match(ui_state->drop_hot_box_key, b->key) &&
                                (rk_state->drag_drop_slot == DE_RegSlot_Storage || rk_state->drag_drop_slot == DE_RegSlot_Deck);
      if(drag_target_is_good)
      {
        if(rk_state->drag_drop_state == RK_DragDropState_Dragging)
        {
          D_Bucket *bucket = d_bucket_make();
          D_BucketScope(bucket)
          {
            Vec4F32 color = v4f32(1,1,1,0.1);
            Rng2F32 rect = pad_2f32(b->rect, -10.0);
            R_Rect2DInst *inst = d_rect(rect, color, 0, 0, 1.0);
            ui_box_equip_draw_bucket(b, bucket);
          }
        }

        // drop dice
        if(rk_drag_drop())
        {
          if(rk_state->drag_drop_slot == DE_RegSlot_Storage)
          {
            DE_StorageSlot *src = rk_state->drag_drop_src;
            de_dice_release(src->dice);
            src->dice = 0;
            sy_instrument_play(de_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 0, 1.0);
            token += 1;
          }
          else if(rk_state->drag_drop_slot == DE_RegSlot_Deck)
          {
            DE_DeckSlot *src = rk_state->drag_drop_src;
            de_dice_release(src->dice);
            src->dice = 0;
            sy_instrument_play(de_state->instruments[S5_InstrumentKind_Boop], 0, 0.1, 0, 1.0);
            token += 1;
          }
        }
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // update deck info 

  de_state->round = round;
  de_state->goal = goal;
  de_state->token = token;
}

RK_SCENE_SETUP(de_setup)
{
  DE_State *de_state = scene->custom_data;
  for(U64 i = 0; i < DE_InstrumentKind_COUNT; i++)
  {
    SY_Instrument **dst = &de_state->instruments[i];
    SY_Instrument *src = 0;
    switch(i)
    {
      case DE_InstrumentKind_Beep:
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
      case DE_InstrumentKind_Boop:
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
      case DE_InstrumentKind_Gear:
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
      case DE_InstrumentKind_Radiation:
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
      case DE_InstrumentKind_AirPressor:
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
      case DE_InstrumentKind_Ping:
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
      case DE_InstrumentKind_Echo:
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
}

RK_SCENE_DEFAULT(de_default)
{
  RK_Scene *ret = rk_scene_alloc();
  ret->name = str8_lit("dice");
  ret->save_path = str8_lit("./src/rubik/scenes/dice/default.tscn");
  ret->setup_fn_name = str8_lit("dice_setup");
  ret->default_fn_name = str8_lit("dice_default");
  ret->update_fn_name = str8_lit("dice_update");

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
  DE_State *de_state = rk_scene_push_custom_data(ret, DE_State);
  de_state->round = 1;
  de_state->goal = 12;
  de_state->token = 0;
  for(U64 slot_index = 0; slot_index < ArrayCount(de_state->deck_slots); slot_index++)
  {
    DE_DeckSlot *slot = &de_state->deck_slots[slot_index];

    DE_Dice *dice = de_dice_alloc();
    {
      // TODO: may overflow char buffer
      strcpy((char*)dice->desc, "just a basic dice");
      dice->kind = DE_DiceKind_Basic;
      dice->base_range = (Vec2U64){1,6};
      dice->scalar = 1;
    }

    slot->dice = dice;
    slot->can_roll = 1;
  }

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
    // TODO: maybe we don't geo2d pass at all, just use ui pass?
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
  ret->setup_fn = de_setup;
  ret->update_fn = de_update;
  ret->default_fn = de_default;
  ret->root = rk_handle_from_node(root);

  // pop ctx
  rk_push_scene(ret);
  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  // TODO(k): call it somewhere else
  de_setup(ret);

  return ret;
}
