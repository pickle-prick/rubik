// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#define D_StackPushImpl(name_upper, name_lower, type, val) \
D_Bucket *bucket = d_top_bucket();\
type old_val = bucket->top_##name_lower->v;\
D_##name_upper##Node *node = push_array(d_thread_ctx->arena, D_##name_upper##Node, 1);\
node->v = (val);\
SLLStackPush(bucket->top_##name_lower, node);\
bucket->stack_gen += 1;\
return old_val

#define D_StackPopImpl(name_upper, name_lower, type) \
D_Bucket *bucket = d_top_bucket();\
type popped_val = bucket->top_##name_lower->v;\
SLLStackPop(bucket->top_##name_lower);\
bucket->stack_gen += 1;\
return popped_val

#define D_StackTopImpl(name_upper, name_lower, type) \
D_Bucket *bucket = d_top_bucket();\
type top_val = bucket->top_##name_lower->v;\
return top_val

#include "generated/draw.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
d_hash_from_string(String8 string)
{
    U64 result = 5381;
    for(U64 i = 0; i < string.size; i += 1)
    {
        result = ((result << 5) + result) + string.str[i];
    }
    return result;
}

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries & bucket submission)

internal void
d_begin_frame(void)
{
    if(d_thread_ctx == 0)
    {
        Arena *arena = arena_alloc(.reserve_size = GB(64), .commit_size = MB(16));
        d_thread_ctx = push_array(arena, D_ThreadCtx, 1);
        d_thread_ctx->arena = arena;
        d_thread_ctx->arena_frame_start_pos = arena_pos(arena);
    }

    arena_pop_to(d_thread_ctx->arena, d_thread_ctx->arena_frame_start_pos);
    d_thread_ctx->top_bucket = 0;
    d_thread_ctx->free_bucket_selection = 0;
}

internal void
d_submit_bucket(OS_Handle os_wnd, R_Handle r_window, D_Bucket *bucket, Vec2F32 ptr)
{
    r_window_submit(os_wnd, r_window, &bucket->passes, ptr);
}

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal D_Bucket *
d_bucket_make(void)
{
    D_Bucket *bucket = push_array(d_thread_ctx->arena, D_Bucket, 1);
    D_BucketStackInits(bucket);
    return bucket;
}

internal void
d_push_bucket(D_Bucket *bucket)
{
    D_BucketSelectionNode *node = d_thread_ctx->free_bucket_selection;
    if(node)
    {
        SLLStackPop(d_thread_ctx->free_bucket_selection);
    }
    else
    {
        node = push_array(d_thread_ctx->arena, D_BucketSelectionNode, 1);
    }
    SLLStackPush(d_thread_ctx->top_bucket, node);
    node->bucket = bucket;
}

internal void
d_pop_bucket(void)
{
    D_BucketSelectionNode *node = d_thread_ctx->top_bucket;
    SLLStackPop(d_thread_ctx->top_bucket);
    SLLStackPush(d_thread_ctx->free_bucket_selection, node);
}

internal D_Bucket *
d_top_bucket(void)
{
    D_Bucket *bucket = 0;
    if(d_thread_ctx->top_bucket != 0)
    {
        bucket = d_thread_ctx->top_bucket->bucket;
    }
    return bucket;
}

//- rjf: 3d rendering pass params
internal R_PassParams_Geo3D *
d_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection, B32 show_grid, B32 show_gizmos, Mat4x4F32 gizmos_xform, Vec3F32 gizmos_origin)
{
    Arena *arena = d_thread_ctx->arena;
    D_Bucket *bucket = d_top_bucket();
    R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
    R_PassParams_Geo3D *params = pass->params_geo3d;
    params->viewport = viewport;
    params->view = view;
    params->projection = projection;
    params->show_grid = show_grid;
    params->show_gizmos = show_gizmos;
    params->gizmos_xform = gizmos_xform;
    params->gizmos_origin = v4f32(gizmos_origin.x, gizmos_origin.y, gizmos_origin.z, 1.0f);
    return params;
}

//- rjf: meshes

