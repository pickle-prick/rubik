#ifndef SERIALIZE_CORE_H
#define SERIALIZE_CORE_H

////////////////////////////////
//~ k: Enums

typedef enum SE_NodeKind
{
  SE_NodeKind_Invalid,
  SE_NodeKind_Int,
  SE_NodeKind_Uint,
  SE_NodeKind_Float,
  SE_NodeKind_String,
  SE_NodeKind_Boolean,
  SE_NodeKind_Array,
  SE_NodeKind_Struct,
  SE_NodeKind_COUNT,
} SE_NodeKind;

typedef S64     SE_Int;
typedef U64     SE_Uint;
typedef F64     SE_Float;
typedef String8 SE_String;
typedef B32     SE_Boolean;

typedef struct SE_Node SE_Node;
struct SE_Node
{
  SE_Node *next;
  SE_Node *prev;
  SE_Node *first;
  SE_Node *last;
  SE_Node *parent;

  U64 children_count;

  SE_NodeKind kind; 
  String8 tag;

  union
  {
    SE_Int     se_int;
    SE_Uint    se_uint;
    SE_Float   se_float;
    SE_String  se_string;
    SE_Boolean se_boolean;
  } v;
};

////////////////////////////////
//~ k: Stacks
typedef struct SE_ParentNode SE_ParentNode;
struct SE_ParentNode
{
  SE_ParentNode *next;
  SE_Node       *v;
};

//- k: Stack globals

global SE_ParentNode *se_g_top_parent;
global Arena         *se_g_top_arena;

//- k: Stack functions
internal void se_push_parent(SE_Node *n);
internal void se_pop_parent();
#define SE_Parent(v) DeferLoop(se_push_parent(v), se_pop_parent())
#define SE_Array(v)  DeferLoop(se_push_parent(se_array((v))), se_pop_parent())
#define SE_Struct(s) DeferLoop(se_struct_begin(s), se_struct_end())

////////////////////////////////
//~ k: Node Build Api

internal void     se_build_begin(Arena *arena);
internal void     se_build_end();
internal SE_Node* se_build_node();
internal SE_Node* se_int(String8 tag, S64 v);
internal SE_Node* se_uint(String8 tag, S64 v);
internal SE_Node* se_float(String8 tag, F32 v);
internal SE_Node* se_boolean(String8 tag, B32 v);
internal SE_Node* se_string(String8 tag, String8 string);
internal SE_Node* se_array(String8 tag);
internal SE_Node* se_struct(String8 tag);
internal SE_Node* se_struct_begin(String8 tag);
internal void     se_struct_end(void);

////////////////////////////////
//~ k: Helper functions

internal SE_Node* se_node_from_tag(SE_Node *first_node, String8 tag);
internal S64      se_s64_from_struct(SE_Node *s, String8 tag);
internal U64      se_u64_from_struct(SE_Node *s, String8 tag);
internal F64      se_f64_from_struct(SE_Node *s, String8 tag);
internal B32      se_b32_from_struct(SE_Node *s, String8 tag);
internal String8  se_string_from_struct(SE_Node *s, String8 tag);

#endif
