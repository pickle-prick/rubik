/////////////////////////////////
// Scene serialization/deserialization

internal String8
rk_scene_to_tscn(RK_Scene *scene)
{
  String8 ret = scene->save_path;
  RK_ResourceBucket *res_bucket = scene->res_bucket;
  Temp scratch = scratch_begin(0,0);

  // block
  U64 block_cap = MB(3);
  U64 block_size = 0;
  U8 *blocks = push_array(scratch.arena, U8, block_cap);

  se_build_begin(scratch.arena);

  SE_Node *root = se_struct();
  SE_Parent(root)
  {
    /////////////////////////////////////////////////////////////////////////////////////
    // scene info

    se_str_with_tag(str8_lit("name"), scene->name);
    if(scene->setup_fn_name.size) se_str_with_tag(str8_lit("setup_fn_name"), scene->setup_fn_name);
    if(scene->update_fn_name.size) se_str_with_tag(str8_lit("update_fn_name"), scene->update_fn_name);
    if(scene->default_fn_name.size) se_str_with_tag(str8_lit("default_fn_name"), scene->default_fn_name);
    se_v2u64_with_tag(str8_lit("hot_key"), v2u64(scene->hot_key.u64[0], scene->hot_key.u64[1]));
    se_v2u64_with_tag(str8_lit("active_key"), v2u64(scene->active_key.u64[0], scene->active_key.u64[1]));
    se_b32_with_tag(str8_lit("omit_grid"), scene->omit_grid);
    se_b32_with_tag(str8_lit("omit_gizmo3d"), scene->omit_gizmo3d);
    se_b32_with_tag(str8_lit("omit_light"), scene->omit_light);
    se_u64_with_tag(str8_lit("gizmo3d_mode"), scene->gizmo3d_mode);
    if(scene->custom_data)
    {
      U64 start = block_size;
      U64 size = rk_size_from_fat(scene->custom_data);
      if((block_size+size) > block_cap)
      {
        U8 *new_blocks = push_array(scratch.arena, U8, block_cap*2);
        MemoryCopy(new_blocks, blocks, block_size);
        blocks = new_blocks;
        block_cap *= 2;
      }
      MemoryCopy(blocks+start, scene->custom_data, size);
      block_size += size;
      se_v2u64_with_tag(str8_lit("custom_data"), v2u64(start, start+size));
    }
    // AssertAlways(*(blocks+120) == 1);

    if(res_bucket->res_count > 0) SE_Array_WithTag(str8_lit("ext_resources"))
    {
      U64 res_count = 0;
      for(U64 slot_idx = 0; slot_idx < res_bucket->hash_table_size; slot_idx++)
      {
        RK_ResourceBucketSlot *slot = &res_bucket->hash_table[slot_idx];
        for(RK_Resource *res = slot->first; res != 0; res = res->hash_next)
        {
          if(res->src_kind == RK_ResourceSrcKind_External)
          {
            res_count++;
            RK_Key key = res->key;
            SE_Struct()
            {
              se_str_with_tag(str8_lit("path"), res->path);
              se_str_with_tag(str8_lit("name"), res->name);
              se_u64_with_tag(str8_lit("kind"), res->kind);
              se_v2u64_with_tag(str8_lit("key"), v2u64(res->key.u64[0],res->key.u64[1]));
              switch(res->kind)
              {
                case RK_ResourceKind_Texture2D:
                {
                  RK_Texture2D *tex2d = (RK_Texture2D*)&res->v;
                  se_u64_with_tag(str8_lit("sample_kind"), tex2d->sample_kind);
                }break;
                case RK_ResourceKind_SpriteSheet:
                {
                  RK_SpriteSheet *sheet = (RK_SpriteSheet*)&res->v;
                  se_str_with_tag(str8_lit("path_1"), res->path_1);
                }break;
                case RK_ResourceKind_TileSet:
                {
                  // RK_TileSet *tileset = (RK_TileSet*)&res->v;
                }break;
                case RK_ResourceKind_Material:
                case RK_ResourceKind_PackedScene:
                case RK_ResourceKind_Skin:
                case RK_ResourceKind_Mesh:
                case RK_ResourceKind_Animation:
                {
                  NotImplemented;
                }break;
                default: {InvalidPath;}break;
              }
            }
          }
        }
      }
    }

    RK_Node *root = rk_node_from_handle(&scene->root);
    RK_Node *node = root;
    SE_Array_WithTag(str8_lit("nodes"))
    {
      while(node != 0)
      {
        RK_NodeRec rec = rk_node_df_pre(node, root, 0);

        if(!node->is_foreign) SE_Struct()
        {
          se_v2u64_with_tag(str8_lit("key"), v2u64(node->key.u64[0], node->key.u64[1]));
          if(node->parent != 0)
          {
            se_v2u64_with_tag(str8_lit("parent"), v2u64(node->parent->key.u64[0], node->parent->key.u64[1]));
          }
          se_u64_with_tag(str8_lit("flags"), node->flags);
          se_u64_with_tag(str8_lit("type_flags"), node->type_flags);
          se_str_with_tag(str8_lit("name"), node->name);

          if(node->custom_data != 0)
          {
            U64 start = block_size;
            U64 size = rk_size_from_fat(node->custom_data);
            if((block_size+size) > block_cap)
            {
              U8 *new_blocks = push_array(scratch.arena, U8, block_cap*2);
              MemoryCopy(new_blocks, blocks, sizeof(U8)*block_size);
              blocks = new_blocks;
              block_cap *= 2;
            }
            MemoryCopy(blocks+start, node->custom_data, size);
            block_size += size;
            se_v2u64_with_tag(str8_lit("custom_data"), v2u64(start, start+size));
          }

          ///////////////////////////////////////////////////////////////////////////////
          // Function table

          if(node->first_setup_fn != 0)
          {
            SE_Array_WithTag(str8_lit("setup"))
            {
              for(RK_UpdateFnNode *fn = node->first_setup_fn; fn != 0; fn = fn->next)
              {
                se_str(fn->name);
              }
            }
          }
          if(node->first_update_fn != 0)
          {
            SE_Array_WithTag(str8_lit("update"))
            {
              for(RK_UpdateFnNode *fn = node->first_update_fn; fn != 0; fn = fn->next)
              {
                se_str(fn->name);
              }
            }
          }
          if(node->first_fixed_update_fn != 0)
          {
            SE_Array_WithTag(str8_lit("fixed_update"))
            {
              for(RK_UpdateFnNode *fn = node->first_fixed_update_fn; fn != 0; fn = fn->next)
              {
                se_str(fn->name);
              }
            }
          }

          ///////////////////////////////////////////////////////////////////////////////
          // Equipments

          if(node->type_flags & RK_NodeTypeFlag_Node2D) SE_Struct_WithTag(str8_lit("node2d"))
          {
            RK_Node2D* node2d = node->node2d;
            SE_Struct_WithTag(str8_lit("transform"))
            {
              se_v2f32_with_tag(str8_lit("position"), node2d->transform.position);
              se_f32_with_tag(str8_lit("rotation"), node2d->transform.rotation);
              se_v2f32_with_tag(str8_lit("scale"), node2d->transform.scale);
              se_f32_with_tag(str8_lit("skew"), node2d->transform.skew);
            }
            se_f32_with_tag(str8_lit("z_index"), node2d->z_index);
          }
          if(node->type_flags & RK_NodeTypeFlag_Node3D) SE_Struct_WithTag(str8_lit("node3d"))
          {
            RK_Node3D* node3d = node->node3d;
            SE_Struct_WithTag(str8_lit("transform"))
            {
              se_v3f32_with_tag(str8_lit("position"), node3d->transform.position);
              se_v4f32_with_tag(str8_lit("rotation"), node3d->transform.rotation);
              se_v3f32_with_tag(str8_lit("scale"), node3d->transform.scale);
            }
          }
          if(node->type_flags & RK_NodeTypeFlag_Camera3D) SE_Struct_WithTag(str8_lit("camera3d"))
          {
            RK_Camera3D *camera3d = node->camera3d;
            se_u64_with_tag(str8_lit("projection"), camera3d->projection);
            se_u64_with_tag(str8_lit("viewport_shading"), camera3d->viewport_shading);
            se_u64_with_tag(str8_lit("polygon_mode"), camera3d->polygon_mode);
            se_v4f32_with_tag(str8_lit("viewport"), camera3d->viewport);
            se_b32_with_tag(str8_lit("hide_cursor"), camera3d->hide_cursor);
            se_b32_with_tag(str8_lit("lock_cursor"), camera3d->lock_cursor);
            se_b32_with_tag(str8_lit("is_active"), camera3d->is_active);
            se_f32_with_tag(str8_lit("zn"), camera3d->zn);
            se_f32_with_tag(str8_lit("zf"), camera3d->zf);
            se_v4f32_with_tag(str8_lit("v"), camera3d->v);
          }
          if(node->type_flags & RK_NodeTypeFlag_MeshInstance3D) SE_Struct_WithTag(str8_lit("mesh_inst3d"))
          {
            RK_MeshInstance3D *mesh_inst3d = node->mesh_inst3d;
          }
          if(node->type_flags & RK_NodeTypeFlag_SceneInstance) SE_Struct_WithTag(str8_lit("scene_inst"))
          {
            RK_SceneInstance *scene_inst = node->scene_inst;
          }
          if(node->type_flags & RK_NodeTypeFlag_AnimationPlayer) SE_Struct_WithTag(str8_lit("animation_player"))
          {
            RK_AnimationPlayer *animation_player = node->animation_player;
          }
          if(node->type_flags & RK_NodeTypeFlag_DirectionalLight) SE_Struct_WithTag(str8_lit("directional_light"))
          {
          }
          if(node->type_flags & RK_NodeTypeFlag_PointLight) SE_Struct_WithTag(str8_lit("point_light"))
          {
          }
          if(node->type_flags & RK_NodeTypeFlag_SpotLight) SE_Struct_WithTag(str8_lit("spoint_light"))
          {
          }
          if(node->type_flags & RK_NodeTypeFlag_Sprite2D) SE_Struct_WithTag(str8_lit("sprite2d"))
          {
            RK_Sprite2D *sprite2d = node->sprite2d;
            se_handle_with_tag(str8_lit("tex"), sprite2d->tex);
            se_u64_with_tag(str8_lit("shape"), sprite2d->shape);
            se_v2f32_with_tag(str8_lit("size"), sprite2d->size.v);
            se_u64_with_tag(str8_lit("anchor"), sprite2d->anchor);
            se_v4f32_with_tag(str8_lit("color"), sprite2d->color);
            se_b32_with_tag(str8_lit("omit_texture"), sprite2d->omit_texture);

            if(sprite2d->string.size > 0)
            {
              se_str_with_tag(str8_lit("string"), sprite2d->string);
              se_f32_with_tag(str8_lit("font_size"), sprite2d->font_size);
              se_v4f32_with_tag(str8_lit("font_color"), sprite2d->font_color);
              se_u64_with_tag(str8_lit("tab_size"), sprite2d->tab_size);
              se_u64_with_tag(str8_lit("text_raster_flag"), sprite2d->text_raster_flags);
            }
          }
          if(node->type_flags & RK_NodeTypeFlag_AnimatedSprite2D) SE_Struct_WithTag(str8_lit("animated_sprite2d"))
          {
            RK_AnimatedSprite2D *animated_sprite2d = node->animated_sprite2d;
            se_handle_with_tag(str8_lit("sheet"), animated_sprite2d->sheet);
            se_b32_with_tag(str8_lit("flipped"), animated_sprite2d->flipped);
            se_v2f32_with_tag(str8_lit("size"), animated_sprite2d->size);
            se_u64_with_tag(str8_lit("curr_tag"), animated_sprite2d->curr_tag);
            se_u64_with_tag(str8_lit("next_tag"), animated_sprite2d->next_tag);
            se_b32_with_tag(str8_lit("loop"), animated_sprite2d->loop);
            se_b32_with_tag(str8_lit("is_animating"), animated_sprite2d->is_animating);
            se_f32_with_tag(str8_lit("ts_ms"), animated_sprite2d->ts_ms);
          }
          if(node->type_flags & RK_NodeTypeFlag_TileMapLayer) SE_Struct_WithTag(str8_lit("tilemap_layer"))
          {
            RK_TileMapLayer *tilemap_layer = node->tilemap_layer;
            se_str_with_tag(str8_lit("name"), tilemap_layer->name);
          }
          if(node->type_flags & RK_NodeTypeFlag_TileMap) SE_Struct_WithTag(str8_lit("tilemap"))
          {
            RK_TileMap* tilemap= node->tilemap;
            se_v2u64_with_tag(str8_lit("size"), v2u64(tilemap->size.x, tilemap->size.y));
            se_v2f32_with_tag(str8_lit("tile_size"), tilemap->tile_size);
            se_2x2f32_with_tag(str8_lit("mat"), tilemap->mat);
            se_2x2f32_with_tag(str8_lit("mat_inv"), tilemap->mat_inv);
          }
        }
        node = rec.next;
      }
    }
  }

  se_build_end();
  se_yml_node_to_file(root, scene->save_path);

  // write block blob
  {
    String8 blob_path = scene->save_path;
    blob_path = str8_chop_last_dot(blob_path);
    blob_path = push_str8_cat(scratch.arena, blob_path, str8_lit(".blob"));
    FILE *file = fopen((char*)blob_path.str, "w");
    fwrite(blocks, block_size, 1, file);
    fclose(file);
  }

  scratch_end(scratch);
  return ret;
}

