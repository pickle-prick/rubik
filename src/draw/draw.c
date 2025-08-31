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
d_submit_bucket(OS_Handle os_wnd, R_Handle r_window, D_Bucket *bucket)
{
  r_window_submit(os_wnd, r_window, &bucket->passes);
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

internal B32
d_bucket_is_empty(D_Bucket *bucket)
{
  B32 ret = 1;
  for(R_PassNode *pass = bucket->passes.first; pass != 0; pass = pass->next)
  {
    switch(pass->v.kind)
    {
      case R_PassKind_UI:
      {
        ret = pass->v.params_ui->rects.count == 0;
      }break;
      case R_PassKind_Blur:
      {
        NotImplemented;
      }break;
      case R_PassKind_Geo2D:
      {
        ret = pass->v.params_geo2d->batches.array_size == 0;
      }break;
      case R_PassKind_Geo3D:
      {
        ret = pass->v.params_geo3d->mesh_batches.array_size == 0;
      }break;
      default:{InvalidPath;}break;
    }
    if(ret == 0) break;
  }
  return ret;
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
d_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D, 0);
  R_PassParams_Geo3D *params = pass->params_geo3d;
  {
    params->viewport   = viewport;
    params->view       = view;
    params->projection = projection;
  }
  return params;
}

//- k: 2d rendering pass params
internal R_PassParams_Geo2D *
d_geo2d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo2D, 0);
  R_PassParams_Geo2D *params = pass->params_geo2d;
  {
    params->viewport   = viewport;
    params->view       = view;
    params->projection = projection;
    // clip?
  }
  return params;
}

//- k: meshes

internal R_Mesh3DInst *
d_mesh(R_Handle vertices, R_Handle indices,
       U64 vertex_buffer_offset, U64 indice_buffer_offset, U64 indice_count,
       R_GeoTopologyKind topology, R_GeoPolygonKind polygon, R_GeoVertexFlags vertex_flags,
       Mat4x4F32 *joint_xforms, U64 joint_count,
       U64 material_idx,
       F32 line_width, B32 retain_order)
{
  // NOTE(k): if joint_count > 0, then we can't do mesh instancing
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D, 1);
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
    U64 line_width_f64 = (F64)line_width;
    U64 buffer[] = {
      vertices.u64[0],
      vertices.u64[1],
      indices.u64[0],
      indices.u64[1],
      vertex_buffer_offset,
      indice_buffer_offset,
      indice_count,
      (U64)topology,
      (U64)polygon,
      (U64)vertex_flags,
      material_idx,
      *(U64 *)(&line_width_f64),
      // NOTE(k): only for diffuse texture
      (U64)d_top_tex2d_sample_kind(),
    };
    hash = d_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
    slot_idx = hash%params->mesh_batches.slots_count;
  }

  // Map hash -> existing batch group node
  B32 hash_existed = 0;
  R_BatchGroup3DMapNode *node = 0;
  for(R_BatchGroup3DMapNode *n = params->mesh_batches.slots[slot_idx]; n != 0; n = n->next)
  {
    if(n->hash == hash) {
      node = n;
      hash_existed = 1;
      break;
    }
  }

  if(retain_order)
  {
    node = 0;
    // NOTE(k): if retain order is required, we only reuse group if last one has the same hash
    R_BatchGroup3DMapNode *last = params->mesh_batches.last;
    if(last && last->hash == hash)
    {
      node = last;
    }
  }

  // No batch group node? -> make a new one
  if(node == 0) 
  {
    node = push_array(arena, R_BatchGroup3DMapNode, 1);

    // insert into hash table
    if(!hash_existed) SLLStackPush(params->mesh_batches.slots[slot_idx], node);
    // push back to array 
    DLLPushBack_NP(params->mesh_batches.first, params->mesh_batches.last, node, insert_next, insert_prev);
    params->mesh_batches.array_size++;

    node->hash                           = hash;
    node->batches                        = r_batch_list_make(sizeof(R_Mesh3DInst));
    node->params.mesh_vertices           = vertices;
    node->params.vertex_buffer_offset    = vertex_buffer_offset;
    node->params.mesh_indices            = indices;
    node->params.indice_buffer_offset    = indice_buffer_offset;
    node->params.indice_count            = indice_count;
    node->params.line_width              = line_width;
    node->params.mesh_geo_topology       = topology;
    node->params.mesh_geo_polygon        = polygon;
    node->params.mesh_geo_vertex_flags   = vertex_flags;
    node->params.diffuse_tex_sample_kind = d_top_tex2d_sample_kind();
    node->params.mat_idx                 = material_idx;
  }

  // Push a new instance to the batch group, then return it
  R_Mesh3DInst *inst = (R_Mesh3DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->joint_xforms  = joint_xforms;
  inst->joint_count   = joint_count;
  inst->material_idx  = material_idx;
  return inst;
}

