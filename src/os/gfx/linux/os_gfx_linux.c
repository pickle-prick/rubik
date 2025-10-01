// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

// window
internal OS_LNX_Window *
os_lnx_window_from_x11window(Window window)
{
  OS_LNX_Window *result = 0;
  for(OS_LNX_Window *w = os_lnx_gfx_state->first_window; w != 0; w = w->next)
  {
    if(w->window == window)
    {
      result = w;
      break;
    }
  }
  return result;
}

internal OS_LNX_Window *
os_lnx_window_from_handle(OS_Handle window)
{
  return (OS_LNX_Window *)window.u64[0];
}

internal Window
os_lnx_x11window_from_handle(OS_Handle window)
{
  OS_LNX_Window *lnx_window = os_lnx_window_from_handle(window);
  return lnx_window->window;
}

// internal void
// os_lnx_get_selection_mime_type()
// {
//   Atom targets = XInternAtom(os_lnx_gfx_state->display, "TARGETS", False);
//   Atom clipboard = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", False);
//   Atom prop = XInternAtom(os_lnx_gfx_state->display, "XSEL_FORMAT", False);
//   Window w = os_lnx_gfx_state->first_window->window;

//   // ask for TARGETS (what formats are avaiable)
//   Display *d = os_lnx_gfx_state->display;
//   XConvertSelection(os_lnx_gfx_state->display, clipboard, targets, prop, w, CurrentTime);

//   // wait for SelectionNotify event
//   XEvent ev;
//   for(; XPending(d) > 0;)
//   {
//     XNextEvent(d, &ev);
//     if(ev.type == SelectionNotify && ev.xselection.selection == clipboard)
//     {
//       if(ev.xselection.property)
//       {
//         Atom actual_type;
//         int actual_format;
//         unsigned long nitems, bytes_after;
//         unsigned char *data = NULL;
//         XGetWindowProperty(d, w, prop, 0, (~0L), False,
//                            AnyPropertyType, &actual_type, &actual_format,
//                            &nitems, &bytes_after, &data);

//         // char *type_name = XGetAtomName(os_lnx_gfx_state->display, actual_type);
//         // printf("Actual type: %s\n", type_name ? type_name : "(null)");
//         // printf("Format: %d bits\n", actual_format);
//         // printf("Items: %lu\n", nitems);
//         // printf("Bytes left: %lu\n", bytes_after);

//         if(actual_type == XA_ATOM && actual_format == 32)
//         {
//           Atom *atoms = (Atom*)data;
//           printf("Available TARGETS (%lu):\n", nitems);
//           for(U64 i = 0; i < nitems; i++)
//           {
//             char *name = XGetAtomName(os_lnx_gfx_state->display, atoms[i]);
//             if(name)
//             {
//               printf(":  %s\n", name);
//               XFree(name);
//             }
//           }
//         }
//         else
//         {
//           printf("Clipboard did not return TARGETS list\n");
//         }

//         if(data)
//         {
//           XFree(data);
//         }
//       }
//       else
//       {
//         // no clipboard data available
//         printf("No clipboard data\n");
//       }
//       break;
//     }
//   }
// }

