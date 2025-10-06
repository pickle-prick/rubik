// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#define DR_StackPushImpl(name_upper, name_lower, type, val) \
DR_Bucket *bucket = dr_top_bucket();\
type old_val = bucket->top_##name_lower->v;\
DR_##name_upper##Node *node = push_array(dr_thread_ctx->arena, DR_##name_upper##Node, 1);\
node->v = (val);\
SLLStackPush(bucket->top_##name_lower, node);\
bucket->stack_gen += 1;\
return old_val

#define DR_StackPopImpl(name_upper, name_lower, type) \
DR_Bucket *bucket = dr_top_bucket();\
type popped_val = bucket->top_##name_lower->v;\
SLLStackPop(bucket->top_##name_lower);\
bucket->stack_gen += 1;\
return popped_val

#define DR_StackTopImpl(name_upper, name_lower, type) \
DR_Bucket *bucket = dr_top_bucket();\
type top_val = bucket->top_##name_lower->v;\
return top_val

#include "generated/draw.meta.c"

////////////////////////////////
//~ rjf: Basic Helpers

internal U64
dr_hash_from_string(String8 string)
{
  U64 result = 5381;
  for(U64 i = 0; i < string.size; i += 1)
  {
    result = ((result << 5) + result) + string.str[i];
  }
  return result;
}

////////////////////////////////
//~ rjf: Fancy String Type Functions

internal void
dr_fstrs_push(Arena *arena, DR_FStrList *list, DR_FStr *str)
{
  DR_FStrNode *n = push_array_no_zero(arena, DR_FStrNode, 1);
  MemoryCopyStruct(&n->v, str);
  SLLQueuePush(list->first, list->last, n);
  list->node_count += 1;
  list->total_size += str->string.size;
}

internal void
dr_fstrs_push_new_(Arena *arena, DR_FStrList *list, DR_FStrParams *params, DR_FStrParams *overrides, String8 string)
{
  DR_FStr fstr = {string, *params};
  if(!fnt_tag_match(fnt_tag_zero(), overrides->font))
  {
    fstr.params.font = overrides->font;
  }
  if(overrides->raster_flags != 0)
  {
    fstr.params.raster_flags = overrides->raster_flags;
  }
  if(overrides->color.x != 0 || overrides->color.y != 0 || overrides->color.z != 0 || overrides->color.w != 0)
  {
    fstr.params.color = overrides->color;
  }
  if(overrides->size != 0)
  {
    fstr.params.size = overrides->size;
  }
  if(overrides->underline_thickness != 0)
  {
    fstr.params.underline_thickness = overrides->underline_thickness;
  }
  if(overrides->strikethrough_thickness != 0)
  {
    fstr.params.strikethrough_thickness = overrides->strikethrough_thickness;
  }
  dr_fstrs_push(arena, list, &fstr);
}

internal void
dr_fstrs_concat_in_place(DR_FStrList *dst, DR_FStrList *to_push)
{
  if(dst->last != 0 && to_push->first != 0)
  {
    dst->last->next = to_push->first;
    dst->last = to_push->last;
    dst->node_count += to_push->node_count;
    dst->total_size += to_push->total_size;
  }
  else if(to_push->first != 0)
  {
    MemoryCopyStruct(dst, to_push);
  }
  MemoryZeroStruct(to_push);
}

internal DR_FStrList
dr_fstrs_copy(Arena *arena, DR_FStrList *src)
{
  DR_FStrList dst = {0};
  for(DR_FStrNode *src_n = src->first; src_n != 0; src_n = src_n->next)
  {
    DR_FStr fstr = src_n->v;
    fstr.string = push_str8_copy(arena, fstr.string);
    dr_fstrs_push(arena, &dst, &fstr);
  }
  return dst;
}

internal String8
dr_string_from_fstrs(Arena *arena, DR_FStrList *list)
{
  NotImplemented;
  // String8 result = {0};
  // {
  //   Temp scratch = scratch_begin(&arena, 1);
  //   String8List parts = {0};
  //   for(DR_FStrNode *n = list->first; n != 0; n = n->next)
  //   {
  //     if(!fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font))
  //     {
  //       str8_list_push(scratch.arena, &parts, n->v.string);
  //     }
  //   }
  //   result = str8_list_join(arena, &parts, 0);
  //   result = str8_skip_chop_whitespace(result);
  //   scratch_end(scratch);
  // }
  // return result;
}

internal FuzzyMatchRangeList
dr_fuzzy_match_find_from_fstrs(Arena *arena, DR_FStrList *fstrs, String8 needle)
{
  NotImplemented;
  // Temp scratch = scratch_begin(&arena, 1);
  // String8 fstrs_string = {0};
  // fstrs_string.size = fstrs->total_size;
  // fstrs_string.str = push_array(arena, U8, fstrs_string.size);
  // {
  //   // TODO(rjf): the fact that we only increment on non-icon portions is super weird?
  //   // we are only doing that because of the rendering of the fuzzy matches, so maybe
  //   // once that is straightened out, we can fix the code here too...
  //   U64 off = 0;
  //   for(DR_FStrNode *n = fstrs->first; n != 0; n = n->next)
  //   {
  //     if(!fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font))
  //     {
  //       MemoryCopy(fstrs_string.str + off, n->v.string.str, n->v.string.size);
  //       off += n->v.string.size;
  //     }
  //   }
  // }
  // FuzzyMatchRangeList ranges = fuzzy_match_find(arena, needle, fstrs_string);
  // scratch_end(scratch);
  // return ranges;
}

internal DR_FRunList
dr_fruns_from_fstrs(Arena *arena, F32 tab_size_px, DR_FStrList *strs)
{
  DR_FRunList run_list = {0};
  F32 base_align_px = 0;
  for(DR_FStrNode *n = strs->first; n != 0; n = n->next)
  {
    DR_FRunNode *dst_n = push_array(arena, DR_FRunNode, 1);
    dst_n->v.run = fnt_run_from_string(n->v.params.font, n->v.params.size, base_align_px, tab_size_px, n->v.params.raster_flags, n->v.string);
    dst_n->v.color = n->v.params.color;
    dst_n->v.underline_thickness = n->v.params.underline_thickness;
    dst_n->v.strikethrough_thickness = n->v.params.strikethrough_thickness;
    // FIXME: deal with it later
    // dst_n->v.icon = (fnt_tag_match(n->v.params.font, dr_thread_ctx->icon_font));
    SLLQueuePush(run_list.first, run_list.last, dst_n);
    run_list.node_count += 1;
    run_list.dim.x += dst_n->v.run.dim.x;
    run_list.dim.y = Max(run_list.dim.y, dst_n->v.run.dim.y);
    base_align_px += dst_n->v.run.dim.x;
  }
  return run_list;
}

internal Vec2F32
dr_dim_from_fstrs(F32 tab_size_px, DR_FStrList *fstrs)
{
  Temp scratch = scratch_begin(0, 0);
  DR_FRunList fruns = dr_fruns_from_fstrs(scratch.arena, tab_size_px, fstrs);
  Vec2F32 dim = fruns.dim;
  scratch_end(scratch);
  return dim;
}

////////////////////////////////
//~ rjf: Top-Level API
//
// (Frame boundaries)

internal void
dr_begin_frame()
{
  if(dr_thread_ctx == 0)
  {
    Arena *arena = arena_alloc(.reserve_size = GB(64), .commit_size = MB(8));
    dr_thread_ctx = push_array(arena, DR_ThreadCtx, 1);
    dr_thread_ctx->arena = arena;
    dr_thread_ctx->arena_frame_start_pos = arena_pos(arena);
  }
  arena_pop_to(dr_thread_ctx->arena, dr_thread_ctx->arena_frame_start_pos);
  dr_thread_ctx->free_bucket_selection = 0;
  dr_thread_ctx->top_bucket = 0;
}

internal void
dr_submit_bucket(OS_Handle os_window, R_Handle r_window, DR_Bucket *bucket)
{
  r_window_submit(os_window, r_window, &bucket->passes);
}

////////////////////////////////
//~ rjf: Bucket Construction & Selection API
//
// (Bucket: Handle to sequence of many render passes, constructed by this layer)

internal DR_Bucket *
dr_bucket_make(void)
{
  DR_Bucket *bucket = push_array(dr_thread_ctx->arena, DR_Bucket, 1);
  DR_BucketStackInits(bucket);
  return bucket;
}

internal void
dr_push_bucket(DR_Bucket *bucket)
{
  DR_BucketSelectionNode *node = dr_thread_ctx->free_bucket_selection;
  if(node)
  {
    SLLStackPop(dr_thread_ctx->free_bucket_selection);
  }
  else
  {
    node = push_array(dr_thread_ctx->arena, DR_BucketSelectionNode, 1);
  }
  SLLStackPush(dr_thread_ctx->top_bucket, node);
  node->bucket = bucket;
}

internal void
dr_pop_bucket(void)
{
  DR_BucketSelectionNode *node = dr_thread_ctx->top_bucket;
  SLLStackPop(dr_thread_ctx->top_bucket);
  SLLStackPush(dr_thread_ctx->free_bucket_selection, node);
}

internal DR_Bucket *
dr_top_bucket(void)
{
  DR_Bucket *bucket = 0;
  if(dr_thread_ctx->top_bucket != 0)
  {
    bucket = dr_thread_ctx->top_bucket->bucket;
  }
  return bucket;
}

internal B32
dr_bucket_is_empty(DR_Bucket *bucket)
{
  B32 ret = 1;
  for(R_PassNode *pass = bucket->passes.first; pass != 0; pass = pass->next)
  {
    switch(pass->v.kind)
    {
      case R_PassKind_Rect:
      {
        ret = pass->v.params_rect->rects.count == 0;
      }break;
      case R_PassKind_Geo2D:
      {
        ret = pass->v.params_geo2d->batches.array_size == 0;
      }break;
      case R_PassKind_Geo3D:
      {
        ret = pass->v.params_geo3d->mesh_batches.array_size == 0;
      }break;
      default:{ret = 0;}break;
    }
    if(ret == 0) break;
  }
  return ret;
}

////////////////////////////////
//~ rjf: Bucket Stacks
//
// (Pushing/popping implicit draw parameters)

// NOTE(rjf): (The implementation of the push/pop/top functions is auto-generated)

////////////////////////////////
//~ rjf: Core Draw Calls
//
// (Apply to the calling thread's currently selected bucket)

//- rjf: rectangles

internal inline R_Rect2DInst *
dr_rect(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Rect);
  R_PassParams_Rect *params = pass->params_rect;
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
  inst->has_pixel_id            = 0.f;
  inst->pixel_id                = v2f32(0,0);
  inst->line                    = v4f32(0,0,0,0);
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

internal inline R_Rect2DInst *
dr_rect_keyed(Rng2F32 dst, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness, Vec2F32 key)
{
  R_Rect2DInst *ret = dr_rect(dst, color, corner_radius, border_thickness, edge_softness);
  ret->pixel_id = key;
  ret->has_pixel_id = 1.0;
  return ret;
}

//- rjf: images

internal inline R_Rect2DInst *
dr_img(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Rect);
  R_PassParams_Rect *params = pass->params_rect;
  R_BatchGroupRectList *rects = &params->rects;
  R_BatchGroupRectNode *node = rects->last;
  Assert(!r_handle_match(r_handle_zero(), texture));

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
  inst->has_pixel_id            = 0.f;
  inst->pixel_id                = v2f32(0,0);
  inst->line                    = v4f32(0,0,0,0);
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

internal inline R_Rect2DInst *
dr_img_keyed(Rng2F32 dst, Rng2F32 src, R_Handle texture, Vec4F32 color, F32 corner_radius, F32 border_thickness, F32 edge_softness, Vec2F32 key)
{
  R_Rect2DInst *ret = dr_img(dst, src, texture, color, corner_radius, border_thickness, edge_softness);
  ret->pixel_id = key;
  ret->has_pixel_id = 1.0;
  return ret;
}

//- k: lines

internal inline R_Rect2DInst *
dr_line(Vec2F32 a, Vec2F32 b, Vec4F32 color, F32 line_thickness, F32 edge_softness)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Rect);
  R_PassParams_Rect *params = pass->params_rect;
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

  Rng2F32 dst = {.p0 = a, .p1 = b};
  if(dst.x0 > dst.x1) Swap(F32, dst.x0, dst.x1);
  if(dst.y0 > dst.y1) Swap(F32, dst.y0, dst.y1);
  dst = pad_2f32(dst, line_thickness*0.51);
  Vec2F32 center = center_2f32(dst);
  Vec2F32 p0 = sub_2f32(a, center);
  Vec2F32 p1 = sub_2f32(b, center);

  inst->dst                     = dst;
  inst->src                     = r2f32p(0, 0, 0, 0);
  inst->colors[Corner_00]       = color;
  inst->colors[Corner_10]       = color;
  inst->colors[Corner_11]       = color;
  inst->colors[Corner_01]       = color;
  inst->corner_radii[Corner_00] = 0;
  inst->corner_radii[Corner_10] = 0;
  inst->corner_radii[Corner_11] = 0;
  inst->corner_radii[Corner_01] = 0;
  inst->border_thickness        = 0;
  inst->edge_softness           = edge_softness;
  inst->line_thickness          = line_thickness;
  inst->white_texture_override  = 0.f;
  inst->omit_texture            = 1.f;
  inst->has_pixel_id            = 0.f;
  inst->pixel_id                = v2f32(0,0);
  inst->line                    = v4f32(p0.x, p0.y, p1.x, p1.y);
  bucket->last_cmd_stack_gen = bucket->stack_gen;
  return inst;
}

