// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

////////////////////////////////
//~ rjf: Includes

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/sync.h>
#include <X11/extensions/Xfixes.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xresource.h>

////////////////////////////////
//~ rjf: Window State

typedef struct OS_LNX_Window OS_LNX_Window;
struct OS_LNX_Window
{
    OS_LNX_Window *next;
    OS_LNX_Window *prev;
    Window window;
    XID counter_xid;
    U64 counter_value;
    B32 focus_out;
    Vec2F32 client_dim;
};

////////////////////////////////
//~ rjf: State Bundle

typedef struct OS_LNX_GfxState OS_LNX_GfxState;
struct OS_LNX_GfxState
{
    Arena *arena;
    Display *display;
    OS_LNX_Window *first_window;
    OS_LNX_Window *last_window;
    OS_LNX_Window *free_window;
    Atom wm_delete_window_atom;
    Atom wm_sync_request_atom;
    Atom wm_sync_request_counter_atom;
    OS_GfxInfo gfx_info;
    Cursor cursor;
    Cursor next_cursor;
};

////////////////////////////////
//~ rjf: Globals

global OS_LNX_GfxState *os_lnx_gfx_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal OS_LNX_Window *os_lnx_window_from_x11window(Window window);
internal OS_LNX_Window *os_lnx_window_from_handle(OS_Handle window);
internal Window        os_lnx_x11window_from_handle(OS_Handle window);
internal OS_Key        os_lnx_os_key_from_keysym(U32 keysym);

#endif // OS_GFX_LINUX_H