// clipboard
internal String8
os_lnx_get_selection_data(Arena *arena, Atom selection_type, char *mime_type)
{
#if 1
  String8 ret = {0};

  // unpack stuffs
  Display *d = os_lnx_gfx_state->display;
  Window w = os_lnx_gfx_state->first_window->window;
  Window owner;

  Atom XA_MIME = XInternAtom(d, mime_type, False);
  Atom selection = os_lnx_gfx_state->selection_data_atom;
  Atom seln_type;
  int seln_format; // in bits
  unsigned long count;
  unsigned long overflow;
  unsigned char *src = 0;

  // get the window that holds the selection
  owner = XGetSelectionOwner(d, selection_type);

  if(owner == None)
  {
    // Note(k): this requires a fallback to ancient X10 cut-buffers. We will just skip those for now
  }
  else if(owner == w)
  {
    // TODO(k): we can use callback to retrieve the data
  }
  else
  {
    // request that the selection owner copy the data to our window 
    XConvertSelection(d, selection_type, XA_MIME, selection, w, CurrentTime);
    B32 can_convert = 1;
    // U64 wait_start = os_now_microseconds();
    XEvent ev;
    do
    {
      // U64 wait_elapsed = os_now_microseconds() - wait_start;
      // // NOTE(k): we won't get a reponse if the owner can't convert it to XA_MIME
      // // wait one second for a selection response
      // if(wait_elapsed > 1e6)
      // {
      //   can_convert = 0;
      //   break;
      // }
      XNextEvent(d, &ev);
    } while(ev.type != SelectionNotify || ev.xselection.selection != selection_type);

    if(can_convert && ev.xselection.property)
    {
      if(XGetWindowProperty(d, w, selection, 0, INT_MAX/4, False, XA_MIME, &seln_type, &seln_format, &count, &overflow, &src) == Success)
      {
        if(seln_type == XA_MIME)
        {
          ret = push_str8_copy(arena, str8((U8*)src, count));
        }
        else if(seln_type == os_lnx_gfx_state->atom.INCR)
        {
          Temp scratch = scratch_begin(&arena, 1);
          U8 *dst = 0;
          U64 dst_bytes = 0;
          while(1)
          {
            // k: only delete the property after being done with the previous "chunk"
            XDeleteProperty(d, w, selection);
            XFlush(d);

            do
            {
              XNextEvent(d, &ev);
            } while(ev.type != PropertyNotify || ev.xproperty.atom != selection || ev.xproperty.state != PropertyNewValue);

            XFree(src);
            if(XGetWindowProperty(d, w, selection, 0, INT_MAX/4, False, XA_MIME, &seln_type, &seln_format, &count, &overflow, &src) != Success)
            {
              break;
            }

            // no more value
            if(count == 0)
            {
              break;
            }

            U8 *dst_next = push_array(scratch.arena, U8, dst_bytes+count);
            MemoryCopy(dst_next, dst, dst_bytes);
            MemoryCopy(dst_next+dst_bytes, src, count);
            dst_bytes += count;
            dst = dst_next;
          }

          // reading has data -> copy to ret
          if(dst_bytes > 0)
          {
            ret = push_str8_copy(arena, str8((U8*)dst, dst_bytes));
          }

          scratch_end(scratch);
        }
        XFree(src);
      }
    }
    else
    {
      // NOTE(Next): cannot convert?
    }
  }
  return ret;
#else
  String8 ret = {0};

  // unpack stuffs
  Display *d = os_lnx_gfx_state->display;
  Window w = os_lnx_gfx_state->first_window->window;

  // atoms
  Atom fmtid_src = XInternAtom(d, mime_type, False);
  Atom bufid = selection_type;
  Atom fmtid = fmtid_src;
  Atom propid = os_lnx_gfx_state->selection_data_atom;
  Atom incrid = os_lnx_gfx_state->atom.INCR;

  XEvent ev;
  XConvertSelection(d, bufid, fmtid, propid, w, CurrentTime);
  do
  {
    XNextEvent(d, &ev);
  } while(ev.type != SelectionNotify || ev.xselection.selection != bufid);

  char *result;
  unsigned long ressize, restail;
  int resbits;
  if(ev.xselection.property)
  {
    XGetWindowProperty(d, w, propid, 0, LONG_MAX/4, True, AnyPropertyType,
                       &fmtid, &resbits, &ressize, &restail, (unsigned char**)&result);

    // oneshot, no need to stream
    if(fmtid == fmtid_src && fmtid != incrid)
    {
      U64 bytes = (resbits/8)*ressize;
      String8 src = str8((U8*)result, bytes);
      ret = push_str8_copy(arena, src);
    }
    XFree(result);

    // streaming
    if(fmtid == incrid)
    {
      Temp scratch = scratch_begin(&arena, 1);
      U8 *dst = 0;
      U64 dst_bytes = 0;

      do
      {
        do
        {
          XNextEvent(d, &ev);
        } while(ev.type != PropertyNotify || ev.xproperty.atom != propid || ev.xproperty.state != PropertyNewValue);

        XGetWindowProperty(d, w, propid, 0, LONG_MAX/4, True, AnyPropertyType,
                           &fmtid, &resbits, &ressize, &restail, (unsigned char**)&result);

        U64 chunk_bytes = (resbits/8)*ressize;
        if(chunk_bytes > 0)
        {
          U8 *dst_new = push_array(scratch.arena, U8, dst_bytes+chunk_bytes);
          MemoryCopy(dst_new, dst, dst_bytes);
          MemoryCopy(dst_new+dst_bytes, result, chunk_bytes);
          dst_bytes += chunk_bytes;
          dst = dst_new;
        }

        XFree(result);
      } while(ressize > 0);

      // reading finished -> copy to ret
      if(dst_bytes > 0)
      {
        String8 src = str8(dst, dst_bytes);
        ret = push_str8_copy(arena, src);
      }
      scratch_end(scratch);
    }
  }
  else
  {
    // request failed, e.g. owner can't convert to the target format
  }
  return ret;
#endif
}

////////////////////////////////
//~ rjf: @os_hooks Main Initialization API (Implemented Per-OS)

