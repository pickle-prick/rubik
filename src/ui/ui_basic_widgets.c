internal UI_Signal
ui_button(String8 string) {
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_text_alignment(UI_TextAlign_Center);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable |
                                           UI_BoxFlag_DrawBackground |
                                           UI_BoxFlag_DrawBorder |
                                           UI_BoxFlag_DrawText |
                                           UI_BoxFlag_DrawHotEffects |
                                           UI_BoxFlag_DrawActiveEffects,
                                           string);
    UI_Signal interact = ui_signal_from_box(box);
    return interact;
}

internal UI_Signal
ui_label(String8 string)
{
    ui_set_next_text_padding(3);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_DrawText, str8_zero());
    ui_box_equip_display_string(box, string);
    UI_Signal interact = ui_signal_from_box(box);
    return interact;
}

internal UI_Signal
ui_labelf(char *fmt, ...)
{
    Temp scratch = scratch_begin(0, 0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);
    UI_Signal result = ui_label(string);
    scratch_end(scratch);
    return result;
}

internal UI_Signal
ui_spacer(UI_Size size) {
    UI_Box *parent = ui_top_parent();
    ui_set_next_pref_size(parent->child_layout_axis, size);
    UI_Box *box = ui_build_box_from_key(0, ui_key_zero());
    UI_Signal interact = ui_signal_from_box(box);
    return interact;
}

////////////////////////////////
//~ rjf: Floating Panes
internal UI_Box *
ui_pane_begin(Rng2F32 rect, String8 string) {
    ui_push_rect(rect);
    ui_set_next_child_layout_axis(Axis2_Y);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable | UI_BoxFlag_DrawBorder| UI_BoxFlag_DrawBackground, string);
    ui_pop_rect();
    ui_push_parent(box);
    ui_push_pref_width(ui_pct(1.0, 0));
    return box;
}

internal UI_Signal
ui_pane_end(void) {
    ui_pop_pref_width();
    UI_Box *box = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}

////////////////////////////////
//~ rjf: Simple Layout Widgets

internal UI_Box *ui_row_begin(void) { return ui_named_row_begin(str8_lit("")); }
internal UI_Signal ui_row_end(void) { return ui_named_row_end(); }
internal UI_Box *ui_column_begin(void) { return ui_named_column_begin(str8_lit("")); }
internal UI_Signal ui_column_end(void) { return ui_named_column_end(); }

internal UI_Box *
ui_named_row_begin(String8 string) {
    ui_set_next_child_layout_axis(Axis2_X);
    // TODO: will this cause memory leak?
    UI_Box *box = ui_build_box_from_string(0, string);
    ui_push_parent(box);
    return box;
}

internal UI_Signal
ui_named_row_end(void) {
    UI_Box *box = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}

internal UI_Box *
ui_named_column_begin(String8 string) {
    ui_set_next_child_layout_axis(Axis2_Y);
    UI_Box *box = ui_build_box_from_string(0, string);
    ui_push_parent(box);
    return box;
}

internal UI_Signal
ui_named_column_end(void) {
    UI_Box *box = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}

typedef struct UI_LineEditDrawData UI_LineEditDrawData;
struct UI_LineEditDrawData
{
  String8 edited_string;
  TxtPt   cursor;
  TxtPt   mark;
  Rng2F32 parent_rect;
};