internal R_Mesh2DInst *
d_sprite(R_Handle vertices, R_Handle indices,
         U64 vertex_buffer_offset, U64 indice_buffer_offset, U64 indice_count,
         R_GeoTopologyKind topology, R_GeoPolygonKind polygon, R_GeoVertexFlags vertex_flags,
         R_Handle tex, F32 line_width)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo2D, 1);
  R_PassParams_Geo2D *params = pass->params_geo2d;

  if(params->batches.slots_count == 0)
  {
    params->batches.slots_count = 64;
    params->batches.slots = push_array(arena, R_BatchGroup2DMapNode*, params->batches.slots_count);
  }

  // TODO(k): since we're using dynamic drawing list, we won't have any grouping, the cost of draw issuing will be huge

  // Hash batch group based on some params
  U64 hash = 0;
  U64 slot_idx = 0;
  {
    U64 line_width_f64 = (F64)line_width;
    U64 buffer[] = {
      vertices.u64[0],
      vertices.u64[1],
      indices.u64[0],
      indices.u64[1],
      vertex_buffer_offset,
      indice_buffer_offset,
      (U64)topology,
      (U64)polygon,
      (U64)vertex_flags,
      *(U64 *)(&line_width_f64),
      (U64)d_top_tex2d_sample_kind(),
    };
    hash = d_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
    slot_idx = hash%params->batches.slots_count;
  }

  // map hash -> existing batch group node
  B32 hash_existed = 0;
  R_BatchGroup2DMapNode *node = 0;
  for(R_BatchGroup2DMapNode *n = params->batches.slots[slot_idx]; n != 0; n = n->next)
  {
    if(n->hash == hash) {
      node = n;
      hash_existed = 1;
      break;
    }
  }

  // no batch group node? -> make a new one then
  if(node == 0)
  {
    node = push_array(arena, R_BatchGroup2DMapNode, 1);

    // insert into hash table
    if(!hash_existed)
    {
      SLLStackPush(params->batches.slots[slot_idx], node);
    }

    // push back to array 
    DLLPushBack_NP(params->batches.first, params->batches.last, node, insert_next, insert_prev);
    params->batches.array_size++;

    // fill info
    node->hash = hash;
    node->batches = r_batch_list_make(sizeof(R_Mesh2DInst));
    node->params.vertices = vertices;
    node->params.indices = indices;
    node->params.vertex_buffer_offset = vertex_buffer_offset;
    node->params.indice_buffer_offset = indice_buffer_offset;
    node->params.indice_count = indice_count;
    node->params.topology = topology;
    node->params.polygon = polygon;
    node->params.vertex_flags = vertex_flags;
    node->params.line_width = line_width;
    node->params.tex = tex;
    node->params.tex_sample_kind = d_top_tex2d_sample_kind();
  }

  // push a new inst to the batch group, then return it
  R_Mesh2DInst *inst = (R_Mesh2DInst*)r_batch_list_push_inst(arena, &node->batches, 256);
  return inst;
}

//- rjf: collating one pre-prepped bucket into parent bucket

