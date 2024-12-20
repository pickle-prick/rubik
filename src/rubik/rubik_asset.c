/////////////////////////////////
// Scene serialization/deserialization

internal RK_Scene *
rk_scene_from_se_node(SE_Node* root)
{
    RK_Scene *ret = rk_scene_alloc();
    Arena *arena = ret->arena;

    SE_Node *nodes = 0;

    // Parse scene basic info
    // TODO: refactor this
    for(SE_Node *n = root->first; n != 0; n = n->next)
    {
        if(str8_match(n->tag, str8_lit("name"), 0))     
        {
            ret->name = push_str8_copy(arena, n->v.se_string);
        }

        if(str8_match(n->tag, str8_lit("viewport_shading"), 0))     
        {
            ret->viewport_shading = n->v.se_uint;
        }

        if(str8_match(n->tag, str8_lit("polygon_mode"), 0))     
        {
            ret->polygon_mode = n->v.se_uint;
        }

        if(str8_match(n->tag, str8_lit("global_light"), 0))
        {
            ret->global_light.x = se_f64_from_struct(n, str8_lit("x"));
            ret->global_light.y = se_f64_from_struct(n, str8_lit("y"));
            ret->global_light.z = se_f64_from_struct(n, str8_lit("z"));
        }

        if(str8_match(n->tag, str8_lit("nodes"), 0))     
        {
            nodes = n;
        }
    }

    if(nodes != 0)
    {
        RK_Bucket_Scope(ret->bucket) RK_MeshCacheTable_Scope(&ret->mesh_cache_table)
        {
            U64 total_push_count = 0;
            U64 total_pop_count = 0;
            for(SE_Node *n = nodes->first; n != 0; n = n->next)
            {
                String8 name       = se_string_from_struct(n, str8_lit("name"));
                RK_NodeKind kind   = se_u64_from_struct(n, str8_lit("kind"));
                RK_NodeFlags flags = se_u64_from_struct(n, str8_lit("flags"));
                F64 animation_dt   = se_f64_from_struct(n, str8_lit("animation_dt"));

                // Position
                SE_Node *pos_senode = se_node_from_tag(n->first, str8_lit("pos"));
                Vec3F32 pos = {0};
                {
                    pos.x = se_f64_from_struct(pos_senode, str8_lit("x"));
                    pos.y = se_f64_from_struct(pos_senode, str8_lit("y"));
                    pos.z = se_f64_from_struct(pos_senode, str8_lit("z"));
                }

                // Scale
                SE_Node *scale_senode = se_node_from_tag(n->first, str8_lit("scale"));
                Vec3F32 scale = {0};
                {
                    scale.x = se_f64_from_struct(scale_senode, str8_lit("x"));
                    scale.y = se_f64_from_struct(scale_senode, str8_lit("y"));
                    scale.z = se_f64_from_struct(scale_senode, str8_lit("z"));
                }

                // Rotation
                SE_Node *rot_senode = se_node_from_tag(n->first, str8_lit("rot"));
                Vec4F32 rot = {0};
                {
                    rot.x = se_f64_from_struct(rot_senode, str8_lit("x"));
                    rot.y = se_f64_from_struct(rot_senode, str8_lit("y"));
                    rot.z = se_f64_from_struct(rot_senode, str8_lit("z"));
                    rot.w = se_f64_from_struct(rot_senode, str8_lit("w"));
                }

                U64 push_count = se_u64_from_struct(n, str8_lit("push_count"));
                U64 pop_count = se_u64_from_struct(n, str8_lit("pop_count"));
                U64 children_count = se_u64_from_struct(n, str8_lit("children_count"));
                for(U64 i = 0; i < pop_count; i++)
                {
                    rk_pop_parent();
                    total_pop_count++;
                }

                // Allocate/fill info
                RK_Node *rk_node = rk_build_node_from_string(0, name);
                rk_node->name    = name;
                rk_node->kind    = kind;
                rk_node->flags   = flags;
                rk_node->pos     = pos;
                rk_node->scale   = scale;
                rk_node->rot     = rot;
                rk_node->anim_dt = animation_dt;

                switch(kind)
                {
                    case RK_NodeKind_Camera:
                    {
                        SE_Node *senode = se_node_from_tag(n->first, str8_lit("camera"));
                        rk_node->v.camera.hide_cursor = se_b32_from_struct(senode, str8_lit("hide_cursor"));
                        rk_node->v.camera.lock_cursor = se_b32_from_struct(senode, str8_lit("lock_cursor"));
                        rk_node->v.camera.show_grid = se_b32_from_struct(senode, str8_lit("show_grid"));
                        rk_node->v.camera.show_gizmos = se_b32_from_struct(senode, str8_lit("show_gizmos"));
                        RK_ProjectionKind projection = se_u64_from_struct(senode, str8_lit("projection"));
                        rk_node->v.camera.projection = projection;

                        switch(projection)
                        {
                            case RK_ProjectionKind_Perspective:
                            {
                                rk_node->v.camera.p.fov = se_f64_from_struct(senode, str8_lit("fov"));
                                rk_node->v.camera.p.zn = se_f64_from_struct(senode, str8_lit("zn"));
                                rk_node->v.camera.p.zf = se_f64_from_struct(senode, str8_lit("zf"));
                            }break;
                            case RK_ProjectionKind_Orthographic:
                            {
                                rk_node->v.camera.o.left = se_f64_from_struct(senode, str8_lit("left"));
                                rk_node->v.camera.o.right = se_f64_from_struct(senode, str8_lit("right"));
                                rk_node->v.camera.o.bottom = se_f64_from_struct(senode, str8_lit("bottom"));
                                rk_node->v.camera.o.top = se_f64_from_struct(senode, str8_lit("top"));
                                rk_node->v.camera.o.zn = se_f64_from_struct(senode, str8_lit("zn"));
                                rk_node->v.camera.o.zf = se_f64_from_struct(senode, str8_lit("zf"));
                            }break;
                            default:{InvalidPath;}break;
                        }
                        B32 is_active = se_b32_from_struct(senode, str8_lit("is_active"));
                        RK_CameraNode *camera_node = rk_scene_camera_push(ret, rk_node);
                        if(is_active)
                        {
                            ret->active_camera = camera_node;
                        }
                    }break;
                    case RK_NodeKind_MeshRoot:
                    {
                        SE_Node *mesh_node = se_node_from_tag(n->first, str8_lit("mesh"));
                        U64 mesh_kind = se_u64_from_struct(mesh_node, str8_lit("kind"));
                        String8 model_path = se_string_from_struct(mesh_node, str8_lit("path"));
                        rk_node->v.mesh_root.kind = mesh_kind;
                        rk_node->v.mesh_root.path = push_str8_copy(arena, model_path);

                        rk_push_parent(rk_node);
                        switch(mesh_kind)
                        {
                            case RK_MeshKind_Box:
                            {
                                rk_box_node_default(name);
                            }break;
                            case RK_MeshKind_Plane:
                            {
                                rk_plane_node_default(name);
                            }break;
                            case RK_MeshKind_Sphere:
                            {
                                rk_sphere_node_default(name);
                            }break;
                            case RK_MeshKind_Capsule:
                            {
                                rk_capsule_node_default(name);
                            }break;
                            case RK_MeshKind_Cylinder:
                            {
                                rk_cylinder_node_default(name);
                            }break;
                            case RK_MeshKind_Model:
                            {
                                // TODO(k): cache models
                                String8 model_directory = str8_chop_last_slash(model_path);
                                RK_Model *model;
                                RK_Path_Scope(model_directory)
                                {
                                    model = rk_model_from_gltf_cached(model_path);
                                }
                                rk_node_from_model(model, rk_active_seed_key());
                                rk_node->skeleton_anims = model->anims; 
                            }break;
                            default:{InvalidPath;}break;
                        }
                        rk_pop_parent();
                    }break;
                    default: {}break;
                }

                if(ret->root == 0) ret->root = rk_node;

                // Load functions
                SE_Node *functions_node = se_node_from_tag(n->first, str8_lit("update_functions"));
                for(SE_Node *sn = functions_node->first; sn != 0; sn = sn->next)
                {
                    AssertAlways(sn->kind == SE_NodeKind_String);
                    String8 fn_name = sn->v.se_string;
                    RK_FunctionNode *fn = rk_function_from_string(fn_name);
                    rk_node_push_fn(arena, rk_node, fn->ptr, fn->alias);
                }

                if(children_count > 0)
                {
                    rk_push_parent(rk_node);
                    total_push_count++;
                }
            }

            // NOTE(k): pop remaining parents?
            U64 rest_pop_count = total_push_count-total_pop_count;
            for(U64 i = 0; i < rest_pop_count; i++)
            {
                rk_pop_parent();
            }
        }
    }
    return ret;
}