internal RK_Scene *
rk_scene_from_tscn(String8 path)
{
  RK_Scene *ret = rk_scene_alloc();
  Arena *arena = ret->arena;
  Temp scratch = scratch_begin(0,0);

  String8 blob_path = str8_chop_last_dot(path);
  blob_path = push_str8_cat(scratch.arena, blob_path, str8_lit(".blob"));
  U8 *blob = 0;
  {
    OS_Handle f = os_file_open(OS_AccessFlag_Read, (blob_path));
    FileProperties f_props = os_properties_from_file(f);
    U64 size = f_props.size;
    U8 *data = push_array(scratch.arena, U8, f_props.size);
    os_file_read(f, rng_1u64(0,size), data);
    blob = data;
    os_file_close(f);
  }

  rk_push_node_bucket(ret->node_bucket);
  rk_push_res_bucket(ret->res_bucket);
  rk_push_handle_seed(ret->handle_seed);

  SE_Node *se_node = se_yml_node_from_file(scratch.arena, path);
  String8 name = se_str_from_tag(se_node, str8_lit("name"));
  ret->name = push_str8_copy(arena, name);
  ret->save_path = push_str8_copy(arena, path);
  ret->setup_fn_name = push_str8_copy(arena, se_str_from_tag(se_node, str8_lit("setup_fn_name")));
  ret->update_fn_name = push_str8_copy(arena, se_str_from_tag(se_node, str8_lit("udpate_fn_name")));
  ret->default_fn_name = push_str8_copy(arena, se_str_from_tag(se_node, str8_lit("default_fn_name")));
  ret->hot_key = rk_key_from_v2u64(se_v2u64_from_tag(se_node, str8_lit("hot_key")));
  ret->active_key = rk_key_from_v2u64(se_v2u64_from_tag(se_node, str8_lit("active_key")));
  ret->omit_grid = se_b32_from_tag(se_node, str8_lit("omit_grid"));
  ret->omit_gizmo3d = se_b32_from_tag(se_node, str8_lit("omit_gizmo3d"));
  ret->omit_light = se_b32_from_tag(se_node, str8_lit("omit_light"));
  ret->gizmo3d_mode = se_u64_from_tag(se_node, str8_lit("gizmo3d_mode"));

  // scene custom_data
  {
    Vec2U64 range = se_v2u64_from_tag(se_node, str8_lit("custom_data"));
    if(range.y-range.x > 0)
    {
      U64 size = range.y-range.x;
      U8 *dst = push_array_fat_sized(arena, size, ret);
      MemoryCopy(dst, blob+range.x, size);
      ret->custom_data = dst;
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // load resource

  SE_Node *first_ext_res_node = se_arr_from_tag(se_node, str8_lit("ext_resources"));
  for(SE_Node *ext_res_node = first_ext_res_node; ext_res_node != 0; ext_res_node = ext_res_node->next)
  {
    Arena *res_arena = ret->res_bucket->arena_ref;

    Vec2U64 key_v2u64 = se_v2u64_from_tag(ext_res_node, str8_lit("key"));
    String8 name = se_str_from_tag(ext_res_node, str8_lit("name"));
    String8 path = se_str_from_tag(ext_res_node, str8_lit("path"));
    RK_ResourceKind kind = se_u64_from_tag(ext_res_node, str8_lit("kind"));

    RK_Key key = rk_key_make(key_v2u64.x, key_v2u64.y);
    RK_Resource *res = 0;

    switch(kind)
    {
      case RK_ResourceKind_Texture2D:
      {
        R_Tex2DSampleKind sample_kind = se_u64_from_tag(ext_res_node, str8_lit("sample_kind"));
        RK_Handle handle = rk_tex2d_from_path(path, 0, key);
        res = rk_res_from_handle(&handle);
        res->v.tex2d.sample_kind = sample_kind;
      }break;
      case RK_ResourceKind_SpriteSheet:
      {
        String8 img_path = se_str_from_tag(ext_res_node, str8_lit("path"));
        String8 meta_path = se_str_from_tag(ext_res_node, str8_lit("path_1"));
        RK_Handle handle = rk_spritesheet_from_image(img_path, meta_path, key);
        res = rk_res_from_handle(&handle);
      }break;
      case RK_ResourceKind_TileSet:
      {
        String8 dir = se_str_from_tag(ext_res_node, str8_lit("path"));
        RK_Handle handle = rk_tileset_from_dir(dir, key);
        res = rk_res_from_handle(&handle);
      }break;
      default:{InvalidPath;}break;
    }

    AssertAlways(res != 0);
    res->name = push_str8_copy(res_arena, name);
    res->path = push_str8_copy(res_arena, path);
  }

  ///////////////////////////////////////////////////////////////////////////////////////
  // build node

  SE_Node *first_n_node = se_arr_from_tag(se_node, str8_lit("nodes"));
  RK_Node *root = 0;
  RK_Node *active_camera = 0;
  for(SE_Node *n = first_n_node; n != 0; n = n->next)
  {
    Vec2U64 key_v2u64 = se_v2u64_from_tag(n, str8_lit("key"));
    Vec2U64 parent_key_v2u64 = se_v2u64_from_tag(n, str8_lit("parent"));
    RK_NodeFlags flags = se_u64_from_tag(n, str8_lit("flags"));
    RK_NodeTypeFlags type_flags = se_u64_from_tag(n, str8_lit("type_flags"));
    String8 name = se_str_from_tag(n, str8_lit("name"));

    RK_Key key = rk_key_make(key_v2u64.x, key_v2u64.y);
    RK_Key parent_key = rk_key_make(parent_key_v2u64.x, parent_key_v2u64.y);
    RK_Node *parent = 0;

    if(!rk_key_match(parent_key, rk_key_zero()))
    {
      parent = rk_node_from_key(parent_key);
    }

    // build node
    RK_Node *node = 0;
    RK_Parent_Scope(parent)
    {
      node = rk_build_node_from_key(type_flags, flags, key);
    }
    rk_node_equip_display_string(node, name);

    // function table 
    // TODO(XXX): missing setup and fixed_update
    for(SE_Node *fn = se_arr_from_tag(n, str8_lit("update")); fn != 0; fn = fn->next)
    {
      AssertAlways(fn->kind == SE_NodeKind_String);
      String8 fn_name = fn->v.se_str;
      // TODO(XXX): support setup and fixed_update
      rk_node_push_fn_(node, fn_name);
    }

    // custom data
    {
      Vec2U64 range = se_v2u64_from_tag(n, str8_lit("custom_data"));
      if(range.y-range.x > 0)
      {
        U64 size = range.y-range.x;
        U8 *dst = push_array_fat_sized(arena, size, node);
        MemoryCopy(dst, blob+range.x, size);
        node->custom_data = dst;
      }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    // equipments

    if(type_flags & RK_NodeTypeFlag_Node2D)
    {
      RK_Node2D *dst = node->node2d;
      SE_Node *src = se_struct_from_tag(n, str8_lit("node2d"));
      SE_Node *transform_src = se_struct_from_tag(src, str8_lit("transform"));
      if(src)
      {
        dst->transform.position = se_v2f32_from_tag(transform_src, str8_lit("position"));
        dst->transform.rotation = se_f32_from_tag(transform_src, str8_lit("rotation"));
        dst->transform.scale = se_v2f32_from_tag(transform_src, str8_lit("scale"));
        dst->transform.skew = se_f32_from_tag(transform_src, str8_lit("skew"));
        dst->z_index = se_f32_from_tag(src, str8_lit("z_index"));
      }
    }

    if(type_flags & RK_NodeTypeFlag_Node3D)
    {
      RK_Node3D *dst = node->node3d;
      SE_Node *src = se_struct_from_tag(n, str8_lit("node3d"));
      SE_Node *transform_src = se_struct_from_tag(src, str8_lit("transform"));
      if(src)
      {
        dst->transform.position = se_v3f32_from_tag(transform_src, str8_lit("position"));
        dst->transform.rotation = se_v4f32_from_tag(transform_src, str8_lit("rotation"));
        dst->transform.scale = se_v3f32_from_tag(transform_src, str8_lit("scale"));
      }
    }

    if(type_flags & RK_NodeTypeFlag_Camera3D)
    {
      RK_Camera3D *dst = node->camera3d;
      SE_Node *src = se_struct_from_tag(n, str8_lit("camera3d"));
      if(src)
      {
        dst->projection = se_u64_from_tag(src, str8_lit("projection"));
        dst->viewport_shading = se_u64_from_tag(src, str8_lit("viewport_shading"));
        dst->polygon_mode = se_u64_from_tag(src, str8_lit("polygon_mode"));
        Vec4F32 viewport = se_v4f32_from_tag(src, str8_lit("viewport"));
        dst->viewport = (Rng2F32){viewport.x, viewport.y, viewport.z, viewport.w};
        dst->hide_cursor = se_b32_from_tag(src, str8_lit("hide_cursor"));
        dst->lock_cursor = se_b32_from_tag(src, str8_lit("lock_cursor"));
        dst->is_active = se_b32_from_tag(src, str8_lit("is_active"));
        dst->zn = se_f32_from_tag(src, str8_lit("zn"));
        dst->zf = se_f32_from_tag(src, str8_lit("zf"));
        Vec4F32 v = se_v4f32_from_tag(src, str8_lit("v"));
        dst->v[0] = v.v[0];
        dst->v[1] = v.v[1];
        dst->v[2] = v.v[2];
        dst->v[3] = v.v[3];

        if(dst->is_active)
        {
          active_camera = node;
        }
      }
    }

    if(type_flags & RK_NodeTypeFlag_Sprite2D)
    {
      RK_Sprite2D *dst = node->sprite2d;
      SE_Node *src = se_struct_from_tag(n, str8_lit("sprite2d"));
      if(src)
      {
        SE_Handle tex = se_handle_from_tag(src, str8_lit("tex"));
        MemoryCopy(&dst->tex, &tex, sizeof(SE_Handle));

        dst->shape = se_u64_from_tag(src, str8_lit("shape"));
        dst->size.v = se_v2f32_from_tag(src, str8_lit("size"));
        dst->anchor = se_u64_from_tag(src, str8_lit("anchor"));
        dst->color = se_v4f32_from_tag(src, str8_lit("color"));
        dst->omit_texture = se_b32_from_tag(src, str8_lit("omit_texture"));
      }
    }

    if(type_flags & RK_NodeTypeFlag_AnimatedSprite2D)
    {
      RK_AnimatedSprite2D *dst = node->animated_sprite2d;
      SE_Node *src = se_struct_from_tag(n, str8_lit("animated_sprite2d"));
      if(src)
      {
        dst->sheet = rk_handle_from_se(se_handle_from_tag(src, str8_lit("sheet")));
        dst->flipped = se_b32_from_tag(src, str8_lit("flipped"));
        dst->size = se_v2f32_from_tag(src, str8_lit("size"));
        dst->curr_tag = se_u64_from_tag(src, str8_lit("curr_tag"));
        dst->next_tag = se_u64_from_tag(src, str8_lit("next_tag"));
        dst->loop = se_b32_from_tag(src, str8_lit("loop"));
        dst->is_animating = se_b32_from_tag(src, str8_lit("is_animating"));
        dst->ts_ms = se_f32_from_tag(src, str8_lit("ts_ms"));
      }
    }

    if(type_flags & RK_NodeTypeFlag_TileMapLayer)
    {
      RK_TileMapLayer *dst = node->tilemap_layer;
      SE_Node *src = se_struct_from_tag(n, str8_lit("tilemap_layer"));
      if(src)
      {
        dst->name = push_str8_copy(arena, se_str_from_tag(src, str8_lit("name")));
      }
    }

    if(type_flags & RK_NodeTypeFlag_TileMap)
    {
      RK_TileMap*dst = node->tilemap;
      SE_Node *src = se_struct_from_tag(n, str8_lit("tilemap"));
      if(src)
      {
        Vec2U64 size = se_v2u64_from_tag(src, str8_lit("size"));
        dst->size.x = size.x;
        dst->size.y = size.y;
        dst->tile_size = se_v2f32_from_tag(src, str8_lit("tile_size"));
        dst->mat = se_2x2f32_from_tag(src, str8_lit("mat"));
        dst->mat_inv = se_2x2f32_from_tag(src, str8_lit("mat_inv"));
      }
    }

    if(n == first_n_node) root = node;
  }

  ret->root = rk_handle_from_node(root);
  ret->active_camera = rk_handle_from_node(active_camera);

  rk_pop_node_bucket();
  rk_pop_res_bucket();
  rk_pop_handle_seed();
  scratch_end(scratch);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
// GLTF2.0

internal RK_Handle
rk_tex2d_from_gltf_tex(cgltf_texture *tex_src, String8 gltf_directory)
{
  RK_Handle ret = {0};

  // tex2d path
  Temp scratch = scratch_begin(0,0);
  String8List path_parts = {0};
  str8_list_push(scratch.arena, &path_parts, gltf_directory);
  str8_list_push(scratch.arena, &path_parts, str8_cstring(tex_src->image->uri));
  String8 path = str8_path_list_join_by_style(scratch.arena, &path_parts, PathStyle_Relative);
  ret = rk_tex2d_from_path(path, 1, rk_key_zero());
  scratch_end(scratch);
  return ret;
}

internal RK_Node *
rk_node_from_gltf_node(cgltf_node *cn, cgltf_data *data, RK_Key seed_key, String8 gltf_directory)
{
  Temp scratch = scratch_begin(0,0);
  RK_ResourceBucket *res_bucket = rk_top_res_bucket();
  U64 cn_idx = cgltf_node_index(data, cn);

  // Make key
  // NOTE: cn->name is not unique within gltf
  RK_Key key = rk_key_from_stringf(seed_key, "node_%d", cn_idx);

  // NOTE(k): every node may refer to a mesh or a camera
  cgltf_mesh *mesh_src = cn->mesh;
  cgltf_camera *camera_src = cn->camera;

  // NOTE(k): a node that refers to a mesh may also refer to a skin
  cgltf_skin *skin_src = cn->skin;

  RK_NodeTypeFlags type_flags = RK_NodeTypeFlag_Node3D;
  if(camera_src != 0)
  {
    type_flags |= RK_NodeTypeFlag_Camera3D;
  }

  RK_Node *ret = rk_build_node_from_key(type_flags, 0, key);
  rk_node_equip_display_string(ret, str8_cstring(cn->name));

  // transform
  RK_Transform3D *transform = &ret->node3d->transform;

  // NOTE(k): only the joint transforms are applied to the skinned mesh; the transform of the skinned mesh node MUST be ignored
  if(skin_src == 0)
  {
    if(cn->has_matrix)
    {
      rk_trs_from_xform((Mat4x4F32*)cn->matrix, &transform->position, &transform->rotation, &transform->scale);
    }
    else
    {
      if(cn->has_translation)
      {
        transform->position.x = cn->translation[0];
        transform->position.y = cn->translation[1];
        transform->position.z = cn->translation[2];
      }

      if(cn->has_rotation)
      {
        transform->rotation.x = cn->rotation[0];
        transform->rotation.y = cn->rotation[1];
        transform->rotation.z = cn->rotation[2];
        transform->rotation.w = cn->rotation[3];
      }

      if(cn->has_scale)
      {
        transform->scale.x = cn->scale[0];
        transform->scale.y = cn->scale[1];
        transform->scale.z = cn->scale[2];
      }
    }
  }

  // fill camera info
  if(camera_src != 0)
  {
    // TODO(XXX): to be implemented
  }

  // TODO(k): mesh gpu instancing

  // mesh
  RK_Handle skin = {0};
  if(mesh_src != 0) RK_Parent_Scope(ret)
  {
    if(skin_src != 0)
    {
      U64 skin_idx = cgltf_skin_index(data, skin_src);
      RK_Key skin_key = rk_res_key_from_stringf(RK_ResourceKind_Skin, seed_key, "skin_%d", skin_idx);
      skin = rk_handle_from_res(rk_res_from_key(skin_key));

      if(rk_handle_is_zero(skin))
      {
        RK_Resource *res = rk_res_alloc(skin_key);
        skin = rk_handle_from_res(res);
        RK_Skin *skin_dst = (RK_Skin*)&res->v;

        skin_dst->bind_count = skin_src->joints_count;
        skin_dst->binds = push_array(res_bucket->arena_ref, RK_Bind, skin_src->joints_count);
        Mat4x4F32 *inverse_bind_matrices = 0;
        {
          cgltf_accessor *accessor = cn->skin->inverse_bind_matrices;
          U64 offset = accessor->offset + accessor->buffer_view->offset;
          AssertAlways(accessor->type == cgltf_type_mat4);
          AssertAlways(accessor->stride == sizeof(Mat4x4F32));
          inverse_bind_matrices = (Mat4x4F32 *)(((U8 *)accessor->buffer_view->buffer->data)+offset);
        }

        for(U64 i = 0; i < skin_dst->bind_count; i++)
        {
          U64 joint_idx = cgltf_node_index(data, skin_src->joints[i]);
          RK_Key joint_key = rk_key_from_stringf(seed_key, "node_%d", joint_idx);
          skin_dst->binds[i].joint = joint_key;
          skin_dst->binds[i].inverse_bind_matrix = inverse_bind_matrices[i];
        }
      }
    }

    RK_Mesh *meshes = push_array(res_bucket->arena_ref, RK_Mesh, mesh_src->primitives_count);
    U64 mesh_idx = cgltf_mesh_index(data, mesh_src);
    for(U64 pi = 0; pi < mesh_src->primitives_count; pi++)
    {
      RK_Key mesh_key = rk_res_key_from_stringf(RK_ResourceKind_Mesh, seed_key, "mesh_%d_%d", mesh_idx, pi);

      RK_Handle mesh = {0};
      mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
      RK_Mesh *mesh_dst = 0;

      if(rk_handle_is_zero(mesh))
      {
        RK_Resource *res = rk_res_alloc(mesh_key);
        mesh = rk_handle_from_res(res);
        mesh_dst = (RK_Mesh*)&res->v;

        cgltf_primitive *primitive = &mesh_src->primitives[pi];

        // topology
        switch(primitive->type)
        {
          case cgltf_primitive_type_lines:          {mesh_dst->topology = R_GeoTopologyKind_Lines; }break;
          case cgltf_primitive_type_line_strip:     {mesh_dst->topology = R_GeoTopologyKind_LineStrip; }break;
          case cgltf_primitive_type_triangles:      {mesh_dst->topology = R_GeoTopologyKind_Triangles; }break;
          case cgltf_primitive_type_triangle_strip: {mesh_dst->topology = R_GeoTopologyKind_TriangleStrip; }break;
          case cgltf_primitive_type_points:
          case cgltf_primitive_type_line_loop:
          case cgltf_primitive_type_triangle_fan:
          default:{InvalidPath;}break;
        }

        // mesh indices
        {
          cgltf_accessor *accessor = mesh_src->primitives[pi].indices;
          AssertAlways(accessor->type == cgltf_type_scalar);
          Assert(accessor->component_type == cgltf_component_type_r_32u);
          AssertAlways(accessor->stride == 4);
          U64 offset = accessor->offset+accessor->buffer_view->offset;
          void *indices_src = (U8 *)accessor->buffer_view->buffer->data+offset;
          U64 size = sizeof(U32) * accessor->count;
          R_Handle indices = r_buffer_alloc(R_ResourceKind_Static, size, indices_src, size);
          mesh_dst->indices = indices;
          mesh_dst->indice_count = accessor->count;
        }

        // mesh vertex attributes
        {
          cgltf_attribute *attrs = mesh_src->primitives[pi].attributes;
          cgltf_size attr_count = mesh_src->primitives[pi].attributes_count;
          U64 vertex_count = attrs[0].data->count;
          R_Vertex *vertices_src = push_array(scratch.arena, R_Vertex, vertex_count);
          U64 size = sizeof(R_Vertex) * vertex_count;

          for(U64 vi = 0; vi < vertex_count; vi++)
          {
            R_Vertex *v = &vertices_src[vi];
            for(U64 ai = 0; ai < attr_count; ai++)
            {
              cgltf_attribute *attr = &attrs[ai];
              cgltf_accessor *accessor = attr->data;
              // cgltf_accessor *accessor = &attr->data[attr->index];

              U64 src_offset = accessor->offset + accessor->buffer_view->offset + accessor->stride*vi;
              U8 *src = (U8 *)accessor->buffer_view->buffer->data + src_offset;
              switch(attr->type)
              {
                case cgltf_attribute_type_position:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->pos, src, sizeof(v->pos));
                }break;
                case cgltf_attribute_type_normal:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->nor, src, sizeof(v->nor));
                }break;
                case cgltf_attribute_type_tangent:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->tan, src, sizeof(v->tan));
                }break;
                case cgltf_attribute_type_texcoord:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->tex, src, sizeof(v->tex));
                }break;
                case cgltf_attribute_type_color:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->col, src, sizeof(v->col));
                }break;
                case cgltf_attribute_type_joints:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_16u);
                  for(U64 k = 0; k < 4; k++) { v->joints.v[k] = ((U16 *)src)[k]; }
                }break;
                case cgltf_attribute_type_weights:
                {
                  AssertAlways(accessor->component_type == cgltf_component_type_r_32f);
                  MemoryCopy(&v->weights, src, sizeof(v->weights));
                }break;
                default: {}break;
              }
            }
          }
          mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, size, (void*)vertices_src, size);
          mesh_dst->vertex_count = vertex_count;
        }

        // load material
        RK_Handle mat = {0};
        {
          cgltf_material *mat_src = primitive->material;
          U64 mat_idx = cgltf_material_index(data, mat_src);
          RK_Key mat_key = rk_res_key_from_stringf(RK_ResourceKind_Material, seed_key, "material_%d", mat_idx);
          mat = rk_handle_from_res(rk_res_from_key(mat_key));

          if(rk_handle_is_zero(mat))
          {
            RK_Resource *res = rk_res_alloc(mat_key);
            mat = rk_handle_from_res(res);
            RK_Material *mat_dst = (RK_Material*)&res->v;

            // NOTE(k): GLTF supports 2 PBR workflows
            // 1. Metallic-Roughness workflow (Modern) (has_pbr_emtallic_roughness)
            // 2. Specular-Glossiness workflow (legacy)
            // GLTF's PBR relies on image-based lighting(IBL) for embient/reflections
            // use environment maps instead of GlobalAmbient/AmbientColor
            // focus on DiffuseColor (albedo), Reflectance (metallic), and SpecularPower (roughness/glossiness) for core PBR behavior
            {
              res->name = push_str8_copy(res_bucket->arena_ref, str8_cstring(mat_src->name));
              // textures
              mat_dst->textures[R_GeoTexKind_Emissive]      = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_Diffuse]       = rk_tex2d_from_gltf_tex(mat_src->pbr_metallic_roughness.base_color_texture.texture, gltf_directory);
              mat_dst->textures[R_GeoTexKind_Specular]      = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_SpecularPower] = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_Ambient]       = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_Opacity]       = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_Normal]        = rk_handle_zero();
              mat_dst->textures[R_GeoTexKind_Bump]          = rk_handle_zero();

              mat_dst->v.has_emissive_texture           = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Emissive]);
              mat_dst->v.has_diffuse_texture            = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Diffuse]);
              mat_dst->v.has_specular_texture           = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Specular]);
              mat_dst->v.has_specular_power_texture     = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_SpecularPower]);
              mat_dst->v.has_ambient_texture            = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Ambient]);
              mat_dst->v.has_opacity_texture            = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Opacity]);
              mat_dst->v.has_normal_texture             = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Normal]);
              mat_dst->v.has_bump_texture               = rk_handle_is_zero(mat_dst->textures[R_GeoTexKind_Bump]);

              // colors
              mat_dst->v.diffuse_color = *(Vec4F32*)mat_src->pbr_metallic_roughness.base_color_factor;
              mat_dst->v.ambient_color = mat_dst->v.diffuse_color;

              // f32
              mat_dst->v.opacity = mat_dst->v.diffuse_color.w;
              mat_dst->v.specular_power = 0;
              mat_dst->v.index_of_refraction = mat_src->ior.ior;
              mat_dst->v.alpha_cutoff = mat_src->alpha_cutoff;
            }
          }
        }
        mesh_dst->material = mat;
      }

      // create mesh_inst3d for every primitive
      String8 name = push_str8f(scratch.arena, "%S_%d", ret->name, pi);
      RK_Key key = rk_key_from_stringf(seed_key, "node_%d_%d", cn_idx, pi);
      RK_NodeFlags flags = 0;
      if(skin_src != 0) flags |= RK_NodeFlag_Float;
      RK_NodeTypeFlags type_flags = RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_MeshInstance3D;
      RK_Node *inst3d = rk_build_node_from_key(type_flags, flags, key);
      rk_node_equip_display_string(inst3d, name);
      inst3d->mesh_inst3d->mesh = mesh;
      inst3d->mesh_inst3d->skin = skin;
      inst3d->mesh_inst3d->skin_seed = seed_key;
    }
  }

  scratch_end(scratch);

  // recursive
  for(U64 i = 0; i < cn->children_count; i++) RK_Parent_Scope(ret)
  {
    rk_node_from_gltf_node(cn->children[i], data, seed_key, gltf_directory);
  }
  return ret;
}