internal R_Mesh3DInst *
d_mesh(R_Handle mesh_vertices, R_Handle mesh_indices,
       R_GeoTopologyKind mesh_geo_topology, R_GeoVertexFlags mesh_geo_vertex_flags, R_Handle albedo_tex,
       Mat4x4F32 *joint_xforms, U64 joint_count,
       Mat4x4F32 inst_xform, U64 inst_key)
{
    Arena *arena = d_thread_ctx->arena;
    D_Bucket *bucket = d_top_bucket();
    R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
    R_PassParams_Geo3D *params = pass->params_geo3d;

    if(params->mesh_batches.slots_count == 0)
    {
        params->mesh_batches.slots_count = 64;
        params->mesh_batches.slots = push_array(arena, R_BatchGroup3DMapNode *, params->mesh_batches.slots_count);
    }

    // Hash batch group based on 3d params
    U64 hash = 0;
    U64 slot_idx = 0;
    {
        U64 buffer[] = {
            mesh_vertices.u64[0],
            mesh_vertices.u64[1],
            mesh_indices.u64[0],
            mesh_indices.u64[1],
            (U64)mesh_geo_topology,
            (U64)mesh_geo_vertex_flags,
            albedo_tex.u64[0],
            albedo_tex.u64[1],
            (U64)d_top_tex2d_sample_kind(),
        };
        hash = d_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
        slot_idx = hash%params->mesh_batches.slots_count;
    }

    // Map hash -> existing batch group node
    R_BatchGroup3DMapNode *node = 0;
    {
        for(R_BatchGroup3DMapNode *n = params->mesh_batches.slots[slot_idx]; n != 0; n = n->next)
        {
            if(n->hash == hash) {
                node = n;
                break;
            }
        }
    }

    // No batch group node? -> make a new one
    if(node == 0) 
    {
        node = push_array(arena, R_BatchGroup3DMapNode, 1);
        SLLStackPush(params->mesh_batches.slots[slot_idx], node);
        node->hash = hash;
        node->batches = r_batch_list_make(sizeof(R_Mesh3DInst));
        node->params.mesh_vertices          = mesh_vertices;
        node->params.mesh_indices           = mesh_indices;
        node->params.mesh_geo_topology      = mesh_geo_topology;
        node->params.mesh_geo_vertex_flags  = mesh_geo_vertex_flags;
        node->params.albedo_tex             = albedo_tex;
        node->params.albedo_tex_sample_kind = d_top_tex2d_sample_kind();
        node->params.xform                  = mat_4x4f32(1.0f);
    }

    // Push a new instance to the batch group, then return it
    R_Mesh3DInst *inst = (R_Mesh3DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
    inst->xform        = inst_xform;
    inst->key          = inst_key;
    inst->joint_xforms = joint_xforms;
    inst->joint_count  = joint_count;
    return inst;
}

//- rjf: collating one pre-prepped bucket into parent bucket

internal void
d_sub_bucket(D_Bucket *bucket)
{
    Arena *arena = d_thread_ctx->arena;
    D_Bucket *src = bucket;
    D_Bucket *dst = d_top_bucket();
    Rng2F32 dst_clip = d_top_clip();
    B32 dst_clip_is_set = !(dst_clip.x0 == 0 && dst_clip.x1 == 0 && dst_clip.y0 == 0 && dst_clip.y1 == 0);
    for(R_PassNode *n = src->passes.first; n != 0; n = n->next)
    {
        R_Pass *src_pass = &n->v;
        R_Pass *dst_pass = r_pass_from_kind(arena, &dst->passes, src_pass->kind);
        switch(dst_pass->kind)
        {
            default:{dst_pass->params = src_pass->params;}break;
            case R_PassKind_UI:
            {
                R_PassParams_UI *src_ui = src_pass->params_ui;
                R_PassParams_UI *dst_ui = dst_pass->params_ui;
                for(R_BatchGroup2DNode *src_group_n = src_ui->rects.first;
                        src_group_n != 0;
                        src_group_n = src_group_n->next)
                {
                    R_BatchGroup2DNode *dst_group_n = push_array(arena, R_BatchGroup2DNode, 1);
                    SLLQueuePush(dst_ui->rects.first, dst_ui->rects.last, dst_group_n);
                    dst_ui->rects.count += 1;
                    MemoryCopyStruct(&dst_group_n->params, &src_group_n->params);
                    dst_group_n->batches = src_group_n->batches;
                    dst_group_n->params.xform = d_top_xform2d();
                    if(dst_clip_is_set)
                    {
                        B32 clip_is_set = !(dst_group_n->params.clip.x0 == 0 &&
                                dst_group_n->params.clip.y0 == 0 &&
                                dst_group_n->params.clip.x1 == 0 &&
                                dst_group_n->params.clip.y1 == 0);
                        dst_group_n->params.clip = clip_is_set ? intersect_2f32(dst_clip, dst_group_n->params.clip) : dst_clip;
                    }
                }
            }break;
        }
    }
}

////////////////////////////////
// Fancy String Type Functions

internal D_FancyRunList
d_fancy_run_list_from_fancy_string_list(Arena *arena, F32 tab_size_px, F_RasterFlags flags, D_FancyStringList *strs) {
    D_FancyRunList run_list = {0};
    F32 base_align_px = 0;
    for(D_FancyStringNode *n = strs->first; n != 0; n = n->next)
    {
        D_FancyRunNode *dst_n = push_array(arena, D_FancyRunNode, 1);
        dst_n->v.run = f_push_run_from_string(arena, n->v.font, n->v.size,
                base_align_px, tab_size_px, flags, n->v.string);
        dst_n->v.color = n->v.color;
        dst_n->v.underline_thickness = n->v.underline_thickness;
        dst_n->v.strikethrough_thickness = n->v.strikethrough_thickness;
        SLLQueuePush(run_list.first, run_list.last, dst_n);
        run_list.node_count += 1;
        run_list.dim.x += dst_n->v.run.dim.x;
        run_list.dim.y = Max(run_list.dim.y, dst_n->v.run.dim.y);
        base_align_px += dst_n->v.run.dim.x;
    }
    return run_list;
}

//~ k: text rendering
internal void d_truncated_fancy_run_list(Vec2F32 p, D_FancyRunList *list, F32 max_x, F_Run trailer_run)
{
    // TODO: handle trailer_run, max_x, underline_thickness, trikethrough
    // B32 trailer_enabled = (list->dim.x > max_x && trailer_run.dim.x < max_x);

    for(D_FancyRunNode *n = list->first; n != 0; n = n->next)
    {
        F32 off_x = p.x;
        F32 off_y = p.y;

        F_Piece *piece_first = n->v.run.pieces.v;
        F_Piece *piece_opl = n->v.run.pieces.v + n->v.run.pieces.count;

        for(F_Piece *piece = piece_first; piece < piece_opl; piece++)
        {
            Rng2F32 dst = r2f32p(piece->rect.x0+off_x, piece->rect.y0+off_y,
                                 piece->rect.x1+off_x, piece->rect.y1+off_y);
            Rng2F32 src = r2f32p(piece->subrect.x0, piece->subrect.y0, piece->subrect.x1, piece->subrect.y1);
            Vec2F32 size = dim_2f32(dst);
            AssertAlways(!r_handle_match(piece->texture, r_handle_zero()));

            // NOTE(k): Space will have 0 extent
            if(size.x > 0 && size.y > 0)
            {
                d_rect(dst, v4f32(1,1,1,0.3), 1.0, 1.0, 1.0);
                d_img(dst, src, piece->texture, n->v.color, 0,0,0);
            }
        }
    }
}

////////////////////////////////
//~ rjf: Core Draw Calls
//
// (Apply to the calling thread's currently selected bucket)

internal inline R_Rect2DInst *
d_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
    Arena *arena = d_thread_ctx->arena;
    D_Bucket *bucket = d_top_bucket();
    AssertAlways(bucket != 0);
    R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI);
    R_PassParams_UI *params = pass->params_ui;
    R_BatchGroup2DList *rects = &params->rects;
    R_BatchGroup2DNode *node = rects->last;

    if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen)
    {
        node = push_array(arena, R_BatchGroup2DNode, 1);
        SLLQueuePush(rects->first, rects->last, node);
        rects->count += 1;
        node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
        node->params.tex             = r_handle_zero();
        node->params.tex_sample_kind = bucket->top_tex2d_sample_kind->v;
        node->params.xform           = bucket->top_xform2d->v;
        node->params.clip            = bucket->top_clip->v;
        node->params.transparency    = bucket->top_transparency->v;
    }
    R_Rect2DInst *inst = (R_Rect2DInst *)r_batch_list_push_inst(arena, &node->batches, 256);

    inst->dst                     = dst;
    inst->src                     = r2f32p(0, 0, 0, 0);
    inst->colors[Corner_00]       = color;
    inst->colors[Corner_10]       = color;
    inst->colors[Corner_11]       = color;
    inst->colors[Corner_01]       = color;
    inst->corner_radii[Corner_00] = corner_radius;
    inst->corner_radii[Corner_10] = corner_radius;
    inst->corner_radii[Corner_11] = corner_radius;
    inst->corner_radii[Corner_01] = corner_radius;
    inst->border_thickness        = border_thickness;
    inst->edge_softness           = edge_softness;
    inst->white_texture_override  = 1.f;
    bucket->last_cmd_stack_gen = bucket->stack_gen;
    return inst;
}