internal RK_Scene *
rk_scene_from_file(String8 path)
{
    Temp temp = scratch_begin(0,0);
    SE_Node *se_node = se_yml_node_from_file(temp.arena, path);
    RK_Scene *ret = rk_scene_from_se_node(se_node);
    ret->path = push_str8_copy(ret->arena, path);
    temp_end(temp);
    return ret;
}

internal void
rk_scene_to_file(RK_Scene *scene, String8 path)
{
    String8List strs = {0};
    Temp temp = scratch_begin(0,0);

    se_build_begin(temp.arena);

    SE_Node *root = se_struct(str8_zero());
    SE_Parent(root)
    {
        // Generate base cfg (name, viewport_shading, global_light, polygon_mode)
        {
            se_string(str8_lit("name"), scene->name);
            se_int(str8_lit("viewport_shading"), scene->viewport_shading);
            se_int(str8_lit("polygon_mode"), scene->polygon_mode);
            SE_Struct(str8_lit("global_light"))
            {
                se_float(str8_lit("x"), scene->global_light.x);
                se_float(str8_lit("y"), scene->global_light.y);
                se_float(str8_lit("z"), scene->global_light.z);
            }
        }

        // Generate nodes
        SE_Node *nodes = se_array(str8_lit("nodes"));
        SE_Parent(nodes)
        {
            RK_Key parent_key = {0};
            RK_Node *node = scene->root;

            U64 level = 0;
            S64 pass_level = -1;
            U64 push_count = 0;
            U64 pop_count = 0;
            while(node != 0)
            {
                RK_NodeRec rec = rk_node_df_pre(node, 0);

                // TODO(k): this is not the prettiest thing in the word
                if(pass_level != -1 && level <= pass_level)
                {
                    push_count = 0;
                    pop_count = pass_level - level;
                    pass_level = -1;
                }
                if(pass_level == -1)
                {
                    if(node->parent)
                    {
                        parent_key = node->parent->key;
                    }

                    SE_Struct(str8_zero())
                    {
                        se_string(str8_lit("name"), node->name);
                        se_uint(str8_lit("key"), node->key.u64[0]);
                        se_uint(str8_lit("parent_key"), parent_key.u64[0]);
                        se_uint(str8_lit("kind"), node->kind);
                        se_uint(str8_lit("flags"), node->flags);
                        se_uint(str8_lit("push_count"), push_count);
                        se_uint(str8_lit("pop_count"), pop_count);
                        U64 children_count = node->children_count;
                        if(node->kind == RK_NodeKind_MeshRoot) children_count = 0;
                        se_uint(str8_lit("children_count"), children_count);
                        se_float(str8_lit("animation_dt"), node->anim_dt);

                        SE_Struct(str8_lit("pos"))
                        {
                            se_float(str8_lit("x"), node->pos.x);
                            se_float(str8_lit("y"), node->pos.y);
                            se_float(str8_lit("z"), node->pos.z);
                        }
                        SE_Struct(str8_lit("rot"))
                        {
                            se_float(str8_lit("x"), node->rot.x);
                            se_float(str8_lit("y"), node->rot.y);
                            se_float(str8_lit("z"), node->rot.z);
                            se_float(str8_lit("w"), node->rot.w);
                        }
                        SE_Struct(str8_lit("scale"))
                        {
                            se_float(str8_lit("x"), node->scale.x);
                            se_float(str8_lit("y"), node->scale.y);
                            se_float(str8_lit("z"), node->scale.z);
                        }

                        SE_Array(str8_lit("update_functions"))
                        {
                            for(RK_UpdateFnNode *n = node->first_update_fn; n != 0; n = n->next)
                            {
                                se_string(str8_zero(), n->name);
                            }
                        }

                        if(node->kind == RK_NodeKind_MeshRoot)
                        {
                            SE_Struct(str8_lit("mesh"))
                            {
                                se_uint(str8_lit("kind"), node->v.mesh_root.kind);
                                se_string(str8_lit("path"), node->v.mesh_root.path);
                            }
                            // NOTE: we want to ignore its children
                            pass_level = level;
                        }

                        if(node->kind == RK_NodeKind_Camera)
                        {
                            SE_Struct(str8_lit("camera"))
                            {
                                se_uint(str8_lit("projection"), node->v.camera.projection);
                                se_boolean(str8_lit("hide_cursor"), node->v.camera.hide_cursor);
                                se_boolean(str8_lit("lock_cursor"), node->v.camera.lock_cursor);
                                se_boolean(str8_lit("show_gizmos"), node->v.camera.show_gizmos);
                                se_boolean(str8_lit("show_grid"), node->v.camera.show_grid);
                                se_boolean(str8_lit("is_active"), node == scene->active_camera->v);

                                switch(node->v.camera.projection)
                                {
                                    case RK_ProjectionKind_Perspective:
                                    {
                                        se_float(str8_lit("fov"), node->v.camera.p.fov);
                                        se_float(str8_lit("zn"), node->v.camera.p.zn);
                                        se_float(str8_lit("zf"), node->v.camera.p.zf);
                                    }break;
                                    case RK_ProjectionKind_Orthographic:
                                    {
                                        se_float(str8_lit("left"), node->v.camera.o.left);
                                        se_float(str8_lit("right"), node->v.camera.o.right);
                                        se_float(str8_lit("bottom"), node->v.camera.o.bottom);
                                        se_float(str8_lit("top"), node->v.camera.o.top);
                                        se_float(str8_lit("zn"), node->v.camera.o.zn);
                                        se_float(str8_lit("zf"), node->v.camera.o.zf);
                                    }break;
                                    default:{InvalidPath;}break;
                                }
                            }
                        }
                    }
                }

                node = rec.next;
                push_count = rec.push_count;
                pop_count = rec.pop_count;
                level += (push_count - pop_count);
            }
        }
    }

    se_yml_node_to_file(root, path);
    se_build_end();
    scratch_end(temp);
}

