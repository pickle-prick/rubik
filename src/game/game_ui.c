////////////////////////////////
//~ k: Floating/Fixed Panes

internal UI_Box *
g_ui_pane_begin(Rng2F32 rect, B32 *open, String8 string)
{
    // TODO: top-box is not resizing where collapsing
    Vec2F32 rect_dim = dim_2f32(rect);

    //- k: build the top container
    ui_set_next_fixed_x(rect.p0.x);
    ui_set_next_fixed_y(rect.p0.y);
    ui_set_next_fixed_width(rect_dim.x);
    ui_set_next_pref_height(ui_children_sum(1.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    UI_Box *box = ui_build_box_from_string(UI_BoxFlag_Clip|UI_BoxFlag_Clickable, string);

    ui_push_parent(box);
    ui_push_pref_width(ui_pct(1.0, .0));

    //- k: build the header
    ui_set_next_child_layout_axis(Axis2_X);
    UI_Box *header_box = ui_build_box_from_stringf(UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawBackground|UI_BoxFlag_DrawDropShadow, "###header");
    UI_Parent(header_box)
    {
        ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
        if(ui_clicked(ui_expanderf(*open, "###expander"))) { *open = !(*open); }
        ui_set_next_pref_size(Axis2_X, ui_text_dim(3, 0.0));
        ui_label(string);
        ui_spacer(ui_pct(1.0, 0.0));
        ui_set_next_pref_size(Axis2_X, ui_px(39,0.0));
        if(ui_clicked(ui_closef("###close"))) { /* TODO */ }
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
g_ui_pane_end(void)
{
    ui_pop_pref_width();
    UI_Box *box = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}

internal UI_Box *
g_ui_dropdown_begin(String8 string)
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

    // ui_set_next_hover_cursor(is_focus_active ? OS_Cursor_IBar : OS_Cursor_HandPoint);
    ui_set_next_hover_cursor(OS_Cursor_HandPoint);
    UI_Box *box = ui_build_box_from_key(UI_BoxFlag_Clickable|
                                           UI_BoxFlag_DrawBackground|
                                           UI_BoxFlag_DrawBorder|
                                           UI_BoxFlag_ClickToFocus|
                                           UI_BoxFlag_DrawText|
                                           UI_BoxFlag_DrawHotEffects|
                                           UI_BoxFlag_DrawActiveEffects,
                                           key);
    ui_box_equip_display_string(box, string);
    UI_Signal sig = ui_signal_from_box(box);

    if(ui_clicked(sig) && is_focus_active)
    {
        ui_set_auto_focus_hot_key(ui_key_zero());
        ui_set_auto_focus_active_key(ui_key_zero());
        is_focus_hot = is_focus_active = 0;
    }

    if(ui_clicked(sig) && is_focus_hot)
    {
        ui_set_auto_focus_active_key(key);
    }

    ui_set_next_fixed_x(box->fixed_position.x);
    ui_set_next_fixed_y(box->fixed_position.y + box->fixed_size.y);
    ui_set_next_pref_width(ui_px(box->fixed_size.x, 0.0));
    ui_set_next_child_layout_axis(Axis2_Y);
    if(is_focus_hot)
    {
        ui_set_next_pref_height(ui_children_sum(0.0));
    }
    else
    {
        ui_set_next_pref_height(ui_px(0.0, 1.0));
    }
    UI_Box *list_container = ui_build_box_from_stringf(UI_BoxFlag_Floating|
                                                       UI_BoxFlag_DrawBackground|
                                                       UI_BoxFlag_DrawBorder|
                                                       UI_BoxFlag_DrawHotEffects|
                                                       UI_BoxFlag_DrawActiveEffects|
                                                       UI_BoxFlag_DrawSideLeft|
                                                       UI_BoxFlag_DrawSideRight|
                                                       UI_BoxFlag_DrawSideBottom|
                                                       UI_BoxFlag_DrawSideTop|
                                                       UI_BoxFlag_Clip,
                                                       "###%S-dropdown_container", string);
    ui_pop_focus_hot();
    ui_pop_focus_active();
    ui_push_parent(list_container);
    ui_push_pref_width(ui_pct(1.0, 0.0));
    ui_push_text_alignment(UI_TextAlign_Center);
    return list_container;
}

internal UI_Signal
g_ui_dropdown_end(void)
{
    ui_pop_pref_width();
    ui_pop_text_alignment();
    UI_Box *list_container = ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(list_container);
    return sig;
}

internal void
g_ui_dropdown_hide(void)
{
    ui_set_auto_focus_hot_key(ui_key_zero());
}
