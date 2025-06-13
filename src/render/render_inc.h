#ifndef RENDER_INC_H
#define RENDER_INC_H

////////////////////////////////
//~ rjf: Backend Constants

// TODO: move it some where in the main
#define R_BACKEND_VULKAN 1

#if R_BACKEND_VULKAN
# define R_BACKEND R_BACKEND_VULKAN
#endif

////////////////////////////////
//~ rjf: Main Includes

#include "render_core.h"

////////////////////////////////
//~ rjf: Backend Includes

#if R_BACKEND == R_BACKEND_VULKAN
# include "vulkan/render_vulkan.h"
#else
# error Renderer backend not specified.
#endif

#endif // RENDER_INC_H
