#ifndef FONT_CACHE_H
#define FONT_CACHE_H

////////////////////////////////
//~ rjf: Rasterization Flags

typedef U32 F_RasterFlags;
enum
{
    F_RasterFlag_Smooth = (1 << 0),
    F_RasterFlag_Hinted = (1 << 1),
};

////////////////////////////////
//~ rjf: Handles & Tags

typedef struct F_Tag F_Tag;
struct F_Tag
{
    U64 u64[2];
};

////////////////////////////////
//~ rjf: Draw Package Types (For Cache Queries)

typedef struct F_Piece F_Piece;
struct F_Piece
{
    R_Handle texture;
    Vec2S16  texture_dim;
    Rng2S16  rect;
    Rng2F32  subrect;
    F32      advance;
    // TODO: what is decode_size
    U16      decode_size;
};

typedef struct F_PieceArray F_PieceArray;
struct F_PieceArray
{
    F_Piece *v;
    U64 count;
};

typedef struct F_Run F_Run;
struct F_Run
{
    F_PieceArray pieces;
    U64          ascent;
    U64          descent;
    Vec2F32      dim;
};

////////////////////////////////
//~ rjf: Font Path -> Handle * Metrics * Path Cache Types

typedef struct F_FontHashNode F_FontHashNode;
struct F_FontHashNode
{
    F_FontHashNode *hash_next;
    F_Tag tag;
    FP_Handle handle;
    FP_Metrics metrics;
    String8 path;
};

typedef struct F_FontHashSlot F_FontHashSlot;
struct F_FontHashSlot
{
    F_FontHashNode *first;
    F_FontHashNode *last;
};

////////////////////////////////
//~ rjf: Atlas Types

typedef struct F_Atlas F_Atlas;
struct F_Atlas
{
    FP_Handle atlas_handle;
    R_Handle  texture;
    Vec2S16   dim;
};

////////////////////////////////
//~ rjf: Rasterization Cache Types
typedef struct F_Hash2StyleRasterCacheNode F_Hash2StyleRasterCacheNode;
struct F_Hash2StyleRasterCacheNode
{
    F_Hash2StyleRasterCacheNode *hash_next;
    F_Hash2StyleRasterCacheNode *hash_prev;
    U64 style_hash;
    F32 ascent;
    F32 descent;
    F_Atlas atlas;
};

typedef struct F_Hash2StyleRasterCacheSlot F_Hash2StyleRasterCacheSlot;
struct F_Hash2StyleRasterCacheSlot
{
    F_Hash2StyleRasterCacheNode *first;
    F_Hash2StyleRasterCacheNode *last;
};

////////////////////////////////
//~ rjf: Metrics

typedef struct F_Metrics F_Metrics;
struct F_Metrics
{
    F32 ascent;
    F32 descent;
    F32 line_gap;
    F32 capital_height;
};

////////////////////////////////
//~ rjf: Main State Type

typedef struct F_State F_State;
struct F_State
{
    Arena *arena;

    // rjf: font table
    U64 font_hash_table_size;
    F_FontHashSlot *font_hash_table;

    // rjf: hash -> raster cache table
    U64 hash2style_slots_count;
    F_Hash2StyleRasterCacheSlot *hash2style_slots;

    // rjf: atlas list
    F_Atlas *first_atlas;
    F_Atlas *last_atlas;
};

////////////////////////////////
//~ rjf: Globals

global F_State *f_state = 0;

////////////////////////////////
//~ rjf: Basic Functions

internal U128 f_hash_from_string(String8 string);
internal U64 f_little_hash_from_string(String8 string);
internal U64 f_little_hash_from_bytes(U8 *bytes, U64 count);
// internal Vec2S32 f_vertex_from_corner(Corner corner);

////////////////////////////////
//~ rjf: Font Tags

internal F_Tag f_tag_zero(void);
internal B32 f_tag_match(F_Tag a, F_Tag b);
internal FP_Handle f_handle_from_tag(F_Tag tag);
internal FP_Metrics f_fp_metrics_from_tag(F_Tag tag);
internal F_Tag f_tag_from_path(String8 path);
// internal F_Tag f_tag_from_static_data_string(String8 *data_ptr);
// internal String8 f_path_from_tag(F_Tag tag);

////////////////////////////////
//~ rjf: Atlas

// internal Rng2S16 f_atlas_region_alloc(Arena *arena, F_Atlas *atlas, Vec2S16 needed_size);
// internal void f_atlas_region_release(F_Atlas *atlas, Rng2S16 region);

////////////////////////////////
//~ rjf: Piece Type Functions

// internal F_Piece *f_piece_chunk_list_push_new(Arena *arena, F_PieceChunkList *list, U64 cap);
// internal void f_piece_chunk_list_push(Arena *arena, F_PieceChunkList *list, U64 cap, F_Piece *piece);
// internal F_PieceArray f_piece_array_from_chunk_list(Arena *arena, F_PieceChunkList *list);
// internal F_PieceArray f_piece_array_copy(Arena *arena, F_PieceArray *src);

////////////////////////////////
//~ rjf: Rasterization Cache

internal F_Hash2StyleRasterCacheNode *f_hash2style_from_tag_size_flags(F_Tag tag, F32 size, F_RasterFlags flags);
internal F_Run f_push_run_from_string(Arena *arena, F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, F_RasterFlags flags, String8 string);
// internal String8List f_wrapped_string_lines_from_font_size_string_max(Arena *arena, F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, String8 string, F32 max);
internal Vec2F32 f_dim_from_tag_size_string(F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, String8 string);
// internal Vec2F32 f_dim_from_tag_size_string_list(F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, String8List list);
// internal F32 f_column_size_from_tag_size(F_Tag tag, F32 size);
// internal U64 f_char_pos_from_tag_size_string_p(F_Tag tag, F32 size, F32 base_align_px, F32 tab_size_px, String8 string, F32 p);

////////////////////////////////
//~ rjf: Metrics

internal F_Metrics f_metrics_from_tag_size(F_Tag tag, F32 size);
// internal F32 f_line_height_from_metrics(F_Metrics *metrics);

////////////////////////////////
//~ rjf: Main Calls

internal void f_init(void);

#endif // FONT_CACHE_H