internal void
os_gfx_init(void)
{
  //- rjf: initialize basics
  Arena *arena = arena_alloc();
  os_lnx_gfx_state = push_array(arena, OS_LNX_GfxState, 1);
  os_lnx_gfx_state->arena = arena;
  os_lnx_gfx_state->display = XOpenDisplay(0);
  
  //- rjf: calculate atoms
  os_lnx_gfx_state->wm_delete_window_atom        = XInternAtom(os_lnx_gfx_state->display, "WM_DELETE_WINDOW", 0);
  os_lnx_gfx_state->wm_sync_request_atom         = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST", 0);
  os_lnx_gfx_state->wm_sync_request_counter_atom = XInternAtom(os_lnx_gfx_state->display, "_NET_WM_SYNC_REQUEST_COUNTER", 0);
  os_lnx_gfx_state->selection_data_atom          = XInternAtom(os_lnx_gfx_state->display, "XSEL_DATA", 0);
  os_lnx_gfx_state->atom.INCR                    = XInternAtom(os_lnx_gfx_state->display, "INCR", 0);
  os_lnx_gfx_state->atom.CLIPBOARD               = XInternAtom(os_lnx_gfx_state->display, "CLIPBOARD", 0);
  
  //- rjf: open im
  os_lnx_gfx_state->xim = XOpenIM(os_lnx_gfx_state->display, 0, 0, 0);
  
  //- rjf: fill out gfx info
  os_lnx_gfx_state->gfx_info.double_click_time = 0.25f;
  os_lnx_gfx_state->gfx_info.caret_blink_time = 0.5f;
  os_lnx_gfx_state->gfx_info.default_refresh_rate = 60.f;
  
  //- rjf: fill out cursors
  {
    struct
    {
      OS_Cursor cursor;
      unsigned int id;
    }
    map[] =
    {
      {OS_Cursor_Pointer,         XC_left_ptr},
      {OS_Cursor_IBar,            XC_xterm},
      {OS_Cursor_LeftRight,       XC_sb_h_double_arrow},
      {OS_Cursor_UpDown,          XC_sb_v_double_arrow},
      {OS_Cursor_DownRight,       XC_bottom_right_corner},
      {OS_Cursor_DownLeft,        XC_bottom_left_corner},
      {OS_Cursor_UpRight,         XC_top_right_corner},
      {OS_Cursor_UpLeft,          XC_top_left_corner},
      {OS_Cursor_UpDownLeftRight, XC_fleur},
      {OS_Cursor_HandPoint,       XC_hand1},
      {OS_Cursor_Disabled,        XC_X_cursor},
    };
    for EachElement(idx, map)
    {
      os_lnx_gfx_state->cursors[map[idx].cursor] = XCreateFontCursor(os_lnx_gfx_state->display, map[idx].id);
    }
  }
}

////////////////////////////////
//~ rjf: @os_hooks Graphics System Info (Implemented Per-OS)

internal OS_GfxInfo *
os_get_gfx_info(void)
{
  return &os_lnx_gfx_state->gfx_info;
}

////////////////////////////////
//~ rjf: @os_hooks Clipboards (Implemented Per-OS)

internal void
os_set_clipboard_text(String8 string)
{
  
}

internal String8
os_get_clipboard_text(Arena *arena)
{
  String8 ret = os_lnx_get_selection_data(arena, os_lnx_gfx_state->atom.CLIPBOARD, "UTF8_STRING");
  return ret;
}

internal String8
os_get_clipboard_image(Arena *arena)
{
  String8 ret = os_lnx_get_selection_data(arena, os_lnx_gfx_state->atom.CLIPBOARD, "image/png");
  return ret;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Rng2F32 rect, OS_WindowFlags flags, String8 title)
{
  Vec2F32 resolution = dim_2f32(rect);

  //- rjf: allocate window
  OS_LNX_Window *w = os_lnx_gfx_state->free_window;
  if(w)
  {
    SLLStackPop(os_lnx_gfx_state->free_window);
  }
  else
  {
    w = push_array_no_zero(os_lnx_gfx_state->arena, OS_LNX_Window, 1);
  }
  MemoryZeroStruct(w);
  DLLPushBack(os_lnx_gfx_state->first_window, os_lnx_gfx_state->last_window, w);

  //- rjf: create window & equip with x11 info
  w->window = XCreateWindow(os_lnx_gfx_state->display,
      XDefaultRootWindow(os_lnx_gfx_state->display),
      0, 0, resolution.x, resolution.y,
      0,
      CopyFromParent,
      InputOutput,
      CopyFromParent,
      0,
      0);
  XSelectInput(os_lnx_gfx_state->display, w->window,
               ExposureMask|
               PointerMotionMask|
               ButtonPressMask|
               ButtonReleaseMask|
               KeyPressMask|
               KeyReleaseMask|
               StructureNotifyMask| // for detecting ConfigureNotify (window resizing)
               PropertyChangeMask| // for clipboard data streaming
               FocusChangeMask);
  Atom protocols[] =
  {
    os_lnx_gfx_state->wm_delete_window_atom,
    os_lnx_gfx_state->wm_sync_request_atom,
  };
  XSetWMProtocols(os_lnx_gfx_state->display, w->window, protocols, ArrayCount(protocols));
  {
    XSyncValue initial_value;
    XSyncIntToValue(&initial_value, 0);
    w->counter_xid = XSyncCreateCounter(os_lnx_gfx_state->display, initial_value);
  }
  XChangeProperty(os_lnx_gfx_state->display, w->window, os_lnx_gfx_state->wm_sync_request_counter_atom, XA_CARDINAL, 32, PropModeReplace, (U8 *)&w->counter_xid, 1);

  //- rjf: create xic
  w->xic = XCreateIC(os_lnx_gfx_state->xim,
      XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
      XNClientWindow, w->window,
      XNFocusWindow, w->window,
      NULL);

  //- rjf: attach name
  Temp scratch = scratch_begin(0, 0);
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);

  //- rjf: convert to handle & return
  OS_Handle handle = {(U64)w};
  return handle;
}