/////////////////////////////////
// GLTF2.0

internal RK_Model *
rk_model_from_gltf_cached(String8 gltf_path)
{
    RK_Model *ret = 0;

    // Try fetch model from cache first
    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key cache_key = rk_key_from_string(rk_key_zero(), gltf_path);
    U64 slot_idx = 0;

    slot_idx = cache_key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, cache_key))
        {
            AssertAlways(n->kind == RK_MeshKind_Model);
            ret = n->v;
            n->rc++;
        }
    }

    if(ret == 0)
    {
        ret = rk_model_from_gltf(cache_table->arena, gltf_path);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->key = cache_key;
        cache_node->v = ret;
        cache_node->rc = 1;
        cache_node->kind = RK_MeshKind_Model;
        DLLPushBack(slot->first, slot->last, cache_node);
    }
    return ret;
}

internal RK_Model *
rk_model_from_gltf(Arena *arena, String8 gltf_path)
{
    RK_Model *model = 0;

    cgltf_options opts = {0};
    cgltf_data *data;
    cgltf_result ret = cgltf_parse_file(&opts, (char *)gltf_path.str, &data);

    if(ret == cgltf_result_success)
    {
        //- Load buffers
        cgltf_options buf_ld_opts = {0};
        cgltf_result buf_ld_ret = cgltf_load_buffers(&buf_ld_opts, data, (char *)gltf_path.str);
        AssertAlways(buf_ld_ret == cgltf_result_success);
        AssertAlways(data->scenes_count == 1);

        //- Select the first scene (TODO: only load default scene for now)
        cgltf_scene *root_scene = &data->scenes[0];
        AssertAlways(root_scene->nodes_count == 1);
        cgltf_node *root= root_scene->nodes[0];
        model = rk_mnode_from_gltf_node(arena, root, 0, 0, 1024);

        //- Load skeleton animations after model is loaded
        U64 anim_count = data->animations_count;
        RK_MeshSkeletonAnimation **anims = push_array(arena, RK_MeshSkeletonAnimation*, anim_count);
        for(U64 i = 0; i < anim_count; i++)
        {
            anims[i] = rk_skeleton_anim_from_gltf_animation(&data->animations[i]);
        }
        model->anim_count = anim_count;
        model->anims = anims;
        model->path = push_str8_copy(arena, gltf_path);

        cgltf_free(data);
    }
    else
    {
        InvalidPath;
    }

    return model;
}

internal RK_Key
rk_key_from_gltf_node(cgltf_node *cn)
{
    return rk_key_from_string(rk_key_zero(), str8((U8*)(&cn), 8));
}

internal RK_ModelNode *
rk_mnode_from_hash_table(RK_Key key, RK_ModelNode_HashSlot *hash_table, U64 hash_table_size)
{
    RK_ModelNode *ret = 0;
    U64 slot_idx = key.u64[0] % hash_table_size;
    for(RK_ModelNode *n = hash_table[slot_idx].first; n != 0; n = n->hash_next)
    {
        if(rk_key_match(n->key, key))
        {
            ret = n;
            break;
        }
    }
    return ret;
}

