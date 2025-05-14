#define SE_YML_INDENT_SIZE 1

//- k: Stack functions

internal void 
se_yml_push_indent(Arena *arena, S64 indent)
{
  Assert(indent < 300);
  if(se_g_top_indent != 0)
  {
    indent += se_g_top_indent->v; 
  }
  Assert(indent < 300);
  SE_YML_IndentNode *n = push_array(arena, SE_YML_IndentNode, 1);
  n->v = indent;
  SLLStackPush(se_g_top_indent, n);
}

internal void
se_yml_pop_indent()
{
  SLLStackPop(se_g_top_indent);
}

//~ API

internal String8
se_yml_indent_str(Arena *arena)
{
  String8 ret = {0};

  S64 indent = 0;
  if(se_g_top_indent) indent = se_g_top_indent->v;
  Assert(indent < 300);
  if(indent > 0)
  {
    U64 size = indent*SE_YML_INDENT_SIZE;
    ret.str = push_array(arena, U8, size+1);
    ret.size = size;
    MemorySet(ret.str, ' ', size*sizeof(U8));
  }
  return ret;
}

internal void
se_yml_node_to_file(SE_Node *_root, String8 path)
{
  Temp scratch = scratch_begin(0,0);

  String8List strs = {0};

  se_yml_push_indent(scratch.arena, -1);
  U64 push_count = 1;
  U64 pop_count = 0;
  SE_Node *root = _root;
  while(root != 0)
  {
    SE_NodeRec rec = se_node_rec_df_pre(root, _root);

    // push indent first
    String8 indent_str = se_yml_indent_str(scratch.arena);
    str8_list_push(scratch.arena, &strs, indent_str);

    B32 tagged = root->parent ? root->parent->kind == SE_NodeKind_Struct : 0;
    B32 is_arr_item = root->parent ? root->parent->kind == SE_NodeKind_Array : 0;
    if(tagged)
    {
      str8_list_pushf(scratch.arena, &strs, "%S: ", root->tag);
    }
    if(is_arr_item)
    {
      str8_list_pushf(scratch.arena, &strs, "- ");
    }

    switch(root->kind)
    {
      case SE_NodeKind_S64:
      {
        str8_list_pushf(scratch.arena, &strs, "s64(%I64d)\n", root->v.se_s64);
      }break;
      case SE_NodeKind_U64:
      {
        str8_list_pushf(scratch.arena, &strs, "u64(%I64u)\n", root->v.se_u64);
      }break;
      case SE_NodeKind_F32:
      {
        str8_list_pushf(scratch.arena, &strs, "f32(%.4f)\n", root->v.se_f32);
      }break;
      case SE_NodeKind_B32:
      {
        str8_list_pushf(scratch.arena, &strs, "b32(%d)\n", root->v.se_b32);
      }break;
      case SE_NodeKind_String:
      {
        str8_list_pushf(scratch.arena, &strs, "str(%S)\n", root->v.se_str);
      }break;
      case SE_NodeKind_Vec2U64:
      {
        str8_list_pushf(scratch.arena, &strs, "v2u64(%I64u, %I64u)\n", root->v.se_v2u64.x, root->v.se_v2u64.y);
      }break;
      case SE_NodeKind_Vec2F32:
      {
        str8_list_pushf(scratch.arena, &strs, "v2f32(%.4f, %.4f)\n", root->v.se_v2f32.x, root->v.se_v2f32.y);
      }break;
      case SE_NodeKind_Vec3F32:
      {
        str8_list_pushf(scratch.arena, &strs, "v3f32(%.4f, %.4f, %.4f)\n", root->v.se_v3f32.x, root->v.se_v3f32.y, root->v.se_v3f32.z);
      }break;
      case SE_NodeKind_Vec4F32:
      {
        str8_list_pushf(scratch.arena, &strs, "v4f32(%.4f, %.4f, %.4f, %.4f)\n", root->v.se_v4f32.x, root->v.se_v4f32.y, root->v.se_v4f32.z, root->v.se_v4f32.w);
      }break;
      case SE_NodeKind_Mat2x2F32:
      {
        str8_list_pushf(scratch.arena, &strs, "2x2f32(%.4f, %.4f, %.4f, %.4f)\n", root->v.se_2x2f32.v[0], root->v.se_2x2f32.v[1], root->v.se_2x2f32.v[2], root->v.se_2x2f32.v[3]);
      }break;
      case SE_NodeKind_Mat3x3F32:
      case SE_NodeKind_Mat4x4F32:
      {
        NotImplemented;
      }break;
      case SE_NodeKind_Array:
      {
        str8_list_pushf(scratch.arena, &strs, "\n");
      }break;
      case SE_NodeKind_Struct:
      {
        if(root != _root) str8_list_pushf(scratch.arena, &strs, "\n");
      }break;
      default:{InvalidPath;}break;
    }

    // DEBUG
    pop_count += rec.pop_count;
    push_count += rec.push_count;

    for(U64 i = 0; i < rec.pop_count; i++) se_yml_pop_indent();
    if(rec.push_count > 0) se_yml_push_indent(scratch.arena, (S64)rec.push_count);
    root = rec.next;
  }
  se_yml_pop_indent();

  // write to file
  FILE *file = fopen((char*)path.str, "w");
  for(String8Node *n = strs.first; n != 0; n = n->next)
  {
    fwrite(n->string.str, n->string.size, 1, file);
  }
  fclose(file);
  scratch_end(scratch);
}