internal inline R_Rect2DInst *
dr_line_keyed(Vec2F32 a, Vec2F32 b, Vec4F32 color, F32 line_thickness, F32 edge_softness, Vec2F32 key)
{
  R_Rect2DInst *ret = dr_line(a, b, color, line_thickness, edge_softness);
  ret->pixel_id = key;
  ret->has_pixel_id = 1.0;
  return ret;
}

//- rjf: blurs

internal R_PassParams_Blur *
dr_blur(Rng2F32 rect, F32 blur_size, F32 corner_radius)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Blur);
  R_PassParams_Blur *params = pass->params_blur;
  params->rect = rect;
  params->clip = dr_top_clip();
  params->blur_size = blur_size;
  params->corner_radii[Corner_00] = corner_radius;
  params->corner_radii[Corner_01] = corner_radius;
  params->corner_radii[Corner_10] = corner_radius;
  params->corner_radii[Corner_11] = corner_radius;
  return params;
}

//- k: noise

internal R_PassParams_Noise *
dr_noise(Rng2F32 rect, F32 elapsed_secs)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Noise);
  R_PassParams_Noise *params = pass->params_noise;
  params->rect = rect;
  params->clip = dr_top_clip();
  params->elapsed_secs = elapsed_secs;
  return params;
}

//- k: edge