internal void
os_window_close(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XDestroyWindow(os_lnx_gfx_state->display, w->window);
}

internal void
os_window_set_title(OS_Handle handle, String8 title)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  Temp scratch = scratch_begin(0, 0);
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  String8 title_copy = push_str8_copy(scratch.arena, title);
  XStoreName(os_lnx_gfx_state->display, w->window, (char *)title_copy.str);
  scratch_end(scratch);
}

internal void
os_window_first_paint(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XMapWindow(os_lnx_gfx_state->display, w->window);
}

internal void
os_window_equip_repaint(OS_Handle handle, OS_WindowRepaintFunctionType *repaint, void *user_data)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_focus(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal B32
os_window_is_focused(OS_Handle handle)
{
#if 1
  B32 ret = 0;
  Assert(!os_handle_match(handle, os_handle_zero()));
  OS_LNX_Window *window = os_lnx_window_from_handle(handle);
  if(window)
  {
    ret = !window->focus_out;
  }
  return ret;
#else
  ProfBeginFunction();
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  Window focused_window = 0;
  int revert_to = 0;
  XGetInputFocus(os_lnx_gfx_state->display, &focused_window, &revert_to);
  B32 result = (w->window == focused_window);
  ProfEnd();
  return result;
#endif
}

internal B32
os_window_is_fullscreen(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  return 0;
}

internal void
os_window_set_fullscreen(OS_Handle handle, B32 fullscreen)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal B32
os_window_is_maximized(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return 0;}
  return 0;
}

