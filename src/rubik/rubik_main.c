#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

////////////////////////////////
//~ k: Build Options

#define BUILD_TITLE "Rubik"
#define OS_FEATURE_GRAPHICAL 1

// Third Party
#define CGLTF_IMPLEMENTATION
#include "external/cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"
// #include "external/linmath.h"
#include "external/xxHash/xxhash.c"
#include "external/xxHash/xxhash.h"

// [h]
#include "base/base_inc.h"
#include "os/os_inc.h"
#include "render/render_inc.h"
#include "font_provider/font_provider_inc.h"
#include "font_cache/font_cache.h"
#include "draw/draw.h"
#include "ui/ui_inc.h"
#include "serialize/serialize_inc.h"
#include "./rubik_ui_widget.h"
#include "./rubik_core.h"
#include "./rubik_asset.h"
#include "./scenes/rubik_scenes_inc.h"

// [c]
#include "base/base_inc.c"
#include "os/os_inc.c"
#include "render/render_inc.c"
#include "font_provider/font_provider_inc.c"
#include "font_cache/font_cache.c"
#include "draw/draw.c"
#include "ui/ui_inc.c"
#include "serialize/serialize_inc.c"
#include "./rubik_ui_widget.c"
#include "./rubik_core.c"
#include "./rubik_asset.c"
#include "./scenes/rubik_scenes_inc.c"

