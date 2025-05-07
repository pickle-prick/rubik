#define SE_YML_INDENT_SIZE 2

//- k: Stack functions

internal void 
se_yml_push_indent(Arena *arena, S64 indent)
{
  S64 acc_indent = indent;
  if(se_g_top_indent != 0)
  {
    acc_indent += se_g_top_indent->v; 
  }
  Assert(acc_indent < 8);
  SE_YML_IndentNode *n = push_array(arena, SE_YML_IndentNode, 1);
  n->v = acc_indent;
  SLLStackPush(se_g_top_indent, n);
}

internal void
se_yml_pop_indent()
{
  SLLStackPop(se_g_top_indent);
}

//~ API

internal void
se_yml_node_to_file(SE_Node *n, String8 path)
{
  Temp temp = scratch_begin(0,0);
  String8List strs = {0};

  se_yml_push_indent(temp.arena, -1);
  se_yml_push_node_to_strlist(temp.arena, &strs, n);
  se_yml_pop_indent();

  U8 *path_cstring = push_array(temp.arena, U8, path.size+1);
  MemoryCopy(path_cstring, path.str, path.size);
  path_cstring[path.size] = '\0';
  FILE *file = fopen((char*)path_cstring, "w");
  if(file)
  {
    for(String8Node *n = strs.first; n != 0; n = n->next)
    {
      fwrite(n->string.str, n->string.size, 1, file);
    }
    fclose(file);
  }
  else
  {
    InvalidPath;
  }
  temp_end(temp);
}

internal SE_Node *
se_yml_node_from_file(Arena *arena, String8 path)
{
  Temp temp = scratch_begin(&arena,1);

  // Read file
  OS_Handle f = os_file_open(OS_AccessFlag_Read, (path));
  FileProperties f_props = os_properties_from_file(f);
  U64 size = f_props.size;
  U8 *data = push_array(temp.arena, U8, f_props.size);
  os_file_read(f, rng_1u64(0,size), data);
  String8 str = str8(data, size);

  String8List lines = wrapped_lines_from_string(temp.arena, str, 300, 300, 2);

  se_build_begin(arena);
  // k: top struct
  SE_Node *root = se_struct(str8_zero());
  se_push_parent(root);

  String8Node *line_node = lines.first;
  while(line_node != 0)
  {
    line_node = se_yml_node_from_strlist(temp.arena, line_node, root, 0);
  }

  se_pop_parent();
  scratch_end(temp);
  se_build_end();
  return root;
}

//~ Helper functions

internal void
se_yml_push_node_to_strlist(Arena *arena, String8List *strs, SE_Node *node)
{
  String8 prefix = {0};
  SE_Node *parent = node->parent;

  // Indent
  {
    S64 top_indent = se_g_top_indent->v;
    top_indent = ClampBot(0, top_indent);
    U64 size = top_indent*SE_YML_INDENT_SIZE;
    prefix.str = push_array(arena, U8, size+1);
    prefix.size = size;
    MemorySet(prefix.str, ' ', size);
    prefix.str[size] = '\0';
  }

  // List prefix
  if(parent != 0 && parent->kind == SE_NodeKind_Array)
  {
    prefix = push_str8f(arena, "%S- ", prefix);
  }

  // Tag
  if(node->tag.size > 0)
  {
    prefix = push_str8f(arena, "%S%S: ", prefix, node->tag);
  }

  str8_list_pushf(arena, strs, "%S", prefix);
  switch(node->kind)
  {
    case SE_NodeKind_Int:
    {
      str8_list_pushf(arena, strs, "s%I64d\n", node->v.se_int);
    }break;
    case SE_NodeKind_Uint:
    {
      str8_list_pushf(arena, strs, "u%I64u\n", node->v.se_int);
    }break;
    case SE_NodeKind_Float:
    {
      str8_list_pushf(arena, strs, "f%I64f\n", node->v.se_float);
    }break;
    case SE_NodeKind_String:
    {
      str8_list_pushf(arena, strs, "\"%S\"\n", node->v.se_string);
    }break;
    case SE_NodeKind_Boolean:
    {
      String8 v = node->v.se_boolean ? str8_lit("true") : str8_lit("false");
      str8_list_pushf(arena, strs, "%S\n", v);
    }break;
    case SE_NodeKind_Array:
    case SE_NodeKind_Struct:
    {
      if(prefix.size > 0) str8_list_pushf(arena, strs, "\n");
    }break;
    default: {InvalidPath;}break;
  }

  se_yml_push_indent(arena, 1);
  for(SE_Node *c = node->first; c != 0; c = c->next)
  {
    se_yml_push_node_to_strlist(arena, strs, c);
  }
  se_yml_pop_indent();
}

