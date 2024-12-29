////////////////////////////////
//~ k: Basic widget

internal UI_Signal rk_icon_button(RK_IconKind kind, String8 string)
{
    String8 display_string = ui_display_part_from_key_string(string);
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clickable|
                                           UI_BoxFlag_DrawBorder|
                                           UI_BoxFlag_DrawBackground|
                                           UI_BoxFlag_DrawHotEffects|
                                           UI_BoxFlag_DrawActiveEffects,
                                           string);
    UI_Parent(box)
    {
        if(display_string.size == 0)
        {
            ui_spacer(ui_pct(1,0));
        }
        else
        {
            ui_spacer(ui_em(1.f, 1.f));
        }

        UI_TextAlignment(UI_TextAlign_Center)
            RK_Font(RK_FontSlot_Icons)
            UI_PrefWidth(ui_em(2.f, 1.f))
            UI_PrefHeight(ui_pct(1, 0))
            UI_FlagsAdd(UI_BoxFlag_DisableTextTrunc|UI_BoxFlag_DrawTextWeak)
            ui_label(rk_icon_kind_text_table[kind]);

        if(display_string.size != 0)
        {
            UI_PrefWidth(ui_pct(1.f, 0.f))
            {
                UI_Box *box = ui_label(display_string).box;
            }
        }
        if(display_string.size == 0)
        {
            ui_spacer(ui_pct(1, 0));
        }
        else
        {
            ui_spacer(ui_em(1.f, 1.f));
        }
    }

    UI_Signal result = ui_signal_from_box(box);
    return result;
}

internal UI_Signal rk_icon_buttonf(RK_IconKind kind, char *fmt, ...)
{
    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, fmt, args);
    va_end(args);
    UI_Signal sig = rk_icon_button(kind, string);
    scratch_end(scratch);
    return sig;
}

////////////////////////////////
//~ k: Floating/Fixed Panes

