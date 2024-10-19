#include "render_core.c"

#if R_BACKEND == R_BACKEND_VULKAN
# include "vulkan/render_vulkan.c"
#else
# error Renderer backend not specified.
#endif
