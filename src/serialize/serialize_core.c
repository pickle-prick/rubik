internal void
se_push_parent(SE_Node *v)
{
  SE_ParentNode *n = push_array(se_g_top_arena, SE_ParentNode, 1);
  n->v = v;
  SLLStackPush(se_g_top_parent, n);
}

internal void
se_pop_parent()
{
  SLLStackPop(se_g_top_parent);
}

internal void
se_build_begin(Arena *arena)
{
  se_g_top_arena = arena;
}

internal void 
se_build_end()
{
  se_g_top_arena = 0;
}

internal SE_Node *
se_push_node(SE_NodeKind kind, String8 tag, B32 has_value, void *value, U64 value_size)
{
  SE_Node *ret = push_array(se_g_top_arena, SE_Node, 1);
  SE_ParentNode *parent = se_g_top_parent;

  if(parent)
  {
    if(has_value)
    {
      MemoryCopy(&ret->v, value, value_size);
    }
    ret->parent = parent->v;
    DLLPushBack(parent->v->first, parent->v->last, ret);
    parent->v->children_count++;
  }

  ret->tag = push_str8_copy(se_g_top_arena, tag);
  ret->kind = kind;
  return ret;
}

internal SE_Node *
se_str_with_tag(String8 tag, String8 v)
{
  SE_Node *ret = 0;
  String8 string = push_str8_copy(se_g_top_arena, v);
  ret = se_push_node(SE_NodeKind_String, tag, 1, &string, sizeof(String8));
  return ret;
}

internal SE_Node *
se_multiline_str_with_tag(String8 tag, String8 v)
{
  SE_Node *ret = se_array_with_tag(tag);
  se_push_parent(ret);
  
  char *by = "\n";
  String8List lines = str8_split(se_g_top_arena, v, (U8*)by, 1, StringSplitFlag_KeepEmpties);
  for(String8Node *n = lines.first; n != 0; n = n->next)
  {
    se_str(n->string);
  }

  se_pop_parent();
  return ret;
}

////////////////////////////////
//~ k: Helper functions

internal SE_NodeRec
se_node_rec_df(SE_Node *node, SE_Node *root, U64 sib_member_off, U64 child_member_off)
{
  SE_NodeRec ret = {0};
  if(*MemberFromOffset(SE_Node **, node, child_member_off) != 0)
  {
    ret.next = *MemberFromOffset(SE_Node **, node, child_member_off);
    ret.push_count = 1;
  }
  else for(SE_Node *n = node; n != 0 && n != root; n = n->parent)
  {
    if(*MemberFromOffset(SE_Node**, n, sib_member_off) != 0)
    {
      ret.next = *MemberFromOffset(SE_Node**, n, sib_member_off);
      break;
    }
    ret.pop_count++;
  }
  return ret;
}

internal SE_Node *
se_child_from_tag(SE_Node *node, String8 tag)
{
  SE_Node *ret = 0;
  for(SE_Node *n = node->first; n != 0; n = n->next)
  {
    if(str8_match(tag, n->tag, 0))
    {
      ret = n;
      break;
    }
  }
  return ret;
}

internal S64
se_s64_from_tag(SE_Node *struct_node, String8 tag)
{
  S64 ret = 0;
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_S64);
   ret = node->v.se_s64;
  }
  return ret;
}

internal U64
se_u64_from_tag(SE_Node *struct_node, String8 tag)
{
  U64 ret = 0;
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_U64);
   ret = node->v.se_u64;
  }
  return ret;
}

internal F32
se_f32_from_tag(SE_Node *struct_node, String8 tag)
{
  F32 ret = 0;
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_F32);
   ret = node->v.se_f32;
  }
  return ret;
}

internal B32
se_b32_from_tag(SE_Node *struct_node, String8 tag)
{
  B32 ret = 0;
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_B32);
   ret = node->v.se_b32;
  }
  return ret;
}

internal String8 
se_str_from_tag(SE_Node *struct_node, String8 tag)
{
  String8 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_String);
   ret = node->v.se_str;
  }
  return ret;
}

internal Vec2U64
se_v2u64_from_tag(SE_Node *struct_node, String8 tag)
{
  Vec2U64 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Vec2U64);
   ret = node->v.se_v2u64;
  }
  return ret;
}

internal Vec2F32
se_v2f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Vec2F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Vec2F32);
   ret = node->v.se_v2f32;
  }
  return ret;
}

internal Vec3F32
se_v3f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Vec3F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Vec3F32);
   ret = node->v.se_v3f32;
  }
  return ret;
}

internal Vec4F32
se_v4f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Vec4F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Vec4F32);
   ret = node->v.se_v4f32;
  }
  return ret;
}

internal Mat2x2F32
se_2x2f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Mat2x2F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Mat2x2F32);
   ret = node->v.se_2x2f32;
  }
  return ret;
}

internal Mat3x3F32
se_3x3f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Mat3x3F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Mat3x3F32);
   ret = node->v.se_3x3f32;
  }
  return ret;
}

internal Mat4x4F32
se_4x4f32_from_tag(SE_Node *struct_node, String8 tag)
{
  Mat4x4F32 ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Mat4x4F32);
   ret = node->v.se_4x4f32;
  }
  return ret;
}

internal SE_Handle
se_handle_from_tag(SE_Node *struct_node, String8 tag)
{
  SE_Handle ret = {0};
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
   Assert(node->kind == SE_NodeKind_Handle);
   ret = node->v.se_handle;
  }
  return ret;
}

internal SE_Node *
se_arr_from_tag(SE_Node *struct_node, String8 tag)
{
  SE_Node *ret = 0;
  SE_Node *node = se_child_from_tag(struct_node, tag);
  if(node)
  {
    if(node->kind == SE_NodeKind_Array) ret = node->first;
  }
  return ret;
}

internal SE_Node *
se_struct_from_tag(SE_Node *struct_node, String8 tag)
{
  SE_Node *ret = se_child_from_tag(struct_node, tag);
  if(ret) Assert(ret->kind == SE_NodeKind_Struct);
  return ret;
}