internal void
os_window_set_maximized(OS_Handle handle, B32 maximized)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_minimize(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_bring_to_front(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_set_monitor(OS_Handle handle, OS_Handle monitor)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_clear_custom_border_data(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_title_bar(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_edges(OS_Handle handle, F32 thickness)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal void
os_window_push_custom_title_bar_client_area(OS_Handle handle, Rng2F32 rect)
{
  if(os_handle_match(handle, os_handle_zero())) {return;}
}

internal Rng2F32
os_rect_from_window(OS_Handle handle)
{
  if(os_handle_match(handle, os_handle_zero())) {return r2f32p(0, 0, 0, 0);}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(os_lnx_gfx_state->display, w->window, &atts);
  Rng2F32 result = r2f32p((F32)atts.x, (F32)atts.y, (F32)atts.x + (F32)atts.width, (F32)atts.y + (F32)atts.height);
  return result;
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle, B32 forced)
{
  ProfBeginFunction();
#if 0
  // TODO: this will causing a performace spike sometimes like 16ms what fuck?
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  XWindowAttributes atts = {0};
  Status s = XGetWindowAttributes(os_lnx_gfx_state->display, w->window, &atts);
  Rng2F32 result = r2f32p(0, 0, (F32)atts.width, (F32)atts.height);
#else
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
  Rng2F32 result = {0,0, w->client_dim.x, w->client_dim.y};
  if(forced)
  {
    // TODO: this will also causing a performace spike sometimes like 16ms what fuck?
    Window root;
    int x_return,y_return;
    unsigned int width_return,height_return,border_width_return,depth_return;
    XGetGeometry(os_lnx_gfx_state->display, w->window,
                 &root, &x_return,&y_return, &width_return,&height_return,
                 &border_width_return, &depth_return);
    result = r2f32p(0, 0, (F32)width_return, (F32)height_return);
  }
#endif
  ProfEnd();
  return result;
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
  ProfBeginFunction();
  F32 dpi = 0.0;

  // Trey "Xft.dpi" from the XResourceDatabase...
  if(dpi <= 0.0)
  {
    char *resource_manager; 
    XrmDatabase db;
    XrmValue value;
    char *type;

    XrmInitialize();

    resource_manager = XResourceManagerString(os_lnx_gfx_state->display);
    if(resource_manager)
    {
      db = XrmGetStringDatabase(resource_manager);

      // Get the vlue of Xft.dpi from the Database
      if(XrmGetResource(db, "Xft.dpi", "String", &type, &value))
      {
        if(value.addr && type && strcmp(type, "String") == 0)
        {
          dpi = atof(value.addr);
        }
      }
      XrmDestroyDatabase(db);
    }
  }

  // If that failed, try the XSETTINGS keys
  // TODO

  // If that failed, try the GDK_SCALE envvar...
  // TODO

  // If that failed, calculate dpi using DisplayWidth/DisplayWidthMM/25.4f
  if(dpi <= 0.0)
  {
    // TODO(k): handle multiple monitors
    int screen = DefaultScreen(os_lnx_gfx_state->display);

    // screen dimensions in pixels
    int width_px = DisplayWidth(os_lnx_gfx_state->display, screen);
    int height_px = DisplayHeight(os_lnx_gfx_state->display, screen);

    // screen dimensions in millimeters
    int width_mm = DisplayWidthMM(os_lnx_gfx_state->display, screen);
    int height_mm = DisplayHeightMM(os_lnx_gfx_state->display, screen);

    // calculate DPI
    F32 dpi_x = ((F32)width_px / (width_mm / 25.4));
    F32 dpi_y = ((F32)height_px / (height_mm / 25.4));

    dpi = (dpi_x+dpi_y) / 2.f;
  }

  // Nothing or a bad value, just fall back to 1.0
  if(dpi <= 0.0)
  {
    dpi = 96.f;
  }

  ProfEnd();
  return dpi;
}

////////////////////////////////
//~ rjf: @os_hooks Monitors (Implemented Per-OS)

internal OS_HandleArray
os_push_monitors_array(Arena *arena)
{
  OS_HandleArray result = {0};
  return result;
}

internal OS_Handle
os_primary_monitor(void)
{
  OS_Handle result = {0};
  return result;
}

internal OS_Handle
os_monitor_from_window(OS_Handle window)
{
  OS_Handle result = {0};
  return result;
}

internal String8
os_name_from_monitor(Arena *arena, OS_Handle monitor)
{
  return str8_zero();
}

internal Vec2F32
os_dim_from_monitor(OS_Handle monitor)
{
  return v2f32(0, 0);
}

////////////////////////////////
//~ rjf: @os_hooks Events (Implemented Per-OS)

internal void
os_send_wakeup_event(void)
{
  
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait)
{
  ProfBeginFunction();
  B32 set_mouse_cursor = 0;
  OS_EventList evts = {0};
  for(;XPending(os_lnx_gfx_state->display) > 0 || (wait && evts.count == 0);)
  {
    XEvent evt = {0};
    XNextEvent(os_lnx_gfx_state->display, &evt);
    switch(evt.type)
    {
      default:{}break;
      //- rjf: key presses/releases
      case KeyPress:
      case KeyRelease:
      {
        // rjf: determine flags
        OS_Modifiers modifiers = 0;
        if(evt.xkey.state & ShiftMask)   { modifiers |= OS_Modifier_Shift; }
        if(evt.xkey.state & ControlMask) { modifiers |= OS_Modifier_Ctrl; }
        if(evt.xkey.state & Mod1Mask)    { modifiers |= OS_Modifier_Alt; }
        
        // rjf: map keycode -> keysym & codepoint
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xkey.window);
        KeySym keysym = 0;
        U8 text[256] = {0};
        // NOTE(k): text_size will be 0 if ControlMask is maksed
        U64 text_size = Xutf8LookupString(window->xic, &evt.xkey, (char *)text, sizeof(text), &keysym, 0);
        // NOTE(k): Xutf8LookupString won't produce keysym on keyPress (should it be keyRelease?)
        if(keysym == 0) keysym = XLookupKeysym(&evt.xkey, 0);
        
        // rjf: map keysym -> OS_Key
        B32 is_right_sided = 0;
        OS_Key key = OS_Key_Null;
        switch(keysym)
        {
          default:
          {
            if(0){}
            else if(XK_F1 <= keysym && keysym <= XK_F24) { key = (OS_Key)(OS_Key_F1 + (keysym - XK_F1)); }
            else if('0' <= keysym && keysym <= '9')      { key = OS_Key_0 + (keysym-'0'); }
          }break;
          case XK_Escape:{key = OS_Key_Esc;};break;
          case XK_BackSpace:{key = OS_Key_Backspace;}break;
          case XK_Delete:{key = OS_Key_Delete;}break;
          case XK_Return:{key = OS_Key_Return;}break;
          case XK_Pause:{key = OS_Key_Pause;}break;
          case XK_Tab:{key = OS_Key_Tab;}break;
          case XK_Left:{key = OS_Key_Left;}break;
          case XK_Right:{key = OS_Key_Right;}break;
          case XK_Up:{key = OS_Key_Up;}break;
          case XK_Down:{key = OS_Key_Down;}break;
          case XK_Home:{key = OS_Key_Home;}break;
          case XK_End:{key = OS_Key_End;}break;
          case XK_Page_Up:{key = OS_Key_PageUp;}break;
          case XK_Page_Down:{key = OS_Key_PageDown;}break;
          case XK_Alt_L:{ key = OS_Key_Alt; }break;
          case XK_Alt_R:{ key = OS_Key_Alt; is_right_sided = 1;}break;
          case XK_Shift_L:{ key = OS_Key_Shift; }break;
          case XK_Shift_R:{ key = OS_Key_Shift; is_right_sided = 1;}break;
          case XK_Control_L:{ key = OS_Key_Ctrl; }break;
          case XK_Control_R:{ key = OS_Key_Ctrl; is_right_sided = 1;}break;
          case '-':{key = OS_Key_Minus;}break;
          case '=':{key = OS_Key_Equal;}break;
          case '[':{key = OS_Key_LeftBracket;}break;
          case ']':{key = OS_Key_RightBracket;}break;
          case ';':{key = OS_Key_Semicolon;}break;
          case '\'':{key = OS_Key_Quote;}break;
          case '.':{key = OS_Key_Period;}break;
          case ',':{key = OS_Key_Comma;}break;
          case '/':{key = OS_Key_Slash;}break;
          case '\\':{key = OS_Key_BackSlash;}break;
          case '\t':{key = OS_Key_Tab;}break;
          case '`':{key = OS_Key_Tick;}break;
          case 'a':case 'A':{key = OS_Key_A;}break;
          case 'b':case 'B':{key = OS_Key_B;}break;
          case 'c':case 'C':{key = OS_Key_C;}break;
          case 'd':case 'D':{key = OS_Key_D;}break;
          case 'e':case 'E':{key = OS_Key_E;}break;
          case 'f':case 'F':{key = OS_Key_F;}break;
          case 'g':case 'G':{key = OS_Key_G;}break;
          case 'h':case 'H':{key = OS_Key_H;}break;
          case 'i':case 'I':{key = OS_Key_I;}break;
          case 'j':case 'J':{key = OS_Key_J;}break;
          case 'k':case 'K':{key = OS_Key_K;}break;
          case 'l':case 'L':{key = OS_Key_L;}break;
          case 'm':case 'M':{key = OS_Key_M;}break;
          case 'n':case 'N':{key = OS_Key_N;}break;
          case 'o':case 'O':{key = OS_Key_O;}break;
          case 'p':case 'P':{key = OS_Key_P;}break;
          case 'q':case 'Q':{key = OS_Key_Q;}break;
          case 'r':case 'R':{key = OS_Key_R;}break;
          case 's':case 'S':{key = OS_Key_S;}break;
          case 't':case 'T':{key = OS_Key_T;}break;
          case 'u':case 'U':{key = OS_Key_U;}break;
          case 'v':case 'V':{key = OS_Key_V;}break;
          case 'w':case 'W':{key = OS_Key_W;}break;
          case 'x':case 'X':{key = OS_Key_X;}break;
          case 'y':case 'Y':{key = OS_Key_Y;}break;
          case 'z':case 'Z':{key = OS_Key_Z;}break;
          case ' ':{key = OS_Key_Space;}break;
        }

        // k: cache key down/up
#if 0
        B32 cache_key_press = 1;
        if(evt.type == KeyRelease)
        {
          if(XEventsQueued(os_lnx_gfx_state->display, QueuedAfterReading))
          {
            XEvent nextev;
            XPeekEvent(os_lnx_gfx_state->display, &nextev);
            if(nextev.type == KeyPress &&
               nextev.xkey.keycode == evt.xkey.keycode &&
               nextev.xkey.time == evt.xkey.time)
            {
              cache_key_press = 0;
            }
          }
        }
        if(cache_key_press) os_lnx_gfx_state->keydown_cache[key] = evt.type == KeyPress;
#else
        // NOTE(k): we didn't need to peak the next one, key press would always override keyrelease
        os_lnx_gfx_state->keydown_cache[key] = evt.type == KeyPress;
#endif
        
        // rjf: push text event
        if(evt.type == KeyPress && text_size != 0)
        {
          for(U64 off = 0; off < text_size;)
          {
            UnicodeDecode decode = utf8_decode(text+off, text_size-off);
            if(decode.codepoint != 0 && (decode.codepoint >= 32 || decode.codepoint == '\t'))
            {
              OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Text);
              e->window.u64[0] = (U64)window;
              e->character = decode.codepoint;
            }
            if(decode.inc == 0)
            {
              break;
            }
            off += decode.inc;
          }
        }
        
        // rjf: push key event
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == KeyPress ? OS_EventKind_Press : OS_EventKind_Release);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->key = key;
          e->right_sided = is_right_sided;
        }
      }break;
      //- rjf: mouse button presses/releases
      case ButtonPress:
      case ButtonRelease:
      {
        // rjf: determine flags
        OS_Modifiers modifiers = 0;
        if(evt.xbutton.state & ShiftMask)   { modifiers |= OS_Modifier_Shift; }
        if(evt.xbutton.state & ControlMask) { modifiers |= OS_Modifier_Ctrl; }
        if(evt.xbutton.state & Mod1Mask)    { modifiers |= OS_Modifier_Alt; }
        
        // rjf: map button -> OS_Key
        OS_Key key = OS_Key_Null;
        switch(evt.xbutton.button)
        {
          default:{}break;
          case Button1:{key = OS_Key_LeftMouseButton;}break;
          case Button2:{key = OS_Key_MiddleMouseButton;}break;
          case Button3:{key = OS_Key_RightMouseButton;}break;
        }
        
        // rjf: push event
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xbutton.window);
        if(key != OS_Key_Null)
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == ButtonPress ? OS_EventKind_Press : OS_EventKind_Release);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->key = key;
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }
        else if(evt.xbutton.button == Button4 ||
                evt.xbutton.button == Button5)
        {
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_Scroll);
          e->window.u64[0] = (U64)window;
          e->modifiers = modifiers;
          e->delta = v2f32(0, evt.xbutton.button == Button4 ? -1.f : +1.f);
          e->pos = v2f32((F32)evt.xbutton.x, (F32)evt.xbutton.y);
        }

        // k: cache button down/up
        os_lnx_gfx_state->keydown_cache[key] = evt.type == ButtonPress;
      }break;

      //- rjf: mouse motion
      case MotionNotify:
      {
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
        OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_MouseMove);
        e->window.u64[0] = (U64)window;
        e->pos.x = (F32)evt.xmotion.x;
        e->pos.y = (F32)evt.xmotion.y;
        set_mouse_cursor = 1;
        if(window)
        {
          window->last_mouse.x = (F32)evt.xmotion.x;
          window->last_mouse.y = (F32)evt.xmotion.y;
        }
      }break;

      //- rjf: window focus/unfocus
      case FocusIn:
      case FocusOut:
      {
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
        // TODO(k): window shoule be 0 sometimes, but apparently not, don't know why
        AssertAlways(window != 0);
        window->focus_out = evt.type == FocusOut;
      }break;
      //- k: window reszing
      case ConfigureNotify:
      {
        // printf("width: %d\n", evt.xconfigure.width);
        // printf("height: %d\n", evt.xconfigure.height);
        OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
        window->client_dim.x = evt.xconfigure.width;
        window->client_dim.y = evt.xconfigure.height;
      }break;
      //- rjf: client messages
      case ClientMessage:
      {
        if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_delete_window_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_WindowClose);
          e->window.u64[0] = (U64)window;
        }
        else if((Atom)evt.xclient.data.l[0] == os_lnx_gfx_state->wm_sync_request_atom)
        {
          OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
          if(window != 0)
          {
            window->counter_value = 0;
            window->counter_value |= evt.xclient.data.l[2];
            window->counter_value |= (evt.xclient.data.l[3] << 32);
            XSyncValue value;
            XSyncIntToValue(&value, window->counter_value);
            XSyncSetCounter(os_lnx_gfx_state->display, window->counter_xid, value);
          }
        }
      }break;
    }
  }
  if(set_mouse_cursor) ProfScope("set mouse cursor")
  {
    Window root_window = 0;
    Window child_window = 0;
    int root_rel_x = 0;
    int root_rel_y = 0;
    int child_rel_x = 0;
    int child_rel_y = 0;
    unsigned int mask = 0;
    if(XQueryPointer(os_lnx_gfx_state->display, XDefaultRootWindow(os_lnx_gfx_state->display),
                     &root_window, &child_window,
                     &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
    {
      XDefineCursor(os_lnx_gfx_state->display, child_window, os_lnx_gfx_state->cursors[os_lnx_gfx_state->last_set_cursor]);
      XFlush(os_lnx_gfx_state->display);
    }
  }
  ProfEnd();
  return evts;
}

internal OS_Modifiers
os_get_modifiers(void)
{
  ProfBeginFunction();
  OS_Modifiers modifiers = 0;
  if(os_key_is_down(OS_Key_Shift))
  {
    modifiers |= OS_Modifier_Shift;
  }
  if(os_key_is_down(OS_Key_Ctrl))
  {
    modifiers |= OS_Modifier_Ctrl;
  }
  if(os_key_is_down(OS_Key_Alt))
  {
    modifiers |= OS_Modifier_Alt;
  }
  ProfEnd();
  return modifiers;
}

internal B32
os_key_is_down(OS_Key key)
{
  ProfBeginFunction();
#if 1
  B32 ret = os_lnx_gfx_state->keydown_cache[key];
#else
  // NOTE(k): this is very slow
  char keys_return[32];
  XQueryKeymap(os_lnx_gfx_state->display, keys_return);

  // NOTE(k): we have two kinds of key (mouse/pointer, keyboard)
  B32 is_pointer = 0;
  U32 keysym = 0;
  U32 pointer_btn_mask = 0;
  // TODO: we better write a table for mapping
  switch(key)
  {
    case OS_Key_LeftMouseButton:   {is_pointer = 1; pointer_btn_mask = Button1Mask;}break;
    case OS_Key_MiddleMouseButton: {is_pointer = 1; pointer_btn_mask = Button2Mask;}break;
    case OS_Key_RightMouseButton:  {is_pointer = 1; pointer_btn_mask = Button3Mask;}break;
    case OS_Key_Left:              {keysym = XK_Left;}break;
    case OS_Key_Right:             {keysym = XK_Right;}break;
    case OS_Key_Up:                {keysym = XK_Up;}break;
    case OS_Key_Down:              {keysym = XK_Down;}break;
    case OS_Key_W:                 {keysym = XK_W;}break;
    case OS_Key_S:                 {keysym = XK_S;}break;
    case OS_Key_A:                 {keysym = XK_A;}break;
    case OS_Key_D:                 {keysym = XK_D;}break;
    case OS_Key_Space:             {keysym = XK_space;}break;
    // TODO: not ideal, we don't care left/right
    case OS_Key_Shift:             {keysym = XK_Shift_L;}break;
    case OS_Key_Ctrl:              {keysym = XK_Control_L;}break;
    case OS_Key_Alt:               {keysym = XK_Alt_L;}break;
    default:                       {InvalidPath; }break;
  }

  U32 ret = 0;
  if(!is_pointer)
  {
    KeyCode kc2 = XKeysymToKeycode(os_lnx_gfx_state->display, keysym);
    if(kc2 != NoSymbol)
    {
      ret = (keys_return[kc2 >> 3] & (1 << (kc2 & 7))) != 0;
    }
  }
  else
  {
    Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;
    XQueryPointer(os_lnx_gfx_state->display,
                  DefaultRootWindow(os_lnx_gfx_state->display),
                  &root_return, &child_return,
                  &root_x, &root_y, &win_x, &win_y, &mask_return);
    ret = (mask_return & Button1Mask) != 0;
  }
#endif
  ProfEnd();
  return ret;
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  ProfBeginFunction();
  Vec2F32 result = {0};
  if(os_handle_match(handle, os_handle_zero())) {return v2f32(0, 0);}
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];
#if 0
  {
    // TODO(k): XQueryPointer is a Round-Trip call to the X server, can be really slow
    Window root_window = 0;
    Window child_window = 0;
    int root_rel_x = 0;
    int root_rel_y = 0;
    int child_rel_x = 0;
    int child_rel_y = 0;
    unsigned int mask = 0;
    if(XQueryPointer(os_lnx_gfx_state->display, w->window, &root_window, &child_window, &root_rel_x, &root_rel_y, &child_rel_x, &child_rel_y, &mask))
    {
      result.x = child_rel_x;
      result.y = child_rel_y;
    }
  }
#else
  result.x = w->last_mouse.x;
  result.y = w->last_mouse.y;
#endif
  ProfEnd();
  return result;
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  os_lnx_gfx_state->last_set_cursor = cursor;
}

