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

internal String8List
se_yml_node_to_strlist(Arena *arena, SE_Node *root_)
{
  Temp scratch = scratch_begin(&arena, 1);
  String8List strs = {0};
  se_yml_push_indent(scratch.arena, -1);
  U64 push_count = 1;
  U64 pop_count = 0;
  SE_Node *n = root_;
  while(n != 0)
  {
    SE_NodeRec rec = se_node_rec_df_pre(n, root_);

    // push indent first
    String8 indent_str = se_yml_indent_str(arena);
    str8_list_push(arena, &strs, indent_str);

    B32 tagged = n->parent ? n->parent->kind == SE_NodeKind_Struct : 0;
    B32 is_arr_item = n->parent ? n->parent->kind == SE_NodeKind_Array : 0;
    if(tagged)
    {
      str8_list_pushf(arena, &strs, "%S: ", n->tag);
    }
    if(is_arr_item)
    {
      str8_list_pushf(arena, &strs, "- ");
    }

    switch(n->kind)
    {
      case SE_NodeKind_S64:
      {
        str8_list_pushf(arena, &strs, "s64(%I64d)\n", n->v.se_s64);
      }break;
      case SE_NodeKind_U64:
      {
        str8_list_pushf(arena, &strs, "u64(%I64u)\n", n->v.se_u64);
      }break;
      case SE_NodeKind_F32:
      {
        str8_list_pushf(arena, &strs, "f32(%.4f)\n", n->v.se_f32);
      }break;
      case SE_NodeKind_B32:
      {
        str8_list_pushf(arena, &strs, "b32(%d)\n", n->v.se_b32);
      }break;
      case SE_NodeKind_String:
      {
        str8_list_pushf(arena, &strs, "str(%S)\n", n->v.se_str);
      }break;
      case SE_NodeKind_Vec2U64:
      {
        str8_list_pushf(arena, &strs, "v2u64(%I64u, %I64u)\n", n->v.se_v2u64.x, n->v.se_v2u64.y);
      }break;
      case SE_NodeKind_Vec2F32:
      {
        str8_list_pushf(arena, &strs, "v2f32(%.4f, %.4f)\n", n->v.se_v2f32.x, n->v.se_v2f32.y);
      }break;
      case SE_NodeKind_Vec3F32:
      {
        str8_list_pushf(arena, &strs, "v3f32(%.4f, %.4f, %.4f)\n", n->v.se_v3f32.x, n->v.se_v3f32.y, n->v.se_v3f32.z);
      }break;
      case SE_NodeKind_Vec4F32:
      {
        str8_list_pushf(arena, &strs, "v4f32(%.4f, %.4f, %.4f, %.4f)\n", n->v.se_v4f32.x, n->v.se_v4f32.y, n->v.se_v4f32.z, n->v.se_v4f32.w);
      }break;
      case SE_NodeKind_Mat2x2F32:
      {
        str8_list_pushf(arena, &strs, "2x2f32(%.4f, %.4f, %.4f, %.4f)\n", n->v.se_2x2f32.v[0][0], n->v.se_2x2f32.v[0][1], n->v.se_2x2f32.v[1][0], n->v.se_2x2f32.v[1][1]);
      }break;
      case SE_NodeKind_Mat3x3F32:
      {
        str8_list_pushf(arena, &strs, "3x3f32(%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)\n",
                                              n->v.se_3x3f32.v[0][0],
                                              n->v.se_3x3f32.v[0][1],
                                              n->v.se_3x3f32.v[0][2],
                                              n->v.se_3x3f32.v[1][0],
                                              n->v.se_3x3f32.v[1][1],
                                              n->v.se_3x3f32.v[1][2],
                                              n->v.se_3x3f32.v[2][0],
                                              n->v.se_3x3f32.v[2][1],
                                              n->v.se_3x3f32.v[2][2]);
      }break;
      case SE_NodeKind_Mat4x4F32:
      {
        str8_list_pushf(arena, &strs, "4x4f32(%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f)\n",
                                              n->v.se_4x4f32.v[0][0],
                                              n->v.se_4x4f32.v[0][1],
                                              n->v.se_4x4f32.v[0][2],
                                              n->v.se_4x4f32.v[0][3],
                                              n->v.se_4x4f32.v[1][0],
                                              n->v.se_4x4f32.v[1][1],
                                              n->v.se_4x4f32.v[1][2],
                                              n->v.se_4x4f32.v[1][3],
                                              n->v.se_4x4f32.v[2][0],
                                              n->v.se_4x4f32.v[2][1],
                                              n->v.se_4x4f32.v[2][2],
                                              n->v.se_4x4f32.v[2][3],
                                              n->v.se_4x4f32.v[3][0],
                                              n->v.se_4x4f32.v[3][1],
                                              n->v.se_4x4f32.v[3][2],
                                              n->v.se_4x4f32.v[3][3]);
      }break;
      case SE_NodeKind_Array:
      {
        str8_list_pushf(arena, &strs, "\n");
      }break;
      case SE_NodeKind_Struct:
      {
        if(n != root_) str8_list_pushf(arena, &strs, "\n");
      }break;
      case SE_NodeKind_Handle:
      {
        str8_list_pushf(arena, &strs, "handle(%I64u, %I64u, %I64u, %I64u, %I64u, %I64u)\n",
                                              n->v.se_handle.u64[0],
                                              n->v.se_handle.u64[1],
                                              n->v.se_handle.u64[2],
                                              n->v.se_handle.u64[3],
                                              n->v.se_handle.u64[4],
                                              n->v.se_handle.u64[5]);
      }break;
      default:{InvalidPath;}break;
    }

    // DEBUG
    pop_count += rec.pop_count;
    push_count += rec.push_count;

    for(U64 i = 0; i < rec.pop_count; i++) se_yml_pop_indent();
    if(rec.push_count > 0) se_yml_push_indent(scratch.arena, (S64)rec.push_count);
    n = rec.next;
  }
  se_yml_pop_indent();
  scratch_end(scratch);
  return strs;
}

