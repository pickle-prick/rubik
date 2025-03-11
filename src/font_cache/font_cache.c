internal F_Hash2StyleRasterCacheNode *
f_hash2style_from_tag_size_flags(F_Tag tag, F32 size, F_RasterFlags flags)
{
    U64 style_hash = {0};
    {
        F64 size_f64 = size;
        U64 buffer[] = {
            tag.u64[0],
            tag.u64[1],
            *(U64 *)(&size_f64),
            (U64)flags,
        };
        style_hash = f_little_hash_from_bytes((U8 *)buffer, sizeof(buffer));
    }

    // style hash -> style node
    F_Hash2StyleRasterCacheNode *hash2style_node = 0;
    U64 slot_idx = style_hash % f_state->hash2style_slots_count;
    F_Hash2StyleRasterCacheSlot *slot = &f_state->hash2style_slots[slot_idx];

    for(F_Hash2StyleRasterCacheNode *n = slot->first; n != 0; n = n->hash_next)
    {
        if(n->style_hash == style_hash)
        {
            hash2style_node = n;
            break;
        }
    }

    if(Unlikely(hash2style_node == 0))
    {
        F_Metrics metrics = f_metrics_from_tag_size(tag, size);
        hash2style_node = push_array(f_state->arena, F_Hash2StyleRasterCacheNode, 1);
        DLLPushBack_NP(slot->first, slot->last, hash2style_node, hash_next, hash_prev);

        // Rasterization
        Temp temp = scratch_begin(0,0);
        FP_RasterResult raster_ret = fp_raster(temp.arena, f_handle_from_tag(tag), size, flags, rng_1u64(32, 126));
        R_Handle texture_handle = r_tex2d_alloc(R_ResourceKind_Static,
                                                R_Tex2DSampleKind_Linear,
                                                vec_2s32(raster_ret.atlas_dim.x, raster_ret.atlas_dim.y),
                                                R_Tex2DFormat_R8, raster_ret.atlas);

        hash2style_node->style_hash         = style_hash;
        hash2style_node->ascent             = metrics.ascent;
        hash2style_node->descent            = metrics.descent;
        hash2style_node->atlas.atlas_handle = raster_ret.atlas_handle;
        hash2style_node->atlas.texture      = texture_handle;
        hash2style_node->atlas.dim          = raster_ret.atlas_dim;
    }

    return hash2style_node;
}

internal F_Tag
f_tag_from_path(String8 path)
{
    // Produce tag hash from hash of path
    F_Tag result = {0};
    {
        U128 hash = f_hash_from_string(path);
        MemoryCopy(&result, &hash, sizeof(result));
        // TODO(k): don't quite understand this
        result.u64[1] |= bit64;
    }

    // tag -> slot index
    U64 slot_idx = result.u64[1] % f_state->font_hash_table_size;

    F_FontHashNode *existing_node = 0;
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0; n = n->hash_next) 
    {
        if(MemoryMatchStruct(&result, &n->tag)) 
        {
            existing_node = n;
            break;
        }
    }

    // Allocate & push new node if we don't have an existing one
    F_FontHashNode *new_node = 0;
    if(existing_node == 0)
    {
        F_FontHashSlot *slot = &f_state->font_hash_table[slot_idx];
        new_node          = push_array(f_state->arena, F_FontHashNode, 1);
        new_node->tag     = result;
        new_node->handle  = fp_font_open(path);
        new_node->metrics = fp_metrics_from_font(new_node->handle);

        new_node->path = push_str8_copy(f_state->arena, path);
        SLLQueuePush_N(slot->first, slot->last, new_node, hash_next);
    }
    return result;
}

internal FP_Metrics
f_fp_metrics_from_tag(F_Tag tag)
{
    U64 slot_idx = tag.u64[1] % f_state->font_hash_table_size;
    F_FontHashNode *existing_node = 0;
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0; n = n->hash_next) 
    {
        if(MemoryMatchStruct(&tag, &n->tag))
        {
            existing_node = n;
            break;
        }
    }

    AssertAlways(existing_node != 0);
    FP_Metrics ret = {0};
    ret = existing_node->metrics;
    return ret;
}

internal F_Metrics
f_metrics_from_tag_size(F_Tag tag, F32 size) 
{
    FP_Metrics font_metrics = f_fp_metrics_from_tag(tag);

    F_Metrics ret = {0};
    ret.ascent   = floor_f32(size) * font_metrics.ascent / font_metrics.design_units_per_em;
    ret.descent  = floor_f32(size) * font_metrics.descent / font_metrics.design_units_per_em;
    ret.line_gap = floor_f32(size) * font_metrics.line_gap / font_metrics.design_units_per_em;
    return ret;
}

internal U128
f_hash_from_string(String8 string)
{
    union
    {
        XXH128_hash_t xxhash;
        U128 u128;
    }
    hash;
    hash.xxhash = XXH3_128bits(string.str, string.size);
    return hash.u128;
}

