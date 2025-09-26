#ifndef RUBIK_CORE_H
#define RUBIK_CORE_H

/////////////////////////////////
//~ Generated code

// #include "generated/rubik.meta.h"

/////////////////////////////////
//~ State

typedef struct RK_State RK_State;
struct RK_State
{
};

/////////////////////////////////
//~ Globals

global RK_State *rk_state;

/////////////////////////////////
//~ State accessor/mutator

// entry call
internal void rk_init(OS_Handle os_wnd, R_Handle r_wnd);
internal B32  rk_frame(void);

#endif
