#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// [h]
#define CGLTF_IMPLEMENTATION
#include "external/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "external/linmath.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "./game_core.h"
#include "./game_asset.h"
#include "./game.h"

// [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "./game_core.c"
#include "./game_asset.c"
#include "./game.c"

internal void
ui_draw(OS_Handle os_wnd)
{
    Rng2F32 w_rect = os_client_rect_from_window(os_wnd);
    Temp scratch = scratch_begin(0,0);

    //- k: draw background color
    Vec4F32 bg_color = v4f32(0,0,0,0);
    // d_rect(w_rect, bg_color, 0,0,0);
    // TODO: draw window border

    // Recusivly drawing boxes
    UI_Box *b = ui_root_from_state(ui_state);
    while(!ui_box_is_nil(b))
    {
        UI_BoxRec rec = ui_box_rec_df_post(b, &ui_g_nil_box);

        // TODO: DEBUG
        if(0)
        {
            d_rect(b->rect, v4f32(0,0.3,1,0.3), 0,1,0);
        }

        //- k: draw the border
        if(b->flags & UI_BoxFlag_DrawBorder)
        {
            d_rect(b->rect, v4f32(0.102,0.102,0.098,1.0), 0,0,0);
        }

        //- k: draw drop_shadw
        if(b->flags & UI_BoxFlag_DrawDropShadow)
        {
            d_rect(b->rect, v4f32(0,0.3,1,0.3), 0.8f, 0, 8.0f);
        }

        //- k: draw background
        if(b->flags & UI_BoxFlag_DrawBackground)
        {
            // Main rectangle
            // TODO: use color palette
            Vec4F32 box_bg = rgba_from_u32(0X212121FF);
            R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), box_bg, 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, b->corner_radii);

            if(b->flags & UI_BoxFlag_DrawHotEffects)
            {
                // TODO: blend with active_t
                F32 effective_active_t = b->active_t;
                if (!(b->flags & UI_BoxFlag_DrawActiveEffects))
                {
                    effective_active_t = 0;
                }

                F32 t = b->hot_t * (1 - effective_active_t);

                // Brighten
                // TODO: use color palette
                R_Rect2DInst *inst = d_rect(b->rect, v4f32(0,0,0,0), 0, 0, 1.f);
                Vec4F32 color = rgba_from_u32(0xFFFFFFFF);
                color.w *= t * 0.3f;
                inst->colors[Corner_00] = color;
                inst->colors[Corner_01] = color;
                inst->colors[Corner_10] = color;
                inst->colors[Corner_11] = color;
                inst->colors[Corner_10].w *= t;
                inst->colors[Corner_11].w *= t;
                MemoryCopyArray(inst->corner_radii, b->corner_radii);
            }

            // Active effect extension
            if(b->flags & UI_BoxFlag_DrawActiveEffects)
            {
                Vec4F32 shadow_color = rgba_from_u32(0xFFFFFFFF);
                shadow_color.x *= 0.3f;
                shadow_color.y *= 0.3f;
                shadow_color.z *= 0.3f;
                shadow_color.w *= 0.5f * b->active_t;

                Vec2F32 shadow_size = {
                    (b->rect.x1 - b->rect.x0) * 0.60f * b->active_t,
                    (b->rect.y1 - b->rect.y0) * 0.60f * b->active_t,
                };

                // k: top -> button dark effect
                {
                    R_Rect2DInst *inst = d_rect(r2f32p(b->rect.x0, b->rect.y0, b->rect.x1, b->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
                    inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // k: bottom -> top light effect
                {
                    R_Rect2DInst *inst = d_rect(r2f32p(b->rect.x0, b->rect.y1 - shadow_size.y, b->rect.x1, b->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
                    inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.3f, 0.3f, 0.3f, 0.3f*b->active_t);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // k: left -> right dark effect
                {
                    R_Rect2DInst *inst = d_rect(r2f32p(b->rect.x0, b->rect.y0, b->rect.x0 + shadow_size.x, b->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = inst->colors[Corner_01] = shadow_color;
                    inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // k: right -> left dark effect
                {
                    R_Rect2DInst *inst = d_rect(r2f32p(b->rect.x1 - shadow_size.x, b->rect.y0, b->rect.x1, b->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.0f);
                    inst->colors[Corner_10] = inst->colors[Corner_11] = shadow_color;
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }
            }
        }

        // Draw string
        if(b->flags & UI_BoxFlag_DrawText)
        {
            // TODO: handle font color
            Vec2F32 text_position = ui_box_text_position(b);

            // Max width
            F32 max_x = 100000.0f;
            F_Run ellipses_run = {0};

            if(!(b->flags & UI_BoxFlag_DisableTextTrunc))
            {
                max_x = (b->rect.x1-text_position.x-b->text_padding);
                ellipses_run = f_push_run_from_string(scratch.arena, b->font, b->font_size, 0, b->tab_size, 0, str8_lit("..."));
            }

            d_truncated_fancy_run_list(text_position, &b->display_string_runs, b->rect.max.x, ellipses_run);
        }

        if(b->flags & UI_BoxFlag_Clip)
        {
            // TODO
        }

        // Call custom draw callback
        if(b->custom_draw != 0)
        {
            b->custom_draw(b, b->custom_draw_user_data);
        }
        b = rec.next;
    }
    scratch_end(scratch);
}

internal void
entry_point(CmdLine *cmd_line)
{
    Vec2F32 default_resolution = {900, 900};
    String8 window_title = str8_lit("Rubik");

    //- Open window
    OS_Handle window = os_window_open(default_resolution, 0, window_title);
    os_window_first_paint(window);

    //- Render
    r_init((char *)window_title.str, window, true);
    R_Handle wnd = r_window_equip(window);

    /////////////////////////////////
    //~ Game loop

    Arena *frame_arena = arena_alloc();

    //- Time delta
    // TODO: using moving average to smooth out the value
    U64 frame_us     = os_now_microseconds();
    U64 cpu_start_us = 0;
    U64 cpu_end_us   = 0;
    U64 cpu_dt_us    = 0;
    U64 gpu_start_us = 0;
    U64 gpu_end_us   = 0;
    U64 gpu_dt_us    = 0;

    //- Resource
    F_Tag main_font = f_tag_from_path("./fonts/Mplus1Code-Medium.ttf");
    F32 main_font_size = 30;

    //- User interaction
    Vec2F32 mouse = {0};
    char temp_str[512];
    OS_EventList events;

    //- Window
    Rng2F32 window_rect = {0};

    UI_State *ui = ui_state_alloc();
    ui_select_state(ui);

    // TODO: Testing for now, should wrap into some kind of struct
    TxtPt cursor = {0,4};
    TxtPt mark   = {0,4};
    U8 edit_buffer[300] = "test";
    // TODO: is this size per line
    U64 edit_string_size_out[] = {4};
    TxtPt cursor2 = {0,4};
    TxtPt mark2   = {0,4};
    U8 edit_buffer2[300] = "test";
    // TODO: is this size per line
    U64 edit_string_size_out2[] = {4};

    g_init(window);
    G_Scene *scene = g_scene_load();

    B32 window_should_close = 0;
    while(!window_should_close)
    {
        U64 dt = os_now_microseconds() - frame_us;
        frame_us = os_now_microseconds();

        //- Poll events
        events = os_get_events(frame_arena, 0);

        // TODO: don't need to fetch rect every frame
        window_rect = os_client_rect_from_window(window);
        mouse = os_mouse_from_window(window);

        /////////////////////////////////
        //- Begin of frame

        r_begin_frame();
        U64 id = r_window_begin_frame(window, wnd);
        d_begin_frame();
        cpu_start_us = os_now_microseconds();
        gpu_end_us = os_now_microseconds();
        gpu_dt_us = gpu_end_us - gpu_start_us;

        /////////////////////////////////
        // Create the top bucket (for UI)

        D_Bucket *bucket = d_bucket_make();
        d_push_bucket(bucket);

        /////////////////////////////////
        // Build ui

        //- Build event list for ui
        UI_EventList ui_events = {0};
        OS_Event *os_evt_first = events.first;
        OS_Event *os_evt_opl = events.last + 1;
        for(OS_Event *os_evt = os_evt_first; os_evt < os_evt_opl; os_evt++)
        {
            if(os_evt == 0) continue;

            UI_Event ui_evt = zero_struct;

            UI_EventKind kind = UI_EventKind_Null;
            switch(os_evt->kind)
            {
                default:{}break;
                case OS_EventKind_Press:       {kind = UI_EventKind_Press;}break;
                case OS_EventKind_Release:     {kind = UI_EventKind_Release;}break;
                case OS_EventKind_MouseMove:   {kind = UI_EventKind_MouseMove;}break;
                case OS_EventKind_Text:        {kind = UI_EventKind_Text;}break;
                case OS_EventKind_Scroll:      {kind = UI_EventKind_Scroll;}break;
            }

            ui_evt.kind         = kind;
            ui_evt.key          = os_evt->key;
            ui_evt.pos          = os_evt->pos;
            ui_evt.delta_2f32   = os_evt->delta;
            ui_evt.string       = os_evt->character ? str8_from_32(ui_build_arena(), str32(&os_evt->character, 1)) : str8_zero();
            ui_evt.timestamp_us = os_evt->timestamp_us;
            ui_evt.modifiers    = os_evt->modifiers;

            if(ui_evt.key == OS_Key_Backspace && ui_evt.kind == UI_EventKind_Press)
            {
                ui_evt.kind       = UI_EventKind_Edit;
                ui_evt.flags      = UI_EventFlag_Delete | UI_EventFlag_KeepMark;
                ui_evt.delta_unit = UI_EventDeltaUnit_Char;
                ui_evt.delta_2s32 = v2s32(-1,0);
            }

            if(ui_evt.kind == UI_EventKind_Text)
            {
                ui_evt.flags = UI_EventFlag_KeepMark;
            }

            if(ui_evt.key == OS_Key_Return && ui_evt.kind == UI_EventKind_Press)
            {
                ui_evt.slot = UI_EventActionSlot_Accept;
            }

            if(ui_evt.key == OS_Key_A && (ui_evt.modifiers & OS_Modifier_Ctrl) && ui_evt.kind == UI_EventKind_Press)
            {
                ui_evt.kind       = UI_EventKind_Navigate;
                ui_evt.flags      = UI_EventFlag_KeepMark;
                ui_evt.delta_unit = UI_EventDeltaUnit_Whole;
                ui_evt.delta_2s32 = v2s32(1,0);

                UI_Event ui_evt_0 = ui_evt;
                ui_evt_0.flags      = 0;
                ui_evt_0.delta_unit = UI_EventDeltaUnit_Whole;
                ui_evt_0.delta_2s32 = v2s32(-1,0);
                ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt_0);
            }

            if(ui_evt.key == OS_Key_Left && ui_evt.kind == UI_EventKind_Press)
            {
                ui_evt.kind       = UI_EventKind_Navigate;
                ui_evt.flags      = 0;
                ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
                ui_evt.delta_2s32 = v2s32(-1,0);
            }

            if(ui_evt.key == OS_Key_Right && ui_evt.kind == UI_EventKind_Press)
            {
                ui_evt.kind       = UI_EventKind_Navigate;
                ui_evt.flags      = 0;
                ui_evt.delta_unit = ui_evt.modifiers & OS_Modifier_Ctrl ? UI_EventDeltaUnit_Word : UI_EventDeltaUnit_Char;
                ui_evt.delta_2s32 = v2s32(1,0);
            }

            ui_event_list_push(ui_build_arena(), &ui_events, &ui_evt);

            // TODO: we may want to handle this somewhere else
            if(os_evt->kind == OS_EventKind_WindowClose) {window_should_close = 1;}
        }

        // Begin & push initial stack values
        ui_begin_build(window, &ui_events, dt / 1000000.0f);
        ui_push_font(main_font);
        ui_push_font_size(main_font_size);
        // ui_push_text_padding(main_font_size*0.3f);
        // ui_push_text_raster_flags(...);

        UI_Box *bg_box = &ui_g_nil_box;

        UI_Rect(window_rect)
        UI_ChildLayoutAxis(Axis2_X)
        UI_Focus(UI_FocusKind_On)
        {
            bg_box = ui_build_box_from_stringf(UI_BoxFlag_FixedSize|
                                               UI_BoxFlag_Clickable|
                                               UI_BoxFlag_DefaultFocusNav,
                                               "###background_%p", window.u64[0]);
        }

        // UI_Flags(UI_BoxFlag_ClickToFocus) 
        ui_push_parent(bg_box);
        UI_Pane(r2f32p(10, 10, 710, 310), str8_lit("stats"))
        {
            ui_set_next_pref_size(Axis2_Y, ui_pct(1.0, 1.0));
            UI_Column
            {
                UI_Row
                {
                    ui_label(str8_lit("frame time ms"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%.6f", (dt/1000.0));
                }
                UI_Row
                {
                    ui_label(str8_lit("cpu time ms"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%.6f", (cpu_dt_us/1000.0));
                }
                UI_Row
                {
                    ui_label(str8_lit("gpu time ms"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%.6f", (gpu_dt_us/1000.0));
                }
                ui_spacer(ui_px(3.0, 0));
                UI_Row
                {
                    ui_label(str8_lit("fps"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%6.2f", 1 / (dt / 1000000.0));
                }
                ui_spacer(ui_px(3.0, 0));
                UI_Row
                {
                    ui_label(str8_lit("hot_box"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%lu", ui_state->hot_box_key.u64[0]);
                }
                Vec2F32 mouse = os_mouse_from_window(window);
                UI_Row
                {
                    ui_label(str8_lit("mouse"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%.2f, %.2f", mouse.x, mouse.y);
                }
                UI_Row
                {
                    ui_label(str8_lit("name1"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_line_edit(&cursor, &mark,
                                 edit_buffer, sizeof(edit_buffer), edit_string_size_out,
                                 str8_lit("test"), str8_lit("name1"));
                }
                UI_Row
                {
                    ui_label(str8_lit("name2"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    if(ui_committed(ui_line_edit(&cursor2, &mark2,
                                    edit_buffer2, sizeof(edit_buffer2), edit_string_size_out2,
                                    str8(edit_buffer2, edit_string_size_out2[0]), str8_lit("name2")))) {}
                }
                UI_Row
                {
                    ui_label(str8_lit("drag mouse start"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%.2f, %.2f", ui_state->drag_start_mouse.x, ui_state->drag_start_mouse.y);
                }
                UI_Row
                {
                    ui_label(str8_lit("geo3d hot id"));
                    ui_spacer(ui_pct(1.0, 0.0));
                    ui_labelf("%lu", id);
                }
            }
        }

        // UI_Flags(UI_BoxFlag_ClickToFocus) UI_Parent(bg_box) {
        //     UI_FixedX(600) UI_FixedY(600) UI_PrefWidth(ui_px(x/3.0f, 1)) UI_PrefHeight(ui_px(y/3.0f, 1)) {
        //         Rng2F32 region = r2f32p(0, 0, x, y);
        //         ui_image(demo_image, R_Tex2DSampleKind_Nearest, region, v4f32(1,1,1,1), 0.0f, str8_lit("demo_image"));
        //     }
        // }

        /////////////////////////////////
        //~ Draw game

        g_update_and_render(scene, events, dt, id);

        ui_end_build();

        /////////////////////////////////
        //~ Draw ui
        ui_draw(window);
        R_Rect2DInst *cursor = d_rect(r2f32p(mouse.x-15,mouse.y-15, mouse.x+15,mouse.y+15), v4f32(0,0.3,1,0.3), 15, 0.0, 0.7);
        d_sub_bucket(g_state->bucket_rect);
        d_sub_bucket(g_state->bucket_geo3d);

        cpu_end_us = os_now_microseconds();
        cpu_dt_us = cpu_end_us - cpu_start_us;

        /////////////////////////////////
        //~ End of frame

        gpu_start_us = os_now_microseconds();
        d_submit_bucket(window, wnd, bucket, mouse);
        r_window_end_frame(window, wnd);
        r_end_frame();
        arena_clear(frame_arena);
    }

    /////////////////////////////////
    //~ Cleanup

    os_window_close(window);
}
