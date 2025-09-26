#ifndef FONT_PROVIDER_STB_TRUETYPE_H
#define FONT_PROVIDER_STB_TRUETYPE_H

// #define STB_RECT_PACK_IMPLEMENTATION
// #include "external/stb/stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb/stb_truetype.h"


typedef struct FP_STBTT_Font FP_STBTT_Font;
struct FP_STBTT_Font
{
  FP_STBTT_Font *next;
  stbtt_fontinfo info;
  String8 buffer;
  // metrics
  S32 design_units_per_em;
  S32 ascent;
  S32 descent;
  S32 line_gap;
  S32 capital_height;
};

typedef struct FP_STBTT_State FP_STBTT_State;
struct FP_STBTT_State
{
  Arena *arena;
  U64 font_count;
};

////////////////////////////////
// Globals

global FP_STBTT_State *fp_stbtt_state = 0;

#endif // FONT_PROVIDER_STB_TRUETYPE_H
