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
se_build_node()
{
    SE_Node *n = push_array(se_g_top_arena, SE_Node, 1);
    SE_ParentNode *parent = se_g_top_parent;

    if(parent)
    {
        n->parent = parent->v;
        DLLPushBack(parent->v->first, parent->v->last, n);
        parent->v->children_count++;
    }
    return n;
}

internal SE_Node *
se_int(String8 tag, S64 v)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Int;
        n->v.se_int = v;
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;
}

internal SE_Node *
se_uint(String8 tag, S64 v)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Uint;
        n->v.se_uint = v;
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;
}

internal SE_Node *
se_float(String8 tag, F32 v)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Float;
        n->v.se_float = v;
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;
}

internal SE_Node *
se_boolean(String8 tag, B32 v)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Boolean;
        n->v.se_boolean = v;
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;
}

internal SE_Node *
se_string(String8 tag, String8 string)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_String;
        n->v.se_string = push_str8_copy(se_g_top_arena, string);
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;
}

internal SE_Node *
se_array(String8 tag)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Array;
        n->tag = push_str8_copy(se_g_top_arena, tag);
    }
    return n;

}

internal SE_Node *
se_struct(String8 tag)
{
    SE_Node *n = se_build_node();
    {
        n->kind = SE_NodeKind_Struct;
        n->tag  = tag;
    }
    return n;
}

internal SE_Node *se_struct_begin(String8 tag)
{
    SE_Node *n = se_struct(tag);
    se_push_parent(n);
    return n;
}

internal void se_struct_end(void)
{
    se_pop_parent();
}

////////////////////////////////
//~ k: Helper functions

internal SE_Node *
se_node_from_tag(SE_Node *first_node, String8 tag)
{
    SE_Node *ret = 0;
    for(SE_Node *n = first_node; n != 0; n = n->next)
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
se_s64_from_struct(SE_Node *s, String8 tag)
{
    SE_Node *field_node = se_node_from_tag(s->first, tag);
    Assert(field_node->kind == SE_NodeKind_Int);
    return field_node->v.se_int;
}

internal U64 
se_u64_from_struct(SE_Node *s, String8 tag)
{
    SE_Node *field_node = se_node_from_tag(s->first, tag);
    Assert(field_node->kind == SE_NodeKind_Uint);
    return field_node->v.se_uint;
}

internal F64 
se_f64_from_struct(SE_Node *s, String8 tag)
{
    SE_Node *field_node = se_node_from_tag(s->first, tag);
    Assert(field_node->kind == SE_NodeKind_Float);
    return field_node->v.se_float;
}

internal B32 
se_b32_from_struct(SE_Node *s, String8 tag)
{
    SE_Node *field_node = se_node_from_tag(s->first, tag);
    Assert(field_node->kind == SE_NodeKind_Boolean);
    return field_node->v.se_boolean;
}

internal String8 
se_string_from_struct(SE_Node *s, String8 tag)
{
    SE_Node *field_node = se_node_from_tag(s->first, tag);
    Assert(field_node->kind == SE_NodeKind_String);
    return field_node->v.se_string;
}
