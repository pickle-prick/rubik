#ifndef FONT_PROVIDER_INC_H
#define FONT_PROVIDER_INC_H

////////////////////////////////
//~ rjf: Backend Constants

#define FP_BACKEND_STB_TRUETYPE 1
#define FP_BACKEND_DWRITE       2
#define FP_BACKEND_FREETYPE     3

////////////////////////////////
//~ rjf: Decide On Backend

#if !defined(FP_BACKEND)
# if OS_WINDOWS
#  define FP_BACKEND FP_BACKEND_STB_TRUETYPE
# elif OS_LINUX
#  define FP_BACKEND FP_BACKEND_STB_TRUETYPE
// #  define FP_BACKEND FP_BACKEND_FREETYPE
# endif
#endif

////////////////////////////////
//~ rjf: Main Includes

#include "font_provider.h"

////////////////////////////////
//~ rjf: Backend Includes

#if FP_BACKEND == FP_BACKEND_DWRITE
# include "dwrite/font_provider_dwrite.h"
#elif FP_BACKEND == FP_BACKEND_FREETYPE
# include "freetype/font_provider_freetype.h"
#elif FP_BACKEND == FP_BACKEND_STB_TRUETYPE
# include "stbtt/font_provider_stbtt.h"
#else
# error Font provider backend not specified.
#endif

#endif // FONT_PROVIDER_INC_H