internal void 
os_hide_cursor(OS_Handle window)
{
  Window wnd = os_lnx_x11window_from_handle(window);
  XFixesHideCursor(os_lnx_gfx_state->display, wnd);
  XSync(os_lnx_gfx_state->display, 0);
}

internal void 
os_show_cursor(OS_Handle window)
{
  Window wnd = os_lnx_x11window_from_handle(window);
  XFixesShowCursor(os_lnx_gfx_state->display, wnd);
  XSync(os_lnx_gfx_state->display, 0);
}

internal void
os_wrap_cursor(OS_Handle window, F32 dst_x, F32 dst_y)
{
  Window wnd = os_lnx_x11window_from_handle(window);
  XWarpPointer(os_lnx_gfx_state->display, 0, wnd, 0,0,0,0, (int)dst_x, (int)dst_y);
  XSync(os_lnx_gfx_state->display, 0);
}

////////////////////////////////
//~ k: @os_hooks Vulkan (Implemented Per-OS)

internal VkSurfaceKHR
os_vulkan_surface_from_window(OS_Handle window, VkInstance instance)
{
  Window lnx_x11window = os_lnx_x11window_from_handle(window);

  VkXlibSurfaceCreateInfoKHR sci = {0};
  PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
  VkSurfaceKHR surface;

  vkCreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
  AssertAlways(vkCreateXlibSurfaceKHR && "X11: Vulkan instance missing VK_KHR_xlib_surface extension");

  sci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
  sci.dpy    = os_lnx_gfx_state->display;
  sci.window = lnx_x11window;

  AssertAlways(vkCreateXlibSurfaceKHR(instance, &sci, NULL, &surface) == VK_SUCCESS && "Failed to create xlib surface");
  return surface;
}

internal char *
os_vulkan_surface_ext()
{
  return "VK_KHR_xlib_surface";
}

////////////////////////////////
//~ rjf: @os_hooks Native User-Facing Graphical Messages (Implemented Per-OS)

internal void
os_graphical_message(B32 error, String8 title, String8 message)
{
  if(error)
  {
    fprintf(stderr, "[X] ");
  }
  fprintf(stderr, "%.*s\n", str8_varg(title));
  fprintf(stderr, "%.*s\n\n", str8_varg(message));
}

internal String8
os_graphical_pick_file(Arena *arena, String8 initial_path)
{
  return str8_zero();
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  // TODO(rjf)
}

internal void
os_open_in_browser(String8 url)
{
  // TODO(rjf)
}

////////////////////////////////
//~ rjf: @os_hooks IME

internal void
os_set_ime_position(OS_Handle window, Vec2S32 position)
{
  // TODO(k)
}
