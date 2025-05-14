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

// TODO(XXX): do we need these functions
// internal SE_Node *
// se_node_from_tag(SE_Node *first_node, String8 tag)
// {
//   SE_Node *ret = 0;
//   for(SE_Node *n = first_node; n != 0; n = n->next)
//   {
//     if(str8_match(tag, n->tag, 0))
//     {
//       ret = n;
//       break;
//     }
//   }
//   return ret;
// }
// 
// internal S64 
// se_s64_from_struct(SE_Node *s, String8 tag)
// {
//   SE_Node *field_node = se_node_from_tag(s->first, tag);
//   Assert(field_node->kind == SE_NodeKind_Int);
//   return field_node->v.se_int;
// }
// 
// internal U64 
// se_u64_from_struct(SE_Node *s, String8 tag)
// {
//   SE_Node *field_node = se_node_from_tag(s->first, tag);
//   Assert(field_node->kind == SE_NodeKind_Uint);
//   return field_node->v.se_uint;
// }
// 
// internal F64 
// se_f64_from_struct(SE_Node *s, String8 tag)
// {
//   SE_Node *field_node = se_node_from_tag(s->first, tag);
//   Assert(field_node->kind == SE_NodeKind_Float);
//   return field_node->v.se_float;
// }
// 
// internal B32 
// se_b32_from_struct(SE_Node *s, String8 tag)
// {
//   SE_Node *field_node = se_node_from_tag(s->first, tag);
//   Assert(field_node->kind == SE_NodeKind_Boolean);
//   return field_node->v.se_boolean;
// }
// 
// internal String8 
// se_string_from_struct(SE_Node *s, String8 tag)
// {
//   SE_Node *field_node = se_node_from_tag(s->first, tag);
//   Assert(field_node->kind == SE_NodeKind_String);
//   return field_node->v.se_string;
// }