internal R_PassParams_Edge *
dr_edge(F32 elapsed_secs)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Edge);
  R_PassParams_Edge *params = pass->params_edge;
  params->elapsed_secs = elapsed_secs;
  return params;
}

//- k: crt

internal R_PassParams_Crt *
dr_crt(F32 warp, F32 scan, F32 elapsed_secs)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Crt);
  R_PassParams_Crt *params = pass->params_crt;
  params->warp = warp;
  params->scan = scan;
  params->elapsed_secs = elapsed_secs;
  return params;
}


//- rjf: 3d rendering pass params

internal R_PassParams_Geo3D *
dr_geo3d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo3D);
  R_PassParams_Geo3D *params = pass->params_geo3d;
  params->viewport = viewport;
  params->view = view;
  params->projection = projection;
  return params;
}

//- rjf: meshes

internal R_Mesh3DInst *
dr_mesh(R_Handle vertices, R_Handle indices, U64 vertex_buffer_offset, U64 indice_buffer_offset, U64 indice_count, R_GeoTopologyKind topology, R_GeoPolygonKind polygon, R_GeoVertexFlags vertex_flags, Mat4x4F32 *joint_xforms, U64 joint_count, U64 material_idx, F32 line_width, B32 retain_order)
{
  // NOTE(k): if joint_count > 0, then we can't do mesh instancing
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
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
      (U64)dr_top_tex2d_sample_kind(),
    };
    hash = dr_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
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
    node->params.diffuse_tex_sample_kind = dr_top_tex2d_sample_kind();
    node->params.mat_idx                 = material_idx;
  }

  // Push a new instance to the batch group, then return it
  R_Mesh3DInst *inst = (R_Mesh3DInst *)r_batch_list_push_inst(arena, &node->batches, 256);
  inst->joint_xforms  = joint_xforms;
  inst->joint_count   = joint_count;
  inst->material_idx  = material_idx;
  return inst;
}


