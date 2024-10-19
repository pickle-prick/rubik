#include "font_provider.c"

#if FP_BACKEND == FP_BACKEND_STB_TRUETYPE
# include "stb_truetype/font_provider_stb_truetype.c"
#else
# error Font provider backend not specified.
#endif
