// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

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
os_lnx_window_from_handle(OS_Handle window) {
    return (OS_LNX_Window *)window.u64[0];
}

internal Window
os_lnx_x11window_from_handle(OS_Handle window) {
    OS_LNX_Window *lnx_window = os_lnx_window_from_handle(window);
    return lnx_window->window;
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

    //- rjf: fill out gfx info
    os_lnx_gfx_state->gfx_info.double_click_time = 0.2f;
    os_lnx_gfx_state->gfx_info.caret_blink_time = 0.5f;
    os_lnx_gfx_state->gfx_info.default_refresh_rate = 60.f;
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
    String8 result = {0};
    return result;
}

////////////////////////////////
//~ rjf: @os_hooks Windows (Implemented Per-OS)

internal OS_Handle
os_window_open(Vec2F32 resolution, OS_WindowFlags flags, String8 title)
{
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
                 // NOTE(k): for detecting ConfigureNotify (window resizing)
                 StructureNotifyMask|
                 // NOTE(k): for detecting window focus in/out
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
    Assert(!os_handle_match(handle, os_handle_zero()));
    OS_LNX_Window *window = os_lnx_window_from_handle(handle);
    return !window->focus_out;
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
    return r2f32p(0, 0, 0, 0);
}

internal Rng2F32
os_client_rect_from_window(OS_Handle handle, B32 forced)
{
    Rng2F32 rect = {0};
    OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];

    // NOTE(k): cached value is updated by events, in some cased, we need get rect first before any events processing
    if(forced)
    {
        // NOTE(k): this is a slow operation
        if(!os_handle_match(handle, os_handle_zero()))
        {
            Window root;
            int x_return,y_return;
            unsigned int width_return,height_return,border_width_return,depth_return;
            XGetGeometry(os_lnx_gfx_state->display, w->window,
                         &root, &x_return,&y_return, &width_return,&height_return,
                         &border_width_return, &depth_return);
            rect.p0.x = 0;
            rect.p0.y = 0;
            rect.p1.x = width_return;
            rect.p1.y = height_return;
        }
    }
    else
    {
        rect.p0.x = 0;
        rect.p0.y = 0;
        rect.p1.x = w->client_dim.x;
        rect.p1.y = w->client_dim.y;
    }
    return rect;
}