internal UI_Box *
rk_ui_pane_begin(Rng2F32 *rect, B32 *open, String8 string)
{
    //- k: build bouding box for resizing
    {
        F32 boundary_thickness = ui_top_font_size()*0.36;
        F32 half_boundary_thickness = boundary_thickness/2.f;

        //~ corner squares [4]

        //- top left boundary square
        Rng2F32 topleft_boundary_rect = {0};
        topleft_boundary_rect.x0 = rect->x0 - half_boundary_thickness;
        topleft_boundary_rect.y0 = rect->y0 - half_boundary_thickness;
        topleft_boundary_rect.x1 = rect->x0 + half_boundary_thickness;
        topleft_boundary_rect.y1 = rect->y0 + half_boundary_thickness;

        UI_Rect(topleft_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_UpLeft);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_topleft", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    Vec2F32 p0 = rect->p0;
                    ui_store_drag_struct(&p0);
                }
                Vec2F32 p0 = *ui_get_drag_struct(Vec2F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->p0 = add_2f32(p0, drag_delta);
            }
        }

        //- bottom right boundary square
        Rng2F32 bottomright_boundary_rect = {0};
        bottomright_boundary_rect.x0 = rect->x1 - half_boundary_thickness;
        bottomright_boundary_rect.y0 = rect->y1 - half_boundary_thickness;
        bottomright_boundary_rect.x1 = rect->x1 + half_boundary_thickness;
        bottomright_boundary_rect.y1 = rect->y1 + half_boundary_thickness;

        UI_Rect(bottomright_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_DownRight);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_bottomright", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    Vec2F32 p1 = rect->p1;
                    ui_store_drag_struct(&p1);
                }
                Vec2F32 p1 = *ui_get_drag_struct(Vec2F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->p1 = add_2f32(p1, drag_delta);
            }
        }

        //- top right boundary square
        Rng2F32 topright_boundary_rect = {0};
        topright_boundary_rect.x0 = rect->x1 - half_boundary_thickness;
        topright_boundary_rect.y0 = rect->y0 - half_boundary_thickness;
        topright_boundary_rect.x1 = rect->x1 + half_boundary_thickness;
        topright_boundary_rect.y1 = rect->y0 + half_boundary_thickness;

        UI_Rect(topright_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_UpRight);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_topright", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    Vec2F32 p = {rect->x1, rect->y0};
                    ui_store_drag_struct(&p);
                }
                Vec2F32 p = *ui_get_drag_struct(Vec2F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->x1 = p.x + drag_delta.x;
                rect->y0 = p.y + drag_delta.y;
            }
        }

        //- bottom left boundary square
        Rng2F32 bottomleft_boundary_rect = {0};
        bottomleft_boundary_rect.x0 = rect->x0 - half_boundary_thickness;
        bottomleft_boundary_rect.y0 = rect->y1 - half_boundary_thickness;
        bottomleft_boundary_rect.x1 = rect->x0 + half_boundary_thickness;
        bottomleft_boundary_rect.y1 = rect->y1 + half_boundary_thickness;

        UI_Rect(bottomleft_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_DownLeft);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_bottomleft", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    Vec2F32 p = {rect->x0, rect->y1};
                    ui_store_drag_struct(&p);
                }
                Vec2F32 p = *ui_get_drag_struct(Vec2F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->x0 = p.x + drag_delta.x;
                rect->y1 = p.y + drag_delta.y;
            }
        }

        //~ boudary rect

        //- top boundary rect
        Rng2F32 top_boundary_rect = {0};
        top_boundary_rect.x0 = rect->x0 + half_boundary_thickness;
        top_boundary_rect.y0 = rect->y0 - half_boundary_thickness;
        top_boundary_rect.x1 = rect->x1 - half_boundary_thickness;
        top_boundary_rect.y1 = rect->y0 + half_boundary_thickness;

        UI_Rect(top_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_UpDown);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_top_boundary", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    F32 y0 = rect->y0;
                    ui_store_drag_struct(&y0);
                }
                F32 y0 = *ui_get_drag_struct(F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->y0 = y0 + drag_delta.y;
            }
        }

        //- bottom boundary rect
        Rng2F32 bottom_boundary_rect = {0};
        bottom_boundary_rect.x0 = rect->x0 + half_boundary_thickness;
        bottom_boundary_rect.y0 = rect->y1 - half_boundary_thickness;
        bottom_boundary_rect.x1 = rect->x1 - half_boundary_thickness;
        bottom_boundary_rect.y1 = rect->y1 + half_boundary_thickness;

        UI_Rect(bottom_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_UpDown);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_bottom_boundary", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    F32 y1 = rect->y1;
                    ui_store_drag_struct(&y1);
                }
                F32 y1 = *ui_get_drag_struct(F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->p1.y = y1 + drag_delta.y;
            }
        }

        //- left boundary rect
        Rng2F32 left_boundary_rect = {0};
        left_boundary_rect.x0 = rect->x0 - half_boundary_thickness;
        left_boundary_rect.y0 = rect->y0 + half_boundary_thickness;
        left_boundary_rect.x1 = rect->x0 + half_boundary_thickness;
        left_boundary_rect.y1 = rect->y1 - half_boundary_thickness;

        UI_Rect(left_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_LeftRight);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_left_boundary", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    F32 x0 = rect->x0;
                    ui_store_drag_struct(&x0);
                }
                F32 x0 = *ui_get_drag_struct(F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->x0 = x0 + drag_delta.x;
            }
        }

        //- right boundary rect
        Rng2F32 right_boundary_rect = {0};
        right_boundary_rect.x0 = rect->x1 - half_boundary_thickness;
        right_boundary_rect.y0 = rect->y0 + half_boundary_thickness;
        right_boundary_rect.x1 = rect->x1 + half_boundary_thickness;
        right_boundary_rect.y1 = rect->y1 - half_boundary_thickness;

        UI_Rect(right_boundary_rect)
        {
            ui_set_next_hover_cursor(OS_Cursor_LeftRight);
            UI_Box *box = ui_build_box_from_stringf(UI_BoxFlag_Clickable, "###%p_right_boundary", rect);
            UI_Signal sig = ui_signal_from_box(box);

            if(ui_dragging(sig))
            {
                if(ui_pressed(sig)) 
                {
                    F32 x1 = rect->x1;
                    ui_store_drag_struct(&x1);
                }
                F32 x1 = *ui_get_drag_struct(F32);
                Vec2F32 drag_delta = ui_drag_delta();
                rect->x1 = x1 + drag_delta.x;
            }
        }
    }

    // TODO: top-box is not resizing where collapsing
    Vec2F32 rect_dim = dim_2f32(*rect);

    //- k: build the top container
    ui_set_next_fixed_x(rect->p0.x);
    ui_set_next_fixed_y(rect->p0.y);
    ui_set_next_fixed_width(rect_dim.x);
    ui_set_next_pref_height(ui_children_sum(1.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    ui_set_next_focus_hot(UI_FocusKind_Root);
    ui_set_next_focus_active(UI_FocusKind_Root);
    ui_set_next_corner_radius_00(3);
    ui_set_next_corner_radius_10(3);
    ui_set_next_corner_radius_11(3);
    ui_set_next_corner_radius_01(3);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clip|UI_BoxFlag_Clickable|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_RoundChildrenByParent, string);

    ui_push_parent(box);
    ui_push_pref_width(ui_pct(1.0, .0));

    //- k: build the header
    ui_set_next_child_layout_axis(Axis2_X);
    ui_set_next_hover_cursor(OS_Cursor_UpDownLeftRight);
    UI_Box *header_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow|UI_BoxFlag_MouseClickable, "###header");
    UI_Parent(header_box)
    {
        ui_set_next_pref_size(Axis2_X, ui_px(39,1.0));
        if(ui_clicked(ui_expanderf(*open, "###expander"))) { *open = !(*open); }
        ui_set_next_pref_size(Axis2_X, ui_text_dim(3, 1.f));
        ui_label(string);
        ui_spacer(ui_pct(1.0, 0.f));
        ui_set_next_pref_size(Axis2_X, ui_px(39,1.f));
        if(ui_clicked(ui_closef("###close"))) { /* TODO */ }
    }
    UI_Signal header_signal = ui_signal_from_box(header_box);
    if(ui_dragging(header_signal))
    {
        typedef struct RK_UI_PaneDragData RK_UI_PaneDragData;
        struct RK_UI_PaneDragData
        {
            Rng2F32 start_rect;
        };

        if(ui_pressed(header_signal))
        {
            RK_UI_PaneDragData drag_data = { *rect };
            ui_store_drag_struct(&drag_data);
        }
        RK_UI_PaneDragData *drag_data = ui_get_drag_struct(RK_UI_PaneDragData);
        Vec2F32 drag_delta = ui_drag_delta();
        *rect = shift_2f32(drag_data->start_rect, drag_delta);
    }

    //- k: build the content container
    F32 content_height = rect_dim.y - header_box->fixed_size.y;
    if(!(*open)) { ui_set_next_flags(UI_BoxFlag_Disabled); }

    ui_set_next_pref_height(ui_px(content_height, 0.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    UI_Box *content_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AnimatePosY|UI_BoxFlag_DrawBackground, "###container");
    content_box->pref_size[Axis2_Y].value = mix_1f32(content_height, 0, content_box->disabled_t);

    ui_pop_parent();
    ui_push_parent(content_box);
    return box;
}