//- k: 2d rendering pass params

internal R_PassParams_Geo2D *
dr_geo2d_begin(Rng2F32 viewport, Mat4x4F32 view, Mat4x4F32 projection)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo2D);
  R_PassParams_Geo2D *params = pass->params_geo2d;
  {
    params->viewport   = viewport;
    params->view       = view;
    params->projection = projection;
    // clip?
  }
  return params;
}

internal R_Mesh2DInst *
dr_sprite(R_Handle vertices, R_Handle indices,
          U64 vertex_buffer_offset, U64 indice_buffer_offset, U64 indice_count,
          R_GeoTopologyKind topology, R_GeoPolygonKind polygon, R_GeoVertexFlags vertex_flags,
          R_Handle tex, F32 line_width)
{
  Arena *arena = dr_thread_ctx->arena;
  DR_Bucket *bucket = dr_top_bucket();
  R_Pass *pass = r_pass_from_kind(arena, &bucket->passes, R_PassKind_Geo2D);
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
      (U64)dr_top_tex2d_sample_kind(),
    };
    hash = dr_hash_from_string(str8((U8 *)buffer, sizeof(buffer)));
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
    node->params.tex_sample_kind = dr_top_tex2d_sample_kind();
  }

  // push a new inst to the batch group, then return it
  R_Mesh2DInst *inst = (R_Mesh2DInst*)r_batch_list_push_inst(arena, &node->batches, 256);
  return inst;
}

