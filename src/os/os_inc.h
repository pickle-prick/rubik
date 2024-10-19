#ifndef OS_INC_H
#define OS_INC_H

#include "os/core/os_core.h"
#include "os/gfx/os_gfx.h"

#if OS_LINUX
# include "os/core/linux/os_core_linux.h"
# include "os/gfx/linux/os_gfx_linux.h"
#else
# error OS core/gfx layer not implemented for this os
#endif


#endif // OS_INC_H