internal RK_Handle
rk_packed_scene_from_gltf(String8 gltf_path)
{
  RK_Handle ret = {0};
  RK_Key key = rk_res_key_from_string(RK_ResourceKind_PackedScene, rk_key_zero(), gltf_path);

  Arena *arena = rk_top_res_bucket()->arena_ref;
  ret = rk_handle_from_res(rk_res_from_key(key));

  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    res->src_kind = RK_ResourceSrcKind_External;
    RK_PackedScene *packed = (RK_PackedScene*)rk_res_unwrap(res);
    ret = rk_handle_from_res(res);

    cgltf_options opts = {0};
    cgltf_data *data;
    cgltf_result parse_ret = cgltf_parse_file(&opts, (char*)gltf_path.str, &data);
    AssertAlways(parse_ret == cgltf_result_success && "Failed to load gltf");

    //- Load buffers
    cgltf_options buf_ld_opts = {0};
    cgltf_result buf_ld_ret = cgltf_load_buffers(&buf_ld_opts, data, (char *)gltf_path.str);
    AssertAlways(buf_ld_ret == cgltf_result_success);
    AssertAlways(data->scenes_count == 1);

    //- Select the first scene (TODO(XXX): only load default scene for now)
    cgltf_scene *root_scene = &data->scenes[0];
    AssertAlways(root_scene->nodes_count == 1);
    cgltf_node *cgltf_root = root_scene->nodes[0];

    // fill info
    packed->root = rk_node_from_gltf_node(cgltf_root, data, key, str8_chop_last_slash(gltf_path));
    res->path = push_str8_copy(arena, gltf_path);

    //- Load skeleton animations after model is loaded
    if(data->animations_count > 0) RK_Parent_Scope(packed->root)
    {
      // create AnimationPlayer node
      RK_Key node_key = rk_key_from_stringf(key, "animation_player");
      RK_Node *n = rk_build_node_from_key(RK_NodeTypeFlag_AnimationPlayer, 0, node_key);
      rk_node_equip_display_string(n, str8_lit("animation_player"));

      // fill info
      n->animation_player->animation_count = data->animations_count;
      n->animation_player->animations = push_array(arena, RK_Handle, data->animations_count);

      for(U64 i = 0; i < data->animations_count; i++)
      {
        n->animation_player->animations[i] = rk_animation_from_gltf_animation(data, &data->animations[i], key);
      }
    }

    cgltf_free(data); 
  }
  return ret;
}

