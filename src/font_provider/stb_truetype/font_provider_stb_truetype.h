#ifndef FONT_PROVIDER_STB_TRUETYPE_H
#define FONT_PROVIDER_STB_TRUETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "external/stb/stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "external/stb/stb_truetype.h"


typedef struct FP_STBTT_Font FP_STBTT_Font;
struct FP_STBTT_Font {
        FP_STBTT_Font  *next;
        U64            generation;

        stbtt_fontinfo info;

        void           *buffer;
        U64            buffer_cap;
        // TODO(@k): we may need to store font buffer size/used
        int            idx;
};

typedef struct FP_STBTT_Atlas FP_STBTT_Atlas;
struct FP_STBTT_Atlas {
        FP_STBTT_Atlas     *next;
        U64                generation;

        stbtt_packedchar   *chardata;
        Vec2S32            dim;
};

typedef struct FP_STBTT_State FP_STBTT_State;
struct FP_STBTT_State {
        Arena          *arena;

        FP_STBTT_Font  *first_free_font;
        FP_STBTT_Atlas *first_free_atlas;
};
// Globals
/////////////////////////////////////////////////////////////////////////////////////////
global FP_STBTT_State *fp_stbtt_state = 0;

#endif // FONT_PROVIDER_STB_TRUETYPE_H
