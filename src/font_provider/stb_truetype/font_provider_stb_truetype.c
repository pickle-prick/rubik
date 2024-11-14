// TODO(@k): 512x512 may not be enough
#define BITMAP_W  1024
#define BITMAP_H  1024

fp_hook void
fp_init(void) {
        Arena *arena = arena_alloc();
        fp_stbtt_state = push_array(arena, FP_STBTT_State, 1);
        fp_stbtt_state->arena = arena;
}

fp_hook FP_Handle
fp_font_open(String8 path) {
        Arena *arena = fp_stbtt_state->arena;

        OS_Handle file = os_file_open(OS_AccessFlag_Read, path);
        FileProperties props = os_properties_from_file(file);

        FP_STBTT_Font *font = fp_stbtt_state->first_free_font;
        if (font != 0) {
                SLLStackPop(fp_stbtt_state->first_free_font);
                if (font->buffer_cap < props.size) {
                        free(font->buffer);
                        font->buffer = malloc(props.size);
                        font->buffer_cap = props.size;
                }
                font->generation = font->generation + 1;
                font->idx = -1;
        } else {
                font = push_array(arena, FP_STBTT_Font, 1);
                font->idx = -1;
                font->buffer = malloc(props.size);
        }
        font->idx = 0;

        int read_ret = 0;
        // TODO(@k): do we need to append \0 here?
        read_ret = os_file_read(file, rng_1u64(0,props.size), font->buffer);
        AssertAlways(read_ret > 0);

        // Init the font info
        AssertAlways(stbtt_InitFont(&font->info, font->buffer, 0) != 0);

        FP_Handle ret = {0};
        ret.u64[0] = (U64)font;
        ret.u64[1] = font->generation;
        return ret;
}

fp_hook void
fp_font_close(FP_Handle handle) {
        if (fp_handle_match(fp_handle_zero(), handle)) {return;}

        U64 gen = handle.u64[1];
        FP_STBTT_Font *font = (FP_STBTT_Font *)handle.u64[0];
        AssertAlways(font->generation == gen);
        SLLStackPush(fp_stbtt_state->first_free_font, font);
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle handle) {
        FP_Metrics ret = {0};
        if (fp_handle_match(fp_handle_zero(), handle)) {return ret;}

        U64 gen = handle.u64[1];
        FP_STBTT_Font *font  = (FP_STBTT_Font *)handle.u64[0];
        stbtt_fontinfo *info = &font->info;
        AssertAlways(font->generation == gen);

        int ascent,descent,line_gap = 0;
        stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);

        ret.ascent   = ascent;
        ret.descent  = -descent;
        ret.line_gap = line_gap;
        ret.design_units_per_em = ttUSHORT(info->data + info->head + 18);
        return ret;
}

// TODO(@k): fix this later
fp_hook FP_RasterResult
fp_raster(Arena *arena, FP_Handle font_handle, F32 size, FP_RasterFlags flags, Rng1U64 range) {
        FP_RasterResult ret = {0};
        if (fp_handle_match(fp_handle_zero(), font_handle)) {return ret;}

        U64 gen = font_handle.u64[1];
        FP_STBTT_Font *font = (FP_STBTT_Font *)font_handle.u64[0];
        AssertAlways(font->generation == gen);

        stbtt_pack_context ctx = {0};
        U8 *pixels = push_array(arena, U8, BITMAP_W*BITMAP_H);
        AssertAlways(stbtt_PackBegin(&ctx, pixels, BITMAP_W,BITMAP_H, 0,1, NULL) != 0);
        if ((flags & FP_RasterFlag_Smooth) == FP_RasterFlag_Smooth) {
                stbtt_PackSetOversampling(&ctx, 2,2);
        }

        stbtt_pack_range pr = {0};
        pr.chardata_for_range = push_array(fp_stbtt_state->arena, stbtt_packedchar, range.max-range.min);
        pr.array_of_unicode_codepoints      = NULL;
        pr.font_size                        = size; // height
        // ASCII 32..126 is 95 glyphs
        pr.first_unicode_codepoint_in_range = range.min;
        pr.num_chars                        = range.max-range.min;

        AssertAlways(stbtt_PackFontRanges(&ctx, font->buffer, font->idx, &pr,1) != 0);
        stbtt_PackEnd(&ctx);

        FP_STBTT_Atlas *atlas = fp_stbtt_state->first_free_atlas;
        if (atlas == 0) {
                atlas = push_array(fp_stbtt_state->arena, FP_STBTT_Atlas, 1);
        } else {
                // TODO(@k): we can't reuse chardata
                U64 gen = atlas->generation;
                SLLStackPop(fp_stbtt_state->first_free_atlas);
                MemoryZeroStruct(atlas);
                atlas->generation = gen + 1;
        }

        atlas->chardata = pr.chardata_for_range;
        atlas->dim = vec_2s32(BITMAP_W,BITMAP_H);

        ret.atlas_handle.u64[0] = (U64)atlas;
        ret.atlas_handle.u64[1] = atlas->generation;
        ret.atlas_dim = vec_2s16(BITMAP_W,BITMAP_H);
        ret.atlas = pixels;
        return ret;
}

fp_hook FP_PackedQuad fp_get_packed_quad(FP_Handle atlas_handle, int char_index, float *xpos, float *ypos) {
        U64 gen = atlas_handle.u64[1];
        FP_STBTT_Atlas *atlas = (FP_STBTT_Atlas *)atlas_handle.u64[0];
        Assert(atlas->generation == gen);

        // TODO(@k): fix it later, what is "aligned to int"
        FP_PackedQuad ret = {0};
        stbtt_GetPackedQuad(atlas->chardata, atlas->dim.x, atlas->dim.y, char_index, xpos, ypos, (stbtt_aligned_quad *)&ret, 0);

        // We want src in pixel space
        ret.s0 *= atlas->dim.x;
        ret.s1 *= atlas->dim.x;
        ret.t0 *= atlas->dim.y;
        ret.t1 *= atlas->dim.y;
        return ret;
}
