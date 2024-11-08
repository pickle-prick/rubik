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
        if(ui_clicked(ui_closef("###close"))) {}
    }

    //- k: build the content container
    F32 content_height = rect_dim.y - header_box->fixed_size.y;
    if(!(*open))
    {
        // ui_set_next_fixed_y(header_box->rect.p0.y-content_height);
        ui_set_next_flags(UI_BoxFlag_Disabled);
    }
    ui_set_next_pref_height(ui_px(content_height, 0.0));
    UI_Box *content_box = ui_build_box_from_stringf(UI_BoxFlag_Clip|UI_BoxFlag_AnimatePosY|UI_BoxFlag_DrawBackground, "###container");
    content_box->pref_size[Axis2_Y].value = mix_1f32(content_height, 0, content_box->disabled_t);
    ui_push_parent(content_box);
    return box;
}

internal UI_Signal
g_ui_pane_end(void)
{
    ui_pop_pref_width();
    UI_Box *box = ui_pop_parent();
    ui_pop_parent();
    UI_Signal sig = ui_signal_from_box(box);
    return sig;
}