internal F32
os_dpi_from_window(OS_Handle handle)
{
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

internal OS_Key
os_lnx_os_key_from_keysym(U32 keysym) {
    OS_Key key = OS_Key_Null;
    switch(keysym)
    {
        default:
        {
            if(0){}
            else if(XK_F1 <= keysym && keysym <= XK_F24) { key = (OS_Key)(OS_Key_F1 + (keysym - XK_F1)); }
            else if('0' <= keysym && keysym <= '9')      { key = OS_Key_0 + (keysym-'0'); }
        }break;
        case XK_Escape:    {key = OS_Key_Esc;};break;
        case XK_Return:    {key = OS_Key_Return;}break;
        case XK_BackSpace: {key = OS_Key_Backspace;};break;
        case XK_Left:      {key = OS_Key_Left;}break;
        case XK_Right:     {key = OS_Key_Right;}break;
        case XK_Up:        {key = OS_Key_Up;}break;
        case XK_Down:      {key = OS_Key_Down;}break;
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
    return key;
}

internal OS_EventList
os_get_events(Arena *arena, B32 wait) {
    OS_EventList evts = {0};

    for(;XPending(os_lnx_gfx_state->display) > 0 || (wait && evts.count == 0);)
    {
        XEvent evt = {0};
        XNextEvent(os_lnx_gfx_state->display, &evt);

        // Update cursor
        if(os_lnx_gfx_state->cursor != os_lnx_gfx_state->next_cursor)
        {
            os_lnx_gfx_state->cursor = os_lnx_gfx_state->next_cursor;
            OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
            XDefineCursor(os_lnx_gfx_state->display, evt.xclient.window, os_lnx_gfx_state->cursor);
        }

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

                // rjf: map keycode -> keysym
                U32 keysym = XLookupKeysym(&evt.xkey, 0);

                // rjf: map keysym -> OS_Key
                OS_Key key = os_lnx_os_key_from_keysym(keysym);

                // rjf: push event
                OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
                OS_Event *e = os_event_list_push_new(arena, &evts, evt.type == KeyPress ? OS_EventKind_Press : OS_EventKind_Release);
                e->window.u64[0] = (U64)window;
                e->modifiers     = modifiers;
                e->key           = key;

                if(keysym >= 32 && keysym < 127 && e->kind == OS_EventKind_Press && !(e->modifiers & OS_Modifier_Ctrl)) {
                    e->kind      = OS_EventKind_Text;
                    e->character = os_codepoint_from_modifiers_and_key(e->modifiers, e->key);
                }
            }break;

            //- rjf: mouse button presses/releases
            case ButtonPress:
            case ButtonRelease:
            {
                // rjf: determine flags
                OS_Modifiers modifiers = 0;
                if(evt.xkey.state & ShiftMask)   { modifiers |= OS_Modifier_Shift; }
                if(evt.xkey.state & ControlMask) { modifiers |= OS_Modifier_Ctrl; }
                if(evt.xkey.state & Mod1Mask)    { modifiers |= OS_Modifier_Alt; }

                // rjf: map button -> OS_Key
                OS_Key key = OS_Key_Null;
                Vec2F32 delta = {0};
                OS_EventKind kind = evt.type == ButtonPress ? OS_EventKind_Press : OS_EventKind_Release;
                switch(evt.xbutton.button)
                {
                    default:{}break;
                    case Button1:{key = OS_Key_LeftMouseButton;}break;
                    case Button2:{key = OS_Key_MiddleMouseButton;}break;
                    case Button3:{key = OS_Key_RightMouseButton;}break;
                    case Button4:{delta.y = +30.0f; kind = OS_EventKind_Scroll;}break;
                    case Button5:{delta.y = -30.0f; kind = OS_EventKind_Scroll;}break;
                }

                // rjf: push event
                OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
                OS_Event *e = os_event_list_push_new(arena, &evts, kind);
                e->window.u64[0] = (U64)window;
                e->modifiers = modifiers;
                e->key = key;
                e->delta = delta;
                e->pos.x = (F32)evt.xmotion.x;
                e->pos.y = (F32)evt.xmotion.y;
            }break;

            //- rjf: mouse motion
            case MotionNotify:
            {
                OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
                OS_Event *e = os_event_list_push_new(arena, &evts, OS_EventKind_MouseMove);
                e->window.u64[0] = (U64)window;
                e->pos.x = (F32)evt.xmotion.x;
                e->pos.y = (F32)evt.xmotion.y;
            }break;

            //- rjf: window focus/unfocus
            case FocusIn:
            case FocusOut:
            {
                OS_LNX_Window *window = os_lnx_window_from_x11window(evt.xclient.window);
                window->focus_out = evt.type == FocusOut;
            }break;

            //- k: window reszing
            case ConfigureNotify:
            {
                // printf("width: %d\n", evt.xconfigure.width);
                // printf("width: %d\n", evt.xconfigure.height);
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
    return evts;
}

internal OS_Modifiers
os_get_modifiers(void)
{
    return 0;
}

internal B32
os_key_is_down(OS_Key key)
{
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
  return ret;
}

internal Vec2F32
os_mouse_from_window(OS_Handle handle)
{
  Vec2F32 mouse = {0};
  OS_LNX_Window *w = (OS_LNX_Window *)handle.u64[0];

  if (!os_handle_match(handle, os_handle_zero())) {
    Window root_return, child_return;
    int root_x_return,root_y_return;
    int win_x_return,win_y_return;
    unsigned int mask_return;

    XQueryPointer(os_lnx_gfx_state->display, w->window,
                  &root_return, &child_return,
                  &root_x_return, &root_y_return,
                  &win_x_return, &win_y_return, &mask_return);
    mouse.x = win_x_return;
    mouse.y = win_y_return;
  }

  return mouse;
}

////////////////////////////////
//~ rjf: @os_hooks Cursors (Implemented Per-OS)

internal void
os_set_cursor(OS_Cursor cursor)
{
  String8 cursor_name = {0};
  Cursor c;
  // = XcursorLibraryLoadCursor(dpy, "sb_v_double_arrow");
  switch(cursor)
  {
    case OS_Cursor_Pointer:         {cursor_name = str8_lit("arrow");}break;
    case OS_Cursor_IBar:            {cursor_name = str8_lit("xterm");}break;
    case OS_Cursor_LeftRight:       {cursor_name = str8_lit("sb_h_double_arrow");}break;
    case OS_Cursor_UpDown:          {cursor_name = str8_lit("sb_v_double_arrow");}break;
    case OS_Cursor_DownRight:       {cursor_name = str8_lit("bottom_right_corner");}break;
    case OS_Cursor_DownLeft:        {cursor_name = str8_lit("bottom_left_corner");}break;
    case OS_Cursor_UpRight:         {cursor_name = str8_lit("top_right_corner");}break;
    case OS_Cursor_UpLeft:          {cursor_name = str8_lit("top_left_corner");}break;
    case OS_Cursor_UpDownLeftRight: {cursor_name = str8_lit("move");}break;
    case OS_Cursor_HandPoint:       {cursor_name = str8_lit("hand2");}break;
    default:                        {cursor_name = str8_lit("arrow");}break;
  }

  c = XcursorLibraryLoadCursor(os_lnx_gfx_state->display, (char *)cursor_name.str);
  os_lnx_gfx_state->next_cursor = c;
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
  
}

////////////////////////////////
//~ rjf: @os_hooks Shell Operations

internal void
os_show_in_filesystem_ui(String8 path)
{
  
}