internal void
se_yml_node_to_file(SE_Node *root, String8 path)
{
  Temp scratch = scratch_begin(0,0);

  String8List strs = se_yml_node_to_strlist(scratch.arena, root);
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
se_yml_node_from_string(Arena *arena, String8 string)
{
  SE_Node *ret = 0;
  Temp scratch = scratch_begin(&arena, 1);

  ProfBegin("split lines");
  String8List lines = str8_split_by_string_chars(scratch.arena, string, str8_lit("\n"), StringSplitFlag_KeepEmpties);
  ProfEnd();

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
        if(!char_is_space(*opl)){
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

        /////////////////////////////////////////////////////////////////////////////////
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
          if(first == opl && has_child && next_line_node)
          {
            // could only be array or struct
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
          if(str8_match(type_str, str8_lit("handle"), 0)) kind = SE_NodeKind_Handle;
        }

        String8 values_src = str8_range(first,opl);
        void *values = 0;
        B32 has_value = 1;
        U64 value_size = 0;

        switch(kind)
        {
          case SE_NodeKind_S64:
          {
            // skip trailling ')'
            values_src.size--;
            S64 *v = push_array(scratch.arena, S64, 1);
            *v = s64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_U64:
          {
            // skip trailling ')'
            values_src.size--;
            U64 *v = push_array(scratch.arena, U64, 1);
            *v = u64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_F32:
          {
            // skip trailling ')'
            values_src.size--;
            F32 *v = push_array(scratch.arena, F32, 1);
            *v = (F32)f64_from_str8(values_src);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_B32:
          {
            // skip trailling ')'
            values_src.size--;
            B32 *v = push_array(scratch.arena, B32, 1);
            *v = (B32)s64_from_str8(values_src, 10);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_String:
          {
            // skip trailling ')'
            values_src.size--;
            String8 *v = push_array(scratch.arena, String8, 1);
            *v = push_str8_copy(arena, values_src);
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec2U64:
          {
            U64 idx = 0;
            Vec2U64 *v = push_array(scratch.arena, Vec2U64, 1);
            for(U8 *c = first; c < opl && idx < 2; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx++] = u64_from_str8(str8_range(first, c), 10);
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec2F32:
          {
            U64 idx = 0;
            Vec2F32 *v = push_array(scratch.arena, Vec2F32, 1);
            for(U8 *c = first; c < opl && idx < 2; c++,idx)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec3F32:
          {
            U64 idx = 0;
            Vec3F32 *v = push_array(scratch.arena, Vec3F32, 1);
            for(U8 *c = first; c < opl && idx < 3; c++,idx)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Vec4F32:
          {
            U64 idx = 0;
            Vec4F32 *v = push_array(scratch.arena, Vec4F32, 1);
            for(U8 *c = first; c < opl && idx < 4; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                (*v).v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v);
          }break;
          case SE_NodeKind_Mat2x2F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 4);
            for(U8 *c = first; c < opl && idx < 4; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v)*4;
          }break;
          case SE_NodeKind_Mat3x3F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 9);
            for(U8 *c = first; c < opl && idx < 9; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v)*9;
          }break;
          case SE_NodeKind_Mat4x4F32:
          {
            U64 idx = 0;
            F32 *v = push_array(scratch.arena, F32, 16);
            for(U8 *c = first; c < opl && idx < 16; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx++] = (F32)f64_from_str8(str8_range(first, c));
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v)*16;
          }break;
          case SE_NodeKind_Array:
          case SE_NodeKind_Struct:{ has_value=0; }break;
          case SE_NodeKind_Handle:
          {
            U64 idx = 0;
            U64 *v = push_array(scratch.arena, U64, 6);
            for(U8 *c = first; c < opl && idx < 6; c++)
            {
              if(*c == ',' || c == (opl-1))
              {
                v[idx++] = u64_from_str8(str8_range(first, c),10);
                first = c+1;
                for(;first < opl; first++) { if(!char_is_space(*first)) break; }
              }
            }
            values = v;
            value_size = sizeof(*v)*6;
          }break;
          default:{}break;
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
  ret = root;
  ProfEnd();
  return ret;
}

internal SE_Node *
se_yml_node_from_file(Arena *arena, String8 path)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);

  // read file
  ProfBegin("read file");
  OS_Handle f = os_file_open(OS_AccessFlag_Read, (path));
  FileProperties f_props = os_properties_from_file(f);
  U64 size = f_props.size;
  U8 *data = push_array(scratch.arena, U8, f_props.size);
  os_file_read(f, rng_1u64(0,size), data);
  ProfEnd();
  SE_Node *ret = se_yml_node_from_string(arena, str8(data,size));
  scratch_end(scratch);
  return ret;
}