internal RK_Handle
rk_animation_from_gltf_animation(cgltf_data *data, cgltf_animation *animation_src, RK_Key seed)
{
  RK_Handle ret = {0};
  Arena *arena = rk_top_res_bucket()->arena_ref;

  // NOTE(k): animation is unique in gltf, so we don't fetch from cache
  U64 animation_idx = cgltf_animation_index(data, animation_src);
  RK_Key key = rk_res_key_from_stringf(RK_ResourceKind_Animation, seed, "animation_%d", animation_idx);
  ret = rk_handle_from_res(rk_res_from_key(key));

  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    res->src_kind = RK_ResourceSrcKind_Bundled;
    ret = rk_handle_from_res(res);
    RK_Animation *anim = (RK_Animation*)&res->v;

    U64 track_count = animation_src->channels_count;
    F32 max_track_duration_sec = 0;
    RK_Track *tracks = push_array(arena, RK_Track, track_count);

    // load tracks
    for(U64 i = 0; i < track_count; i++)
    {
      RK_Track *track = &tracks[i];
      cgltf_animation_channel *channel = &animation_src->channels[i];

      RK_TrackTargetKind target_kind;
      switch(channel->target_path)
      {
        case cgltf_animation_path_type_translation: { target_kind = RK_TrackTargetKind_Position3D; }break;
        case cgltf_animation_path_type_rotation:    { target_kind = RK_TrackTargetKind_Rotation3D; }break;
        case cgltf_animation_path_type_scale:       { target_kind = RK_TrackTargetKind_Scale3D; }break;
        case cgltf_animation_path_type_weights:     { target_kind = RK_TrackTargetKind_MorphWeight3D; }break;
        default:                                    { InvalidPath; }break;
      }

      // target key
      RK_Key target_key;
      {
        U64 target_node_idx = cgltf_node_index(data, channel->target_node);
        target_key = rk_key_from_stringf(seed, "node_%d", target_node_idx);
      }

      // interpolation
      RK_InterpolationKind interpolation;
      switch(channel->sampler->interpolation)
      {
        case cgltf_interpolation_type_linear:       { interpolation = RK_InterpolationKind_Linear; }break;
        case cgltf_interpolation_type_step:         { interpolation = RK_InterpolationKind_Step; }break;
        case cgltf_interpolation_type_cubic_spline: { interpolation = RK_InterpolationKind_Cubic; }break;
        default:                                    { InvalidPath; }break;
      }

      // timestamps
      cgltf_accessor *input_accessor = channel->sampler->input;
      U8 *input_src = ((U8*)input_accessor->buffer_view->buffer->data) + input_accessor->offset + input_accessor->buffer_view->offset;
      Assert(input_accessor->type == cgltf_type_scalar);
      Assert(input_accessor->component_type == cgltf_component_type_r_32f);
      // values (position/rotation/scale/morph_weights)
      cgltf_accessor *output_accessor = channel->sampler->output;
      U8 *output_src = ((U8*)output_accessor->buffer_view->buffer->data) + output_accessor->offset + output_accessor->buffer_view->offset;

      U64 frame_count = input_accessor->count;

      for(U64 frame_idx = 0; frame_idx < frame_count; frame_idx++)
      {
        RK_TrackFrame *frame = push_array(arena, RK_TrackFrame, 1);
        frame->ts_sec = *(F32 *)(input_src+(frame_idx*input_accessor->stride));

        switch(target_kind)
        {
          case RK_TrackTargetKind_Position3D:
          {
            Assert(output_accessor->type == cgltf_type_vec3);
            Assert(output_accessor->component_type == cgltf_component_type_r_32f);
            Assert(output_accessor->stride == sizeof(F32)*3);
            frame->v.position3d = *(Vec3F32 *)(output_src+(frame_idx*output_accessor->stride));
          }break;
          case RK_TrackTargetKind_Scale3D:
          {
            Assert(output_accessor->type == cgltf_type_vec3);
            Assert(output_accessor->component_type == cgltf_component_type_r_32f);
            Assert(output_accessor->stride == sizeof(F32)*3);
            frame->v.scale3d = *(Vec3F32 *)(output_src+(frame_idx*output_accessor->stride));
          }break;
          case RK_TrackTargetKind_Rotation3D:
          {
            Assert(output_accessor->type == cgltf_type_vec4);
            Assert(output_accessor->component_type == cgltf_component_type_r_32f);
            Assert(output_accessor->stride == sizeof(F32)*4);
            frame->v.rotation3d = *(QuatF32 *)(output_src+(frame_idx*output_accessor->stride));
          }break;
          case RK_TrackTargetKind_MorphWeight3D:
          {
            Assert(output_accessor->type == cgltf_type_scalar);
            Assert(output_accessor->component_type == cgltf_component_type_r_32f);
            Assert(output_accessor->stride == sizeof(F32));
            Assert(output_accessor->count <= RK_MAX_MORPH_TARGET_COUNT);
            MemoryCopy(&frame->v.morph_weights3d, (output_src+(frame_idx*output_accessor->stride)), sizeof(frame->v.morph_weights3d));
          }break;
          default: {InvalidPath;}break;
        }

        // last frame in the track, update duration
        if(frame_idx == frame_count-1)
        {
          track->duration_sec = frame->ts_sec;
          if(frame->ts_sec > max_track_duration_sec) max_track_duration_sec = frame->ts_sec;
        }
        BTPushback_PLRSHZ(track->frame_btree_root,frame,parent,left,right,btree_size,btree_height,0);
      }

      track->target_kind   = target_kind;
      track->target_key    = target_key;
      track->interpolation = interpolation;
      track->frame_count   = frame_count;
    }

    String8 name = push_str8_copy(arena, str8_cstring(animation_src->name));
    res->name = name;
    anim->name = name;
    anim->duration_sec = max_track_duration_sec;
    anim->track_count = animation_src->channels_count;
    anim->tracks = tracks;
  }
  return ret;
}

/////////////////////////////////
//~ Other resources

internal RK_Image
rk_image_from_path(Arena *arena, String8 path)
{
  RK_Image ret = {0};
  S32 x,y,n;

  // TODO(XXX): we may use arena allocation for stbi_load, so that a realloc is avoidable
  U8 *src = stbi_load((char*)path.str, &x, &y, &n, 4);
  U64 size = x*y*4;
  U8 *dst = push_array(arena, U8, size);
  MemoryCopy(dst, src, size);
  stbi_image_free(src);

  ret.data = dst;
  ret.x = x;
  ret.y = y;
  ret.n = n;
  ret.path = push_str8_copy(arena, path);
  ret.filename = push_str8_copy(arena, str8_skip_last_slash(path));
  return ret;
}

internal RK_Handle
rk_tex2d_from_path(String8 path, B32 is_bundled, RK_Key key_override)
{
  RK_Handle ret = {0};
  Temp scratch = scratch_begin(0,0);
  // TODO(XXX): we should consider sampler kind here

  // NOTE(XXX): we should use relative path here, verify if that's case later 
  RK_Key key = key_override;
  if(rk_key_match(key, rk_key_zero()))
  {
    key = rk_res_key_from_string(RK_ResourceKind_Texture2D, rk_key_zero(), path);
  }
  RK_Handle res = rk_handle_from_res(rk_res_from_key(key));
  Arena *arena = rk_top_res_bucket()->arena_ref;
  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    ret = rk_handle_from_res(res);
    RK_Texture2D *tex2d = (RK_Texture2D*)&res->v;
    R_Tex2DSampleKind sample_kind = R_Tex2DSampleKind_Nearest;
    {
      RK_Image image = rk_image_from_path(scratch.arena, path);
      tex2d->tex = r_tex2d_alloc(R_ResourceKind_Static, sample_kind,
                                 v2s32(image.x,image.y), R_Tex2DFormat_RGBA8, image.data);
      tex2d->sample_kind = sample_kind;
      tex2d->size.x = image.x;
      tex2d->size.y = image.y;

      res->path = push_str8_copy(arena, image.path);
      res->name = push_str8_copy(arena, image.filename);
      res->src_kind = is_bundled ? RK_ResourceSrcKind_Bundled : RK_ResourceSrcKind_External;
    }
  }
  scratch_end(scratch);
  return ret;
}

internal RK_Handle *
rk_tex2d_from_dir(Arena *arena, String8 dir, U64 *count, B32 is_bundled, B32 sort)
{
  Temp scratch = scratch_begin(0,0);
  OS_FileIter *it = os_file_iter_begin(scratch.arena, dir, OS_FileIterFlag_SkipFolders);

  String8List files = {0};

  // first iteration to find count & do the allocation
  for(OS_FileInfo info = {0}; os_file_iter_next(scratch.arena, it, &info);)
  {
    B32 is_file = !(info.props.flags & FilePropertyFlag_IsFolder);
    B32 is_png = str8_ends_with(info.name, str8_lit(".png"), 0);

    if(is_file && is_png)
    {
      String8List parts = {0};
      str8_list_push(scratch.arena, &parts, dir);
      str8_list_push(scratch.arena, &parts, info.name);
      String8 path = str8_path_list_join_by_style(scratch.arena, &parts, PathStyle_Relative);
      str8_list_push(scratch.arena, &files, path);
    }
  }
  *count = files.node_count;

  String8 *files_sorted = push_array(scratch.arena, String8, files.node_count);
  U64 i = 0;
  for(String8Node *n = files.first; n != 0; n = n->next, i++)
  {
    files_sorted[i] = n->string;
  }
  // sorted based on trailling number (_number)
  if(sort) qsort(files_sorted, files.node_count, sizeof(String8), rk_path_cmp);

  RK_Handle *ret = push_array(arena, RK_Handle, files.node_count);
  for(i = 0; i < files.node_count; i++)
  {
    String8 path = files_sorted[i];
    ret[i] = rk_tex2d_from_path(path, is_bundled, rk_key_zero());
  }
  scratch_end(scratch);
  return ret;
}

internal String8
rk_string_from_token(Arena *arena, U8 *json, jsmntok_t *token)
{
  U64 len = token->end-token->start;
  String8 ret = {0};
  ret.str = push_array(arena, U8, len+1);
  ret.size = len;
  MemoryCopy(ret.str, json+token->start, len);
  return ret;
}

internal U64
rk_u64_from_token(U8 *json, jsmntok_t *token)
{
  String8 str = {json+token->start, token->end-token->start};
  U64 ret = u64_from_str8(str, 10);
  return ret;
}

internal U64
rk_b32_from_token(U8 *json, jsmntok_t *token)
{
  String8 str = {json+token->start, token->end-token->start};
  B32 ret = str8_match(str, str8_lit("true"), 0);
  return ret;
}

internal F64
rk_f64_from_token(U8 *json, jsmntok_t *token)
{
  String8 str = {json+token->start, token->end-token->start};
  U64 ret = f64_from_str8(str);
  return ret;
}

internal void 
rk_token_pass(jsmntok_t *tokens, U64 *i)
{
  jsmntok_t *t = &tokens[*i];
  switch(t->type)
  {
    case JSMN_UNDEFINED:
    {
      // noop
    }break;
    case JSMN_OBJECT:
    {
      (*i)++; // skip object itself
      for(U64 j = 0; j < t->size; j++)
      {
        (*i)++; // skip the key
        rk_token_pass(tokens, i);
      }
    }break;
    case JSMN_ARRAY:
    {
      (*i)++;
      for(U64 j = 0; j < t->size; j++)
      {
        rk_token_pass(tokens, i);
      }
    }break;
    case JSMN_STRING:
    {
      AssertAlways(t->size == 1 || t->size == 0);
      (*i)++;
      if(t->size == 1)
      {
        rk_token_pass(tokens, i);
      }
    }break;
    case JSMN_PRIMITIVE:
    {
      (*i)++;
    }break;
    default:{InvalidPath;}break;
  }
}