internal SE_Node *
se_yml_node_from_file(Arena *arena, String8 path)
{
  SE_Node *ret = 0;
  Temp scratch = scratch_begin(&arena,1);

  // Read file
  OS_Handle f = os_file_open(OS_AccessFlag_Read, (path));
  FileProperties f_props = os_properties_from_file(f);
  U64 size = f_props.size;
  U8 *data = push_array(scratch.arena, U8, f_props.size);
  os_file_read(f, rng_1u64(0,size), data);
  String8 str = str8(data, size);

  String8List lines = wrapped_lines_from_string(scratch.arena, str, 300, 300, 2);

  se_build_begin(arena);

  SE_Node *root = se_struct();
  SE_Parent(root)
  {
    for(String8Node *line_node = lines.first; line_node != 0;)
    {
      String8Node *next_line_node = line_node->next;

      ///////////////////////////////////////////////////////////////////////////////////
      // remove trailling empty space

      U8 *first = line_node->string.str;
      U8 *opl = first + line_node->string.size;
      for(;opl > first;)
      {
        opl -= 1;
        if (!char_is_space(*opl)){
          opl += 1;
          break;
        }
      }

      ///////////////////////////////////////////////////////////////////////////////////
      // compute line indent

      U64 space_count = 0;
      for(;first < opl; first++)
      {
        if(!char_is_space(*first))
        {
          break;
        }
        space_count++;
      }
      U64 indent = space_count / SE_YML_INDENT_SIZE;

      // skip empty line
      String8 line = str8_range(first, opl);
      if(line.size > 0)
      {
        U64 next_indent = 0;
        for(String8Node *next = next_line_node; next != 0;)
        {
          U8 *first = next->string.str;
          U8 *opl = first + next->string.size;
          U64 space_count = 0;
          for(;first < opl; first++)
          {
            if(!char_is_space(*first))
            {
              break;
            }
            space_count++;
          }

          // non empty line found
          if(first < opl)
          {
            next_indent = space_count / SE_YML_INDENT_SIZE;
            break;
          }

          // skip next empty line
          next = next->next;
          next_line_node = next;
        }

        ///////////////////////////////////////////////////////////////////////////////////
        // parse node

        B32 has_child = next_indent > indent; 
        if(has_child) AssertAlways(next_indent-indent == 1);
        S64 pop_count = indent - next_indent;

        // parse tag if any
        String8 tag = {0};
        for(U8* c = first; c < opl; c++)
        {
          if(*c == '(')
          {
            break;
          }
          if(*c == ':')
          {
            tag = str8_range(first, c);
            first = c+1; // skip tag
            for(;first < opl && char_is_space(*first); first++); /* skip empty space if any */
            break;
          }
        }

        // parse kind
        SE_NodeKind kind = SE_NodeKind_Invalid;

        // line is tagged
        if(tag.size > 0)
        {
          // no value left this line, array or struct
          if(first == opl)
          {
            // could only be array or struct
            AssertAlways(next_line_node != 0);
            U8* first = next_line_node->string.str;
            U8* opl = first + next_line_node->string.size;
            for(;first < opl; first++)
            {
              if(!char_is_space(*first))
              {
                kind = *first == '-' ? SE_NodeKind_Array: SE_NodeKind_Struct;
                break;
              }
            }
          }
        }
        // non-tagged line
        else
        {
          if(*line.str == '-')
          {
            if(line.size == 1)
            {
              kind = SE_NodeKind_Struct;
            }
            else
            {
              // skip inline array indicator and empty space
              for(; first < opl; first++)
              {
                if((*first != '-') && (*first != ' '))
                {
                  break;
                }
              }
            }
          }
        }

        // generic values if not struct or array
        if(kind == SE_NodeKind_Invalid)
        {
          String8 type_str = {0};
          for(U8 *c = first; c < opl; c++)
          {
            if(*c == '(')
            {
              type_str = str8_range(first,c);
              first = c+1;
              opl -= 1;
              break;
            }
          }

          if(str8_match(type_str, str8_lit("s64"), 0))    kind = SE_NodeKind_S64;
          if(str8_match(type_str, str8_lit("u64"), 0))    kind = SE_NodeKind_U64;
          if(str8_match(type_str, str8_lit("f32"), 0))    kind = SE_NodeKind_F32;
          if(str8_match(type_str, str8_lit("b32"), 0))    kind = SE_NodeKind_B32;
          if(str8_match(type_str, str8_lit("str"), 0))    kind = SE_NodeKind_String;
          if(str8_match(type_str, str8_lit("v2u64"), 0))  kind = SE_NodeKind_Vec2U64;
          if(str8_match(type_str, str8_lit("v2f32"), 0))  kind = SE_NodeKind_Vec2F32;
          if(str8_match(type_str, str8_lit("v3f32"), 0))  kind = SE_NodeKind_Vec3F32;
          if(str8_match(type_str, str8_lit("v4f32"), 0))  kind = SE_NodeKind_Vec4F32;
          if(str8_match(type_str, str8_lit("2x2f32"), 0)) kind = SE_NodeKind_Mat2x2F32;
          if(str8_match(type_str, str8_lit("3x3f32"), 0)) kind = SE_NodeKind_Mat3x3F32;
          if(str8_match(type_str, str8_lit("4x4f32"), 0)) kind = SE_NodeKind_Mat4x4F32;
        }

        String8 values_src = str8_range(first,opl);
        void *values = 0;
        B32 has_value = 1;
        U64 value_size = 0;

        switch(kind)
        {
          case SE_NodeKind_S64:
          {
            S64 *v = push_array(scratch.arena, S64, 1);
            *v = s64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_U64:
          {
            U64 *v = push_array(scratch.arena, U64, 1);
            *v = u64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_F32:
          {
            F32 *v = push_array(scratch.arena, F32, 1);
            *v = (F32)f64_from_str8(values_src);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_B32:
          {
            B32 *v = push_array(scratch.arena, B32, 1);
            *v = (B32)s64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }
          case SE_NodeKind_String:
          {
            String8 *v = push_array(scratch.arena, String8, 1);
            *v = push_str8_copy(scratch.arena, values_src);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec2U64:
          {
            U64 idx = 0;
            Vec2U64 *v = push_array(scratch.arena, Vec2U64, 1);
            for(U8 *c = first; c < opl && idx < 2; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx] = u64_from_str8(str8_range(first, c), 10);
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec2F32:
          {
            U64 idx = 0;
            Vec2F32 *v = push_array(scratch.arena, Vec2F32, 1);
            for(U8 *c = first; c < opl && idx < 2; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec3F32:
          {
            U64 idx = 0;
            Vec3F32 *v = push_array(scratch.arena, Vec3F32, 1);
            for(U8 *c = first; c < opl && idx < 3; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec4F32:
          {
            U64 idx = 0;
            Vec4F32 *v = push_array(scratch.arena, Vec4F32, 1);
            for(U8 *c = first; c < opl && idx < 4; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Mat2x2F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 4);
            for(U8 *c = first; c < opl && idx < 4; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v)*4;
          }break;
          case SE_NodeKind_Mat3x3F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 9);
            for(U8 *c = first; c < opl && idx < 9; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v)*9;
          }break;
          case SE_NodeKind_Mat4x4F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 16);
            for(U8 *c = first; c < opl && idx < 16; c++,idx++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
              }
            }
            values = v;
            value_size = sizeof(*v)*16;
          }break;
          case SE_NodeKind_Array:
          case SE_NodeKind_Struct:{ has_value=0; }break;
          default:{InvalidPath;}break;
        }

        SE_Node *node = se_push_node(kind, tag, has_value, values, value_size);
        if(has_child) se_push_parent(node);
        for(int i = 0; i < pop_count; i++) se_pop_parent();
      }

      line_node = next_line_node;
    }
  }

  se_build_end();
  scratch_end(scratch);
  return ret;
}

// internal SE_Node *
// se_yml_node_from_file(Arena *arena, String8 path)
// {
//   Temp temp = scratch_begin(&arena,1);
// 
//   // Read file
//   OS_Handle f = os_file_open(OS_AccessFlag_Read, (path));
//   FileProperties f_props = os_properties_from_file(f);
//   U64 size = f_props.size;
//   U8 *data = push_array(temp.arena, U8, f_props.size);
//   os_file_read(f, rng_1u64(0,size), data);
//   String8 str = str8(data, size);
// 
//   String8List lines = wrapped_lines_from_string(temp.arena, str, 300, 300, 2);
// 
//   se_build_begin(arena);
//   // k: top struct
//   SE_Node *root = se_struct(str8_zero());
//   se_push_parent(root);
// 
//   String8Node *line_node = lines.first;
//   while(line_node != 0)
//   {
//     line_node = se_yml_node_from_strlist(temp.arena, line_node, root, 0);
//   }
// 
//   se_pop_parent();
//   scratch_end(temp);
//   se_build_end();
//   return root;
// }
// 
// //~ Helper functions
// 
// internal void
// se_yml_push_node_to_strlist(Arena *arena, String8List *strs, SE_Node *node)
// {
//   String8 prefix = {0};
//   SE_Node *parent = node->parent;
// 
//   // Indent
//   {
//     S64 top_indent = se_g_top_indent->v;
//     top_indent = ClampBot(0, top_indent);
//     U64 size = top_indent*SE_YML_INDENT_SIZE;
//     prefix.str = push_array(arena, U8, size+1);
//     prefix.size = size;
//     MemorySet(prefix.str, ' ', size);
//     prefix.str[size] = '\0';
//   }
// 
//   // List prefix
//   if(parent != 0 && parent->kind == SE_NodeKind_Array)
//   {
//     prefix = push_str8f(arena, "%S- ", prefix);
//   }
// 
//   // Tag
//   if(node->tag.size > 0)
//   {
//     prefix = push_str8f(arena, "%S%S: ", prefix, node->tag);
//   }
// 
//   str8_list_pushf(arena, strs, "%S", prefix);
//   switch(node->kind)
//   {
//     case SE_NodeKind_Int:
//     {
//       str8_list_pushf(arena, strs, "s%I64d\n", node->v.se_int);
//     }break;
//     case SE_NodeKind_Uint:
//     {
//       str8_list_pushf(arena, strs, "u%I64u\n", node->v.se_int);
//     }break;
//     case SE_NodeKind_Float:
//     {
//       str8_list_pushf(arena, strs, "f%I64f\n", node->v.se_float);
//     }break;
//     case SE_NodeKind_String:
//     {
//       str8_list_pushf(arena, strs, "\"%S\"\n", node->v.se_string);
//     }break;
//     case SE_NodeKind_Boolean:
//     {
//       String8 v = node->v.se_boolean ? str8_lit("true") : str8_lit("false");
//       str8_list_pushf(arena, strs, "%S\n", v);
//     }break;
//     case SE_NodeKind_Array:
//     case SE_NodeKind_Struct:
//     {
//       if(prefix.size > 0) str8_list_pushf(arena, strs, "\n");
//     }break;
//     default: {InvalidPath;}break;
//   }
// 
//   se_yml_push_indent(arena, 1);
//   for(SE_Node *c = node->first; c != 0; c = c->next)
//   {
//     se_yml_push_node_to_strlist(arena, strs, c);
//   }
//   se_yml_pop_indent();
// }
// 
// internal U64 
// se_yml_whitespaces_from_str(String8 string)
// {
//   U64 whitespace_count = 0;
//   U8 *first = string.str;
//   U8 *opl = first + string.size;
//   for(; first < opl; first++)
//   {
//     if (!char_is_space(*first)) break;
//     whitespace_count++;
//   }
//   return whitespace_count;
// }
// 
// internal String8Node *
// se_yml_node_from_strlist(Arena *arena, String8Node *line_node, SE_Node *struct_parent, SE_Node *array_parent)
// {
//   SE_Node *n = se_build_node();
// 
//   B32 is_struct_entry = struct_parent != 0;
//   B32 is_array_entry  = array_parent != 0;
// 
//   String8Node *next_line_node = line_node->next;
//   U64 next_line_indent = 0;
//   if(next_line_node != 0)
//   {
//     next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;
//   }
// 
//   // k: parse/skip indent
//   String8 string = line_node->string;
//   U64 whitespace_count = se_yml_whitespaces_from_str(string);
//   U64 indent = whitespace_count / SE_YML_INDENT_SIZE;
//   string = str8_skip(string, whitespace_count);
// 
//   // k: parse/skip tag
//   String8 tag = {0};
//   {
//     U8 *first = string.str;
//     U8 *opl = first + string.size;
//     U64 tag_str_size = 0;
//     for(;first < opl; first++)
//     {
//       // k: no tag, it's a string
//       if(*first == '"')
//       {
//         break;
//       }
//       // k: list entry
//       if(*first == '-')
//       {
//         string = str8_skip(string, 2);
//         Assert(is_array_entry);
//         break;
//       }
//       // k: it's a tag
//       if(*first == ':')
//       {
//         tag_str_size = first-string.str;
//         break;
//       }
//     }
//     if(tag_str_size > 0)
//     {
//       tag = str8_chop(string, string.size-tag_str_size);
//     }
//     if(tag.size > 0)
//     {
//       // skip tag + ':'
//       string = str8_skip(string, tag.size+1);
//     }
//   }
//   n->tag = push_str8_copy(arena, tag);
//   string = str8_skip_chop_whitespace(string);
// 
//   if(is_struct_entry)
//   {
//     AssertAlways(tag.size > 0);
//   }
// 
//   if(string.size > 0)
//   {
//     // Pase value (str, int, float, boolean)
//     if(*string.str == '"')
//     {
//       n->kind = SE_NodeKind_String;
//       n->v.se_string = push_str8_copy(arena, str8_skip(str8_chop(string, 1), 1));
//     }
//     else if(str8_match(str8_lit("true"), string, 0))
//     {
//       n->kind = SE_NodeKind_Boolean;
//       n->v.se_boolean = 1;
//     }
//     else if(str8_match(str8_lit("false"), string, 0))
//     {
//       n->kind = SE_NodeKind_Boolean;
//       n->v.se_boolean = 0;
//     }
//     else if(str8_starts_with(string, str8_lit("s"), 0))
//     {
//       // S64
//       n->kind = SE_NodeKind_Int;
//       n->v.se_int = s64_from_str8(str8_skip(string, 1), 10);
//     }
//     else if(str8_starts_with(string, str8_lit("u"), 0))
//     {
//       // U64
//       n->kind = SE_NodeKind_Uint;
//       n->v.se_uint = u64_from_str8(str8_skip(string, 1), 10);
//     }
//     else if(str8_starts_with(string, str8_lit("f"), 0))
//     {
//       // F64
//       n->kind = SE_NodeKind_Float;
//       n->v.se_float = f64_from_str8(str8_skip(string, 1));
//     }
//     else
//     {
//       InvalidPath;
//     }
//   }
// 
//   B32 is_struct = 0;
//   B32 is_array = 0;
// 
//   if(string.size == 1 && *string.str == '-')
//   {
//     is_struct = 1;
//   }
//   // k: this loop skip empty lines
//   else for(; next_line_node != 0 && next_line_indent > indent; next_line_node = next_line_node->next)
//   {
//     // Get Indent
//     next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;
// 
//     // Read next line to check if current line starts a array or struct
//     String8 line_str = str8_skip(next_line_node->string, next_line_indent*SE_YML_INDENT_SIZE);
//     if(line_str.size > 0)
//     {
//       if(*line_str.str == '-')
//       {
//         is_array = 1;
//         break;
//       }
//       else 
//       {
//         is_struct = 1;
//         break;
//       }
//     }
//     else
//     {
//       // k: skip empty lines
//       continue;
//     }
//   }
// 
//   if(is_array || is_struct)
//   {
//     SE_NodeKind kind = 0;
//     SE_Node *struct_parent = 0;
//     SE_Node *array_parent = 0;
//     if(is_struct)
//     {
//       kind = SE_NodeKind_Struct;
//       struct_parent = n;
//     }
//     if(is_array)
//     {
//       kind = SE_NodeKind_Array;
//       array_parent = n;
//     }
//     n->kind = kind;
// 
//     while(next_line_node != 0 && next_line_indent > indent)
//     {
//       SE_Parent(n)
//       {
//         next_line_node = se_yml_node_from_strlist(arena, next_line_node, struct_parent, array_parent);
//         if(next_line_node != 0) next_line_indent = se_yml_whitespaces_from_str(next_line_node->string) / SE_YML_INDENT_SIZE;
//       }
//     }
//   }
// 
//   return next_line_node;
// }
