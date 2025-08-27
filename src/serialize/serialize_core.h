#ifndef SERIALIZE_CORE_H
#define SERIALIZE_CORE_H

////////////////////////////////
//~ k: Enums

typedef enum SE_NodeKind
{
  SE_NodeKind_Invalid,

  // generic types
  SE_NodeKind_S64,
  SE_NodeKind_U64,
  SE_NodeKind_F32,
  SE_NodeKind_B32,
  SE_NodeKind_String,

  // compund types
  SE_NodeKind_Vec2U64,
  SE_NodeKind_Vec2F32,
  SE_NodeKind_Vec3F32,
  SE_NodeKind_Vec4F32,
  SE_NodeKind_Mat2x2F32,
  SE_NodeKind_Mat3x3F32,
  SE_NodeKind_Mat4x4F32,

  // ?
  SE_NodeKind_Array,
  SE_NodeKind_Struct,

  // special
  SE_NodeKind_Handle,
  SE_NodeKind_COUNT,
} SE_NodeKind;

typedef union SE_Handle SE_Handle;
union SE_Handle
{
  // 0 ptr
  // 1 generation,
  // 2 key_0,
  // 3 key_1,
  // 4 run_seed
  U64 u64[6];
  U32 u32[12];
  U16 u16[24];
};

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
    S64       se_s64;
    U64       se_u64;
    F32       se_f32;
    B32       se_b32;
    String8   se_str;
    Vec2U64   se_v2u64;
    Vec2F32   se_v2f32;
    Vec3F32   se_v3f32;
    Vec4F32   se_v4f32;
    Mat2x2F32 se_2x2f32;
    Mat3x3F32 se_3x3f32;
    Mat4x4F32 se_4x4f32;
    SE_Handle se_handle;
  } v;
};