internal U64 
se_yml_whitespaces_from_str(String8 string)
{
  U64 whitespace_count = 0;
  U8 *first = string.str;
  U8 *opl = first + string.size;
  for(; first < opl; first++)
  {
    if (!char_is_space(*first)) break;
    whitespace_count++;
  }
  return whitespace_count;
}

internal String8Node *
se_yml_node_from_strlist(Arena *arena, String8Node *line_node, SE_Node *struct_parent, SE_Node *array_parent)
{
  SE_Node *n = se_build_node();

  B32 is_struct_entry = struct_parent != 0;
  B32 is_array_entry  = array_parent != 0;

  String8Node *next_line_node = line_node->next;
  U64 next_line_indent = 0;
  if(next_line_node != 0)
  {
    next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;
  }

  // k: parse/skip indent
  String8 string = line_node->string;
  U64 whitespace_count = se_yml_whitespaces_from_str(string);
  U64 indent = whitespace_count / SE_YML_INDENT_SIZE;
  string = str8_skip(string, whitespace_count);

  // k: parse/skip tag
  String8 tag = {0};
  {
    U8 *first = string.str;
    U8 *opl = first + string.size;
    U64 tag_str_size = 0;
    for(;first < opl; first++)
    {
      // k: no tag, it's a string
      if(*first == '"')
      {
        break;
      }
      // k: list entry
      if(*first == '-')
      {
        string = str8_skip(string, 2);
        Assert(is_array_entry);
        break;
      }
      // k: it's a tag
      if(*first == ':')
      {
        tag_str_size = first-string.str;
        break;
      }
    }
    if(tag_str_size > 0)
    {
      tag = str8_chop(string, string.size-tag_str_size);
    }
    if(tag.size > 0)
    {
      // skip tag + ':'
      string = str8_skip(string, tag.size+1);
    }
  }
  n->tag = push_str8_copy(arena, tag);
  string = str8_skip_chop_whitespace(string);

  if(is_struct_entry)
  {
    AssertAlways(tag.size > 0);
  }

  if(string.size > 0)
  {
    // Pase value (str, int, float, boolean)
    if(*string.str == '"')
    {
      n->kind = SE_NodeKind_String;
      n->v.se_string = push_str8_copy(arena, str8_skip(str8_chop(string, 1), 1));
    }
    else if(str8_match(str8_lit("true"), string, 0))
    {
      n->kind = SE_NodeKind_Boolean;
      n->v.se_boolean = 1;
    }
    else if(str8_match(str8_lit("false"), string, 0))
    {
      n->kind = SE_NodeKind_Boolean;
      n->v.se_boolean = 0;
    }
    else if(str8_starts_with(string, str8_lit("s"), 0))
    {
      // S64
      n->kind = SE_NodeKind_Int;
      n->v.se_int = s64_from_str8(str8_skip(string, 1), 10);
    }
    else if(str8_starts_with(string, str8_lit("u"), 0))
    {
      // U64
      n->kind = SE_NodeKind_Uint;
      n->v.se_uint = u64_from_str8(str8_skip(string, 1), 10);
    }
    else if(str8_starts_with(string, str8_lit("f"), 0))
    {
      // F64
      n->kind = SE_NodeKind_Float;
      n->v.se_float = f64_from_str8(str8_skip(string, 1));
    }
    else
    {
      InvalidPath;
    }
  }

  B32 is_struct = 0;
  B32 is_array = 0;

  if(string.size == 1 && *string.str == '-')
  {
    is_struct = 1;
  }
  // k: this loop skip empty lines
  else for(; next_line_node != 0 && next_line_indent > indent; next_line_node = next_line_node->next)
  {
    // Get Indent
    next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;

    // Read next line to check if current line starts a array or struct
    String8 line_str = str8_skip(next_line_node->string, next_line_indent*SE_YML_INDENT_SIZE);
    if(line_str.size > 0)
    {
      if(*line_str.str == '-')
      {
        is_array = 1;
        break;
      }
      else 
      {
        is_struct = 1;
        break;
      }
    }
    else
    {
      // k: skip empty lines
      continue;
    }
  }

  if(is_array || is_struct)
  {
    SE_NodeKind kind = 0;
    SE_Node *struct_parent = 0;
    SE_Node *array_parent = 0;
    if(is_struct)
    {
      kind = SE_NodeKind_Struct;
      struct_parent = n;
    }
    if(is_array)
    {
      kind = SE_NodeKind_Array;
      array_parent = n;
    }
    n->kind = kind;

    while(next_line_node != 0 && next_line_indent > indent)
    {
      SE_Parent(n)
      {
        next_line_node = se_yml_node_from_strlist(arena, next_line_node, struct_parent, array_parent);
        if(next_line_node != 0) next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;
      }
    }
  }

  return next_line_node;
}