internal UI_Signal
rk_ui_pane_end(void)
{
    ui_pop_pref_width();
    UI_Box *box = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}

////////////////////////////////
//~ k: Dropdown

internal UI_Box *
rk_ui_dropdown_begin(String8 string)
{
    // Make key
    UI_Key key = ui_key_from_string(ui_active_seed_key(), string);

    // Calculate focus
    B32 is_auto_focus_hot = ui_is_key_auto_focus_hot(key);
    B32 is_auto_focus_active = ui_is_key_auto_focus_active(key);
    ui_push_focus_hot(is_auto_focus_hot ? UI_FocusKind_On : UI_FocusKind_Null);
    ui_push_focus_active(is_auto_focus_active ? UI_FocusKind_On : UI_FocusKind_Null);

    B32 is_focus_hot = ui_is_focus_hot();
    B32 is_focus_active = ui_is_focus_active();
    B32 is_focus_hot_disabled = (!is_focus_hot && ui_top_focus_hot() == UI_FocusKind_On);
    B32 is_focus_active_disabled = (!is_focus_active && ui_top_focus_active() == UI_FocusKind_On);

    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *box = ui_build_box_from_key(UI_BoxFlag_Clickable|
                                        UI_BoxFlag_DrawBorder|
                                        UI_BoxFlag_DrawDropShadow|
                                        UI_BoxFlag_ClickToFocus|
                                        UI_BoxFlag_DrawHotEffects|
                                        UI_BoxFlag_DrawActiveEffects,
                                        key);
    UI_Signal sig1;
    UI_Parent(box)
    {
        UI_PrefWidth(ui_pct(1.0, 0.f))
        {
            ui_label(string);
        }
        UI_Font(ui_icon_font()) UI_PrefWidth(ui_px(ui_top_pref_height().value, 1.f)) UI_TextAlignment(UI_TextAlign_Center)
        {
            sig1 = ui_buttonf(is_focus_hot ? "^" : "v");
        }
    }
    ui_box_equip_display_string(box, string);
    UI_Signal sig = ui_signal_from_box(box);

    if((ui_clicked(sig) || ui_clicked(sig1)) && (!is_focus_active))
    {
        ui_set_auto_focus_hot_key(key);
        ui_set_auto_focus_active_key(key);
    }

    if((ui_clicked(sig) || ui_clicked(sig1)) && is_focus_active)
    {
        ui_set_auto_focus_hot_key(ui_key_zero());
        ui_set_auto_focus_active_key(ui_key_zero());
    }

    ui_set_next_fixed_x(box->fixed_position.x);
    ui_set_next_fixed_y(box->fixed_position.y + box->fixed_size.y);
    ui_set_next_pref_width(ui_px(box->fixed_size.x, 0.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    if(is_focus_active)
    {
        ui_set_next_pref_height(ui_children_sum(0.0));
    }
    else
    {
        ui_set_next_pref_height(ui_px(0.0, 1.0));
    }
    UI_Box *list_container = ui_build_box_from_stringf(UI_BoxFlag_Floating|
                                                       UI_BoxFlag_DrawDropShadow|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_Clip,
                                                       "###%S-dropdown_container", string);
    ui_pop_focus_hot();
    ui_pop_focus_active();
    ui_push_parent(list_container);
    ui_push_pref_width(ui_pct(1.0, 0.0));
    ui_push_text_alignment(UI_TextAlign_Left);
    return list_container;
}

internal UI_Signal
rk_ui_dropdown_end(void)
{
    ui_pop_pref_width();
    ui_pop_text_alignment();
    UI_Box *list_container = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(list_container);
    return sig;
}

internal void
rk_ui_dropdown_hide(void)
{
    ui_set_auto_focus_hot_key(ui_key_zero());
    ui_set_auto_focus_active_key(ui_key_zero());
}

internal UI_Signal
rk_ui_checkbox(B32 *b)
{
#if 0
    UI_Signal result;
    RK_IconKind icon_kind = *b == 0 ? RK_IconKind_CheckHollow : RK_IconKind_CheckFilled;
    result = rk_icon_button(icon_kind, string);
    if(ui_clicked(result))
    {
        *b = !*b;
    }
    return result;
#else
    UI_Signal result;
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *container_box = ui_build_box_from_string(0, str8((U8*)b, sizeof(*b)));

    UI_Parent(container_box)
    {
        ui_spacer(ui_pct(1,0));

        UI_Size size = ui_em(1.0, 1);
        UI_PrefWidth(size) UI_PrefHeight(size)
        {
            ui_set_next_hover_cursor(OS_Cursor_HandPoint);
            UI_Box *outer = ui_build_box_from_stringf(UI_BoxFlag_Clickable|
                                                      UI_BoxFlag_DrawBorder|
                                                      UI_BoxFlag_DrawBackground|
                                                      UI_BoxFlag_DrawHotEffects|
                                                      UI_BoxFlag_DrawActiveEffects,
                                                      "###outer");
            result = ui_signal_from_box(outer);
            F32 active_t = outer->active_t;
            F32 hot_t = outer->hot_t;

            Rng2F32 inner_rect = {0};
            inner_rect.x1 = outer->fixed_size.x;
            inner_rect.y1 = outer->fixed_size.y;
            F32 padding = 0;
            padding = padding + (*b != 0)*outer->fixed_size.x*0.175;
            // padding = padding + mix_1f32(0, -outer->fixed_size.x*0.025, hot_t);
            UI_Parent(outer) UI_Rect(pad_2f32(inner_rect, -(padding))) UI_Palette(ui_state->widget_palette_info.scrollbar_palette)
            {
                UI_Box *inner = ui_build_box_from_stringf(UI_BoxFlag_DrawHotEffects|
                                                          UI_BoxFlag_DrawActiveEffects|
                                                          UI_BoxFlag_DrawBorder|
                                                          UI_BoxFlag_DrawBackground|
                                                          UI_BoxFlag_DrawDropShadow, "###inner");
                inner->hot_t = Clamp(0, hot_t+*b, 1);
                inner->active_t = active_t;
            }
        }

        ui_spacer(ui_pct(1,0));
    }
    if(ui_clicked(result))
    {
        *b = !*b;
    }
    return result;
#endif
}
