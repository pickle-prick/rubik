#ifndef BASE_THREAD_CONTEXT_H
#define BASE_THREAD_CONTEXT_H

////////////////////////////////
//~ Thread Context

typedef struct TCTX TCTX;
struct TCTX
{
  Arena *arenas[3];

  // thread name
  char thread_name[32];
  U64 thread_name_size;

  // source location info
  char *file_name;
  U64 line_number;
};

////////////////////////////////
//~ Thread Context Functions

internal void tctx_init_and_equip(TCTX *tctx);
internal void tctx_release(void);
internal TCTX *tctx_get_equipped(void);

internal void tctx_set_thread_name(String8 string);
internal char *tctx_get_thread_name(void);

internal void tctx_write_srcloc(char *file_name, U64 line_number);
internal void tctx_read_srcloc(char **file_name, U64 *line_number);
#define tctx_write_this_srcloc() tctx_write_srcloc(__FILE__, __LINE__)

internal Arena*    tctx_get_scratch(Arena **conflicts, U64 count);
#define scratch_begin(conflicts, count) temp_begin(tctx_get_scratch((conflicts), (count)))
#define scratch_end(scratch) temp_end(scratch)

#endif // BASE_THREAD_CONTEXT_H