internal UI_BOX_CUSTOM_DRAW(ui_line_edit_draw) {
    UI_LineEditDrawData *draw_data = (UI_LineEditDrawData *)user_data;

    F_Tag font    = box->font;
    F32 font_size = box->font_size;
    F32 tab_size  = box->tab_size;

    Vec2F32 text_position = ui_box_text_position(box);
    String8 edited_string = draw_data->edited_string;
    TxtPt cursor = draw_data->cursor;
    TxtPt mark = draw_data->mark;
    F32 cursor_pixel_off = f_dim_from_tag_size_string(font, font_size, 0, tab_size, str8_prefix(edited_string, cursor.column-1)).x;
    F32 mark_pixel_off = f_dim_from_tag_size_string(font, font_size, 0, tab_size, str8_prefix(edited_string, mark.column-1)).x;
    F32 cursor_thickness = ClampBot(4.f, font_size/6.0f);

    Rng2F32 cursor_rect = {
        text_position.x + cursor_pixel_off - cursor_thickness*0.5f,
        box->rect.y0+2.f,
        text_position.x + cursor_pixel_off + cursor_thickness*0.50f,
        box->rect.y1-2.f,
    };

    Rng2F32 mark_rect = {
        text_position.x + mark_pixel_off - cursor_thickness*0.5f,
        box->rect.y0+4.f,
        text_position.x + mark_pixel_off + cursor_thickness*0.50f,
        box->rect.y1-4.f,
    };
    Rng2F32 select_rect = union_2f32(cursor_rect, mark_rect);

    Vec4F32 bg_color = v4f32(0.8, 0.8, 0.8, 0.3);
    bg_color.w *= box->focus_active_t;
    Vec4F32 select_color = v4f32(0, 0, 1, 0.7);
    select_color.w *= box->focus_active_t;
    Vec4F32 cursor_color = v4f32(0, 0, 0, 0.9);
    cursor_color.w *= box->focus_active_t;
    d_rect(draw_data->parent_rect, bg_color, 0,0,0);
    d_rect(select_rect, select_color, font_size/6.f, 0, 1.f);
    d_rect(cursor_rect, cursor_color, 0, 0, 1.f);
}

internal UI_Signal
ui_line_edit(TxtPt *cursor, TxtPt *mark,
             U8 *edit_buffer, U64 edit_buffer_size, U64 *edit_string_size_out,
             String8 pre_edit_value, String8 string)
{
    // Make key
    UI_Key key = ui_key_from_string(ui_active_seed_key(), string);

    // Calculate focus
    B32 is_auto_focus_hot    = ui_is_key_auto_focus_hot(key);
    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
    ui_push_focus_hot(is_auto_focus_hot ? UI_FocusKind_On : UI_FocusKind_Null);
    ui_push_focus_active(is_auto_focus_active ? UI_FocusKind_On : UI_FocusKind_Null);

    B32 is_focus_hot    = ui_is_focus_hot();
    B32 is_focus_active = ui_is_focus_active();

    ui_set_next_hover_cursor(is_focus_active ? OS_Cursor_IBar : OS_Cursor_HandPoint);
    // Build top-level box
    UI_Box *box = ui_build_box_from_key(UI_BoxFlag_DrawBackground|
                                        UI_BoxFlag_DrawBorder|
                                        UI_BoxFlag_MouseClickable|
                                        UI_BoxFlag_ClickToFocus|
                                        UI_BoxFlag_KeyboardClickable|
                                        UI_BoxFlag_DrawHotEffects,
                                        key);
    // Take navigation actions for editing
    if(is_focus_active)
    {
        Temp scratch = scratch_begin(0,0);
        UI_EventList *events = ui_events();
        for(UI_EventNode *n = events->first; n!=0; n = n->next)
        {
            String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);

            // Don't consume anything that doesn't fit a single-line's operations
            if((n->v.kind != UI_EventKind_Edit && n->v.kind != UI_EventKind_Navigate && n->v.kind != UI_EventKind_Text) || n->v.delta_2s32.y != 0) { continue; }

            // Map this action to an TxtOp
            UI_TxtOp op = ui_single_line_txt_op_from_event(scratch.arena, &n->v, edit_string, *cursor, *mark);

            // Perform replace range
            if(!txt_pt_match(op.range.min, op.range.max) || op.replace.size != 0)
            {
                String8 new_string = ui_push_string_replace_range(scratch.arena, edit_string,
                                                                  r1s64(op.range.min.column, op.range.max.column),
                                                                  op.replace);
                new_string.size = Min(edit_buffer_size, new_string.size);
                MemoryCopy(edit_buffer, new_string.str, new_string.size);
                edit_string_size_out[0] = new_string.size;
            }

            // Commit op's changed cursor & mark to caller-provided state
            *cursor = op.cursor;
            *mark = op.mark;

            // Consume event
            ui_eat_event(events, n);
        }
        scratch_end(scratch);
    }

    // Build contents
    TxtPt mouse_pt = {0};
    UI_Parent(box)
    {
        String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
        if(!is_focus_active && box->focus_active_t < 0.001)
        {
            String8 display_string = ui_display_part_from_key_string(string);
            if(pre_edit_value.size != 0) 
            {
                display_string = pre_edit_value;
            }
            ui_label(display_string);
        }
        else
        {
            // TODO
            F32 total_text_width = f_dim_from_tag_size_string(box->font, box->font_size, 0,
                                                              box->tab_size, edit_string).x;
            ui_set_next_pref_width(ui_px(total_text_width, 1.f));
            UI_Box *editstr_box = ui_build_box_from_stringf(UI_BoxFlag_DrawText|UI_BoxFlag_DisableTextTrunc,
                                                            "###editstr");
            UI_LineEditDrawData *draw_data = push_array(ui_build_arena(), UI_LineEditDrawData, 1);
            {
                draw_data->edited_string = push_str8_copy(ui_build_arena(), edit_string);
                draw_data->cursor        = *cursor;
                draw_data->mark          = *mark;
                draw_data->parent_rect   = box->rect;
            }
            ui_box_equip_display_string(editstr_box, edit_string);
            ui_box_equip_custom_draw(editstr_box, ui_line_edit_draw, draw_data);
            mouse_pt = txt_pt(1, 1+ui_box_char_pos_from_xy(editstr_box, ui_mouse()));
        }
    }

    // Interact
    UI_Signal sig = ui_signal_from_box(box);

    if(!is_focus_active && sig.f&(UI_SignalFlag_DoubleClicked|UI_SignalFlag_KeyboardPressed))
    {
        String8 edit_string = pre_edit_value;
        edit_string.size = Min(edit_buffer_size, pre_edit_value.size);
        MemoryCopy(edit_buffer, edit_string.str, edit_string.size);
        edit_string_size_out[0] = edit_string.size;

        ui_set_auto_focus_active_key(key);
        ui_kill_action();

        // Select all text after actived
        *cursor = txt_pt(1, edit_string.size+1);
        *mark = txt_pt(1, 1);
    }

    if(is_focus_active && sig.f&UI_SignalFlag_KeyboardPressed) 
    {
        ui_set_auto_focus_active_key(ui_key_zero());
        sig.f |= UI_SignalFlag_Commit;
    }

    if(is_focus_active && ui_dragging(sig)) 
    {
        // Update mouse ptr
        if(ui_pressed(sig)) {
            *mark = mouse_pt;
        }
        *cursor = mouse_pt;
    }

    // TODO: fix it later, dragging will override the cursor position
    if(is_focus_active && sig.f&UI_SignalFlag_DoubleClicked) 
    {
        String8 edit_string = str8(edit_buffer, edit_string_size_out[0]);
        *cursor = txt_pt(1, edit_string.size+1);
        *mark = txt_pt(1, 1);
        ui_kill_action();
    }

    // Focus cursor
    {
        // TODO
    }

    // Pop focus
    ui_pop_focus_hot();
    ui_pop_focus_active();

    return sig;
}

