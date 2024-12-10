#if PROFILE

internal ProfTickInfo *pf_tick_alloc()
{
    Arena *arena = arena_alloc();
    ProfTickInfo *tick = push_array(arena, ProfTickInfo, 1);
    tick->arena = arena;
    tick->arena_tick_pos = arena_pos(arena);
    return tick;
}

internal void pf_tick()
{
    pf_idx_pre = pf_tick_count % pf_buffer_count;
    pf_idx_pst = (pf_tick_count+1) % pf_buffer_count;
    pf_tick_count++;

    // initialize if not already
    if(pf_ticks[0] == 0)
    {
        for(U64 i = 0; i < pf_buffer_count; i++)
        {
            pf_ticks[i] = pf_tick_alloc();
        }
    }

    // end last tick's tsc/us counter
    ProfTickInfo *last_tick = pf_ticks[pf_idx_pst];
    last_tick->cycles = rdtsc() - last_tick->tsc_start;
    last_tick->us = os_now_microseconds() - last_tick->us_start;

    // start current tick
    ProfTickInfo *tick = pf_ticks[pf_idx_pre];
    tick->tsc_start = rdtsc();
    tick->us_start = os_now_microseconds();

    // clear arena
    arena_pop_to(tick->arena, tick->arena_tick_pos);

    // alloc hash table
    tick->node_hash_table_size = pf_table_size;
    tick->node_hash_table = push_array(tick->arena, ProfNodeSlot, tick->node_hash_table_size);
}

internal void pf_begin(const char* fmt, ...)
{
    ProfNode *parent_node = pf_top_node;
    // TODO(k): recursive call won't work this way
    U64 seed = 0;

    if(parent_node != 0)
    {
        seed = parent_node->hash;
    }

    ProfTickInfo *tick = pf_ticks[pf_idx_pre];
    Arena *arena = tick->arena;

    ProfNode *node = 0;
    union
    {
        XXH64_hash_t xxhash;
        U64 u64;
    } hash;

    Temp scratch = scratch_begin(0,0);
    va_list args;
    va_start(args, fmt);
    String8 string = push_str8fv(scratch.arena, (char *)fmt, args);
    va_end(args);
    hash.xxhash = XXH3_64bits_withSeed(string.str, string.size, seed);
    U64 slot_idx = hash.u64 % tick->node_hash_table_size;
    ProfNodeSlot *slot = &tick->node_hash_table[slot_idx];
    for(ProfNode *n = slot->first; n != 0; n = n->hash_next)
    {
        if(n->hash == hash.u64)
        {
            node = n;
        }
    }

    // create node if no cache was found
    if(node == 0)
    {
        node = push_array(arena, ProfNode, 1);
        node->hash = hash.u64;
        node->tag = push_str8_copy(arena, string);
        node->file = str8_cstring(__FILE__);
        node->line = __LINE__;
        DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
        slot->count++;
    }

    // insert to the tree
    if(parent_node != 0)
    {
        DLLPushBack(parent_node->first, parent_node->last, node);
        parent_node->child_count++;
    }

    // start the counter
    node->tsc_start = rdtsc();
    node->us_start = os_now_microseconds();

    SLLStackPush(pf_top_node, node);
    scratch_end(scratch);
}

internal void pf_end()
{
    ProfNode *node = pf_top_node;
    Assert(node != 0);
    SLLStackPop(pf_top_node);

    U64 this_tsc = rdtsc() - node->tsc_start;
    U64 this_us = os_now_microseconds() - node->us_start;
    node->total_cycles += this_tsc;
    node->total_us += this_us;
    node->call_count++;
    node->cycles_per_call = node->total_cycles/node->call_count;
    node->us_per_call = node->total_us/node->call_count;
}

#endif