internal RK_ModelNode *
rk_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, RK_ModelNode *parent, RK_ModelNode_HashSlot *hash_table, U64 hash_table_size)
{
    RK_ModelNode *mnode = 0;
    B32 is_mesh = cn->mesh != 0;
    B32 is_skinned = is_mesh ? (cn->skin != 0) : 0;
    RK_Key key = rk_key_from_gltf_node(cn);

    // If it's the root node, create hash table
    if(hash_table == 0)
    {
        hash_table = push_array(arena, RK_ModelNode_HashSlot, hash_table_size);
    }

    mnode = rk_mnode_from_hash_table(key, hash_table, hash_table_size);

    // NOTE(k): If mnode is already created, it should be a joint node within the scene tree
    if(mnode != 0) { return mnode; }

    if(mnode == 0) { mnode = push_array(arena, RK_ModelNode, 1); }

    mnode->name            = push_str8_copy(arena, str8_cstring(cn->name));
    mnode->key             = key;
    mnode->is_mesh_group   = is_mesh;
    mnode->is_skinned      = is_skinned;
    // TODO: we don't actually need to store the hash_table in the RK_ModelNode
    mnode->hash_table      = hash_table;
    mnode->hash_table_size = hash_table_size;

    // Xform
    Mat4x4F32 xform = mat_4x4f32(1.0f);
    {
        if(cn->has_scale)       { xform = mul_4x4f32(make_scale_4x4f32(v3f32(cn->scale[0], cn->scale[1], cn->scale[2])), xform); }
        if(cn->has_rotation)    { xform = mul_4x4f32(mat_4x4f32_from_quat_f32(v4f32(cn->rotation[0], cn->rotation[1], cn->rotation[2], cn->rotation[3])), xform); }
        if(cn->has_translation) { xform = mul_4x4f32(make_translate_4x4f32(v3f32(cn->translation[0], cn->translation[1], cn->translation[2])), xform); }
        // TODO: not sure what's the right order here
        if(cn->has_matrix)      { xform = mul_4x4f32(*(Mat4x4F32*)cn->matrix, xform); }
    }
    mnode->xform = xform;

    // Insert to the node tree
    if(parent)
    {
        mnode->parent = parent;
        DLLPushBack(parent->first, parent->last, mnode);
        parent->children_count++;
    }

    if(is_mesh)
    {
        // Mesh is skinned, unpack joints info
        if(is_skinned)
        {
            mnode->joint_count = cn->skin->joints_count;
            mnode->joints      = push_array(arena, RK_ModelNode*, cn->skin->joints_count);

            Mat4x4F32 *inverse_bind_matrices;
            {
                cgltf_accessor *accessor = cn->skin->inverse_bind_matrices;
                U64 offset = accessor->offset + accessor->buffer_view->offset;
                AssertAlways(accessor->type == cgltf_type_mat4);
                AssertAlways(accessor->stride == sizeof(Mat4x4F32));
                inverse_bind_matrices = (Mat4x4F32 *)(((U8 *)accessor->buffer_view->buffer->data)+offset);
            }

            // TODO: joint node may or may not be within the gltf scene tree, what fuck???
            for(U64 i = 0; i < cn->skin->joints_count; i++)
            {
                // TODO: don't think it's right, what if joint have parent
                RK_ModelNode *joint = rk_mnode_from_gltf_node(arena, cn->skin->joints[i], 0, hash_table, hash_table_size);
                AssertAlways(joint != 0);

                joint->is_joint = 1;
                joint->is_skin = cn->skin->joints[i] == cn->skin->skeleton;
                joint->inverse_bind_matrix = inverse_bind_matrices[i];
                mnode->joints[i] = joint;
            }
        }

        // Create node for every mesh primitive
        for(U64 i = 0; i < cn->mesh->primitives_count; i++)
        {
            String8 name = push_str8f(arena, "%S#mesh#%d", mnode->name, i);
            RK_Key key = rk_key_from_string(mnode->key, name);

            RK_ModelNode *mesh_node = push_array(arena, RK_ModelNode, 1);
            mesh_node->key               = key;
            mesh_node->name              = name;
            mesh_node->xform             = mat_4x4f32(1.0f);
            mesh_node->is_mesh_primitive = 1;

            mesh_node->parent      = mnode;
            DLLPushBack(mnode->first, mnode->last, mesh_node);
            mnode->children_count++;

            cgltf_primitive *primitive = &cn->mesh->primitives[i];
            AssertAlways(primitive->type == cgltf_primitive_type_triangles);

            // Texture
            // TODO(k): implement full PBR rendering
            {
                Temp temp = scratch_begin(&arena,1);
                cgltf_texture *tex = primitive->material->pbr_metallic_roughness.base_color_texture.texture;

                String8List path_parts = {0};
                str8_list_push(temp.arena, &path_parts, rk_top_path());
                str8_list_push(temp.arena, &path_parts, str8_cstring(tex->image->uri));
                String8 path = str8_path_list_join_by_style(temp.arena, &path_parts, PathStyle_Relative);

                int x,y,n;
                U8 *image_data = stbi_load((char*)path.str, &x, &y, &n, 4);
                mesh_node->albedo_tex = r_tex2d_alloc(R_ResourceKind_Static, R_Tex2DSampleKind_Nearest, v2s32(x,y), R_Tex2DFormat_RGBA8, image_data);
                stbi_image_free(image_data);
                scratch_end(temp);
            }

            // Mesh Indices
            {
                cgltf_accessor *accessor = primitive->indices;
                AssertAlways(accessor->type == cgltf_type_scalar);
                AssertAlways(accessor->stride == 4);
                U64 offset = accessor->offset+accessor->buffer_view->offset;
                void *indices_src = (U8 *)accessor->buffer_view->buffer->data+offset;
                U64 size = sizeof(U32) * accessor->count;
                R_Handle indices = r_buffer_alloc(R_ResourceKind_Static, size, indices_src);
                mesh_node->indices = indices;
            }

            // Mesh vertex attributes
            {
                cgltf_attribute *attrs = primitive->attributes;
                cgltf_size attr_count = primitive->attributes_count;
                U64 vertex_count = attrs[0].data->count;
                R_Vertex vertices_src[vertex_count];

                for(U64 i = 0; i < vertex_count; i++)
                {
                    R_Vertex *v = &vertices_src[i];
                    for(U64 j = 0; j < attr_count; j++)
                    {
                        cgltf_attribute *attr = &attrs[j];
                        cgltf_accessor *accessor = attr->data;
                        // cgltf_accessor *accessor = &attr->data[attr->index];

                        U64 src_offset = accessor->offset + accessor->buffer_view->offset + accessor->stride*i;
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
                R_Handle vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(vertices_src), (void *)vertices_src);
                mesh_node->vertices = vertices;
            }
        }
    }

    // Insert node to hash table
    U64 slot_idx = mnode->key.u64[0] % hash_table_size;
    DLLPushBack_NP(hash_table[slot_idx].first, hash_table[slot_idx].last, mnode, hash_next, hash_prev);

    for(U64 i = 0; i < cn->children_count; i++)
    {
        rk_mnode_from_gltf_node(arena, cn->children[i], mnode, mnode->hash_table, mnode->hash_table_size);
    }
    return mnode;
}

internal RK_Node *
rk_node_from_model(RK_Model *model, RK_Key seed_key)
{
    RK_Bucket *bucket = rk_top_bucket();
    Arena *arena = bucket->arena;

    RK_Key key = rk_key_merge(seed_key, model->key);
    RK_Node *n = 0;

    U64 slot_idx = key.u64[0] % bucket->node_hash_table_size;
    for(RK_Node *i = bucket->node_hash_table[slot_idx].first; i != 0; i = i->hash_next)
    {
        if(rk_key_match(key, i->key))
        {
            n = i;
            break;
        }
    }
    if(n != 0) { return n; }

    n = rk_build_node_from_key(0, key);

    n->name = push_str8_copy(arena, model->name);
    model->rc++; // TODO: make use of rc

    // translation, rotation, scale
    rk_trs_from_matrix(&model->xform, &n->pos, &n->rot, &n->scale);

    B32 is_mesh_group     = model->is_mesh_group;
    B32 is_mesh_primitive = model->is_mesh_primitive;
    B32 is_joint          = model->is_joint;
    B32 is_skin           = model->is_skin; // root joint

    if(is_mesh_group)
    {
        n->kind                    = RK_NodeKind_MeshGroup;
        n->v.mesh_grp.is_skinned   = model->is_skinned;
        n->v.mesh_grp.joint_count  = model->joint_count;
        n->v.mesh_grp.joints       = push_array(arena, RK_Node*, model->joint_count);
        n->v.mesh_grp.joint_xforms = push_array(arena, Mat4x4F32, model->joint_count);
        RK_FunctionNode *fn = rk_function_from_string(str8_lit("mesh_grp_fn"));
        rk_node_push_fn(arena, n, fn->ptr, fn->alias);

        for(U64 i = 0; i < model->joint_count; i++)
        {
            n->v.mesh_grp.joints[i] = rk_node_from_model(model->joints[i], seed_key);
        }
        n->v.mesh_grp.root_joint = n->v.mesh_grp.joints[0];
    }

    if(is_mesh_primitive)
    {
        n->kind                        = RK_NodeKind_MeshPrimitive;
        n->v.mesh_primitive.vertices   = model->vertices;
        n->v.mesh_primitive.indices    = model->indices;
        n->v.mesh_primitive.albedo_tex = model->albedo_tex;
    }

    if(is_joint)
    {
        n->kind = RK_NodeKind_MeshJoint;
        n->v.mesh_joint.inverse_bind_matrix = model->inverse_bind_matrix;
    }

    if(is_skin)
    { 
        n->flags |= RK_NodeFlags_Float;
    }

    RK_Parent_Scope(n)
    {
        for(RK_ModelNode *c = model->first; c != 0; c = c->next)
        {
            rk_node_from_model(c, seed_key);
        }
    }
    return n;
}

internal RK_MeshSkeletonAnimation *
rk_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim)
{
    RK_Bucket *bucket = rk_top_bucket();
    Arena *arena = bucket->arena;

    RK_MeshSkeletonAnimation *anim = push_array(arena, RK_MeshSkeletonAnimation, 1);
    anim->name = push_str8_copy(arena, str8_cstring(cgltf_anim->name));

    AssertAlways(cgltf_anim->channels_count == cgltf_anim->samplers_count);
    U64 spline_count = cgltf_anim->channels_count;
    RK_MeshSkeletonAnimSpline *splines = push_array(arena, RK_MeshSkeletonAnimSpline, spline_count);

    F32 duration = 0;
    for(U64 i = 0; i < spline_count; i++)
    {
        RK_MeshSkeletonAnimSpline *spline = &splines[i];
        cgltf_animation_channel *channel = &cgltf_anim->channels[i] ;
        cgltf_animation_sampler *sampler = channel->sampler;

        RK_TransformKind t_kind;
        U64 value_size;
        switch(channel->target_path)
        {
            case cgltf_animation_path_type_translation: { t_kind = RK_TransformKind_Translation; value_size = sizeof(Vec3F32); }break;
            case cgltf_animation_path_type_rotation:    { t_kind = RK_TransformKind_Rotation; value_size = sizeof(Vec4F32); }break;
            case cgltf_animation_path_type_scale:       { t_kind = RK_TransformKind_Scale; value_size = sizeof(Vec3F32); }break;
            case cgltf_animation_path_type_weights:     { t_kind = RK_TransformKind_Invalid; value_size = sizeof(Vec2F32); }break; // TODO: ignored for now
            default:                                    { InvalidPath; }break;
        }

        RK_InterpolationMethod method;
        switch(sampler->interpolation)
        {
            case cgltf_interpolation_type_linear:       { method = RK_InterpolationMethod_Linear; }break;
            case cgltf_interpolation_type_step:         { method = RK_InterpolationMethod_Step; }break;
            case cgltf_interpolation_type_cubic_spline: { method = RK_InterpolationMethod_Cubicspline; }break;
            default:                                    { InvalidPath; }break;
        }

        RK_Key target_key = rk_key_from_gltf_node(channel->target_node);

        // Timestamps
        cgltf_accessor *input_accessor = sampler->input;
        AssertAlways(input_accessor->type == cgltf_type_scalar);
        U8 *input_src = ((U8*)input_accessor->buffer_view->buffer->data) + input_accessor->offset + input_accessor->buffer_view->offset;
        F32 *timestamps = push_array(arena, F32, input_accessor->count);
        MemoryCopy(timestamps, input_src, sizeof(F32) * input_accessor->count);

        // Values
        cgltf_accessor *output_accessor = sampler->output;
        U8 *output_src = ((U8*)output_accessor->buffer_view->buffer->data) + output_accessor->offset + output_accessor->buffer_view->offset;
        U8 *values = push_array(arena, U8, value_size * output_accessor->count);
        MemoryCopy(values, output_src, value_size * output_accessor->count);

        F32 last_frame_dt = timestamps[input_accessor->count-1];
        if(last_frame_dt > duration) { duration = last_frame_dt; }

        // Fill
        spline->transform_kind       = t_kind;
        spline->timestamps           = timestamps;
        spline->values.v             = values;
        spline->target_key           = target_key;
        spline->frame_count          = input_accessor->count;
        spline->interpolation_method = method;
    }

    anim->duration     = duration;
    anim->splines      = splines;
    anim->spline_count = spline_count;
    return anim;
}