internal inline R_Rect2DInst *
d_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color,
      F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
    Arena *arena = d_thread_ctx->arena;
    D_Bucket *bucket = d_top_bucket();
    AssertAlways(bucket);
    R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI);
    R_PassParams_UI *params = pass->params_ui;
    R_BatchGroup2DList *rects = &params->rects;
    R_BatchGroup2DNode *node = rects->last;

    if(node != 0 && bucket->stack_gen == bucket->last_cmd_stack_gen && r_handle_match(node->params.tex, r_handle_zero()))
    {
        node->params.tex = texture; 
    }
    else if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen || !r_handle_match(node->params.tex, texture))
    {
        node = push_array(arena, R_BatchGroup2DNode, 1);
        SLLQueuePush(rects->first, rects->last, node);
        rects->count += 1;
        node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
        node->params.tex             = texture;
        node->params.tex_sample_kind = bucket->top_tex2d_sample_kind->v;
        node->params.xform           = bucket->top_xform2d->v;
        node->params.clip            = bucket->top_clip->v;
        node->params.transparency    = bucket->top_transparency->v;
    }

    R_Rect2DInst *inst = (R_Rect2DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
    inst->dst                     = dst;
    inst->src                     = src;
    inst->colors[Corner_00]       = color;
    inst->colors[Corner_10]       = color;
    inst->colors[Corner_11]       = color;
    inst->colors[Corner_01]       = color;
    inst->corner_radii[Corner_00] = corner_radius;
    inst->corner_radii[Corner_10] = corner_radius;
    inst->corner_radii[Corner_11] = corner_radius;
    inst->corner_radii[Corner_01] = corner_radius;
    inst->border_thickness        = border_thickness;
    inst->edge_softness           = edge_softness;
    inst->white_texture_override  = 0.f;
    bucket->last_cmd_stack_gen = bucket->stack_gen;
    return inst;
}