internal void
d_sub_bucket(D_Bucket *bucket, B32 merge_pass)
{
  ProfBeginFunction();
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *src = bucket;
  D_Bucket *dst = d_top_bucket();
  Rng2F32 dst_clip = d_top_clip();
  B32 dst_clip_is_set = !(dst_clip.x0 == 0 && dst_clip.x1 == 0 && dst_clip.y0 == 0 && dst_clip.y1 == 0);
  for(R_PassNode *n = src->passes.first; n != 0; n = n->next)
  {
    R_Pass *src_pass = &n->v;
    R_Pass *dst_pass = r_pass_from_kind(arena, &dst->passes, src_pass->kind, merge_pass);
    switch(dst_pass->kind)
    {
      default:{dst_pass->params = src_pass->params;}break;
      case R_PassKind_UI:
      {
        R_PassParams_UI *src_ui = src_pass->params_ui;
        R_PassParams_UI *dst_ui = dst_pass->params_ui;
        for(R_BatchGroupRectNode *src_group_n = src_ui->rects.first; src_group_n != 0; src_group_n = src_group_n->next)
        {
          R_BatchGroupRectNode *dst_group_n = push_array(arena, R_BatchGroupRectNode, 1);
          SLLQueuePush(dst_ui->rects.first, dst_ui->rects.last, dst_group_n);
          dst_ui->rects.count += 1;
          MemoryCopyStruct(&dst_group_n->params, &src_group_n->params);
          dst_group_n->batches = src_group_n->batches;
          // dst_group_n->params.xform = d_top_xform2d();
          dst_group_n->params.xform = mul_3x3f32(d_top_xform2d(), dst_group_n->params.xform);
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
  ProfEnd();
}

////////////////////////////////
// Fancy String Type Functions

internal void
d_fancy_string_list_push(Arena *arena, D_FancyStringList *list, D_FancyString *str)
{
  D_FancyStringNode *node = push_array(arena, D_FancyStringNode, 1);
  node->v = *str;
  SLLQueuePush(list->first, list->last, node);
  list->node_count++;
  list->total_size += str->string.size;
}

internal void
d_fancy_string_list_concat_in_place(D_FancyStringList *dst, D_FancyStringList *to_push)
{
  NotImplemented;
}

internal String8
d_string_from_fancy_string_list(Arena *arena, D_FancyStringList *list)
{
  NotImplemented;
}

// NOTE(k): this is for a single line
internal D_FancyRunList
d_fancy_run_list_from_fancy_string_list(Arena *arena, F32 tab_size_px, F_RasterFlags flags, D_FancyStringList *strs)
{
  ProfBeginFunction();
  D_FancyRunList run_list = {0};
  F32 base_align_px = 0;
  for(D_FancyStringNode *n = strs->first; n != 0; n = n->next)
  {
    D_FancyRunNode *dst_n = push_array(arena, D_FancyRunNode, 1);
    dst_n->v.run = f_push_run_from_string(arena, n->v.font, n->v.size, base_align_px, tab_size_px, flags, n->v.string);
    dst_n->v.color = n->v.color;
    dst_n->v.underline_thickness = n->v.underline_thickness;
    dst_n->v.strikethrough_thickness = n->v.strikethrough_thickness;
    SLLQueuePush(run_list.first, run_list.last, dst_n);
    run_list.node_count += 1;
    run_list.dim.x += dst_n->v.run.dim.x;
    run_list.dim.y = Max(run_list.dim.y, dst_n->v.run.dim.y);
    base_align_px += dst_n->v.run.dim.x;
  }
  ProfEnd();
  return run_list;
}

internal D_FancyRunList
d_fancy_run_list_copy(Arena *arena, D_FancyRunList *src)
{
  NotImplemented;
}

//~ k: text rendering
internal void d_truncated_fancy_run_list(Vec2F32 p, D_FancyRunList *list, F32 max_x, F_Run trailer_run)
{
  ProfBeginFunction();
  // TODO: handle underline_thickness, trikethrough
  B32 trailer_enabled = ((p.x+list->dim.x) > max_x && (p.x+trailer_run.dim.x) < max_x);

  F32 off_x = p.x;
  F32 off_y = p.y;
  F32 advance = 0;
  B32 trailer_found = 0;
  Vec4F32 last_color = {0};
  for(D_FancyRunNode *n = list->first; n != 0; n = n->next)
  {
    F_Piece *piece_first = n->v.run.pieces.v;
    F_Piece *piece_opl = n->v.run.pieces.v + n->v.run.pieces.count;

    for(F_Piece *piece = piece_first; piece < piece_opl; piece++)
    {
      if(trailer_enabled && (off_x+advance+piece->advance) > (max_x-trailer_run.dim.x))
      {
        trailer_found = 1; 
        break;
      }

      if(!trailer_enabled && (off_x+advance+piece->advance) > max_x)
      {
        goto end_draw;
      }

      Rng2F32 dst = r2f32p(piece->rect.x0+off_x, piece->rect.y0+off_y, piece->rect.x1+off_x, piece->rect.y1+off_y);
      Rng2F32 src = r2f32p(piece->subrect.x0, piece->subrect.y0, piece->subrect.x1, piece->subrect.y1);
      // TODO(BUG): src wil be all zeros in gcc release build causing a crash but not with clang
      AssertAlways((src.x0 + src.x1 + src.y0 + src.y1) != 0);
      Vec2F32 size = dim_2f32(dst);
      AssertAlways(!r_handle_match(piece->texture, r_handle_zero()));
      last_color = n->v.color;

      // NOTE(k): Space will have 0 extent
      if(size.x > 0 && size.y > 0)
      {
        if(0)
        {
          d_rect(dst, n->v.color, 1.0, 1.0, 1.0);
        }
        d_img(dst, src, piece->texture, n->v.color, 0,0,0);
      }

      advance += piece->advance;
    }

    // TODO(k): underline
    // TODO(k): strikethrough

    if(trailer_found)
    {
      break;
    }
  }

  end_draw:;

  // draw trailer
  if(trailer_found)
  {
    off_x += advance;
    F_Piece *piece_first = trailer_run.pieces.v;
    F_Piece *piece_opl = trailer_run.pieces.v + trailer_run.pieces.count;
    Vec4F32 trailer_piece_color = last_color;

    for(F_Piece *piece = piece_first; piece < piece_opl; piece++)
    {
      R_Handle texture = piece->texture;
      Rng2F32 dst = r2f32p(piece->rect.x0+off_x, piece->rect.y0+off_y, piece->rect.x1+off_x, piece->rect.y1+off_y);
      Rng2F32 src = r2f32p(piece->subrect.x0, piece->subrect.y0, piece->subrect.x1, piece->subrect.y1);
      if(!r_handle_match(texture, r_handle_zero()))
      {
        d_img(dst, src, texture, trailer_piece_color, 0,0,0);
      }
      advance += piece->advance;
    }
  }
  ProfEnd();
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
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI, 1);
  R_PassParams_UI *params = pass->params_ui;
  R_BatchGroupRectList *rects = &params->rects;
  R_BatchGroupRectNode *node = rects->last;

  if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen)
  {
    node = push_array(arena, R_BatchGroupRectNode, 1);
    SLLQueuePush(rects->first, rects->last, node);
    rects->count += 1;
    node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
    node->params.tex             = r_handle_zero();
    node->params.viewport        = bucket->top_viewport->v;
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
  inst->white_texture_override  = 0.f;
  inst->omit_texture            = 1.f;
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
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_UI, 1);
  R_PassParams_UI *params = pass->params_ui;
  R_BatchGroupRectList *rects = &params->rects;
  R_BatchGroupRectNode *node = rects->last;

  if(node != 0 && bucket->stack_gen == bucket->last_cmd_stack_gen && r_handle_match(node->params.tex, r_handle_zero()))
  {
    node->params.tex = texture; 
  }
  else if(node == 0 || bucket->stack_gen != bucket->last_cmd_stack_gen || !r_handle_match(node->params.tex, texture))
  {
    node = push_array(arena, R_BatchGroupRectNode, 1);
    SLLQueuePush(rects->first, rects->last, node);
    rects->count += 1;
    node->batches = r_batch_list_make(sizeof(R_Rect2DInst));
    node->params.tex             = texture;
    node->params.viewport        = bucket->top_viewport->v;
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
  inst->omit_texture            = 0.f;
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

//- rjf: blurs
internal R_PassParams_Blur *
d_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Blur, 0);
  R_PassParams_Blur *params = pass->params_blur;
  params->rect = rect;
  params->clip = d_top_clip();
  params->blur_size = blur_size;
  params->corner_radii[Corner_00] = corner_radius;
  params->corner_radii[Corner_01] = corner_radius;
  params->corner_radii[Corner_10] = corner_radius;
  params->corner_radii[Corner_11] = corner_radius;
  return params;
}

//- k: noisee
internal R_PassParams_Noise *
d_noise(Rng2F32 rect, F32 elapsed_secs)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Noise, 0);
  R_PassParams_Noise *params = pass->params_noise;
  params->rect = rect;
  params->clip = d_top_clip();
  params->elapsed_secs = elapsed_secs;
  return params;
}

//- k: edge
internal R_PassParams_Edge *
d_edge(F32 elapsed_secs)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Edge, 0);
  R_PassParams_Edge *params = pass->params_edge;
  params->elapsed_secs = elapsed_secs;
  return params;
}

//- k: crt
internal R_PassParams_Crt *
d_crt(F32 warp, F32 scan, F32 elapsed_secs)
{
  Arena *arena = d_thread_ctx->arena;
  D_Bucket *bucket = d_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Crt, 0);
  R_PassParams_Crt *params = pass->params_crt;
  params->warp = warp;
  params->scan = scan;
  params->elapsed_secs = elapsed_secs;
  return params;
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
