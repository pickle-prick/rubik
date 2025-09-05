#ifndef SERIALIZE_YAML_H
#define SERIALIZE_YAML_H

////////////////////////////////
//~ k: Stacks

typedef struct SE_YML_IndentNode SE_YML_IndentNode;
struct SE_YML_IndentNode
{
  SE_YML_IndentNode *next;
  S64 v;
};

////////////////////////////////
//- k: Stack globals

thread_static SE_YML_IndentNode *se_g_top_indent;

////////////////////////////////
//- k: Stack functions

internal void se_yml_push_indent(Arena *arena, S64 indent);
internal void se_yml_pop_indent();

////////////////////////////////
//~ Main API

internal String8List se_yml_node_to_strlist(Arena *arena, SE_Node *n);
internal void se_yml_node_to_file(SE_Node *n, String8 path);
internal SE_Node* se_yml_node_from_string(Arena *arena, String8 string);
internal SE_Node* se_yml_node_from_file(Arena *arena, String8 path);

////////////////////////////////
//~ Helper functions

internal void se_yml_push_node_to_strlist(Arena *arena, String8List *strs, SE_Node *node);
internal String8Node* se_yml_node_from_strlist(Arena *arena, String8Node *line, SE_Node *struct_parent, SE_Node *array_parent);

#endif
