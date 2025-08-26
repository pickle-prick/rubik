// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef OS_GFX_LINUX_H
#define OS_GFX_LINUX_H

////////////////////////////////
//~ rjf: Includes

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/sync.h>
#include <X11/cursorfont.h>
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
  XIC xic;
  XID counter_xid;
  U64 counter_value;
  B32 focus_out;
  Vec2F32 client_dim;
  Vec2F32 last_mouse;
};

////////////////////////////////
//~ rjf: State Bundle

typedef struct OS_LNX_GfxState OS_LNX_GfxState;
struct OS_LNX_GfxState
{
  Arena *arena;
  Display *display;
  XIM xim;
  OS_LNX_Window *first_window;
  OS_LNX_Window *last_window;
  OS_LNX_Window *free_window;
  Atom wm_delete_window_atom;
  Atom wm_sync_request_atom;
  Atom wm_sync_request_counter_atom;
  Atom selection_data_atom;
  struct
  {
    Atom INCR;
    Atom CLIPBOARD;
  } atom;
  Cursor cursors[OS_Cursor_COUNT];
  OS_Cursor last_set_cursor;
  OS_GfxInfo gfx_info;
  B32 keydown_cache[OS_Key_COUNT];
};

////////////////////////////////
//~ rjf: Globals

global OS_LNX_GfxState *os_lnx_gfx_state = 0;

////////////////////////////////
//~ rjf: Helpers

// window
internal OS_LNX_Window *os_lnx_window_from_x11window(Window window);
internal OS_LNX_Window *os_lnx_window_from_handle(OS_Handle window);
internal Window        os_lnx_x11window_from_handle(OS_Handle window);

// clipboard
internal String8 os_lnx_get_selection_data(Arena *arena, Atom selectionid, char *mime_type);

#endif // OS_GFX_LINUX_H