internal RK_Handle
rk_spritesheet_from_image(String8 path, String8 meta_path, RK_Key key_override)
{
  RK_Handle ret = {0};

  Temp scratch = scratch_begin(0,0);
  RK_Key key = key_override;
  if(rk_key_match(key, rk_key_zero()))
  {
    key = rk_res_key_from_string(RK_ResourceKind_SpriteSheet, rk_key_zero(), push_str8_cat(scratch.arena, path, meta_path));
  }
  Arena *arena = rk_top_res_bucket()->arena_ref;
  ret = rk_handle_from_res(rk_res_from_key(key));

  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    res->src_kind = RK_ResourceSrcKind_External;
    ret = rk_handle_from_res(res);
    RK_SpriteSheet *sheet = (RK_SpriteSheet*)&res->v;

    /////////////////////////////////////////////////////////////////////////////////
    // load meta

    // parse tokens
    U64 token_count = 3000;
    U8 *json;
    U64 json_size;
    jsmntok_t *tokens = push_array(scratch.arena, jsmntok_t, token_count);
    {
      FileReadAll(scratch.arena, meta_path, &json, &json_size);

      while(1)
      {
        jsmn_parser p;
        jsmn_init(&p);
        int ret = jsmn_parse(&p, (char*)json, json_size, tokens, token_count);

        // bad token, JSON string is corrupted
        if(ret == JSMN_ERROR_INVAL)
        {
          Trap();
        }
        // not enough tokens, JSON string is too large
        else if(ret == JSMN_ERROR_NOMEM)
        {
          token_count *= 2;
          tokens = push_array(scratch.arena, jsmntok_t, token_count);
        }
        // JSON string is too short, expecting more JSON data
        else if(token_count == JSMN_ERROR_PART)
        {
          Trap();
        }
        else
        {
          // A non-negative return value of jsmn_parse is the number of tokens actually 
          // used by the parser
          token_count = ret;
          break;
        }
      }
    }

    U64 frame_count;
    RK_SpriteSheetFrame *frames;
    RK_SpriteSheetTag *tags;
    U64 tag_count;
    Vec2F32 sheet_size = {0};

    // tokens to frames
    {
      jsmntok_t *root = &tokens[0];

      // skip root node
      U64 i = 1; // token index
      while(i < token_count)
      {
        String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);

        if(str8_match(key_str, str8_lit("frames"), 0))
        {
          i++; // skip the key
          U64 size = tokens[i].size;
          i++; // skip the array body
          frame_count = size;
          frames = push_array(arena, RK_SpriteSheetFrame, frame_count);
          for(U64 frame_idx = 0; frame_idx < size; frame_idx++)
          {
            i++;
            RK_SpriteSheetFrame *frame = &frames[frame_idx];
            U64 key_count = tokens[i].size;
            i++;
            for(U64 key_idx = 0; key_idx < key_count; key_idx++)
            {
              String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
              i++; // skip key

              if(str8_match(key_str, str8_lit("frame"), 0))
              {
                U64 key_count = tokens[i].size;
                i++;
                for(U64 key_idx = 0; key_idx < key_count; key_idx++)
                {
                  String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                  i++;
                  if(str8_match(key_str, str8_lit("x"), 0))
                  {
                    frame->x = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("y"), 0))
                  {
                    frame->y = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("w"), 0))
                  {
                    frame->w = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("h"), 0))
                  {
                    frame->h = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else
                  {
                    rk_token_pass(tokens, &i);
                  }
                }
              }
              else if(str8_match(key_str, str8_lit("spriteSourceSize"), 0))
              {
                U64 key_count = tokens[i].size;
                i++;
                for(U64 key_idx = 0; key_idx < key_count; key_idx++)
                {
                  String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                  i++;
                  if(str8_match(key_str, str8_lit("x"), 0))
                  {
                    frame->sprite_source_size.x = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("y"), 0))
                  {
                    frame->sprite_source_size.y = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("w"), 0))
                  {
                    frame->sprite_source_size.w = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("h"), 0))
                  {
                    frame->sprite_source_size.h = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else
                  {
                    rk_token_pass(tokens, &i);
                  }
                }
              }
              else if(str8_match(key_str, str8_lit("sourceSize"), 0))
              {
                U64 key_count = tokens[i].size;
                i++;
                for(U64 key_idx = 0; key_idx < key_count; key_idx++)
                {
                  String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                  i++;
                  if(str8_match(key_str, str8_lit("w"), 0))
                  {
                    frame->source_size.w = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else if(str8_match(key_str, str8_lit("h"), 0))
                  {
                    frame->source_size.h = rk_f64_from_token(json, &tokens[i]);
                    i++; 
                  }
                  else
                  {
                    rk_token_pass(tokens, &i);
                  }
                }
              }
              else if(str8_match(key_str, str8_lit("rotated"), 0))
              {
                frame->rotated = rk_b32_from_token(json, &tokens[i]);
                i++;
              }
              else if(str8_match(key_str, str8_lit("trimmed"), 0))
              {
                frame->trimmed = rk_b32_from_token(json, &tokens[i]);
                i++;
              }
              else if(str8_match(key_str, str8_lit("duration"), 0))
              {
                frame->duration = rk_f64_from_token(json, &tokens[i]);
                i++;
              }
              else
              {
                rk_token_pass(tokens, &i);
              }
            }
          }
        }
        else if(str8_match(key_str, str8_lit("meta"), 0))
        {
          i++; // skip the key
          U64 size = tokens[i].size;
          i++; // skip the body

          for(U64 j = 0; j < size; j++)
          {
            String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
            i++; // skip key
            if(str8_match(key_str, str8_lit("frameTags"), 0))
            {
              U64 size = tokens[i].size;
              i++; // skip the array body
              tag_count = size;
              tags = push_array(arena, RK_SpriteSheetTag, tag_count);
              for(U64 tag_idx = 0; tag_idx < size; tag_idx++)
              {
                U64 size = tokens[i].size;
                i++; // skip object body 

                RK_SpriteSheetTag *tag = &tags[tag_idx];

                for(U64 key_idx = 0; key_idx < size; key_idx++)
                {
                  String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                  i++; // skip key

                  if(str8_match(key_str, str8_lit("name"), 0))
                  {
                    String8 tag_name = str8(json+tokens[i].start, tokens[i].end-tokens[i].start);
                    tag->name = push_str8_copy(arena, tag_name);
                    i++;
                  }
                  else if(str8_match(key_str, str8_lit("from"), 0))
                  {
                    tag->from = rk_u64_from_token(json, &tokens[i]);
                    i++;
                  }
                  else if(str8_match(key_str, str8_lit("to"), 0))
                  {
                    tag->to = rk_u64_from_token(json, &tokens[i]);
                    i++;
                  }
                  else if(str8_match(key_str, str8_lit("direction"), 0))
                  {
                    String8 direction_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                    if(str8_match(direction_str, str8_lit("forward"), 0))
                    {
                      tag->direction = DirH_Right;
                    }
                    else if(str8_match(direction_str, str8_lit("backward"), 0))
                    {
                      tag->direction = DirH_Left;
                    }
                    i++;
                  }
                  else if(str8_match(key_str, str8_lit("color"), 0))
                  {
                    // TODO(XXX): why we need color
                    i++;
                  }
                  else
                  {
                    rk_token_pass(tokens, &i);
                  }
                }
              }
            }
            else if(str8_match(key_str, str8_lit("size"), 0))
            {
              U64 key_count = tokens[i].size;
              i++;
              for (U64 key_idx = 0; key_idx < key_count; key_idx++)
              {
                String8 key_str = rk_string_from_token(scratch.arena, json, &tokens[i]);
                i++;
                if(str8_match(key_str, str8_lit("w"), 0))
                {
                  sheet_size.x = rk_f64_from_token(json, &tokens[i]); 
                  i++;
                }
                else if(str8_match(key_str, str8_lit("h"), 0))
                {
                  sheet_size.y = rk_f64_from_token(json, &tokens[i]); 
                  i++;
                }
                else
                {
                  rk_token_pass(tokens, &i);
                }
              }
            }
            else
            {
              rk_token_pass(tokens, &i);
            }
          }
        }
        else
        {
          rk_token_pass(tokens, &i);
        }
      }
    }

    // acc tag frames total duration
    for(U64 i = 0; i < tag_count; i++)
    {
      F32 duration = 0; 
      RK_SpriteSheetTag *tag = &tags[i];
      for(U64 j = tag->from; j <= tag->to; j++)
      {
        duration += frames[j].duration;
      }

      tag->duration = duration;
    }

    sheet->tex         = rk_tex2d_from_path(path, 1, rk_key_zero());
    sheet->size        = sheet_size;
    sheet->frames      = frames;
    sheet->frame_count = frame_count;
    sheet->tags        = tags;
    sheet->tag_count   = tag_count;

    res->path_0 = push_str8_copy(arena, path);
    res->path_1 = push_str8_copy(arena, meta_path);
  }

  scratch_end(scratch);
  return ret;
}

internal RK_Handle
rk_tileset_from_dir(String8 dir, RK_Key key_override)
{
  RK_Handle ret = {0};
  RK_Key key = key_override;
  if(rk_key_match(key, rk_key_zero()))
  {
    key = rk_res_key_from_string(RK_ResourceKind_TileSet, rk_key_zero(), dir);
  }
  RK_Handle res = rk_handle_from_res(rk_res_from_key(key));
  Arena *arena = rk_top_res_bucket()->arena_ref;
  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    ret = rk_handle_from_res(res);
    RK_TileSet *tileset = (RK_TileSet*)&res->v;
    tileset->textures = rk_tex2d_from_dir(arena, dir, &tileset->texture_count, 1, 1);

    res->path = push_str8_copy(arena, dir);
    res->name = push_str8_copy(arena, str8_skip_last_slash(dir));
    res->src_kind = RK_ResourceSrcKind_External;
  }
  return ret;
}

internal RK_Handle
rk_material_from_color(String8 name, Vec4F32 color)
{
  RK_Handle ret = {0};
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key key = rk_res_key_from_string(RK_ResourceKind_Material, rk_key_zero(), name);
  ret = rk_handle_from_res(rk_res_from_key(key));

  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    ret = rk_handle_from_res(res);
    RK_Material *mat = (RK_Material*)&res->v;

    mat->v.diffuse_color = color;
    mat->v.opacity = 1.0f;

    res->name = push_str8_copy(arena, name);
    res->src_kind = RK_ResourceSrcKind_Sub;
  }
  return ret;
}

