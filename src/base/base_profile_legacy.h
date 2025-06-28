#ifndef BASE_PROFILE_H
#define BASE_PROFILE_H

////////////////////////////////
//~ k: Debug Profile Type

typedef struct ProfNode ProfNode;
struct ProfNode
{
    ProfNode *parent;
    ProfNode *first;
    ProfNode *last;
    ProfNode *next;
    ProfNode *prev;

    U64      child_count;

    ProfNode *hash_next;
    ProfNode *hash_prev;

    U64      hash;
    String8  tag;
    String8  file;
    U64      line;
    // TODO: min/max/avg
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

typedef struct ProfTickInfo ProfTickInfo;
struct ProfTickInfo
{
    Arena        *arena;
    U64          arena_tick_pos;
    U64          node_hash_table_size;
    ProfNodeSlot *node_hash_table;

    U64          cycles;
    U64          us;
    U64          tsc_start;
    U64          us_start;
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
static inline unsigned long long rdtsc() {
 unsigned int aux;
 return __rdtscp(&aux);
}
#else
# define rdtsc (void)0
#endif

////////////////////////////////
//- k: Globals
#define pf_buffer_count 2
#define pf_table_size 1000
global ProfTickInfo *pf_ticks[pf_buffer_count] = {0};
global ProfNode *pf_top_node = 0;
global U64 pf_tick_count = 0;
global U64 pf_idx_pst = 0;
global U64 pf_idx_pre = 0;

////////////////////////////////
//~ k: Debug Profile Defines

#if PROFILE

////////////////////////////////
//- k: Build Helper

internal ProfTickInfo *pf_tick_alloc();
internal void pf_tick();
internal void pf_begin(const char *fmt, ...);
internal void pf_end();
internal ProfNode *pf_node_alloc();

# define ProfTick()     pf_tick()
# define ProfBegin(...) pf_begin(__VA_ARGS__)
# define ProfEnd()      pf_end()
# define ProfTickPst()  pf_ticks[pf_idx_pst]
# define ProfTickPre()  pf_ticks[pf_idx_pre]

#endif

////////////////////////////////
//~ k: Zeroify Undefined Defines

#if !defined(ProfBegin)
# define ProfTick(...)          (0)
# define ProfBegin(...)         (0)
# define ProfEnd()              (0)
# define ProfTickPst()          (0)
# define ProfTickPre()          (0)
#endif

////////////////////////////////
//~ k: Helper Wrappers

// TODO(XXX): revisit this, don't seem to merge same function calls, also huge performance hit
//            may be related to table lookup
#define ProfBeginFunction(...) ProfBegin(this_function_name)
#define ProfScope(...) DeferLoop(ProfBegin(__VA_ARGS__), ProfEnd())

#endif // BASE_PROFILE_H
