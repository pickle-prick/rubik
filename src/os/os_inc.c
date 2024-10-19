#include "os/core/os_core.c"
#include "os/gfx/os_gfx.c"

#if OS_WINDOWS
# include "os/core/win32/os_core_win32.c"
#elif OS_LINUX
# include "os/core/linux/os_core_linux.c"
# include "os/gfx/linux/os_gfx_linux.c"
#else
# error OS core layer not implemented for this operating system.
#endif