internal void
ui_draw(OS_Handle os_wnd)
{
    Rng2F32 w_rect = os_client_rect_from_window(os_wnd);
    Temp scratch = scratch_begin(0,0);

    // Recusivly drawing boxes
    UI_Box *box = ui_root_from_state(ui_state);
    while(!ui_box_is_nil(box))
    {
        UI_BoxRec rec = ui_box_rec_df_post(box, &ui_nil_box);

        if(box->transparency != 0)
        {
            d_push_transparency(box->transparency);
        }

        //- k: draw drop_shadw
        if(box->flags & UI_BoxFlag_DrawDropShadow)
        {
            Rng2F32 drop_shadow_rect = shift_2f32(pad_2f32(box->rect, 8), v2f32(4, 4));
            Vec4F32 drop_shadow_color = rk_rgba_from_theme_color(RK_ThemeColor_DropShadow);
            d_rect(drop_shadow_rect, drop_shadow_color, 0.8f, 0, 8.f);
        }

        //- k: draw background
        if(box->flags & UI_BoxFlag_DrawBackground)
        {
            // Main rectangle
            R_Rect2DInst *inst = d_rect(pad_2f32(box->rect, 1), box->palette->colors[UI_ColorCode_Background], 0, 0, 1.f);
            MemoryCopyArray(inst->corner_radii, box->corner_radii);

            if(box->flags & UI_BoxFlag_DrawHotEffects)
            {
                F32 effective_active_t = box->active_t;
                if (!(box->flags & UI_BoxFlag_DrawActiveEffects))
                {
                    effective_active_t = 0;
                }
                F32 t = box->hot_t * (1-effective_active_t);

                // Brighten
                {
                    R_Rect2DInst *inst = d_rect(box->rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                    color.w *= t*0.2f;
                    inst->colors[Corner_00] = color;
                    inst->colors[Corner_01] = color;
                    inst->colors[Corner_10] = color;
                    inst->colors[Corner_11] = color;
                    inst->colors[Corner_10].w *= t;
                    inst->colors[Corner_11].w *= t;
                    MemoryCopyArray(inst->corner_radii, box->corner_radii);
                }

                // rjf: slight emboss fadeoff
                if(0)
                {
                    Rng2F32 rect = r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y1);
                    R_Rect2DInst *inst = d_rect(rect, v4f32(0, 0, 0, 0), 0, 0, 1.f);
                    inst->colors[Corner_00] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
                    inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
                    inst->colors[Corner_10] = v4f32(0.f, 0.f, 0.f, 0.0f*t);
                    inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.3f*t);
                    MemoryCopyArray(inst->corner_radii, box->corner_radii);
                }

                // Active effect extension
                if(box->flags & UI_BoxFlag_DrawActiveEffects)
                {
                    Vec4F32 shadow_color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                    shadow_color.x *= 0.3f;
                    shadow_color.y *= 0.3f;
                    shadow_color.z *= 0.3f;
                    shadow_color.w *= 0.5f*box->active_t;

                    Vec2F32 shadow_size =
                    {
                        (box->rect.x1 - box->rect.x0)*0.60f*box->active_t,
                        (box->rect.y1 - box->rect.y0)*0.60f*box->active_t,
                    };
                    shadow_size.x = Clamp(0, shadow_size.x, box->font_size*2.f);
                    shadow_size.y = Clamp(0, shadow_size.y, box->font_size*2.f);

                    // rjf: top -> bottom dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x1, box->rect.y0 + shadow_size.y), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.0f);
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: bottom -> top light effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y1 - shadow_size.y, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_10] = v4f32(0, 0, 0, 0);
                        inst->colors[Corner_01] = inst->colors[Corner_11] = v4f32(0.4f, 0.4f, 0.4f, 0.4f*box->active_t);
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: left -> right dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x0, box->rect.y0, box->rect.x0 + shadow_size.x, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_10] = inst->colors[Corner_11] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_00] = shadow_color;
                        inst->colors[Corner_01] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }

                    // rjf: right -> left dark effect
                    {
                        R_Rect2DInst *inst = d_rect(r2f32p(box->rect.x1 - shadow_size.x, box->rect.y0, box->rect.x1, box->rect.y1), v4f32(0, 0, 0, 0), 0, 0, 1.f);
                        inst->colors[Corner_00] = inst->colors[Corner_01] = v4f32(0.f, 0.f, 0.f, 0.f);
                        inst->colors[Corner_10] = shadow_color;
                        inst->colors[Corner_11] = shadow_color;
                        MemoryCopyArray(inst->corner_radii, box->corner_radii);
                    }
                }
            }
        }

        // Draw string
        if(box->flags & UI_BoxFlag_DrawText)
        {
            // TODO: handle font color
            Vec2F32 text_position = ui_box_text_position(box);

            // Max width
            F32 max_x = 100000.0f;
            F_Run ellipses_run = {0};

            if(!(box->flags & UI_BoxFlag_DisableTextTrunc))
            {
                max_x = (box->rect.x1-box->text_padding);
                ellipses_run = f_push_run_from_string(scratch.arena, box->font, box->font_size, 0, box->tab_size, 0, str8_lit("..."));
            }

            d_truncated_fancy_run_list(text_position, &box->display_string_runs, max_x, ellipses_run);
        }

        // NOTE(k): draw focus viz
        if(1)
        {
            B32 focused = (box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive) &&
                    box->flags & UI_BoxFlag_Clickable);
            B32 disabled = 0;
            for(UI_Box *p = box; !ui_box_is_nil(p); p = p->parent)
            {
                if(p->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                {
                    disabled = 1;
                    break;
                }
            }
            if(focused)
            {
                Vec4F32 color = v4f32(0.3f, 0.8f, 0.3f, 1.f);
                if(disabled)
                {
                    color = v4f32(0.8f, 0.3f, 0.3f, 1.f);
                }
                d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), color, 2, 0, 1);
                d_rect(box->rect, color, 2, 2, 1);
            }
            if(box->flags & (UI_BoxFlag_FocusHot|UI_BoxFlag_FocusActive))
            {
                if(box->flags & (UI_BoxFlag_FocusHotDisabled|UI_BoxFlag_FocusActiveDisabled))
                {
                    d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(1, 0, 0, 0.2f), 2, 0, 1);
                }
                else
                {
                    d_rect(r2f32p(box->rect.x0-6, box->rect.y0-6, box->rect.x0+6, box->rect.y0+6), v4f32(0, 1, 0, 0.2f), 2, 0, 1);
                }
            }
        }

        if(box->flags & UI_BoxFlag_Clip)
        {
            Rng2F32 top_clip = d_top_clip();
            Rng2F32 new_clip = pad_2f32(box->rect, -1);
            if(top_clip.x1 != 0 || top_clip.y1 != 0)
            {
                new_clip = intersect_2f32(new_clip, top_clip);
            }
            d_push_clip(new_clip);
        }

        // k: custom draw list
        if(box->flags & UI_BoxFlag_DrawBucket)
        {
            Mat3x3F32 xform = make_translate_3x3f32(box->position_delta);
            D_XForm2DScope(xform)
            {
                d_sub_bucket(box->draw_bucket);
            }
        }

        // Call custom draw callback
        if(box->custom_draw != 0)
        {
            box->custom_draw(box, box->custom_draw_user_data);
        }

        // k: pop stacks
        {
            S32 pop_idx = 0;
            for(UI_Box *b = box; !ui_box_is_nil(b) && pop_idx <= rec.pop_count; b = b->parent)
            {
                pop_idx += 1;
                if(b == box && rec.push_count != 0) continue;

                // k: pop clip
                if(b->flags & UI_BoxFlag_Clip)
                {
                    d_pop_clip();
                }

                // rjf: draw overlay
                if(b->flags & UI_BoxFlag_DrawOverlay)
                {
                    R_Rect2DInst *inst = d_rect(b->rect, b->palette->colors[UI_ColorCode_Overlay], 0, 0, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                //- k: draw the border
                if(b->flags & UI_BoxFlag_DrawBorder)
                {
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1.f), b->palette->colors[UI_ColorCode_Border], 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);

                    // rjf: hover effect
                    if(b->flags & UI_BoxFlag_DrawHotEffects)
                    {
                        Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Hover);
                        color.w *= b->hot_t;
                        R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), color, 0, 1.f, 1.f);
                        MemoryCopyArray(inst->corner_radii, b->corner_radii);
                    }
                }

                // k: debug border rendering
                if(0)
                {
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 1), v4f32(1, 0, 1, 0.25f), 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: draw sides
                {
                    Rng2F32 r = b->rect;
                    F32 half_thickness = 1.f;
                    F32 softness = 0.5f;
                    if(b->flags & UI_BoxFlag_DrawSideTop)
                    {
                        d_rect(r2f32p(r.x0, r.y0-half_thickness, r.x1, r.y0+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideBottom)
                    {
                        d_rect(r2f32p(r.x0, r.y1-half_thickness, r.x1, r.y1+half_thickness), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideLeft)
                    {
                        d_rect(r2f32p(r.x0-half_thickness, r.y0, r.x0+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                    if(b->flags & UI_BoxFlag_DrawSideRight)
                    {
                        d_rect(r2f32p(r.x1-half_thickness, r.y0, r.x1+half_thickness, r.y1), b->palette->colors[UI_ColorCode_Border], 0, 0, softness);
                    }
                }

                // rjf: draw focus overlay
                if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusOverlay) && b->focus_hot_t > 0.01f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Focus);
                    color.w *= 0.2f*b->focus_hot_t;
                    R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 0.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: draw focus border
                if(b->flags & UI_BoxFlag_Clickable && !(b->flags & UI_BoxFlag_DisableFocusBorder) && b->focus_active_t > 0.01f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_Focus);
                    color.w *= b->focus_active_t;
                    R_Rect2DInst *inst = d_rect(pad_2f32(b->rect, 0.f), color, 0, 1.f, 1.f);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // rjf: disabled overlay
                if(b->disabled_t >= 0.005f)
                {
                    Vec4F32 color = rk_rgba_from_theme_color(RK_ThemeColor_DisabledOverlay);
                    color.w *= b->disabled_t;
                    R_Rect2DInst *inst = d_rect(b->rect, color, 0, 0, 1);
                    MemoryCopyArray(inst->corner_radii, b->corner_radii);
                }

                // k: pop transparency
                if(b->transparency != 0)
                {
                    d_pop_transparency();
                }
            }
        }
        box = rec.next;
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

    //- Render initialization
    r_init((char *)window_title.str, window, true);
    R_Handle wnd = r_window_equip(window);

    /////////////////////////////////
    //~ Game loop

    Arena *frame_arena = arena_alloc();

    //- Time delta
    // TODO: using moving average to smooth out the value
    U64 frame_dt_us  = 0;
    U64 frame_us     = os_now_microseconds();
    U64 cpu_start_us = 0;
    U64 cpu_end_us   = 0;
    U64 cpu_dt_us    = 0;
    U64 gpu_start_us = 0;
    U64 gpu_end_us   = 0;
    U64 gpu_dt_us    = 0;

    //- User interaction
    Vec2F32 mouse = {0};
    char temp_str[512];
    OS_EventList events;

    //- Window
    Rng2F32 window_rect = {0};

    UI_State *ui = ui_state_alloc();
    ui_select_state(ui);

    //~ Init game state
    rk_init(window);
    //- Load functions
    for(U64 i = 0; i < ArrayCount(rk_scenes_fn_table); i++)
    {
        rk_register_function(rk_scenes_fn_table[i].name, rk_scenes_fn_table[i].ptr);
    }
    // G_Scene *default_scene = g_scene_from_file(str8_lit("./src/game/scenes/default.scene"));
    RK_Scene *default_scene = rk_default_scene();
    rk_state->active_scene = default_scene;

    B32 window_should_close = 0;
    while(!window_should_close)
    {
        ProfTick();
        ProfBegin("frame");
        rk_state->debug.frame_dt_us = frame_dt_us;
        rk_state->debug.cpu_dt_us = cpu_dt_us;
        rk_state->debug.gpu_dt_us = gpu_dt_us;

        frame_dt_us = os_now_microseconds()-frame_us;
        frame_us = os_now_microseconds();

        //- k: begin of frame
        r_begin_frame();
        U64 id = r_window_begin_frame(window, wnd);
        gpu_end_us = os_now_microseconds();
        gpu_dt_us = (gpu_end_us - gpu_start_us);
        cpu_start_us = os_now_microseconds();
        d_begin_frame();

        //- Poll events
        events = os_get_events(frame_arena, 0);

        // TODO: don't need to fetch rect every frame
        window_rect = os_client_rect_from_window(window);
        mouse = os_mouse_from_window(window);

        /////////////////////////////////
        // Create the top bucket (for UI)

        // D_Bucket *main_ui_bucket = d_bucket_make();
        // d_push_bucket(main_ui_bucket);

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
                case OS_EventKind_FileDrop:    {kind = UI_EventKind_FileDrop;}break;
            }

            ui_evt.kind         = kind;
            ui_evt.key          = os_evt->key;
            ui_evt.modifiers    = os_evt->modifiers;
            ui_evt.string       = os_evt->character ? str8_from_32(ui_build_arena(), str32(&os_evt->character, 1)) : str8_zero();
            ui_evt.pos          = os_evt->pos;
            ui_evt.delta_2f32   = os_evt->delta;
            ui_evt.timestamp_us = os_evt->timestamp_us;

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


        // Begin build ui
        {
            // Gather font info
            F_Tag main_font = rk_font_from_slot(RK_FontSlot_Main);
            F32 main_font_size = rk_font_size_from_slot(RK_FontSlot_Main);
            F_Tag icon_font = rk_font_from_slot(RK_FontSlot_Icons);

            // Build icon info
            UI_IconInfo icon_info = {0};
            {
                icon_info.icon_font = icon_font;
                icon_info.icon_kind_text_map[UI_IconKind_RightArrow]     = rk_icon_kind_text_table[RK_IconKind_RightScroll];
                icon_info.icon_kind_text_map[UI_IconKind_DownArrow]      = rk_icon_kind_text_table[RK_IconKind_DownScroll];
                icon_info.icon_kind_text_map[UI_IconKind_LeftArrow]      = rk_icon_kind_text_table[RK_IconKind_LeftScroll];
                icon_info.icon_kind_text_map[UI_IconKind_UpArrow]        = rk_icon_kind_text_table[RK_IconKind_UpScroll];
                icon_info.icon_kind_text_map[UI_IconKind_RightCaret]     = rk_icon_kind_text_table[RK_IconKind_RightCaret];
                icon_info.icon_kind_text_map[UI_IconKind_DownCaret]      = rk_icon_kind_text_table[RK_IconKind_DownCaret];
                icon_info.icon_kind_text_map[UI_IconKind_LeftCaret]      = rk_icon_kind_text_table[RK_IconKind_LeftCaret];
                icon_info.icon_kind_text_map[UI_IconKind_UpCaret]        = rk_icon_kind_text_table[RK_IconKind_UpCaret];
                icon_info.icon_kind_text_map[UI_IconKind_CheckHollow]    = rk_icon_kind_text_table[RK_IconKind_CheckHollow];
                icon_info.icon_kind_text_map[UI_IconKind_CheckFilled]    = rk_icon_kind_text_table[RK_IconKind_CheckFilled];
            }

            UI_WidgetPaletteInfo widget_palette_info = {0};
            {
                widget_palette_info.tooltip_palette   = rk_palette_from_code(RK_PaletteCode_Floating);
                widget_palette_info.ctx_menu_palette  = rk_palette_from_code(RK_PaletteCode_Floating);
                widget_palette_info.scrollbar_palette = rk_palette_from_code(RK_PaletteCode_ScrollBarButton);
            }

            // Build animation info
            UI_AnimationInfo animation_info = {0};
            {
                animation_info.flags |= UI_AnimationInfoFlag_HotAnimations;
                animation_info.flags |= UI_AnimationInfoFlag_ActiveAnimations;
                animation_info.flags |= UI_AnimationInfoFlag_FocusAnimations;
                animation_info.flags |= UI_AnimationInfoFlag_TooltipAnimations;
                animation_info.flags |= UI_AnimationInfoFlag_ContextMenuAnimations;
                animation_info.flags |= UI_AnimationInfoFlag_ScrollingAnimations;
            }

            // Begin & push initial stack values
            ui_begin_build(window, &ui_events, &icon_info, &widget_palette_info, &animation_info, frame_dt_us / 1000000.0f);

            ui_push_font(main_font);
            ui_push_font_size(main_font_size);
            // ui_push_text_raster_flags(...);
            ui_push_text_padding(main_font_size*0.2f);
            ui_push_pref_width(ui_em(20.f, 1.f));
            ui_push_pref_height(ui_em(1.35f, 1.f));
            ui_push_palette(rk_palette_from_code(RK_PaletteCode_Base));
        }

        /////////////////////////////////
        //~ Draw game

        rk_frame(rk_state->active_scene, events, frame_dt_us, id);
        ui_end_build();

        /////////////////////////////////
        //~ Draw ui

        D_BucketScope(rk_state->bucket_rect)
        {
            ui_draw(window);
            R_Rect2DInst *cursor = d_rect(r2f32p(mouse.x-15,mouse.y-15, mouse.x+15,mouse.y+15), v4f32(0,0.3,1,0.3), 15, 0.0, 0.7);
        }
        // geo3d => rect in this order
        d_push_bucket(rk_state->bucket_geo3d);
        d_sub_bucket(rk_state->bucket_rect);

        /////////////////////////////////
        //~ End of frame

        gpu_start_us = os_now_microseconds();
        d_submit_bucket(window, wnd, d_top_bucket(), mouse);
        r_window_end_frame(window, wnd);
        r_end_frame();
        arena_clear(frame_arena);

        cpu_end_us = os_now_microseconds();
        cpu_dt_us = cpu_end_us - cpu_start_us;
        ProfEnd();
    }

    /////////////////////////////////
    //~ Cleanup

    os_window_close(window);
}