//- rjf: collating one pre-prepped bucket into parent bucket

internal void
dr_sub_bucket(DR_Bucket *bucket)
{
  NotImplemented;
  // Arena *arena = dr_thread_ctx->arena;
  // DR_Bucket *src = bucket;
  // DR_Bucket *dst = dr_top_bucket();
  // Rng2F32 dst_clip = dr_top_clip();
  // B32 dst_clip_is_set = !(dst_clip.x0 == 0 && dst_clip.x1 == 0 &&
  //                         dst_clip.y0 == 0 && dst_clip.y1 == 0);
  // for(R_PassNode *n = src->passes.first; n != 0; n = n->next)
  // {
  //   R_Pass *src_pass = &n->v;
  //   R_Pass *dst_pass = r_pass_from_kind(arena, &dst->passes, src_pass->kind);
  //   switch(dst_pass->kind)
  //   {
  //     default:{dst_pass->params = src_pass->params;}break;
  //     case R_PassKind_Rect:
  //     {
  //       R_PassParams_Rect *src_ui = src_pass->params_rect;
  //       R_PassParams_Rect *dst_ui = dst_pass->params_rect;
  //       for(R_BatchGroup2DNode *src_group_n = src_ui->rects.first;
  //           src_group_n != 0;
  //           src_group_n = src_group_n->next)
  //       {
  //         R_BatchGroup2DNode *dst_group_n = push_array(arena, R_BatchGroup2DNode, 1);
  //         SLLQueuePush(dst_ui->rects.first, dst_ui->rects.last, dst_group_n);
  //         dst_ui->rects.count += 1;
  //         MemoryCopyStruct(&dst_group_n->params, &src_group_n->params);
  //         dst_group_n->batches = src_group_n->batches;
  //         dst_group_n->params.xform = dr_top_xform2d();
  //         B32 clip_is_set = !(dst_group_n->params.clip.x0 == 0 &&
  //                             dst_group_n->params.clip.y0 == 0 &&
  //                             dst_group_n->params.clip.x1 == 0 &&
  //                             dst_group_n->params.clip.y1 == 0);
  //         if(clip_is_set)
  //         {
  //           Rng2F32 og_clip = dst_group_n->params.clip;
  //           Mat3x3F32 xform = dst_group_n->params.xform;
  //           dst_group_n->params.clip = r2f32(xform_3f32(v3f32(og_clip.x0, og_clip.y0, 1), xform).xy,
  //                                            xform_3f32(v3f32(og_clip.x1, og_clip.y1, 1), xform).xy);
  //         }
  //         if(dst_clip_is_set)
  //         {
  //           dst_group_n->params.clip = clip_is_set ? intersect_2f32(dst_clip, dst_group_n->params.clip) : dst_clip;
  //         }
  //       }
  //     }break;
  //   }
  // }
}

