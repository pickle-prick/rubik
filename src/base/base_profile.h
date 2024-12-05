#ifndef BASE_PROFILE_H
#define BASE_PROFILE_H

////////////////////////////////
//~ k: Debug Profile Type

typedef struct ProfNode ProfNode;
struct ProfNode
{
    ProfNode *next;
    ProfNode *prev;
    ProfNode *hash_next;
    ProfNode *hash_prev;
    U64      hash;
    String8  tag;
    String8  file;
    U64      line;
    U64      total_cycles;
    U64      call_count;
    U64      cycles_per_call;
    U64      total_us;
    U64      us_per_call;

    U64      tsc_start;
    U64      us_start;
};

typedef struct ProfNodeSlot ProfNodeSlot;
struct ProfNodeSlot
{
    ProfNode *first;
    ProfNode *last;
    U64      count;
};

////////////////////////////////
//~ k: RDTSC

#if COMPILER_GCC
# include <x86intrin.h>
# define rdtsc __rdtsc
#elif COMPILER_MSVC
# define rdtsc __rdtsc
#elif COMPILER_CLANG
# include <x86intrin.h>
# define rdtsc __rdtscp
#else
# define rdtsc (void)0
#endif

////////////////////////////////
//~ k: Debug Profile Defines

#if PROFILE_TELEMETRY

#define prof_buffer_count 2
global Arena *prof_arenas[prof_buffer_count] = {0};
global U64 prof_hash_table_size = 1024;
global ProfNodeSlot *prof_hash_tables[prof_buffer_count] = {0};
global ProfNode *prof_top_node = 0; // Stacks
global U64 prof_tick_count = 0;
global U64 prof_idx_pst = 0;
global U64 prof_idx_pre = 0;

# define ProfTick()                                                                                  \
do                                                                                                   \
{                                                                                                    \
  prof_idx_pre = prof_tick_count % prof_buffer_count;                                                \
  prof_idx_pst = (prof_tick_count+1) % prof_buffer_count;                                            \
  prof_tick_count++;                                                                                 \
  if(prof_arenas[0] == 0)                                                                            \
  {                                                                                                  \
    for(U64 i = 0; i < prof_buffer_count; i++)                                                       \
    {                                                                                                \
      prof_arenas[i] = arena_alloc();                                                                \
    }                                                                                                \
  }                                                                                                  \
  Arena *arena = prof_arenas[prof_idx_pre];                                                          \
  arena_clear(arena);                                                                                \
  prof_hash_tables[prof_idx_pre] = push_array(arena, ProfNodeSlot, prof_hash_table_size);            \
} while(0)
  
# define ProfBegin(...)                                                            \
do                                                                                 \
{                                                                                  \
  ProfNode *node = 0;                                                              \
  Arena *arena = prof_arenas[prof_idx_pre];                                        \
  union                                                                            \
  {                                                                                \
      XXH64_hash_t xxhash;                                                         \
      U64 u64;                                                                     \
  }                                                                                \
  hash;                                                                            \
  Temp scratch = scratch_begin(0,0);                                               \
  String8 string = push_str8f(scratch.arena, __VA_ARGS__);                         \
  hash.xxhash = XXH3_64bits(string.str, string.size);                              \
  U64 slot_idx = hash.u64 % prof_hash_table_size;                                  \
  ProfNodeSlot *slot = &prof_hash_tables[prof_idx_pre][slot_idx];                  \
  for(ProfNode *n = slot->first; n != 0; n = n->next)                              \
  {                                                                                \
    if(n->hash == hash.u64)                                                        \
    {                                                                              \
        node = n;                                                                  \
    }                                                                              \
  }                                                                                \
  if(node == 0)                                                                    \
  {                                                                                \
    node = push_array(arena, ProfNode, 1);                                         \
    node->hash = hash.u64;                                                         \
    node->tag = push_str8_copy(arena, string);                                     \
    node->file = str8_cstring(__FILE__);                                           \
    node->line = __LINE__;                                                         \
    DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);           \
    slot->count++;                                                                 \
  }                                                                                \
  node->tsc_start = rdtsc();                                                       \
  node->us_start = os_now_microseconds();                                          \
  SLLStackPush(prof_top_node, node);                                               \
  scratch_end(scratch);                                                            \
} while(0)

# define ProfEnd()                                                                 \
do                                                                                 \
{                                                                                  \
  ProfNode *node = prof_top_node;                                                  \
  SLLStackPop(prof_top_node);                                                      \
  U64 this_tsc = rdtsc()-node->tsc_start;                                          \
  U64 this_us = os_now_microseconds()-node->us_start;                              \
  node->total_cycles += this_tsc;                                                  \
  node->total_us += this_us;                                                       \
  node->call_count++;                                                              \
  node->cycles_per_call = node->total_cycles/node->call_count;                     \
  node->us_per_call = node->total_us/node->call_count;                             \
} while(0)
# define ProfTablePst() prof_hash_tables[prof_idx_pst]
# define ProfTablePre() prof_hash_tables[prof_idx_pre]
#endif

////////////////////////////////
//~ k: Zeroify Undefined Defines

#if !defined(ProfBegin)
# define ProfTick(...)          (0)
# define ProfBegin(...)         (0)
# define ProfEnd()              (0)
# define ProTablePst()          (0)
# define ProTablePre()          (0)
#endif

////////////////////////////////
//~ k: Helper Wrappers

#define ProfBeginFunction(...) ProfBegin(this_function_name)
#define ProfScope(...) DeferLoop(ProfBegin(__VA_ARGS__), ProfEnd())

#endif // BASE_PROFILE_H