typedef struct SE_NodeRec SE_NodeRec;
struct SE_NodeRec
{
  SE_Node *next;
  S32 push_count;
  S32 pop_count;
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

thread_static SE_ParentNode *se_g_top_parent;
thread_static Arena         *se_g_top_arena;

//- k: Stack functions

internal void se_push_parent(SE_Node *n);
internal void se_pop_parent();

////////////////////////////////
//~ k: Node Build Api

internal void     se_build_begin(Arena *arena);
internal void     se_build_end();

internal SE_Node* se_push_node(SE_NodeKind kind, String8 tag, B32 has_value, void *value, U64 value_size);
// NOTE(XXX): we have to copy the v, macro won't cut it
internal SE_Node* se_str_with_tag(String8 tag, String8 v);
internal SE_Node* se_multiline_str_with_tag(String8 tag, String8 v);

#define se_s64_with_tag(tag,v)    se_push_node(SE_NodeKind_S64, tag, 1, &v, sizeof(S64))
#define se_u64_with_tag(tag,v)    se_push_node(SE_NodeKind_U64, tag, 1, &v, sizeof(U64))
#define se_f32_with_tag(tag,v)    se_push_node(SE_NodeKind_F32, tag, 1, &v, sizeof(F32))
#define se_b32_with_tag(tag,v)    se_push_node(SE_NodeKind_B32, tag, 1, &v, sizeof(B32))
// #define se_str_with_tag(tag,v)    se_push_node(SE_NodeKind_String, tag, 1, &v, sizeof(String8))
#define se_v2u64_with_tag(tag,v)  se_push_node(SE_NodeKind_Vec2U64, tag, 1, &v, sizeof(Vec2U64))
#define se_v2f32_with_tag(tag,v)  se_push_node(SE_NodeKind_Vec2F32, tag, 1, &v, sizeof(Vec2F32))
#define se_v3f32_with_tag(tag,v)  se_push_node(SE_NodeKind_Vec3F32, tag, 1, &v, sizeof(Vec3F32))
#define se_v4f32_with_tag(tag,v)  se_push_node(SE_NodeKind_Vec4F32, tag, 1, &v, sizeof(Vec4F32))
#define se_2x2f32_with_tag(tag,v) se_push_node(SE_NodeKind_Mat2x2F32, tag, 1, &v, sizeof(Mat2x2F32))
#define se_3x3f32_with_tag(tag,v) se_push_node(SE_NodeKind_Mat3x3F32, tag, 1, &v, sizeof(Mat3x3F32))
#define se_4x4f32_with_tag(tag,v) se_push_node(SE_NodeKind_Mat4x4F32, tag, 1, &v, sizeof(Mat4x4F32))
#define se_handle_with_tag(tag,v) se_push_node(SE_NodeKind_Handle, tag, 1, &v, sizeof(SE_Handle))

#define se_array_with_tag(tag)  se_push_node(SE_NodeKind_Array, tag, 0, 0, 0)
#define se_struct_with_tag(tag) se_push_node(SE_NodeKind_Struct, tag, 0, 0, 0)

#define se_s64(v)    se_s64_with_tag(str8_lit(""), (v))
#define se_u64(v)    se_u64_with_tag(str8_lit(""), (v))
#define se_f32(v)    se_f32_with_tag(str8_lit(""), (v))
#define se_b32(v)    se_b32_with_tag(str8_lit(""), (v))
#define se_str(v)    se_str_with_tag(str8_lit(""), (v))
#define se_v2u64(v)  se_v2u64_with_tag(str8_lit(""), (v))
#define se_v2f32(v)  se_v2f32_with_tag(str8_lit(""), (v))
#define se_v3f32(v)  se_v3f32_with_tag(str8_lit(""), (v))
#define se_v4f32(v)  se_v4f32_with_tag(str8_lit(""), (v))
#define se_2x2f32(v) se_2x2f32_with_tag(str8_lit(""), (v))
#define se_3x3f32(v) se_3x3f32_with_tag(str8_lit(""), (v))
#define se_4x4f32(v) se_4x4f32_with_tag(str8_lit(""), (v))
#define se_handle(v) se_handle_with_tag(str8_lit(""), (v))

#define se_array(v) se_array_with_tag(str8_lit(""))
#define se_struct() se_struct_with_tag(str8_lit(""))

#define SE_Parent(v)         DeferLoop(se_push_parent(v), se_pop_parent())
#define SE_Array()           DeferLoop(se_push_parent(se_array()), se_pop_parent())
#define SE_Array_WithTag(t)  DeferLoop(se_push_parent(se_array_with_tag(t)), se_pop_parent())
#define SE_Struct()          DeferLoop(se_push_parent(se_struct()), se_pop_parent())
#define SE_Struct_WithTag(t) DeferLoop(se_push_parent(se_struct_with_tag(t)), se_pop_parent())

////////////////////////////////
//~ k: Helper functions

internal SE_NodeRec se_node_rec_df(SE_Node *node, SE_Node *root, U64 sib_member_off, U64 child_member_off);
#define se_node_rec_df_pre(node,  root) se_node_rec_df(node, root, OffsetOf(SE_Node, next), OffsetOf(SE_Node, first))
#define se_node_rec_df_post(node, root) se_node_rec_df(node, root, OffsetOf(SE_Node, prev), OffsetOf(SE_Node, last))

internal SE_Node*  se_child_from_tag(SE_Node *node, String8 tag);
internal S64       se_s64_from_tag(SE_Node *struct_node, String8 tag);
internal U64       se_u64_from_tag(SE_Node *struct_node, String8 tag);
internal F32       se_f32_from_tag(SE_Node *struct_node, String8 tag);
internal B32       se_b32_from_tag(SE_Node *struct_node, String8 tag);
internal String8   se_str_from_tag(SE_Node *struct_node, String8 tag);
internal Vec2U64   se_v2u64_from_tag(SE_Node *struct_node, String8 tag);
internal Vec2F32   se_v2f32_from_tag(SE_Node *struct_node, String8 tag);
internal Vec3F32   se_v3f32_from_tag(SE_Node *struct_node, String8 tag);
internal Vec4F32   se_v4f32_from_tag(SE_Node *struct_node, String8 tag);
internal Mat2x2F32 se_2x2f32_from_tag(SE_Node *struct_node, String8 tag);
internal Mat3x3F32 se_3x3f32_from_tag(SE_Node *struct_node, String8 tag);
internal Mat4x4F32 se_4x4f32_from_tag(SE_Node *struct_node, String8 tag);
internal SE_Handle se_handle_from_tag(SE_Node *struct_node, String8 tag);
internal SE_Node*  se_arr_from_tag(SE_Node *struct_node, String8 tag);
internal SE_Node*  se_struct_from_tag(SE_Node *struct_node, String8 tag);

#endif
