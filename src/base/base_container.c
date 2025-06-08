/////////////////////////////////////////////////////////////////////////////////////////
// Dynamic array

#define DARRAY_RAW_DATA(darray) ((U64 *)(darray)-2)
#define DARRAY_CAPACITY(darray) (DARRAY_RAW_DATA(darray)[0])
#define DARRAY_OCCUPIED(darray) (DARRAY_RAW_DATA(darray)[1])
#define DARRAY_HEADER_SIZE      sizeof(U64) * 2

internal U64
darray_size(void *darray)
{
  return darray != NULL ? DARRAY_OCCUPIED(darray) : 0;
}

internal void darray_clear(void *darray)
{
  if(darray != NULL)
  {
    DARRAY_OCCUPIED(darray) = 0;
  }
}

internal void * 
darray_hold(Arena *arena, void *darray, U64 count, U64 item_size)
{
  assert(count > 0 && item_size > 0);
  if(darray == NULL)
  {
    U64 raw_data_size = DARRAY_HEADER_SIZE + count * item_size;
    U64 *base = (U64 *)push_array(arena, U64, raw_data_size);
    base[0] = count;
    base[1] = count;
    return (void *)(base + 2);
  }
  else if(DARRAY_OCCUPIED(darray) + count <= DARRAY_CAPACITY(darray))
  {
    DARRAY_OCCUPIED(darray) += count;
    return darray;
  }
  else
  {
    // reallocate
    U64 size_required = DARRAY_OCCUPIED(darray) + count;
    U64 double_curr = DARRAY_CAPACITY(darray) * 2;
    U64 capacity = size_required > double_curr ? size_required : double_curr;
    U64 occupied = size_required;
    U64 raw_size = DARRAY_HEADER_SIZE + capacity * item_size;

    U64 *base = (U64 *)push_array(arena, U64, raw_size);
    MemoryCopy(base+2, darray, DARRAY_OCCUPIED(darray) * item_size);

    base[0] = capacity;
    base[1] = occupied;
    return (void *)(base + 2);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Quad Tree

internal QuadTree *
quadtree_push(Arena *arena, Rng2F32 rect)
{
  QuadTree *ret = push_array(arena, QuadTree, 1);
  
  Vec2F32 sub_size = scale_2f32(dim_2f32(rect), 0.5);
  F32 xs[3] = {rect.x0, rect.x0+sub_size.x, rect.x1};
  F32 ys[3] = {rect.y0, rect.y0+sub_size.y, rect.y1};

  ret->sub_rects[0] = r2f32p(xs[0], ys[0], xs[1], ys[1]); /* NW */
  ret->sub_rects[1] = r2f32p(xs[1], ys[0], xs[2], ys[1]); /* NE */
  ret->sub_rects[2] = r2f32p(xs[0], ys[1], xs[1], ys[2]); /* SW */
  ret->sub_rects[3] = r2f32p(xs[1], ys[1], xs[2], ys[2]); /* SE */
  ret->rect = rect;
  return ret;
}

internal void
values_from_quadtree(Arena *arena, QuadTree *qt, void ***out)
{
  // push self values
  if(qt->values != 0)
  {
    for(U64 i = 0; i < darray_size(qt->values); i++)
    {
      darray_push(arena, *out, qt->values[i]->v);
    }
  }

  for(U64 i = 0; i < 4; i++)
  {
    if(qt->subs[i] != 0)
    {
      values_from_quadtree(arena, qt->subs[i], out);
    }
  }
}

internal void
quadtree_insert(Arena *arena, QuadTree *qt, Rng2F32 src_rect, void *value)
{
  // NOTE: src_rect is fully contained within the dst_rect 
  if(contains_22f32(qt->rect, src_rect))
  {
    B32 inserted = 0;
    for(U64 i = 0; i < 4; i++)
    {
      Rng2F32 sub_rect = qt->sub_rects[i];
      B32 contained = contains_22f32(sub_rect, src_rect);
      if(contained)
      {
        if(qt->subs[i] == 0)
        {
          qt->subs[i] = quadtree_push(arena, sub_rect);
        }
        quadtree_insert(arena, qt->subs[i], src_rect, value);
        inserted = 1;
        break;
      }
    }

    if(!inserted)
    {
      QuadTreeValue *v = push_array(arena, QuadTreeValue, 1);
      v->rect = src_rect;
      v->v = value;
      darray_push(arena, qt->values, v);
    }
  }
}

internal void
quadtree_query(Arena *arena, QuadTree *qt, Rng2F32 src_rect, void ***out)
{
  // ProfBeginFunction();
  if(overlaps_2f32(qt->rect, src_rect))
  {
    // first, check for values belonging to this qt, add them to the list
    if(qt->values != 0)
    {
      for(U64 i = 0; i < darray_size(qt->values); i++)
      {
        QuadTreeValue *value = qt->values[i];
        if(overlaps_2f32(value->rect, src_rect))
        {
          darray_push(arena, *out, value->v);
        }
      }
    }

    // second, recurse through children and check if they can add to the list
    for(U64 i = 0; i < 4; i++)
    {
      QuadTree *sub = qt->subs[i];
      if(sub != 0)
      {
        // if child is entirely contained within src_rect
        // we can just recursively add all of its children
        // no need to check intersection
        if(contains_22f32(src_rect, sub->rect))
        {
          values_from_quadtree(arena, sub, out);
        }
        else if(overlaps_2f32(src_rect, sub->rect))
        {
          quadtree_query(arena, sub, src_rect, out);
        }
      }
    }
  }
  // ProfEnd();
}