////////////////////////////////
//~ rjf: Draw Call Helpers

//- rjf: text

internal void
dr_truncated_fancy_run_list(Vec2F32 p, DR_FRunList *list, F32 max_x, FNT_Run trailer_run)
{
  //- rjf: total advance > max? -> enable trailer
  B32 trailer_enabled = (list->dim.x > max_x && trailer_run.dim.x < max_x);
  
  //- rjf: draw runs
  F32 advance = 0;
  B32 trailer_found = 0;
  Vec4F32 last_color = {0};
  U64 byte_off = 0;
  for(DR_FRunNode *n = list->first; n != 0; n = n->next)
  {
    DR_FRun *fr = &n->v;
    Rng1F32 pixel_range = {0};
    {
      pixel_range.min = 100000;
      pixel_range.max = 0;
    }
    FNT_Piece *piece_first = fr->run.pieces.v;
    FNT_Piece *piece_opl = piece_first + fr->run.pieces.count;
    F32 pre_advance = advance;
    last_color = fr->color;
    for(FNT_Piece *piece = piece_first;
        piece < piece_opl;
        piece += 1)
    {
      if(trailer_enabled && advance + piece->advance > (max_x - trailer_run.dim.x))
      {
        trailer_found = 1;
        break;
      }
      if(!trailer_enabled && advance + piece->advance > max_x)
      {
        goto end_draw;
      }
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      if(!r_handle_match(texture, r_handle_zero()))
      {
        dr_img(dst, src, texture, fr->color, 0, 0, 0);
        // dr_rect(dst, v4f32(0, 1, 0, 0.5f), 0, 1.f, 0.f);
      }
      advance += piece->advance;
      pixel_range.min = Min(pre_advance, pixel_range.min);
      pixel_range.max = Max(advance, pixel_range.max);
    }
    if(fr->underline_thickness > 0)
    {
      dr_rect(r2f32p(p.x + pixel_range.min,
                     p.y+fr->run.descent+fr->run.descent/8,
                     p.x + pixel_range.max,
                     p.y+fr->run.descent+fr->run.descent/8+fr->underline_thickness),
                     fr->color, 0, 0, 0.8f);
    }
    if(fr->strikethrough_thickness > 0)
    {
      dr_rect(r2f32p(p.x+pre_advance, p.y+fr->run.descent - fr->run.ascent/2, p.x+advance, p.y+fr->run.descent - fr->run.ascent/2 + fr->strikethrough_thickness), fr->color, 0, 0, 1.f);
    }
    if(trailer_found)
    {
      break;
    }
  }
  end_draw:;
  
  //- rjf: draw trailer
  if(trailer_found)
  {
    FNT_Piece *piece_first = trailer_run.pieces.v;
    FNT_Piece *piece_opl = piece_first + trailer_run.pieces.count;
    F32 pre_advance = advance;
    Vec4F32 trailer_piece_color = last_color;
    for(FNT_Piece *piece = piece_first;
        piece < piece_opl;
        piece += 1)
    {
      R_Handle texture = piece->texture;
      Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
      Vec2F32 size = dim_2f32(src);
      Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                           p.y + piece->offset.y,
                           p.x + piece->offset.x + advance + size.x,
                           p.y + piece->offset.y + size.y);
      if(!r_handle_match(texture, r_handle_zero()))
      {
        dr_img(dst, src, texture, trailer_piece_color, 0, 0, 0);
        trailer_piece_color.w *= 0.5f;
      }
      advance += piece->advance;
    }
  }
}

