internal Arena *
arena_alloc_(ArenaParams *params) {
    U64 aligned_res_size = params->reserve_size + ARENA_HEADER_SIZE;
    U64 aligned_cmt_size = params->commit_size  + ARENA_HEADER_SIZE;

    if (params->flags & ArenaFlag_LargePages) {
        aligned_res_size = AlignPow2(params->reserve_size, os_get_system_info()->large_page_size);
        aligned_cmt_size = AlignPow2(params->commit_size, os_get_system_info()->large_page_size);
    } else {
        aligned_res_size = AlignPow2(params->reserve_size, os_get_system_info()->page_size);
        aligned_cmt_size = AlignPow2(params->commit_size, os_get_system_info()->page_size);
    }

    printf("Allocating arena with res %.2fMB and cmt %.2fMB\n", (F32)aligned_res_size/MB(1), (F32)aligned_cmt_size/MB(1));

    // TODO(@k): make use of the optional_backing_buffer
    void *base = 0;
    if(params->flags & ArenaFlag_LargePages)
    {
        // TODO(@k): We expect the aligned_res_size is equal to aligned_cmt_size if we're using large page
        base = os_reserve_large(aligned_res_size);
        os_commit_large(base, aligned_cmt_size);
    } else {
        base = os_reserve(aligned_res_size);
        os_commit(base, aligned_cmt_size);
    }

    AssertAlways(base != 0);

    Arena *arena = (Arena *)base;

    arena->prev = NULL;
    arena->curr = arena;

    arena->res_size = params->reserve_size;
    arena->cmt_size = (U32)params->commit_size;
    arena->flags = params->flags;

    arena->res = aligned_res_size;
    arena->cmt = aligned_cmt_size;

    arena->base_pos = 0;
    arena->pos = ARENA_HEADER_SIZE;

    return arena;
}

internal void
arena_release(Arena *arena) {
    for (Arena *n = arena->curr, *prev = 0; n != 0; n = prev) {
        prev = n->prev;
        os_release(n, n->res);
    }
}

internal void *
arena_push(Arena *_arena, U64 size, U64 align) {
    Arena *curr_arena = _arena->curr;

    U64 pos_pre = AlignPow2(curr_arena->pos, align);
    U64 pos_pst = pos_pre + size;

    // Chain if needed
    if (pos_pst > curr_arena->res) {
        AssertAlways(!(curr_arena->flags & ArenaFlag_NoChain) && "No more budge for this arena");

        U64 res_size = Max(curr_arena->res_size, size);
        U64 cmt_size = Max(curr_arena->cmt_size, size);

        printf("Allocating new block: %fMB\n", (float)res_size/MB(1));

        curr_arena = arena_alloc(.reserve_size = res_size,
                .commit_size = cmt_size,
                .flags = curr_arena->flags);
        curr_arena->base_pos = _arena->base_pos + _arena->res;
        curr_arena->curr = curr_arena;
        curr_arena->prev = _arena;

        // NOTE(@k): Since the size of header is 64, curr pos would be aligned to anything else with pow of 2
        // Unless the caller have some weird alignment requirement
        pos_pre = AlignPow2(curr_arena->pos, align);
        pos_pst = pos_pre + size;
    }

    // At this point, we are sure that we have enough space to store this size with its alignment requirement

    // Commit new page if we need to
    // TODO(@k): not 100% sure why we don't gradually commit new page if ARENA_FLAG_LARGEPAGES is used
    if (pos_pst > curr_arena->cmt) {
        AssertAlways(!(curr_arena->flags & ArenaFlag_LargePages));

        U64 cmt_pst = AlignPow2(pos_pst, curr_arena->cmt_size);
        cmt_pst = AlignPow2(cmt_pst, os_get_system_info()->page_size);
        // NOTE(@k): if cmt_pst exceed the res, we then could not met the alignment requirement which is fine but not ideal
        cmt_pst = Min(cmt_pst, curr_arena->res);

        printf("Commiting new block: %fMB\n", (float)(cmt_pst-curr_arena->cmt)/MB(1));

        os_commit((U8 *)curr_arena + curr_arena->cmt, cmt_pst - curr_arena->cmt);
        curr_arena->cmt = cmt_pst;
    }

    void *ret = (U8 *)curr_arena + pos_pre;
    curr_arena->pos = pos_pst;
    return ret;
}

// TODO(@k): why are we not using macro for this
internal inline U64
arena_pos(Arena *arena) {
    Arena *c = arena->curr;
    return c->base_pos + c->pos;
}

internal void
arena_pop_to(Arena *arena, U64 pos) {
    AssertAlways(pos >= ARENA_HEADER_SIZE);
    Arena *curr = arena->curr;
    for (Arena *prev = 0; pos < curr->base_pos; curr = prev) {
        prev = curr->prev;
        os_release(curr, curr->res);
    }
    U64 new_pos = pos - curr->base_pos;
    AssertAlways(new_pos <= curr->pos);
    AssertAlways(new_pos >= ARENA_HEADER_SIZE);
    curr->curr = curr;
    curr->pos = new_pos;
}

internal void
arena_clear(Arena *arena) {
    arena_pop_to(arena, ARENA_HEADER_SIZE);
}

internal void
arena_pop(Arena *arena, U64 amt) {
    U64 pos = arena->base_pos + arena->pos - amt;
    AssertAlways(pos >= ARENA_HEADER_SIZE);
    arena_pop_to(arena, pos);
}

internal Temp
temp_begin(Arena *arena) {
    Temp t = {
        .arena = arena,
        .pos = arena_pos(arena),
    };
    return t;
}

internal void
temp_end(Temp temp) {
    arena_pop_to(temp.arena, temp.pos);
}