internal RK_Handle
rk_material_from_image(String8 name, String8 path)
{
  RK_Handle ret = {0};
  RK_Key key = rk_res_key_from_string(RK_ResourceKind_Material, rk_key_zero(), name);
  Arena *arena = rk_top_res_bucket()->arena_ref;
  ret = rk_handle_from_res(rk_res_from_key(key));

  if(rk_handle_is_zero(ret))
  {
    RK_Resource *res = rk_res_alloc(key);
    ret = rk_handle_from_res(res);
    RK_Material *mat = (RK_Material*)&res->v;

    mat->textures[R_GeoTexKind_Diffuse] = rk_tex2d_from_path(path, 0, rk_key_zero());
    mat->v.opacity = 1.0;
    mat->v.diffuse_color = v4f32(1,1,1,1);
    mat->v.has_diffuse_texture = 1;
    res->name = push_str8_copy(arena, name);
    res->src_kind = RK_ResourceSrcKind_Sub;
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Node building helper

internal RK_Node *
rk_node_from_packed_scene_node(RK_Node *node, RK_Key seed_key)
{
  RK_Key key = rk_key_merge(seed_key, node->key);
  RK_Node *ret = rk_build_node_from_key(node->type_flags,node->flags, key);
  rk_node_equip_display_string(ret, node->name);
  {
    ret->is_foreign = 1;
    ret->position   = node->position;
    ret->scale      = node->scale;
    ret->rotation   = node->rotation;
  }

  // copy equipment
  {
    if(node->type_flags & RK_NodeTypeFlag_Node2D)
    {
      MemoryCopy(ret->node2d, node->node2d, sizeof(*node->node2d));
    }

    if(node->type_flags & RK_NodeTypeFlag_Node3D)
    {
      MemoryCopy(ret->node3d, node->node3d, sizeof(*node->node3d));
    }

    if(node->type_flags & RK_NodeTypeFlag_Camera3D)
    {
      MemoryCopy(ret->camera3d, node->camera3d, sizeof(*node->camera3d));
    }

    if(node->type_flags & RK_NodeTypeFlag_MeshInstance3D)
    {
      MemoryCopy(ret->mesh_inst3d, node->mesh_inst3d, sizeof(*node->mesh_inst3d));
      ret->mesh_inst3d->skin_seed = seed_key;
    }

    if(node->type_flags & RK_NodeTypeFlag_AnimationPlayer)
    {
      MemoryCopy(ret->animation_player, node->animation_player, sizeof(*node->animation_player));
      ret->animation_player->target_seed = seed_key;
    }
  }

  // recursive
  for(RK_Node *child = node->first; child != 0; child = child->next) RK_Parent_Scope(ret)
  {
    rk_node_from_packed_scene_node(child, seed_key);
  }
  return ret;
}

internal RK_Node *
rk_node_from_packed_scene(String8 string, RK_PackedScene *packed)
{
  RK_Key seed_key = rk_key_from_string(rk_key_zero(), string);
  RK_Node *ret = 0;
  if(packed != 0)
  {
    ret = rk_node_from_packed_scene_node(packed->root, seed_key);
    ret->instance = packed;
    ret->name = push_str8_copy_static(string, ret->name_buffer);
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////
//~ Mesh primitives

internal void
rk_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 i,j,prevrow,thisrow,vertex_idx,indice_idx;
  F32 x,y,z;
  F32 onethird = 1.f/3.f;
  F32 twothirds = 2.f/3.f;

  Vec3F32 start_pos = scale_3f32(size, -0.5);

  U64 vertex_count = (subdivide_h+2) * (subdivide_w+2)*2 + (subdivide_h+2) * (subdivide_d+2)*2 + (subdivide_d+2) * (subdivide_w+2)*2; 
  U64 indice_count = (subdivide_h+1) * (subdivide_w+1)*12 + (subdivide_h+1) * (subdivide_d+1)*12 + (subdivide_d+1) * (subdivide_w+1)*12;

  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  vertex_idx = 0;
  indice_idx = 0;

  Vec3F32 pos,nor;
  Vec2F32 uv;

  // front + back
  y = start_pos.y;
  thisrow = 0;
  prevrow = 0;
  // y advancing loop
  for(j = 0; j < (subdivide_h+2); j++)
  {
    F32 v = j; 
    F32 v2 = v / (subdivide_w + 1.0);
    v /= (2.0 * (subdivide_h+1.0));

    x = start_pos.x;
    // x advancing loop
    for(i = 0; i < (subdivide_w+2); i++)
    {
      F32 u = i;
      F32 u2 = u / (subdivide_w+1.0);
      u /= (3.0 * (subdivide_w+1.0));

      // front
      pos = (Vec3F32){x, y, start_pos.z};
      nor = (Vec3F32){0.0, 0.0, 1.0};
      uv = (Vec2F32){u2,v2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      // back
      pos = (Vec3F32){-x, -y, -start_pos.z};
      nor = (Vec3F32){0.0, 0.0, -1.0};
      uv = (Vec2F32){v2,u2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      if(i > 0 && j > 0)
      {
        U64 i2 = i*2;

        // front
        indices[indice_idx++] = prevrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = prevrow + i2;

        indices[indice_idx++] = prevrow + i2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2;

        // back
        indices[indice_idx++] = prevrow + i2 - 1;
        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;

        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;
      }
      x += size.x / (subdivide_w + 1.0);
    }

    y += size.y / (subdivide_h + 1.0);
    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  // left + right
  y = start_pos.y;
  thisrow = vertex_idx;
  prevrow = 0;
  for(j = 0; j < (subdivide_h+2); j++)
  {
    F32 v = j; 
    F32 v2 = v / (subdivide_h + 1.0);
    v /= (2.0 * (subdivide_h+1.0));

    z = start_pos.z;
    for(i = 0; i < (subdivide_d+2); i++)
    {
      F32 u = i;
      F32 u2 = u / (subdivide_d+1.0);
      u /= (3.0 * (subdivide_d+1.0));

      // left
      pos = (Vec3F32){start_pos.x, y, -z};
      nor = (Vec3F32){-1.0, 0.0, 0.0};
      uv = (Vec2F32){u2,v2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      // right
      pos = (Vec3F32){-start_pos.x, -y, z};
      nor = (Vec3F32){1.0, 0.0, 0.0};
      uv = (Vec2F32){v2,u2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      if(i > 0 && j > 0)
      {
        U64 i2 = i*2;

        // right
        indices[indice_idx++] = prevrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = prevrow + i2;

        indices[indice_idx++] = prevrow + i2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2;

        // left
        indices[indice_idx++] = prevrow + i2 - 1;
        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;

        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;
      }
      z += size.z / (subdivide_d + 1.0);
    }
    y += size.y / (subdivide_h + 1.0);
    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  // top + bottom
  z = start_pos.z;
  thisrow = vertex_idx;
  prevrow = 0;
  for(j = 0; j < (subdivide_d+2); j++)
  {
    F32 v = j; 
    F32 v2 = v / (subdivide_d+1.0);
    v /= (2.0 * (subdivide_d+1.0));

    x = start_pos.x;
    for(i = 0; i < (subdivide_w+2); i++)
    {
      F32 u = i;
      F32 u2 = u / (subdivide_w+1.0);
      u /= (3.0 * (subdivide_w+1.0));

      // top
      pos = (Vec3F32){x, start_pos.y, -z};
      nor = (Vec3F32){0.0, -1.0, 0.0};
      uv = (Vec2F32){u2,v2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      // bottom
      pos = (Vec3F32){-x, -start_pos.y, z};
      nor = (Vec3F32){0.0, 1.0, 0.0};
      uv = (Vec2F32){v2,u2};
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      if(i > 0 && j > 0)
      {
        U64 i2 = i*2;

        // right
        indices[indice_idx++] = prevrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = prevrow + i2;

        indices[indice_idx++] = prevrow + i2;
        indices[indice_idx++] = thisrow + i2 - 2;
        indices[indice_idx++] = thisrow + i2;

        // left
        indices[indice_idx++] = prevrow + i2 - 1;
        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;

        indices[indice_idx++] = prevrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 + 1;
        indices[indice_idx++] = thisrow + i2 - 1;
      }
      x += size.x / (subdivide_w + 1.0);
    }

    z += size.z / (subdivide_d + 1.0);
    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_plane(Arena *arena, Vec2F32 size, U64 subdivide_w, U64 subdivide_d, B32 both_face, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  // TODO(XXX): both_face won't work as expected, since the nor is not flipped (we didn't duplicate vertex)
  U64 i,j,prevrow,thisrow,vertex_idx, indice_idx;
  F32 x,z;

  Vec2F32 start_pos = scale_2f32(size, -0.5);
  // Face Y
  Vec3F32 normal = {0.0f, -1.0f, 0.0f};

  U64 vertex_count   = (subdivide_d+2) * (subdivide_w+2);
  U64 indice_count   = (subdivide_d+1) * (subdivide_w+1) * 6 * (both_face?2:1);
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  vertex_idx = 0;
  indice_idx = 0;

  Vec3F32 pos, nor;
  Vec2F32 uv;

  /* top + bottom */
  z = start_pos.y;
  thisrow = vertex_idx;
  prevrow = 0;
  for(j = 0; j < (subdivide_d+2); j++)
  {
    x = start_pos.x;
    for(i = 0; i < (subdivide_w+2); i++)
    {
      F32 u = i;
      F32 v = j;
      u /= (subdivide_w + 1.0);
      v /= (subdivide_d + 1.0);

      pos = (Vec3F32){-x, 0.0, -z};
      nor = normal;
      uv = (Vec2F32){1.0-u, v}; /* 1.0-uv to match orientation with Quad */
      vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = prevrow + i - 1;
        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i - 1;

        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i;
        indices[indice_idx++] = thisrow + i - 1;

        if(both_face)
        {

          indices[indice_idx++] = prevrow + i - 1;
          indices[indice_idx++] = thisrow + i - 1;
          indices[indice_idx++] = prevrow + i;

          indices[indice_idx++] = prevrow + i;
          indices[indice_idx++] = thisrow + i - 1;
          indices[indice_idx++] = thisrow + i;
        }
      }
      x += size.x / (subdivide_w+1.0);
    }

    z += size.y / (subdivide_d+1.0);
    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);

  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_sphere(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 i,j,prevrow,thisrow,vertex_idx,indice_idx;
  F32 x,y,z;
  F32 scale = height * (is_hemisphere ? 1.0 : 0.5);

  // NOTE(k): only used if we calculate UV2
  // F32 circumference = radius * tau32;
  // F32 horizontal_length = circumference + p_uv2_padding;
  // F32 center_h = 0.5 * circumference / horizontal_length;

  U64 vertex_count = (rings+2)*(radial_segments+1);
  U64 indice_count  = (rings+1)*(radial_segments)*6;

  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  vertex_idx = 0;
  indice_idx = 0;

  thisrow = 0;
  prevrow = 0;
  // NOTE(k): Inlcude north and south pole
  for(j = 0; j < (rings+2); j++)
  {
    F32 v = j;
    F32 w;

    v /= (rings+1);
    w = sinf(pi32*v);
    y = cosf(pi32*v) * (-scale);

    for(i = 0; i < (radial_segments+1); i++)
    {
      F32 u = i;
      u /= radial_segments;

      Vec3F32 pos;
      Vec3F32 nor;

      x = cosf(u*tau32);
      z = sinf(u*tau32);

      if(is_hemisphere && y < 0.0)
      { 
        pos = v3f32(x*radius*w, 0, z*radius*w);
        nor = v3f32(0, -1.0, 0.0);
      }
      else
      {
        pos = v3f32(x*radius*w, y, z*radius*w);
        // TODO: don't understand this yet
        nor = normalize_3f32(v3f32(x*scale*w, radius*(y/scale), z*scale*w));
      }

      // TODO: add tangent
      vertices[vertex_idx++] = (R_Vertex){
        .pos = pos,
          .nor = nor,
      };

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = prevrow + i - 1;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = prevrow + i;

        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow + i;
      }
    }

    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_cylinder(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 i,j,prevrow,thisrow;
  F32 x,y,z,half_height;

  half_height = height/2.0f;

  U64 vertex_count = rings*(radial_segments+1);
  U64 indice_count = (rings-1)*(radial_segments)*6;
  if(cap_top)
  {
    vertex_count += 1 + radial_segments + 1;
    indice_count += radial_segments*3;
  }
  if(cap_bottom)
  {
    vertex_count += 1 + radial_segments + 1;
    indice_count += radial_segments*3;
  }

  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  Vec3F32 pos,nor;

  thisrow = 0;
  prevrow = 0;
  for(j = 0; j < rings; j++)
  {
    F32 v = j;
    v /= (rings-1);
    y = mix_1f32(-half_height, half_height, v);

    for(i = 0; i < (radial_segments+1); i++)
    {
      F32 u = i;
      u /= radial_segments;

      x = cosf(u*tau32);
      z = sinf(u*tau32);

      pos = (Vec3F32){x*radius, y, z*radius};
      nor = (Vec3F32){x, 0, z};
      nor = normalize_3f32(nor);

      vertices[vertex_idx++] = (R_Vertex){
        .pos = pos,
          // TODO: normal, tagnent
          .nor = nor,
      };

      if(i > 0 && j > 0) 
      { 
        // side segment
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = prevrow + i - 1;

        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow + i;
        indices[indice_idx++] = prevrow + i;
      }
    }

    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  if(cap_top)
  {
    y = -half_height;
    pos = (Vec3F32){0,y,0};
    nor = (Vec3F32){0,-1,0};
    vertices[vertex_idx++] = (R_Vertex){pos, nor}; // circle origin 
    thisrow = vertex_idx;

    for(i = 0; i < (radial_segments+1); i++)
    {
      F32 u =i;
      u /= radial_segments;

      x = cosf(u*tau32);
      z = sinf(u*tau32);

      pos = (Vec3F32){x*radius, y, z*radius};
      nor = normalize_3f32(nor);
      vertices[vertex_idx++] = (R_Vertex){.pos = pos, .nor = nor};

      if(i > 0)
      {
        indices[indice_idx++] = thisrow + i;
        indices[indice_idx++] = thisrow - 1;
        indices[indice_idx++] = thisrow + i - 1;
      }
    }
  }

  if(cap_bottom)
  {
    y = half_height;
    pos = (Vec3F32){0,y,0};
    nor = (Vec3F32){0,1,0};
    vertices[vertex_idx++] = (R_Vertex){pos, nor}; // circle origin 
    thisrow = vertex_idx;

    for(i = 0; i < (radial_segments+1); i++)
    {
      F32 u =i;
      u /= radial_segments;

      x = cosf(u*tau32);
      z = sinf(u*tau32);

      pos = (Vec3F32){x*radius, y, z*radius};
      nor = normalize_3f32(nor);
      vertices[vertex_idx++] = (R_Vertex){.pos = pos, .nor = nor};

      if(i > 0)
      {
        indices[indice_idx++] = thisrow + i;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow - 1;
      }
    }
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_cone(Arena *arena, F32 radius, F32 height, U64 radial_segments, B32 cap_bottom, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 vertex_count = (radial_segments+2) + cap_bottom*(radial_segments+2);
  U64 indice_count = radial_segments*3 + cap_bottom*(radial_segments*3);
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices = push_array(arena, U32, indice_count);

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  // push tip
  {
    R_Vertex vertex = {0};
    vertex.pos = v3f32(0,-height,0);
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(0,-height,0);
    vertices[vertex_idx++] = vertex;
  }

  U64 curr_vertex_idx = 0;
  U64 prev_vertex_idx = 0;
  F32 turn = tau32 / (F32)radial_segments;
  for(U64 i = 0; i < (radial_segments+1); i++)
  {
    F32 rad = i*turn;
    // on xz plane
    F32 x = cosf(rad) * radius;
    F32 z = sinf(rad) * radius;

    R_Vertex vertex = {0};
    vertex.pos = v3f32(x,0,z);
    vertex.nor = v3f32(0,1,0);
    vertex.tan = v3f32(0,0,0);

    curr_vertex_idx = vertex_idx;
    vertices[vertex_idx++] = vertex;

    // counter-clock wise
    if(i > 0)
    {
      indices[indice_idx++] = prev_vertex_idx;
      indices[indice_idx++] = curr_vertex_idx;
      indices[indice_idx++] = 0; // origin
    }
    prev_vertex_idx = curr_vertex_idx;
  }

  if(cap_bottom)
  {
    U64 origin_idx = vertex_idx;
    // push_origin
    {
      R_Vertex vertex = {0};
      vertex.pos = v3f32(0,0,0);
      vertex.nor = v3f32(0,1,0);
      vertex.tan = v3f32(1,0,0);
      vertices[vertex_idx++] = vertex;
    }

    for(U64 i = 0; i < (radial_segments+1); i++)
    {
      F32 rad = i*turn;
      // on xz plane
      F32 x = cosf(rad) * radius;
      F32 z = sinf(rad) * radius;

      R_Vertex vertex = {0};
      vertex.pos = v3f32(x,0,z);
      vertex.nor = v3f32(0,1,0);
      vertex.tan = v3f32(0,0,0);

      curr_vertex_idx = vertex_idx;
      vertices[vertex_idx++] = vertex;

      // counter-clock wise
      if(i > 0)
      {
        indices[indice_idx++] = prev_vertex_idx;
        indices[indice_idx++] = origin_idx; // origin
        indices[indice_idx++] = curr_vertex_idx;
      }
      prev_vertex_idx = curr_vertex_idx;
    }
  }

  Assert(vertex_idx == vertex_count);
  Assert(indice_idx == indice_count);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_capsule(Arena *arena, F32 radius, F32 height, U64 radial_segments, U64 rings, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 i,j,prevrow,thisrow,vertex_idx,indice_idx,vertex_count,indice_count;
  F32 x,y,z,u,v,w;

  vertex_count = (rings+2)*(radial_segments+1)*3;
  indice_count = (rings+1)*radial_segments*6*3;

  // TODO: calculate uv2

  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  vertex_idx = 0;
  indice_idx = 0;

  Vec3F32 pos,nor;
  Vec4F32 col = v4f32(1.0,1.0,1.0,1.0);

  // top hemisphere
  thisrow = 0;
  prevrow = 0;
  for(j = 0; j < (rings+2); j++)
  {
    v = j;
    v /= (rings+1);
    w = sinf(0.5*pi32*v);
    y = radius * cosf(0.5*pi32*v);

    for(i = 0; i < (radial_segments+1); i++)
    {
      u = i;
      u /= radial_segments;

      x = -sinf(u*tau32);
      z = cosf(u*tau32);

      pos = (Vec3F32){x*radius*w, y, -z*radius*w};
      nor = normalize_3f32(pos);
      pos = add_3f32(pos, v3f32(0.0, 0.5*height - radius, 0.0));

      vertices[vertex_idx++] = (R_Vertex){ .pos = pos, .nor = nor, .col = col };

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = prevrow + i - 1;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = prevrow + i;

        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow + i;
      }
    }

    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  // cylinder
  // thisrow = vertex_idx;
  prevrow = 0;
  for(j = 0; j < (rings+2); j++)
  {
    v = j;
    v /= (rings+1);

    y = (height - 2.0*radius) * v;
    y = (0.5*height - radius) - y;

    for(i = 0; i < (radial_segments+1); i++)
    {
      u = i;
      u /= radial_segments;

      x = -sinf(u*tau32);
      z = cosf(u*tau32);

      pos = (Vec3F32){x*radius, y, -z*radius};
      nor = (Vec3F32){x, 0.0, -z};

      vertices[vertex_idx++] = (R_Vertex){ .pos = pos, .nor = nor, .col = col };

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = prevrow + i - 1;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = prevrow + i;

        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow + i;
      }
    }

    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  // bottom hemisphere
  // thisrow = vertex_idx;
  prevrow = 0;
  for(j = 0; j < (rings+2); j++)
  {
    v = j;

    v /= (rings+1);
    v += 1.0;
    w = sinf(0.5*pi32*v);
    y = radius * cosf(0.5*pi32*v);

    for(i = 0; i < (radial_segments+1); i++)
    {
      u = i;
      u /= radial_segments;

      x = -sinf(u*tau32);
      z = cosf(u*tau32);

      pos = (Vec3F32){x*radius*w, y, -z*radius*w};
      nor = normalize_3f32(pos);
      pos = add_3f32(pos, v3f32(0.0, -0.5*height + radius, 0.0));

      vertices[vertex_idx++] = (R_Vertex){ .pos = pos, .nor = nor, .col = col };

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = prevrow + i - 1;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = prevrow + i;

        indices[indice_idx++] = prevrow + i;
        indices[indice_idx++] = thisrow + i - 1;
        indices[indice_idx++] = thisrow + i;
      }
    }

    prevrow = thisrow;
    thisrow = vertex_idx;
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_torus(Arena *arena, F32 inner_radius, F32 outer_radius, U64 rings, U64 ring_segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 vertex_count,indice_count,vertex_idx,indice_idx;

  vertex_count = (rings+1) * (ring_segments+1); 
  indice_count = rings * ring_segments * 6;
  vertex_idx = 0;
  indice_idx = 0;
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices       = push_array(arena, U32, indice_count);

  F32 min_radius = inner_radius;
  F32 max_radius = outer_radius;
  if(min_radius > max_radius)
  {
    Swap(F32, min_radius, max_radius);
  }
  F32 radius = (max_radius-min_radius) * 0.5;

  for(U64 i = 0; i < (rings+1); i++)
  {
    U64 prevrow = (i-1) * (ring_segments + 1);
    U64 thisrow = i * (ring_segments+1);
    F32 inci = (F32)i / rings;
    F32 angi = inci * tau32;

    Vec2F32 normali = (i == rings) ? v2f32(0.f, -1.f) : v2f32(-sinf(angi), -cosf(angi));

    for(U64 j = 0; j < (ring_segments+1); j++)
    {
      F32 incj = (F32)j / ring_segments;
      F32 angj = incj * tau32;

      Vec2F32 normalj = (j == ring_segments) ? v2f32(-1.f,0.f) : v2f32(-cosf(angj), sinf(angj));
      Vec2F32 normalk = add_2f32(scale_2f32(normalj,radius), v2f32(min_radius+radius, 0));

      R_Vertex vertex = {0};
      vertex.pos = v3f32(normali.x * normalk.x, normalk.y, normali.y * normalk.x);
      vertex.nor = v3f32(normali.x * normalj.x, normalj.y, normali.y * normalj.x);
      vertex.tan = v3f32(normali.y, 0.f, -normali.x);
      vertex.tex = v2f32(inci, incj);
      vertices[vertex_idx++] = vertex;

      if(i > 0 && j > 0)
      {
        indices[indice_idx++] = thisrow + j - 1;
        indices[indice_idx++] = prevrow + j - 1;
        indices[indice_idx++] = prevrow + j;

        indices[indice_idx++] = thisrow + j - 1;
        indices[indice_idx++] = prevrow + j;
        indices[indice_idx++] = thisrow + j;
      }
    }
  }

  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_circle(Arena *arena, F32 radius, U64 segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 vertex_count = segments+2;
  U64 indice_count = segments*3;
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices = push_array(arena, U32, indice_count);
  F32 turn_rad = tau32 / (F32)segments;

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  {
    Vec3F32 origin = {0,0,0};
    R_Vertex vertex = {0};
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = origin;
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;
  }

  // default to xz plane, face to -z, counter clock wise
  F32 x,z;
  U64 prev_vertex_indice;
  for(U64 i = 0; i < segments+1; i++)
  {
    F32 rad = turn_rad * i;
    x = cosf(rad) * radius;
    z = sinf(rad) * radius;
    R_Vertex vertex = {0};
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = v3f32(x,0,z);
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;

    U64 curr_vertex_indice = vertex_idx-1;
    if(i > 0)
    {
      indices[indice_idx++] = prev_vertex_indice;
      indices[indice_idx++] = 0;
      indices[indice_idx++] = curr_vertex_indice;
    }
    prev_vertex_indice = curr_vertex_indice;
  }

  Assert(vertex_idx == vertex_count);
  Assert(indice_idx == indice_count);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_circle_lined(Arena *arena, F32 radius, U64 segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  U64 vertex_count = segments+1;
  U64 indice_count = segments*2;
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices = push_array(arena, U32, indice_count);
  F32 turn_rad = tau32 / (F32)segments;

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  // default to xz plane, face to -z, counter clock wise
  F32 x,z;
  U64 prev_vertex_indice;
  for(U64 i = 0; i < segments+1; i++)
  {
    F32 rad = turn_rad * i;
    x = cosf(rad) * radius;
    z = sinf(rad) * radius;
    R_Vertex vertex = {0};
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = v3f32(x,0,z);
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;

    U64 curr_vertex_indice = vertex_idx-1;
    if(i > 0)
    {
      indices[indice_idx++] = prev_vertex_indice;
      indices[indice_idx++] = curr_vertex_indice;
    }
    prev_vertex_indice = curr_vertex_indice;
  }

  Assert(vertex_idx == vertex_count);
  Assert(indice_idx == indice_count);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

internal void
rk_mesh_primitive_arc_filled(Arena *arena, F32 radius, F32 pct, U64 segments, B32 both_face, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
  // TODO(XXX): both_face won't work as expected, since the nor is not flipped (we didn't duplicate vertex)
  U64 vertex_count = segments+2;
  U64 indice_count = segments*3*(2*both_face);
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices = push_array(arena, U32, indice_count);
  F32 turn_rad = (tau32*pct) / (F32)segments;

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  // origin
  {
    R_Vertex vertex = {0};
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = v3f32(0,0,0);
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;
  }

  // default to xz plane, face to -z, counter clock wise
  F32 x,z;
  U64 prev_vertex_indice;
  for(U64 i = 0; i < segments+1; i++)
  {
    F32 rad = turn_rad * i;
    x = cosf(rad) * radius;
    z = sinf(rad) * radius;
    R_Vertex vertex = {0};
    vertex.nor = v3f32(0,-1,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = v3f32(x,0,z);
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;

    U64 curr_vertex_indice = vertex_idx-1;
    if(i > 0)
    {
      indices[indice_idx++] = 0; // origin
      indices[indice_idx++] = prev_vertex_indice;
      indices[indice_idx++] = curr_vertex_indice;

      if(both_face)
      {
        indices[indice_idx++] = 0; // origin
        indices[indice_idx++] = curr_vertex_indice;
        indices[indice_idx++] = prev_vertex_indice;
      }
    }
    prev_vertex_indice = curr_vertex_indice;
  }

  Assert(vertex_idx == vertex_count);
  Assert(indice_idx == indice_count);
  *vertices_out = vertices;
  *vertices_count_out = vertex_count;
  *indices_out = indices;
  *indices_count_out = indice_count;
}

//~ Node building helper

internal void
rk_node_equip_box(RK_Node *n, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key mesh_key;
  {
    F64 sizex_f64 = size.x;
    F64 sizey_f64 = size.y;
    F64 sizez_f64 = size.z;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_BoxPrimitive,
      *(U64*)&sizex_f64,
      *(U64*)&sizey_f64,
      *(U64*)&sizez_f64,
      subdivide_w,
      subdivide_h,
      subdivide_d,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;
    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_box(scratch.arena, size, subdivide_w,subdivide_h,subdivide_d, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_BoxPrimitive;            
      mesh_dst->src.box_primitive.subdivide_w = subdivide_w;
      mesh_dst->src.box_primitive.subdivide_h = subdivide_h;
      mesh_dst->src.box_primitive.subdivide_d = subdivide_d;
      mesh_dst->src.box_primitive.size = size;
      scratch_end(scratch);
    }
  }
  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal void
rk_node_equip_plane(RK_Node *n, Vec2F32 size, U64 subdivide_w, U64 subdivide_d)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;

  RK_Key mesh_key;
  {
    F64 sizex_f64 = size.x;
    F64 sizey_f64 = size.y;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_PlanePrimitive,
      *(U64*)&sizex_f64,
      *(U64*)&sizey_f64,
      subdivide_w,
      subdivide_d,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;

    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_plane(scratch.arena, size, subdivide_w,subdivide_d, 0, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_BoxPrimitive;            
      mesh_dst->src.plane_primitive.subdivide_w = subdivide_w;
      mesh_dst->src.plane_primitive.subdivide_d = subdivide_d;
      mesh_dst->src.plane_primitive.size = size;
      scratch_end(scratch);
    }
  }

  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal void
rk_node_equip_sphere(RK_Node *n, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key mesh_key;
  {
    F64 radius_f64 = radius;
    F64 height_f64 = height;
    B64 is_hemisphre = is_hemisphere;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_SpherePrimitive,
      *(U64*)&radius_f64,
      *(U64*)&height_f64,
      radial_segments,
      rings,
      *(U64*)&is_hemisphere,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;

    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_sphere(scratch.arena, radius, height, radial_segments, rings, is_hemisphere, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_SpherePrimitive;
      mesh_dst->src.sphere_primitive.radius = radius;
      mesh_dst->src.sphere_primitive.height = height;
      mesh_dst->src.sphere_primitive.radial_segments = radial_segments;
      mesh_dst->src.sphere_primitive.rings = rings;
      mesh_dst->src.sphere_primitive.is_hemisphere = is_hemisphere;
      scratch_end(scratch);
    }
  }

  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal void
rk_node_equip_cylinder(RK_Node *n, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key mesh_key;
  {
    F64 radius_f64 = radius;
    F64 height_f64 = height;
    B64 cap_top = cap_top;
    B64 cap_bottom = cap_bottom;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_CylinderPrimitive,
      *(U64*)&radius_f64,
      *(U64*)&height_f64,
      radial_segments,
      rings,
      *(U64*)&cap_top,
      *(U64*)&cap_bottom,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;
    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_cylinder(scratch.arena, radius, height, radial_segments, rings, cap_top, cap_bottom, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_CylinderPrimitive;
      mesh_dst->src.cylinder_primitive.radius = radius;
      mesh_dst->src.cylinder_primitive.height = height;
      mesh_dst->src.cylinder_primitive.radial_segments = radial_segments;
      mesh_dst->src.cylinder_primitive.rings = rings;
      mesh_dst->src.cylinder_primitive.cap_top = cap_top;
      mesh_dst->src.cylinder_primitive.cap_bottom = cap_bottom;
      scratch_end(scratch);
    }
  }

  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal void
rk_node_equip_capsule_(RK_Node *n, F32 radius, F32 height, U64 radial_segments, U64 rings)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key mesh_key;
  {
    F64 radius_f64 = radius;
    F64 height_f64 = height;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_CapsulePrimitive,
      *(U64*)&radius_f64,
      *(U64*)&height_f64,
      radial_segments,
      rings,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;
    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_capsule(scratch.arena, radius, height, radial_segments, rings, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_CapsulePrimitive;
      mesh_dst->src.cylinder_primitive.radius = radius;
      mesh_dst->src.cylinder_primitive.height = height;
      mesh_dst->src.cylinder_primitive.radial_segments = radial_segments;
      mesh_dst->src.cylinder_primitive.rings = rings;
      scratch_end(scratch);
    }
  }

  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal void
rk_node_equip_torus(RK_Node *n, F32 inner_radius, F32 outer_radius, U64 rings, U64 ring_segments)
{
  Arena *arena = rk_top_res_bucket()->arena_ref;
  RK_Key mesh_key;
  {
    F64 inner_radius_f64 = inner_radius;
    F64 outer_radius_f64 = outer_radius;
    U64 hash_part[] =
    {
      (U64)RK_MeshSourceKind_CapsulePrimitive,
      *(U64*)&inner_radius_f64,
      *(U64*)&outer_radius_f64,
      rings,
      ring_segments,
    };
    mesh_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
  }

  RK_Handle mesh = rk_handle_from_res(rk_res_from_key(mesh_key));
  if(rk_handle_is_zero(mesh))
  {
    RK_Resource *res = rk_res_alloc(mesh_key);
    mesh = rk_handle_from_res(res);
    RK_Mesh *mesh_dst = (RK_Mesh*)&res->v;
    {
      Temp scratch = scratch_begin(0,0);
      R_Vertex *vertices;
      U64 vertex_count;
      U32 *indices;
      U64 indice_count;
      rk_mesh_primitive_torus(scratch.arena, inner_radius, outer_radius, rings, ring_segments, &vertices, &vertex_count, &indices, &indice_count);
      U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
      U64 indice_buffer_size = sizeof(U32) * indice_count;

      mesh_dst->topology = R_GeoTopologyKind_Triangles;
      mesh_dst->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
      mesh_dst->vertex_count = vertex_count;
      mesh_dst->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
      mesh_dst->indice_count = indice_count;
      mesh_dst->src_kind = RK_MeshSourceKind_TorusPrimitive;
      mesh_dst->src.torus_primitive.inner_radius = inner_radius;
      mesh_dst->src.torus_primitive.outer_radius = outer_radius;
      mesh_dst->src.torus_primitive.rings = rings;
      mesh_dst->src.torus_primitive.ring_segments = ring_segments;
      scratch_end(scratch);
    }
  }

  rk_node_equip_type_flags(n, RK_NodeTypeFlag_MeshInstance3D);
  n->mesh_inst3d->mesh = mesh;
}

internal RK_DrawNode *
rk_drawlist_push_rect(Arena *arena, RK_DrawList *drawlist, Rng2F32 dst, Rng2F32 src)
{
  R_Vertex *vertices = push_array(arena, R_Vertex, 4);
  U64 vertex_count = 4;
  U32 indice_count = 6;
  U32 *indices = push_array(arena, U32, 6);

  Vec4F32 color_zero = v4f32(0,0,0,0);
  // top left 0
  vertices[0].col = color_zero;
  vertices[0].nor = v3f32(0,0,-1);
  vertices[0].pos = v3f32(dst.x0, dst.y0, 0);
  vertices[0].tex = v2f32(src.x0, src.y0);

  // bottom left 1
  vertices[1].col = color_zero;
  vertices[1].nor = v3f32(0,0,-1);
  vertices[1].pos = v3f32(dst.x0, dst.y1, 0);
  vertices[1].tex = v2f32(src.x0, src.y1);

  // top right 2
  vertices[2].col = color_zero;
  vertices[2].nor = v3f32(0,0,-1);
  vertices[2].pos = v3f32(dst.x1, dst.y0, 0);
  vertices[2].tex = v2f32(src.x1, src.y0);

  // bottom right 3
  vertices[3].col = color_zero;
  vertices[3].nor = v3f32(0,0,-1);
  vertices[3].pos = v3f32(dst.x1, dst.y1, 0);
  vertices[3].tex = v2f32(src.x1, src.y1);

  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;

  indices[3] = 1;
  indices[4] = 3;
  indices[5] = 2;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_string(Arena *arena, RK_DrawList *drawlist, Rng2F32 dst, String8 string, D_FancyRunList *list, F_Tag font, F32 font_size, U64 tab_size, F_RasterFlags text_raster_flags)
{
  RK_DrawNode *ret = 0;
  RK_DrawNode *last_to_draw = 0;

  // TODO(XXX): not sure if we should start from bottom left, find out later
  Vec2F32 p = {dst.x0, dst.y1}; // bottom left
  Vec2F32 dst_dim = dim_2f32(dst);
  F32 max_x = dst_dim.x;
  F_Run trailer_run = {0};
  B32 trailer_enabled = ((p.x+list->dim.x) > max_x && (p.x+trailer_run.dim.x) < max_x);

  F32 off_x = p.x;
  F32 off_y = p.y;
  F32 advance = 0;
  B32 trailer_found = 0;
  Vec4F32 last_color = {0};
  for(D_FancyRunNode *n = list->first; n != 0; n = n->next)
  {
    F_Piece *piece_first = n->v.run.pieces.v;
    F_Piece *piece_opl = n->v.run.pieces.v + n->v.run.pieces.count;

    for(F_Piece *piece = piece_first; piece < piece_opl; piece++)
    {
      if(trailer_enabled && (off_x+advance+piece->advance) > (max_x-trailer_run.dim.x))
      {
        trailer_found = 1; 
        break;
      }

      if(!trailer_enabled && (off_x+advance+piece->advance) > max_x)
      {
        goto end_draw;
      }

      Rng2F32 dst = r2f32p(piece->rect.x0+off_x, piece->rect.y0+off_y, piece->rect.x1+off_x, piece->rect.y1+off_y);
      Rng2F32 src = r2f32p(piece->subrect.x0/piece->texture_dim.x,
          piece->subrect.y0/piece->texture_dim.y,
          piece->subrect.x1/piece->texture_dim.x,
          piece->subrect.y1/piece->texture_dim.y);
      // normalize tex coordinates

      // TODO(k): src wil be all zeros in gcc release build but not with clang
      AssertAlways((src.x0 + src.x1 + src.y0 + src.y1) != 0);
      Vec2F32 size = dim_2f32(dst);
      AssertAlways(!r_handle_match(piece->texture, r_handle_zero()));
      last_color = n->v.color;

      // NOTE(k): Space will have 0 extent
      if(size.x > 0 && size.y > 0)
      {
        // if(0)
        // {
        //     d_rect(dst, n->v.color, 1.0, 1.0, 1.0);
        // }
        // d_img(dst, src, piece->texture, n->v.color, 0,0,0);
        RK_DrawNode *d_node = rk_drawlist_push_rect(arena, drawlist, dst, src);
        d_node->albedo_tex = piece->texture;
        d_node->topology = R_GeoTopologyKind_Triangles;
        d_node->polygon = R_GeoPolygonKind_Fill;
        DLLPushBack_NP(ret, last_to_draw, d_node, draw_next, draw_prev);
      }

      advance += piece->advance;
    }

    // TODO(k): underline
    // TODO(k): strikethrough

    if(trailer_found)
    {
      break;
    }
  }
  end_draw:;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_plane(Arena *arena, RK_DrawList *drawlist, Vec2F32 size, B32 both_face)
{
  RK_DrawNode *ret = 0;

  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_plane(arena, size, 0,0, both_face, &vertices, &vertex_count, &indices, &indice_count);

  ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_box(Arena *arena, RK_DrawList *drawlist, Vec3F32 size)
{
  RK_DrawNode *ret = 0;

  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_box(arena, size, 0,0,0, &vertices, &vertex_count, &indices, &indice_count);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_sphere(Arena *arena, RK_DrawList *drawlist, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
{
  RK_DrawNode *ret = 0;

  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_sphere(arena, radius, height, radial_segments, rings, is_hemisphere, &vertices, &vertex_count, &indices, &indice_count);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_cone(Arena *arena, RK_DrawList *drawlist, F32 radius, F32 height, U64 radial_segments)
{
  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_cone(arena, radius, height, radial_segments, 1, &vertices, &vertex_count, &indices, &indice_count);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_line(Arena *arena, RK_DrawList *drawlist, Vec3F32 start, Vec3F32 end)
{
  R_Vertex *vertices = push_array(arena, R_Vertex, 2);
  vertices[0].pos = start;
  vertices[1].pos = end;
  U32 *indices = push_array(arena, U32, 2);
  indices[0] = 0;
  indices[1] = 1;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, 2, indices, 2);
  ret->topology = R_GeoTopologyKind_Lines;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_circle(Arena *arena, RK_DrawList *drawlist, F32 radius, U64 segments)
{
  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_circle(arena, radius, segments, &vertices, &vertex_count, &indices, &indice_count);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_circle_lined(Arena *arena, RK_DrawList *drawlist, F32 radius, U64 segments)
{
  R_Vertex *vertices;
  U64 vertex_count;
  U32 *indices;
  U64 indice_count;
  rk_mesh_primitive_circle_lined(arena, radius, segments, &vertices, &vertex_count, &indices, &indice_count);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Lines;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}

internal RK_DrawNode *
rk_drawlist_push_arc(Arena *arena, RK_DrawList *drawlist, Vec3F32 origin, Vec3F32 a, Vec3F32 b, U64 segments, B32 both_face)
{
  U64 vertex_count = segments+2;
  U64 indice_count = segments*3*(2*both_face);
  R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
  U32 *indices = push_array(arena, U32, indice_count);

  ///////////////////////////////////////////////////////////////////////////////////////
  // create mesh primitives using slerp

  // NOTE(k): assuming a and b has the same length
  Vec3F32 a_norm = sub_3f32(a, origin);
  F32 length = length_3f32(a_norm);
  a_norm = normalize_3f32(a_norm);
  Vec3F32 b_norm = normalize_3f32(sub_3f32(b, origin));
  F32 pct = 1.f / (F32)segments;

  U64 vertex_idx = 0;
  U64 indice_idx = 0;

  U64 prev_vertex_indice;

  // push origin
  {
    R_Vertex vertex = {0};
    // TODO(XXX): deal with normal later
    vertex.nor = v3f32(0,0,0);
    vertex.tan = v3f32(1,0,0);
    vertex.pos = origin;
    vertex.tex = v2f32(0,0);
    vertices[vertex_idx++] = vertex;
  }

  for(U64 i = 0; i < (segments+1); i++)
  {
    F32 t = i*pct;
    Vec3F32 pos = slerp_3f32(a_norm,b_norm,t);
    pos = scale_3f32(pos, length);
    pos = add_3f32(origin, pos);

    R_Vertex vertex;
    // TODO(XXX): deal with normal later
    vertex.nor = v3f32(0,0,0);
    vertex.tan = origin;
    vertex.pos = pos;
    vertex.tex = v2f32(0,0);

    vertices[vertex_idx++] = vertex;
    U64 curr_vertex_indice = vertex_idx-1;

    if(i > 0)
    {
      indices[indice_idx++] = 0; // origin
      indices[indice_idx++] = prev_vertex_indice;
      indices[indice_idx++] = curr_vertex_indice;

      if(both_face)
      {
        indices[indice_idx++] = 0; // origin
        indices[indice_idx++] = curr_vertex_indice;
        indices[indice_idx++] = prev_vertex_indice;
      }
    }

    prev_vertex_indice = curr_vertex_indice;
  }
  Assert(vertex_count == vertex_idx);
  Assert(indice_count == indice_idx);
  U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
  U64 indice_buffer_size = sizeof(U32) * indice_count;

  RK_DrawNode *ret = rk_drawlist_push(arena, drawlist, vertices, vertex_count, indices, indice_count);
  ret->topology = R_GeoTopologyKind_Triangles;
  ret->polygon = R_GeoPolygonKind_Fill;
  return ret;
}
