#ifndef FONT_PROVIDER_H
#define FONT_PROVIDER_H

#define fp_hook C_LINKAGE

////////////////////////////////
//~ rjf: Types

typedef U32 FP_RasterFlags;
enum {
        FP_RasterFlag_Smooth = (1 << 0),
        FP_RasterFlag_Hinted = (1 << 1),
};

typedef struct FP_Handle FP_Handle;
struct FP_Handle {
        U64 u64[2];
};

typedef struct FP_Metrics FP_Metrics;
struct FP_Metrics {
        F32 design_units_per_em;
        F32 ascent;
        F32 descent;
        F32 line_gap;
        // F32 capital_height;
};

typedef struct FP_RasterResult FP_RasterResult;
struct FP_RasterResult {
        FP_Handle atlas_handle;
        Vec2S16   atlas_dim;
        void      *atlas;
};

typedef struct FP_PackedQuad FP_PackedQuad;
struct FP_PackedQuad {
        float x0,y0,s0,t0; // top-left
        float x1,y1,s1,t1; // bottom-right
};

////////////////////////////////
//~ rjf: Basic Type Functions

internal FP_Handle fp_handle_zero(void);
internal B32 fp_handle_match(FP_Handle a, FP_Handle b);

////////////////////////////////
//~ rjf: Backend Hooks

fp_hook void fp_init(void);
fp_hook FP_Handle fp_font_open(String8 path);
// fp_hook FP_Handle fp_font_open_from_static_data_string(char *data_ptr);
fp_hook void fp_font_close(FP_Handle handle);
fp_hook FP_Metrics fp_metrics_from_font(FP_Handle font);
fp_hook FP_RasterResult fp_raster(Arena *arena, FP_Handle font, F32 size, FP_RasterFlags flags, Rng1U64 range);
// TODO(@k): we could seperate rect packing from font provider layer
fp_hook FP_PackedQuad fp_get_packed_quad(FP_Handle atlas_handle, int char_index, float *xpos, float *ypos);

#endif // FONT_PROVIDER_H