internal F_Tag f_tag_zero(void) 
{
    F_Tag result = {0};
    return result;
}

internal B32
f_tag_match(F_Tag a, F_Tag b)
{
    return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

internal FP_Handle
f_handle_from_tag(F_Tag tag)
{
    U64 slot_idx = tag.u64[1] % f_state->font_hash_table_size;
    F_FontHashNode *existing_node = 0;
    for(F_FontHashNode *n = f_state->font_hash_table[slot_idx].first; n != 0; n = n->hash_next)
    {
        if(MemoryMatchStruct(&tag, &n->tag))
        {
            existing_node = n;
            break;
        }
    }

    FP_Handle ret = {0};
    if(existing_node != 0)
    {
        ret = existing_node->handle;
    }
    return ret;
}

internal U64
f_little_hash_from_string(String8 string)
{
    U64 result = XXH3_64bits(string.str, string.size);
    return result;
}

internal U64
f_little_hash_from_bytes(U8 *bytes, U64 count)
{
    U64 hash = 5381;
    for(U64 i = 0; i < count; i++)
    {
        hash = ((hash << 5) + hash) + bytes[i]; // hash * 33 + c
    }
    return hash;
}

internal F_Run
f_push_run_from_string(Arena *arena, F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, F_RasterFlags flags, String8 string)
{
    // TODO: handle tab_size_px
    F_Hash2StyleRasterCacheNode *hash2style_node = f_hash2style_from_tag_size_flags(tag, size, flags);

    F_Run ret = {0};
    ret.ascent       = hash2style_node->ascent;
    ret.descent      = hash2style_node->descent;
    ret.dim.y        = ret.ascent + ret.descent;
    ret.pieces.count = string.size;
    ret.pieces.v     = push_array(arena, F_Piece, ret.pieces.count);

    float xpos = base_align_px;
    float ypos = 0;
    // TODO(k): we could get left bearing of this glphy, then add it to the offsetX
    for(U64 i = 0; i < ret.pieces.count; i++)
    {
        float x = xpos;
        float y = ypos;

        F_Piece *piece = &ret.pieces.v[i];
        // TODO(k): we could query kerning here
        AssertAlways(string.str[i] >= 32);
        FP_PackedQuad quad = fp_get_packed_quad(hash2style_node->atlas.atlas_handle, string.str[i]-32, &x, &y);

        piece->texture     = hash2style_node->atlas.texture;
        piece->texture_dim = hash2style_node->atlas.dim;
        piece->rect        = r2s16p(quad.x0, quad.y0, quad.x1, quad.y1);
        piece->subrect     = r2f32p(quad.s0, quad.t0, quad.s1, quad.t1);
        piece->advance     = x - xpos;
        // TODO: handle unicode later, assume ascii for now
        piece->decode_size = 1;

        xpos = x;
        ypos = y;
    }
    ret.dim.x = xpos;
    ret.dim.y = ret.ascent + ret.descent;
    return ret;
}

internal Vec2F32
f_dim_from_tag_size_string(F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, String8 string)
{
    Temp scratch = scratch_begin(0,0);
    Vec2F32 result = {0};
    F_Run run = f_push_run_from_string(scratch.arena, tag, size, base_align_px, tab_size_px, 0, string);
    result = run.dim;
    scratch_end(scratch);
    return result;
}

internal U64 f_char_pos_from_tag_size_string_p(F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, String8 string, F32 p)
{
    Temp scratch = scratch_begin(0, 0);
    U64 best_offset_bytes = 0;
    F32 best_offset_px    = inf32();
    U64 offset_bytes      = 0;
    F32 offset_px         = 0.f;

    F_Run run = f_push_run_from_string(scratch.arena, tag, size, base_align_px, tab_size_px, 0, string);
    for(U64 idx = 0; idx <= run.pieces.count; idx += 1)
    {
        F32 this_piece_offset_px = abs_f32(offset_px - p);
        if(this_piece_offset_px < best_offset_px)
        {
            best_offset_bytes = offset_bytes;
            best_offset_px = this_piece_offset_px;
        }
        if(idx < run.pieces.count)
        {
            F_Piece *piece = &run.pieces.v[idx];
            offset_px += piece->advance;
            offset_bytes += piece->decode_size;
        }
    }
    scratch_end(scratch);
    return best_offset_bytes;
}

internal void
f_init(void)
{
    Arena *arena = arena_alloc();
    f_state = push_array(arena, F_State, 1);
    f_state->arena = arena;
    f_state->font_hash_table_size = 64;
    f_state->font_hash_table = push_array(arena, F_FontHashSlot, f_state->font_hash_table_size);
    f_state->hash2style_slots_count = 1024;
    f_state->hash2style_slots = push_array(arena, F_Hash2StyleRasterCacheSlot, f_state->hash2style_slots_count);
}
