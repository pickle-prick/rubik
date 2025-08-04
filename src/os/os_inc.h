#ifndef OS_INC_H
#define OS_INC_H

#if !defined(OS_FEATURE_GRAPHICAL)
# define OS_FEATURE_GRAPHICAL 0
#endif

#if !defined(OS_GFX_STUB)
# define OS_GFX_STUB 0
#endif

#include "os/core/os_core.h"
#if OS_FEATURE_GRAPHICAL
#if OS_WINDOWS
#  define VK_USE_PLATFORM_WIN32_KHR
#  define VK_PROTOTYPES
#  include <vulkan/vulkan.h>
#  include <windows.h>
#  include <vulkan/vulkan_win32.h>
#elif OS_LINUX
#  define VK_USE_PLATFORM_XLIB_KHR
#  define VK_PROTOTYPES
#  include <vulkan/vulkan.h>
#endif
#  include "os/gfx/os_gfx.h"
#endif

#if OS_FEATURE_AUDIO
#  include "os/audio/os_audio.h"
#endif

#if OS_WINDOWS
# include "os/core/win32/os_core_win32.h"
#elif OS_LINUX
# include "os/core/linux/os_core_linux.h"
#else
# error OS core layer not implemented for this operating system.
#endif

#if OS_FEATURE_GRAPHICAL
# if OS_GFX_STUB
#  include "os/gfx/stub/os_gfx_stub.h"
# elif OS_WINDOWS
#  include "os/gfx/win32/os_gfx_win32.h"
# elif OS_LINUX
#  include "os/gfx/linux/os_gfx_linux.h"
# else
#  error OS graphical layer not implemented for this operating system.
# endif
#endif

#if OS_FEATURE_AUDIO
# if OS_WINDOWS
// #  include "os/audio/win32/os_audio_win32.h"
#  include "os/audio/linux/os_audio_linux.h"
# elif OS_LINUX
#  include "os/audio/linux/os_audio_linux.h"
# else
#  error OS audio layer not implemented for this operating system.
# endif
#endif

#endif // OS_INC_H