/////////////////////////////////
// Mesh primitives

internal void
rk_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
{
    // R_Vertex vertices_src[8] = {
    //     // Front face
    //     { {-0.5f, -0.5f,  0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { { 0.5f, -0.5f,  0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { { 0.5f,  0.5f,  0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { {-0.5f,  0.5f,  0.5f}, {0}, {0}, {0}, {0}, {0} },

    //     // Back face
    //     { {-0.5f, -0.5f, -0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { { 0.5f, -0.5f, -0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { { 0.5f,  0.5f, -0.5f}, {0}, {0}, {0}, {0}, {0} },
    //     { {-0.5f,  0.5f, -0.5f}, {0}, {0}, {0}, {0}, {0} },
    // };
    // *vertices_count_out = ArrayCount(vertices_src);
    // *vertices_out = push_array(arena, R_Vertex, *vertices_count_out);
    // MemoryCopy(*vertices_out, vertices_src, sizeof(vertices_src));

    // U32 indices_src[3*12] = {
    //     // Front face
    //     0, 1, 2,
    //     2, 3, 0,
    //     // Right face
    //     1, 5, 6,
    //     6, 2, 1,
    //     // Back face
    //     5, 4, 7,
    //     7, 6, 5,
    //     // Left face
    //     4, 0, 3,
    //     3, 7, 4,
    //     // Top face
    //     3, 2, 6,
    //     6, 7, 3,
    //     // Bottom face
    //     4, 5, 1,
    //     1, 0, 4
    // };
    // *indices_count_out = ArrayCount(indices_src);
    // *indices_out = push_array(arena, U32, *indices_count_out);
    // MemoryCopy(*indices_out, indices_src, sizeof(indices_src));

    U64 i,j,prevrow,thisrow,vertex_idx,indice_idx;
    F32 x,y,z;

    Vec3F32 start_pos = scale_3f32(size, -0.5);

    U64 vertex_count = (subdivide_h+2) * (subdivide_w+2)*2 + (subdivide_h+2) * (subdivide_d+2)*2 + (subdivide_d+2) * (subdivide_w+2)*2; 
    U64 indice_count = (subdivide_h+1) * (subdivide_w+1)*12 + (subdivide_h+1) * (subdivide_d+1)*12 + (subdivide_d+1) * (subdivide_w+1)*12;

    R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
    U32 *indices       = push_array(arena, U32, indice_count);

    vertex_idx = 0;
    indice_idx = 0;

    Vec3F32 pos,nor;

    // front + back
    y = start_pos.y;
    thisrow = 0;
    prevrow = 0;
    for(j = 0; j < (subdivide_h+2); j++)
    {
        F32 v = j; 
        F32 v2 = v / (subdivide_w + 1.0);
        v /= (2.0 * (subdivide_h+1.0));

        x = start_pos.x;
        for(i = 0; i < (subdivide_w+2); i++)
        {
            F32 u = i;
            F32 u2 = u / (subdivide_w+1.0);
            u /= (3.0 * (subdivide_w+1.0));

            // front
            pos = (Vec3F32){x, -y, -start_pos.z};
            nor = (Vec3F32){0.0, 0.0, 1.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

            // back
            pos = (Vec3F32){-x, -y, start_pos.z};
            nor = (Vec3F32){0.0, 0.0, -1.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

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
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = prevrow + i2 + 1;

                indices[indice_idx++] = prevrow + i2 + 1;
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = thisrow + i2 + 1;
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

            // right
            pos = (Vec3F32){-start_pos.x, -y, -z};
            nor = (Vec3F32){1.0, 0.0, 0.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

            // left
            pos = (Vec3F32){start_pos.x, -y, z};
            nor = (Vec3F32){-1.0, 0.0, 0.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

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
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = prevrow + i2 + 1;

                indices[indice_idx++] = prevrow + i2 + 1;
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = thisrow + i2 + 1;
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
            pos = (Vec3F32){-x, -start_pos.y, -z};
            nor = (Vec3F32){0.0, 1.0, 0.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

            // bottom
            pos = (Vec3F32){x, start_pos.y, -z};
            nor = (Vec3F32){0.0, -1.0, 0.0};
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor};

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
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = prevrow + i2 + 1;

                indices[indice_idx++] = prevrow + i2 + 1;
                indices[indice_idx++] = thisrow + i2 - 1;
                indices[indice_idx++] = thisrow + i2 + 1;
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
rk_mesh_primitive_plane(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, Vec2F32 size, U64 subdivide_w, U64 subdivide_d)
{
    U64 i,j,prevrow,thisrow,vertex_idx, indice_idx;
    F32 x,z;

    Vec2F32 start_pos = scale_2f32(size, -0.5);
    // Face Y
    Vec3F32 normal = {0.0f, -1.0f, 0.0f};

    // TODO: fix it later
    U64 vertex_count = (subdivide_d+2) * (subdivide_w+2);
    U64 indice_count = (subdivide_d+1) * (subdivide_w+1) * 6;
    R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
    U32 *indices = push_array(arena, U32, indice_count);

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
            uv = (Vec2F32){1.0-u, 1.0-v}; /* 1.0-uv to match orientation with Quad */
            vertices[vertex_idx++] = (R_Vertex){.pos=pos, .nor=nor, .tex=uv};

            if(i > 0 && j > 0)
            {
                indices[indice_idx++] = prevrow + i - 1;
                indices[indice_idx++] = prevrow + i;
                indices[indice_idx++] = thisrow + i - 1;
                indices[indice_idx++] = prevrow + i;
                indices[indice_idx++] = thisrow + i;
                indices[indice_idx++] = thisrow + i - 1;
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
rk_mesh_primitive_sphere(Arena *arena,
                        R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out,
                        F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
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
rk_mesh_primitive_cylinder(Arena *arena,
                          R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out,
                          F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom)
{
    Assert(rings >= 2);
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
rk_mesh_primitive_capsule(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings)
{
    U64 i,j,prevrow,thisrow,vertex_idx,indice_idx;
    F32 x,y,z,u,v,w,vertex_count,indice_count;

    // TODO: fix it
    vertex_count = (rings+2)*(radial_segments+1)*3;
    indice_count = (rings+1)*radial_segments*6*3;

    // TODO: calculate uv2

    R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
    U32 *indices       = push_array(arena, U32, indice_count);

    vertex_idx = 0;
    indice_idx = 0;

    Vec3F32 pos,nor,col;
    col = v3f32(1.0,1.0,1.0);

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

/////////////////////////////////
// Node building helpers

internal RK_Node *
rk_box_node(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d)
{
    Temp temp = scratch_begin(0,0);

    R_Vertex *vertices_src = 0;
    U64 vertice_count     = 0;
    U32 *indices_src       = 0;
    U64 indice_count      = 0;
    rk_mesh_primitive_box(temp.arena, size, subdivide_w, subdivide_h, subdivide_d, &vertices_src, &vertice_count, &indices_src, &indice_count);

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
    n->v.mesh_primitive.indices       = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
    n->v.mesh_primitive.vertice_count = vertice_count;
    n->v.mesh_primitive.indice_count  = indice_count;

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_box_node_cached(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d)
{
    Temp temp = scratch_begin(0,0);
    RK_MeshPrimitive *primitive = 0;

    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key key = {0};
    {
        F64 size_x = size.x;
        F64 size_y = size.y;
        F64 size_z = size.z;
        U64 hash_content[] = {
            *(U64 *)(&size.x),
            *(U64 *)(&size.y),
            *(U64 *)(&size.z),
            subdivide_w,
            subdivide_h,
            subdivide_d,
        };
        key = rk_key_from_string(rk_key_zero(), str8((U8 *)hash_content, sizeof(hash_content)));
    }

    U64 slot_idx = key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            AssertAlways(n->kind == RK_MeshKind_Box);
            primitive = n->v;
            n->rc++;
        }
    }

    if(primitive == 0)
    {
        R_Vertex *vertices_src = 0;
        U64 vertice_count     = 0;
        U32 *indices_src       = 0;
        U64 indice_count      = 0;
        rk_mesh_primitive_box(temp.arena, size, subdivide_w, subdivide_h, subdivide_d, &vertices_src, &vertice_count, &indices_src, &indice_count);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->v = push_array(cache_table->arena, RK_MeshPrimitive, 1);
        cache_node->key = key;
        cache_node->rc  = 1;
        cache_node->kind = RK_MeshKind_Box;
        primitive = cache_node->v;
        DLLPushBack(slot->first, slot->last, cache_node);

        primitive->vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
        primitive->indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
        primitive->vertice_count = vertice_count;
        primitive->indice_count = indice_count;
    }

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = primitive->vertices;
    n->v.mesh_primitive.indices       = primitive->indices;
    n->v.mesh_primitive.vertice_count = primitive->vertice_count;
    n->v.mesh_primitive.indice_count  = primitive->indice_count;

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_plane_node(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d)
{
    Temp temp = scratch_begin(0,0);

    R_Vertex *vertices_src = 0;
    U64 vertices_count     = 0;
    U32 *indices_src       = 0;
    U64 indices_count      = 0;
    rk_mesh_primitive_plane(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, size, subdivide_w, subdivide_d);

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                      = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
    n->v.mesh_primitive.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_plane_node_cached(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d)
{
    Temp temp = scratch_begin(0,0);
    RK_MeshPrimitive *primitive = 0;

    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key key = {0};
    {
        F64 size_x = size.x;
        F64 size_y = size.y;
        U64 hash_content[] = {
            *(U64 *)(&size.x),
            *(U64 *)(&size.y),
            subdivide_w,
            subdivide_d,
        };
        key = rk_key_from_string(rk_key_zero(), str8((U8 *)hash_content, sizeof(hash_content)));
    }

    U64 slot_idx = key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            AssertAlways(n->kind == RK_MeshKind_Plane);
            primitive = n->v;
            n->rc++;
        }
    }

    if(primitive == 0)
    {
        R_Vertex *vertices_src = 0;
        U64 vertice_count     = 0;
        U32 *indices_src       = 0;
        U64 indice_count      = 0;
        rk_mesh_primitive_plane(temp.arena, &vertices_src, &vertice_count, &indices_src, &indice_count, size, subdivide_w, subdivide_d);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->v = push_array(cache_table->arena, RK_MeshPrimitive, 1);
        cache_node->key = key;
        cache_node->rc  = 1;
        cache_node->kind = RK_MeshKind_Plane;
        primitive = cache_node->v;
        DLLPushBack(slot->first, slot->last, cache_node);

        primitive->vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
        primitive->indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
        primitive->vertice_count = vertice_count;
        primitive->indice_count = indice_count;
    }

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = primitive->vertices;
    n->v.mesh_primitive.indices       = primitive->indices;
    n->v.mesh_primitive.vertice_count = primitive->vertice_count;
    n->v.mesh_primitive.indice_count  = primitive->indice_count;

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_sphere_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
{
    Temp temp = scratch_begin(0,0);

    R_Vertex *vertices_src = 0;
    U64 vertice_count     = 0;
    U32 *indices_src       = 0;
    U64 indice_count      = 0;
    rk_mesh_primitive_sphere(temp.arena, &vertices_src, &vertice_count, &indices_src, &indice_count, radius, height, radial_segments, rings, is_hemisphere);

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                      = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
    n->v.mesh_primitive.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_sphere_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
{
    Temp temp = scratch_begin(0,0);
    RK_MeshPrimitive *primitive = 0;

    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key key = {0};
    {
        F64 radius_f64 = radius;
        F64 height_f64 = height;
        U64 hash_content[] = {
            *(U64 *)(&radius_f64),
            *(U64 *)(&height_f64),
            radial_segments,
            rings,
            is_hemisphere
        };
        key = rk_key_from_string(rk_key_zero(), str8((U8 *)hash_content, sizeof(hash_content)));
    }

    U64 slot_idx = key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            AssertAlways(n->kind == RK_MeshKind_Sphere);
            primitive = n->v;
            n->rc++;
        }
    }

    if(primitive == 0)
    {
        R_Vertex *vertices_src = 0;
        U64 vertice_count     = 0;
        U32 *indices_src       = 0;
        U64 indice_count      = 0;
        rk_mesh_primitive_sphere(temp.arena, &vertices_src, &vertice_count, &indices_src, &indice_count, radius, height, radial_segments, rings, is_hemisphere);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->v = push_array(cache_table->arena, RK_MeshPrimitive, 1);
        cache_node->key = key;
        cache_node->rc  = 1;
        cache_node->kind = RK_MeshKind_Sphere;
        primitive = cache_node->v;
        DLLPushBack(slot->first, slot->last, cache_node);

        primitive->vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
        primitive->indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
        primitive->vertice_count = vertice_count;
        primitive->indice_count = indice_count;
    }

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = primitive->vertices;
    n->v.mesh_primitive.indices       = primitive->indices;
    n->v.mesh_primitive.vertice_count = primitive->vertice_count;
    n->v.mesh_primitive.indice_count  = primitive->indice_count;

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_cylinder_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom)
{
    Temp temp = scratch_begin(0,0);

    R_Vertex *vertices_src = 0;
    U64 vertices_count     = 0;
    U32 *indices_src       = 0;
    U64 indices_count      = 0;
    rk_mesh_primitive_cylinder(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, radius, height, radial_segments, rings, cap_top, cap_bottom);

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                      = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
    n->v.mesh_primitive.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_cylinder_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom)
{
    Temp temp = scratch_begin(0,0);
    RK_MeshPrimitive *primitive = 0;

    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key key = {0};
    {
        F64 radius_f64 = radius;
        F64 height_f64 = height;
        U64 hash_content[] = {
            *(U64 *)(&radius_f64),
            *(U64 *)(&height_f64),
            radial_segments,
            rings,
            cap_top,
            cap_bottom,
        };
        key = rk_key_from_string(rk_key_zero(), str8((U8 *)hash_content, sizeof(hash_content)));
    }

    U64 slot_idx = key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            AssertAlways(n->kind == RK_MeshKind_Cylinder);
            primitive = n->v;
            n->rc++;
        }
    }

    if(primitive == 0)
    {
        R_Vertex *vertices_src = 0;
        U64 vertice_count     = 0;
        U32 *indices_src       = 0;
        U64 indice_count      = 0;
        rk_mesh_primitive_cylinder(temp.arena, &vertices_src, &vertice_count, &indices_src, &indice_count, radius, height, radial_segments, rings, cap_top, cap_bottom);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->v    = push_array(cache_table->arena, RK_MeshPrimitive, 1);
        cache_node->key  = key;
        cache_node->rc   = 1;
        cache_node->kind = RK_MeshKind_Cylinder;
        primitive = cache_node->v;
        DLLPushBack(slot->first, slot->last, cache_node);

        primitive->vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
        primitive->indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
        primitive->vertice_count = vertice_count;
        primitive->indice_count = indice_count;
    }

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = primitive->vertices;
    n->v.mesh_primitive.indices       = primitive->indices;
    n->v.mesh_primitive.vertice_count = primitive->vertice_count;
    n->v.mesh_primitive.indice_count  = primitive->indice_count;

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_capsule_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings)
{
    Temp temp = scratch_begin(0,0);
    R_Vertex *vertices_src = 0;
    U64 vertices_count     = 0;
    U32 *indices_src       = 0;
    U64 indices_count      = 0;
    rk_mesh_primitive_capsule(temp.arena, &vertices_src, &vertices_count, &indices_src, &indices_count, radius, height, radial_segments, rings);

    RK_Node *n = rk_build_node_from_stringf(0, "capsule_primitive");
    n->kind                      = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertices_count, (void *)vertices_src);
    n->v.mesh_primitive.indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indices_count, (void *)indices_src);

    scratch_end(temp);
    return n;
}

internal RK_Node *
rk_capsule_node_cached(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings)
{
    Temp temp = scratch_begin(0,0);
    RK_MeshPrimitive *primitive = 0;

    RK_MeshCacheTable *cache_table = rk_top_mesh_cache_table();
    Assert(cache_table != 0);
    RK_Key key = {0};
    {
        F64 radius_f64 = radius;
        F64 height_f64 = height;
        U64 hash_content[] = {
            *(U64 *)(&radius_f64),
            *(U64 *)(&height_f64),
            radial_segments,
            rings,
        };
        key = rk_key_from_string(rk_key_zero(), str8((U8 *)hash_content, sizeof(hash_content)));
    }

    U64 slot_idx = key.u64[0] % cache_table->slot_count;
    RK_MeshCacheSlot *slot = &cache_table->slots[slot_idx];
    for(RK_MeshCacheNode *n = slot->first; n != 0; n = n->next)
    {
        if(rk_key_match(n->key, key))
        {
            AssertAlways(n->kind == RK_MeshKind_Capsule);
            primitive = n->v;
            n->rc++;
        }
    }

    if(primitive == 0)
    {
        R_Vertex *vertices_src = 0;
        U64 vertice_count     = 0;
        U32 *indices_src       = 0;
        U64 indice_count      = 0;
        rk_mesh_primitive_capsule(temp.arena, &vertices_src, &vertice_count, &indices_src, &indice_count, radius, height, radial_segments, rings);

        RK_MeshCacheNode *cache_node = push_array(cache_table->arena, RK_MeshCacheNode, 1);
        cache_node->v = push_array(cache_table->arena, RK_MeshPrimitive, 1);
        cache_node->key = key;
        cache_node->rc  = 1;
        cache_node->kind = RK_MeshKind_Capsule;
        primitive = cache_node->v;
        DLLPushBack(slot->first, slot->last, cache_node);

        primitive->vertices = r_buffer_alloc(R_ResourceKind_Static, sizeof(R_Vertex)*vertice_count, (void *)vertices_src);
        primitive->indices  = r_buffer_alloc(R_ResourceKind_Static, sizeof(U32)*indice_count, (void *)indices_src);
        primitive->vertice_count = vertice_count;
        primitive->indice_count = indice_count;
    }

    RK_Node *n = rk_build_node_from_stringf(0, "primitive");
    n->kind                           = RK_NodeKind_MeshPrimitive;
    n->v.mesh_primitive.vertices      = primitive->vertices;
    n->v.mesh_primitive.indices       = primitive->indices;
    n->v.mesh_primitive.vertice_count = primitive->vertice_count;
    n->v.mesh_primitive.indice_count  = primitive->indice_count;

    scratch_end(temp);
    return n;
}

// internal RK_Node *
// rk_n2d_anim_sprite2d_from_spritesheet(Arena *arena, String8 string, RK_SpriteSheet2D *spritesheet2d) {
//     RK_Node *result = rk_build_node2d_from_string(arena, string);
//     result->kind = RK_NodeKind_AnimatedSprite2D;
// 
//     U64 anim2d_hash_table_size = 3000;
//     RK_Animation2DHashSlot *anim2d_hash_table = push_array(arena, RK_Animation2DHashSlot, anim2d_hash_table_size);
// 
//     for(U64 i = 0; i < spritesheet2d->tag_count; i++) {
//         RK_Sprite2D_FrameTag *tag = &spritesheet2d->tags[i];
//         U64 animation_key = rk_hash_from_string(5381, tag->name);
//         U64 slot_idx = animation_key % anim2d_hash_table_size;
// 
//         RK_Animation2D *anim2d = push_array(arena, RK_Animation2D, 1);
// 
//         F32 total_duration = 0;
//         U64 frame_count = tag->to - tag->from + 1;
//         RK_Sprite2D_Frame *first_frame = spritesheet2d->frames + tag->from;
//         for(U64 frame_idx = 0; frame_idx < frame_count; frame_idx++) {
//             total_duration += (first_frame+frame_idx)->duration;
//         }
// 
//         anim2d->key         = animation_key;
//         anim2d->frames      = first_frame;
//         anim2d->frame_count = frame_count;
//         anim2d->duration    = total_duration;
//         DLLPushBack_NP(anim2d_hash_table[slot_idx].first, anim2d_hash_table[slot_idx].last, anim2d, hash_next, hash_prev);
//         AssertAlways(total_duration > 0);
//     }
// 
//     result->anim_sprite2d.flip_h      = 0;
//     result->anim_sprite2d.flip_v      = 0;
//     result->anim_sprite2d.speed_scale = 1.0f;
//     result->anim_sprite2d.anim2d_hash_table_size = anim2d_hash_table_size;
//     result->anim_sprite2d.anim2d_hash_table = anim2d_hash_table;
//     return result;
// }