internal void
d_text_run(Vec2F32 p, Vec4F32 color, F_Run run)
{
    F_Piece *piece_first = run.pieces.v;
    F_Piece *piece_opl = run.pieces.v + run.pieces.count;

    S16 off_x = p.x;
    S16 off_y = p.y + run.ascent;
    for(F_Piece *piece = piece_first; piece < piece_opl; piece++)
    {
        Rng2F32 dst = r2f32p(piece->rect.x0+off_x, piece->rect.y0+off_y,
                             piece->rect.x1+off_x, piece->rect.y1+off_y);
        Rng2F32 src = r2f32p(piece->subrect.x0, piece->subrect.y0, piece->subrect.x1, piece->subrect.y1);
        Vec2F32 size = dim_2f32(dst);
        AssertAlways(!r_handle_match(piece->texture, r_handle_zero()));
        // NOTE(@k): Space will have 0 extent
        if(size.x > 0 && size.y > 0)
        {
            d_rect(dst, v4f32(1,1,1,0.3), 1.0, 1.0, 1.0);
            d_img(dst, src, piece->texture, color,0,0,0);
        }
    }
}

internal void
d_text(F_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, F_RasterFlags flags, Vec2F32 p, Vec4F32 color, String8 string) {
    // TODO(@k): make use of base_align_px and tab_size_px
    Temp scratch = scratch_begin(0,0);
    F_Run run = f_push_run_from_string(scratch.arena, font, size, base_align_px, tab_size_px, flags, string);
    d_text_run(p, color, run);
    scratch_end(scratch);
}