typedef struct UI_ImageDrawData UI_ImageDrawData;
struct UI_ImageDrawData {
    R_Handle          texture;
    R_Tex2DSampleKind sample_kind;
    Rng2F32           region;
    Vec4F32           tint;
    F32               blur;
};

internal UI_BOX_CUSTOM_DRAW(ui_image_draw)
{
    UI_ImageDrawData *draw_data = (UI_ImageDrawData *)user_data;
    if(r_handle_match(draw_data->texture, r_handle_zero())) {
        R_Rect2DInst *inst = d_rect(box->rect, v4f32(0,0,0,0), 0,0, 1.f);
        MemoryCopyArray(inst->corner_radii, box->corner_radii);
    } else D_Tex2DSampleKindScope(draw_data->sample_kind) {
        R_Rect2DInst *inst = d_img(box->rect, draw_data->region, draw_data->texture, draw_data->tint, 0,0,0);
        MemoryCopyArray(inst->corner_radii, box->corner_radii);
    }

    if(draw_data->blur > 0.01) {
        // TODO: handle blur pass
    }
}

internal UI_Signal
ui_image(R_Handle texture, R_Tex2DSampleKind sample_kind, Rng2F32 region, Vec4F32 tint, F32 blur, String8 string) {
    UI_Box *box = ui_build_box_from_string(0, string);
    UI_ImageDrawData *draw_data = push_array(ui_build_arena(), UI_ImageDrawData, 1);
    draw_data->texture     = texture;
    draw_data->sample_kind = sample_kind;
    draw_data->region      = region;
    draw_data->tint        = tint;
    draw_data->blur        = blur;
    ui_box_equip_custom_draw(box, ui_image_draw, draw_data);
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}
