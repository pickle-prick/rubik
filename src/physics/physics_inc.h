#ifndef PHYSICS_INC_H
#define PHYSICS_INC_H

////////////////////////////////
//~ rjf: Backend Constants

// #define R_BACKEND_VULKAN 1
// 
// #if R_BACKEND_VULKAN
// # define R_BACKEND R_BACKEND_VULKAN
// #endif

////////////////////////////////
//~ Main Includes

#include "physics_core.h"

////////////////////////////////
//~ rjf: Backend Includes

// #if R_BACKEND == R_BACKEND_VULKAN
// # include "vulkan/render_vulkan.h"
// #else
// # error Renderer backend not specified.
// #endif

#endif // PHYSICS_INC_H