internal void
dr_truncated_fancy_run_fuzzy_matches(Vec2F32 p, DR_FRunList *list, F32 max_x, FuzzyMatchRangeList *ranges, Vec4F32 color)
{
  for(FuzzyMatchRangeNode *match_n = ranges->first; match_n != 0; match_n = match_n->next)
  {
    Rng1U64 byte_range = match_n->range;
    Rng1F32 pixel_range = {0};
    {
      pixel_range.min = 100000;
      pixel_range.max = 0;
    }
    F32 last_piece_end_pad = 0;
    U64 byte_off = 0;
    F32 advance = 0;
    F32 ascent = 0;
    F32 descent = 0;
    for(DR_FRunNode *fr_n = list->first; fr_n != 0; fr_n = fr_n->next)
    {
      DR_FRun *fr = &fr_n->v;
      FNT_Run *run = &fr->run;
      ascent = run->ascent;
      descent = run->descent;
      for(U64 piece_idx = 0; piece_idx < run->pieces.count; piece_idx += 1)
      {
        FNT_Piece *piece = &run->pieces.v[piece_idx];
        if(contains_1u64(byte_range, byte_off))
        {
          F32 pre_advance  = advance + piece->offset.x;
          F32 post_advance = advance + piece->advance;
          pixel_range.min = Min(pre_advance,  pixel_range.min);
          pixel_range.max = Max(post_advance, pixel_range.max);
        }
        if(!fr->icon)
        {
          byte_off += piece->decode_size;
        }
        advance += piece->advance;
      }
    }
    if(pixel_range.min < pixel_range.max)
    {
      Rng2F32 rect = r2f32p(p.x + pixel_range.min - ascent/4.f,
                            p.y - descent - ascent - ascent/8.f,
                            p.x + pixel_range.max + ascent/4.f,
                            p.y - descent - ascent + ascent/8.f + list->dim.y);
      rect.x0 = Min(rect.x0, p.x+max_x);
      rect.x1 = Min(rect.x1, p.x+max_x);
      dr_rect(rect, color, (descent+ascent)/4.f, 0, 1.f);
    }
  }
}

internal void
dr_text_run(Vec2F32 p, Vec4F32 color, FNT_Run run)
{
  F32 advance = 0;
  FNT_Piece *piece_first = run.pieces.v;
  FNT_Piece *piece_opl = piece_first + run.pieces.count;
  for(FNT_Piece *piece = piece_first;
      piece < piece_opl;
      piece += 1)
  {
    R_Handle texture = piece->texture;
    Rng2F32 src = r2f32p((F32)piece->subrect.x0, (F32)piece->subrect.y0, (F32)piece->subrect.x1, (F32)piece->subrect.y1);
    Vec2F32 size = dim_2f32(src);
    Rng2F32 dst = r2f32p(p.x + piece->offset.x + advance,
                         p.y + piece->offset.y,
                         p.x + piece->offset.x + advance + size.x,
                         p.y + piece->offset.y + size.y);
    if(size.x != 0 && size.y != 0 && !r_handle_match(texture, r_handle_zero()))
    {
      dr_img(dst, src, texture, color, 0, 0, 0);
    }
    advance += piece->advance;
  }
}

internal void
dr_text(FNT_Tag font, F32 size, F32 base_align_px, F32 tab_size_px, FNT_RasterFlags flags, Vec2F32 p, Vec4F32 color, String8 string)
{
  Temp scratch = scratch_begin(0, 0);
  FNT_Run run = fnt_run_from_string(font, size, base_align_px, tab_size_px, flags, string);
  dr_text_run(p, color, run);
  scratch_end(scratch);
}
