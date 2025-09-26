#define BITMAP_W  2048
#define BITMAP_H  2048

////////////////////////////////
//~ rjf: Backend Hooks

fp_hook void
fp_init(void)
{
  Arena *arena = arena_alloc();
  fp_stbtt_state = push_array(arena, FP_STBTT_State, 1);
  fp_stbtt_state->arena = arena;
}

fp_hook FP_Handle
fp_font_open(String8 path)
{
  Arena *arena = fp_stbtt_state->arena;
  FP_STBTT_Font *font = push_array(arena, FP_STBTT_Font, 1);

  // read content from path
  OS_Handle file = os_file_open(OS_AccessFlag_Read, path);
  FileProperties props = os_properties_from_file(file);
  font->buffer = os_string_from_file_range(arena, file, rng_1u64(0,props.size));

  // init the font info
  AssertAlways(stbtt_InitFont(&font->info, (const unsigned char*)font->buffer.str, 0) != 0);

  // metrics
  int ascent,descent,line_gap = 0;
  stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
  font->design_units_per_em = ttUSHORT(font->info.data + font->info.head + 18);
  font->ascent = ascent;
  font->descent = -descent;
  font->line_gap = line_gap;
  font->capital_height = ascent; // TODO: not sure

  fp_stbtt_state->font_count++;

  FP_Handle ret = {0};
  ret.u64[0] = (U64)font;
  return ret;
}

fp_hook FP_Handle
fp_font_open_from_static_data_string(String8 *data_ptr)
{ 
  Arena *arena = fp_stbtt_state->arena;
  FP_STBTT_Font *font = push_array(arena, FP_STBTT_Font, 1);
  font->buffer = *data_ptr;

  // init the font info
  AssertAlways(stbtt_InitFont(&font->info, (const unsigned char*)font->buffer.str, 0) != 0);

  // metrics
  int ascent,descent,line_gap = 0;
  stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
  font->design_units_per_em = ttUSHORT(font->info.data + font->info.head + 18);
  font->ascent = ascent;
  font->descent = -descent;
  font->line_gap = line_gap;
  font->capital_height = ascent; // TODO: not sure

  fp_stbtt_state->font_count++;

  FP_Handle ret = {0};
  ret.u64[0] = (U64)font;
  return ret;
}

fp_hook void
fp_font_close(FP_Handle handle)
{
  fp_stbtt_state->font_count--;
}

fp_hook FP_Metrics
fp_metrics_from_font(FP_Handle handle)
{
  FP_STBTT_Font *font = (FP_STBTT_Font *)handle.u64[0];

  FP_Metrics ret = {0};
  ret.ascent = (F32)font->ascent;
  ret.descent = (F32)font->descent;
  ret.line_gap = (F32)font->line_gap;
  ret.capital_height = (F32)font->capital_height;
  ret.design_units_per_em = (F32)font->design_units_per_em;
  return ret;
}

fp_hook FP_RasterResult
fp_raster(Arena *arena, FP_Handle handle, F32 size, FP_RasterFlags flags, String8 string)
{
  ProfBeginFunction();

  Temp scratch = scratch_begin(&arena, 1);

  FP_STBTT_Font *font = (FP_STBTT_Font *)handle.u64[0];
  stbtt_fontinfo *info = &font->info;
  String32 string32 = str32_from_8(scratch.arena, string);

  // get scale factor 
  // F32 scale_y = stbtt_ScaleForPixelHeight(info, ((96.f/72.f)*size));
  // F32 scale_y = stbtt_ScaleForPixelHeight(info, size);
  F32 scale_y = stbtt_ScaleForMappingEmToPixels(info, size);
  F32 scale_x = scale_y;

  S32 ascent = font->ascent*scale_x;
  S32 descent = font->descent*scale_x;
  S32 height = ascent+descent;

  typedef struct FP_STBTT_Glyph FP_STBTT_Glyph;
  struct FP_STBTT_Glyph
  {
    int top;
    int advance;
    int lsb;
    unsigned char *bmp;
    int bmp_width;
    int bmp_height;
  };

  FP_STBTT_Glyph *glyphs = push_array(scratch.arena, FP_STBTT_Glyph, string32.size);

  int total_width = 0;
  // get advance leftSideBearing for codepoint
  for EachIndex(idx, string32.size)
  {
    int advance,lsb;
    int codepoint = string32.str[idx];
    stbtt_GetCodepointHMetrics(info, codepoint, &advance, &lsb);

    glyphs[idx].advance = advance*scale_x;
    glyphs[idx].lsb = lsb*scale_x;
    total_width += advance;
  }
  total_width = ceil_f32(total_width*scale_x);

  // mesure bbox and render to bitmap
  for EachIndex(idx, string32.size)
  {
    // get bbox for glyph
    int codepoint = string32.str[idx];
    int x0,y0,x1,y1;
    stbtt_GetCodepointBitmapBox(info, codepoint, scale_x, scale_y, &x0, &y0, &x1, &y1);

    int w = x1-x0;
    int h = y1-y0;
    int top = -y0;

    // render bitmap
    int stride = w;
    unsigned char *bitmap = push_array(scratch.arena, unsigned char, w*h);
    stbtt_MakeCodepointBitmap(info, bitmap, w, h, stride, scale_x, scale_y, codepoint);

    // fill
    glyphs[idx].top = top;
    glyphs[idx].bmp = bitmap;
    glyphs[idx].bmp_width = w;
    glyphs[idx].bmp_height = h;
  }

  // allocate & fill atlas w/ rasterization
  Vec2S32 dim = {(S32)total_width+1, height+1}; // atlas dim
  U64 atlas_size = dim.x*dim.y*4;
  U8 *atlas = push_array(arena, U8, atlas_size);
  S32 baseline = ascent;
  S32 atlas_write_x = 0;
  for EachIndex(idx, string32.size)
  {
    FP_STBTT_Glyph *glyph = &glyphs[idx];
    int top = glyph->top;
    int left = glyph->lsb;
    for(int row = 0; row < glyph->bmp_height; row++)
    {
      int y = baseline - top + row;
      for(int col = 0; col < glyph->bmp_width; col++)
      {
        int x = atlas_write_x + left + col;
        U64 off = (y*dim.x + x)*4;
        if(off+4 <= atlas_size)
        {
          atlas[off+0] = 255;
          atlas[off+1] = 255;
          atlas[off+2] = 255;
          atlas[off+3] = glyph->bmp[row*glyph->bmp_width + col];
        }
      }
    }
    atlas_write_x += glyph->advance;
  }

  // fill result
  FP_RasterResult ret = {0};
  ret.atlas_dim = v2s16(dim.x, dim.y);
  ret.advance = (F32)total_width;
  ret.atlas = atlas;
  scratch_end(scratch);

  ProfEnd();
  return ret;
}
