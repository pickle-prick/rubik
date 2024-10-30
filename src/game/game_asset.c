/////////////////////////////////
// Scene build api

internal G_Scene *
g_scene_load()
{
    Arena *arena = arena_alloc(.reserve_size = MB(64), .commit_size = MB(64));
    G_Scene *scene = push_array(arena, G_Scene, 1);
    {
        scene->arena = arena;
        scene->bucket = g_bucket_make(arena, 3000);
    }

    // Vertex, Normal, TexCoord, Color
    // 3+3+2+4=12 (row)
    // const F32 vertices_src[8*12] =
    // {
    //      // Front face
    //     -0.5f, -0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 0
    //      0.5f, -0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 1
    //      0.5f,  0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 2
    //     -0.5f,  0.5f,  0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 3
    //     
    //     // Back face
    //     -0.5f, -0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 4
    //      0.5f, -0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 5
    //      0.5f,  0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 6
    //     -0.5f,  0.5f, -0.5f, 0,0,0, 0,0, 1,1,1, // Vertex 7
    // };
    // const U32 indices_src[3*12] = 
    // {
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

    // TODO: searilize scene data using yaml or gltf

    G_Scene_Scope(scene)
    {
        G_Bucket_Scope(scene->bucket)
        {
            // Create the origin/world node
            G_Node *root = g_build_node_from_string(str8_lit("root"));
            root->pos = v3f32(0, 0, 0);
            scene->root = root;

            G_Parent_Scope(root)
            {
                G_Node *camera = g_node_camera3d_alloc(str8_lit("main_camera"));
                scene->camera = camera;
                camera->pos          = v3f32(0,-3,-3);
                camera->v.camera.fov = 0.25;
                camera->v.camera.zn  = 0.1f;
                camera->v.camera.zf  = 200.f;
                g_node_push_fn(scene->bucket->arena, camera, camera_fn);

                // Cube
                // G_Node *player = g_node_camera_mesh_inst3d_alloc(str8_lit("player"));
                // G_Mesh *mesh = push_array(scene->bucket->arena, G_Mesh, 1);
                // mesh->vertices = vertices;
                // mesh->indices  = indices;
                // mesh->kind     = G_MeshKind_Box;
                // player->pos         = v3f32(0,-0.51,0);
                // player->v.mesh      = mesh;
                // player->update_fn   = player_fn;
                // player->custom_data = push_array(scene->bucket->arena, G_PlayerData, 1);

                // Dummy1
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/free_droide_de_seguridad_k-2so_by_oscar_creativo/scene.gltf"));
                    }
                    G_Node *dummy = g_build_node_from_string(str8_lit("dummy"));
                    G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                    {
                        G_Node *n = g_node_from_model(m);
                        QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                        n->rot = mul_quat_f32(flip_y, n->rot);

                        dummy->skeleton_anims = m->anims;
                        dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                    }
                }

                // Dummy2
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/blackguard/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/blackguard/scene.gltf"));
                    }
                    G_Node *dummy = g_build_node_from_string(str8_lit("dummy2"));
                    G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                    {
                        G_Node *n = g_node_from_model(m);
                        QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                        n->rot = mul_quat_f32(flip_y, n->rot);

                        dummy->skeleton_anims = m->anims;
                        dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                    }
                    dummy->pos = v3f32(3,0,0);
                }

                // Dummy3
                {
                    G_Model *m;
                    G_Path_Scope(str8_lit("./models/dancing_stormtrooper/"))
                    {
                        m = g_model_from_gltf(scene->arena, str8_lit("./models/dancing_stormtrooper/scene.gltf"));
                    }
                    G_Node *dummy = g_build_node_from_string(str8_lit("dummy3"));
                    G_Parent_Scope(dummy) G_Seed_Scope(dummy->key) 
                    {
                        G_Node *n = g_node_from_model(m);
                        QuatF32 flip_y = make_rotate_quat_f32(v3f32(1,0,0), 0.5);
                        n->rot = mul_quat_f32(flip_y, n->rot);

                        dummy->skeleton_anims = m->anims;
                        dummy->flags |= (G_NodeFlags_Animated | G_NodeFlags_AnimatedSkeleton);
                    }
                    dummy->pos = v3f32(6,0,0);
                }
            }
        }
    }
    return scene;
}

internal G_Model *
g_model_from_gltf(Arena *arena, String8 gltf_path)
{
    G_Model *model = 0;
    cgltf_options opts = {0};
    cgltf_data *data;
    cgltf_result ret = cgltf_parse_file(&opts, (char *)gltf_path.str, &data);
    G_Key model_key = g_key_from_string(g_top_seed(), gltf_path);

    G_Seed_Scope(model_key)
    {
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
            model = g_mnode_from_gltf_node(arena, root, 0, 0, 1024);

            //- Load skeleton animations after model is loaded
            U64 anim_count = data->animations_count;
            G_MeshSkeletonAnimation **anims = push_array(arena, G_MeshSkeletonAnimation*, anim_count);
            for(U64 i = 0; i < anim_count; i++)
            {
                anims[i] = g_skeleton_anim_from_gltf_animation(&data->animations[i]);
            }
            model->anim_count = anim_count;
            model->anims = anims;

            cgltf_free(data);
        }
        else
        {
            InvalidPath;
        }
    }
    return model;
}

internal G_Key
g_key_from_gltf_node(cgltf_node *cn)
{
    return g_key_from_string(g_top_seed(), str8((U8*)(&cn), 8));
}

internal G_ModelNode *
g_mnode_from_hash_table(G_Key key, G_ModelNode_HashSlot *hash_table, U64 hash_table_size)
{
    G_ModelNode *ret = 0;
    U64 slot_idx = key.u64[0] % hash_table_size;
    for(G_ModelNode *n = hash_table[slot_idx].first; n != 0; n = n->hash_next)
    {
        if(g_key_match(n->key, key))
        {
            ret = n;
            break;
        }
    }
    return ret;
}

internal G_ModelNode *
g_mnode_from_gltf_node(Arena *arena, cgltf_node *cn, G_ModelNode *parent, G_ModelNode_HashSlot *hash_table, U64 hash_table_size)
{
    G_ModelNode *mnode = 0;
    B32 is_mesh = cn->mesh != 0;
    B32 is_skinned = is_mesh ? (cn->skin != 0) : 0;
    G_Key key = g_key_from_gltf_node(cn);

    // If it's the root node, create hash table
    if(hash_table == 0)
    {
        hash_table = push_array(arena, G_ModelNode_HashSlot, hash_table_size);
    }

    mnode = g_mnode_from_hash_table(key, hash_table, hash_table_size);

    // NOTE(k): If mnode is already created, it should be a joint node within the scene tree
    if(mnode != 0) { return mnode; }

    if(mnode == 0) { mnode = push_array(arena, G_ModelNode, 1); }

    mnode->name            = push_str8_copy(arena, str8_cstring(cn->name));
    mnode->key             = key;
    mnode->is_mesh_group   = is_mesh;
    mnode->is_skinned      = is_skinned;
    // TODO: we don't actually need to store the hash_table in the G_ModelNode
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
            mnode->joints      = push_array(arena, G_ModelNode*, cn->skin->joints_count);

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
                G_ModelNode *joint = g_mnode_from_gltf_node(arena, cn->skin->joints[i], 0, hash_table, hash_table_size);
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
            String8 name = push_str8f(arena, "%s#mesh#%d", mnode->name.str, i);
            G_Key key = g_key_from_string(mnode->key, name);

            G_ModelNode *mesh_node = push_array(arena, G_ModelNode, 1);
            mesh_node->key         = key;
            mesh_node->name        = name;
            mesh_node->xform       = mat_4x4f32(1.0f);
            mesh_node->is_mesh     = 1;

            mesh_node->parent      = mnode;
            DLLPushBack(mnode->first, mnode->last, mesh_node);
            mnode->children_count++;

            cgltf_primitive *primitive = &cn->mesh->primitives[i];
            AssertAlways(primitive->type == cgltf_primitive_type_triangles);

            // Texture
            // TODO(k): implement full PBR rendering
            {
                Temp temp = scratch_begin(0,0);
                cgltf_texture *tex = primitive->material->pbr_metallic_roughness.base_color_texture.texture;
                String8 path = push_str8_cat(temp.arena, g_top_path(), str8_cstring(tex->image->uri));

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
        g_mnode_from_gltf_node(arena, cn->children[i], mnode, mnode->hash_table, mnode->hash_table_size);
    }
    return mnode;
}

internal G_Node *
g_node_from_model(G_Model *model)
{
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    G_Key key = g_key_merge(g_top_seed(), model->key);
    G_Node *n = 0;

    U64 slot_idx = key.u64[0] % bucket->node_hash_table_size;
    for(G_Node *i = bucket->node_hash_table[slot_idx].first; i != 0; i = i->hash_next)
    {
        if(g_key_match(key, i->key))
        {
            n = i;
            break;
        }
    }
    if(n != 0) { return n; }

    n = g_build_node_from_key(key);

    n->name = push_str8_copy(arena, model->name);
    model->rc++; // TODO: make use of rc

    // translation, rotation, scale
    g_trs_from_matrix(&model->xform, &n->pos, &n->rot, &n->scale);

    B32 is_mesh_group = model->is_mesh_group;
    B32 is_mesh       = model->is_mesh;
    B32 is_joint      = model->is_joint;
    B32 is_skin       = model->is_skin; // root joint

    if(is_mesh_group)
    {
        n->kind                    = G_NodeKind_MeshGroup;
        n->v.mesh_grp.is_skinned   = model->is_skinned;
        n->v.mesh_grp.joint_count  = model->joint_count;
        n->v.mesh_grp.joints       = push_array(arena, G_Node*, model->joint_count);
        n->v.mesh_grp.joint_xforms = push_array(arena, Mat4x4F32, model->joint_count);
        g_node_push_fn(arena, n, mesh_grp_fn);

        for(U64 i = 0; i < model->joint_count; i++)
        {
            n->v.mesh_grp.joints[i] = g_node_from_model(model->joints[i]);
        }
        n->v.mesh_grp.root_joint = n->v.mesh_grp.joints[0];
    }

    if(is_mesh)
    {
        n->kind              = G_NodeKind_Mesh;
        n->v.mesh.kind       = G_MeshKind_Custom;
        n->v.mesh.vertices   = model->vertices;
        n->v.mesh.indices    = model->indices;
        n->v.mesh.albedo_tex = model->albedo_tex;
    }

    if(is_joint)
    {
        n->kind = G_NodeKind_MeshJoint;
        n->v.joint.inverse_bind_matrix = model->inverse_bind_matrix;
    }

    if(is_skin)
    { 
        n->flags |= G_NodeFlags_Float;
    }

    G_Parent_Scope(n)
    {
        for(G_ModelNode *c = model->first; c != 0; c = c->next) { g_node_from_model(c); }
    }
    return n;
}

internal G_MeshSkeletonAnimation *
g_skeleton_anim_from_gltf_animation(cgltf_animation *cgltf_anim)
{
    G_Bucket *bucket = g_top_bucket();
    Arena *arena = bucket->arena;

    G_MeshSkeletonAnimation *anim = push_array(arena, G_MeshSkeletonAnimation, 1);
    anim->name = push_str8_copy(arena, str8_cstring(cgltf_anim->name));

    AssertAlways(cgltf_anim->channels_count == cgltf_anim->samplers_count);
    U64 spline_count = cgltf_anim->channels_count;
    G_MeshSkeletonAnimSpline *splines = push_array(arena, G_MeshSkeletonAnimSpline, spline_count);

    F32 duration = 0;
    for(U64 i = 0; i < spline_count; i++)
    {
        G_MeshSkeletonAnimSpline *spline = &splines[i];
        cgltf_animation_channel *channel = &cgltf_anim->channels[i] ;
        cgltf_animation_sampler *sampler = channel->sampler;

        G_TransformKind t_kind;
        U64 value_size;
        switch(channel->target_path)
        {
            case cgltf_animation_path_type_translation: { t_kind = G_TransformKind_Translation; value_size = sizeof(Vec3F32); }break;
            case cgltf_animation_path_type_rotation:    { t_kind = G_TransformKind_Rotation; value_size = sizeof(Vec4F32); }break;
            case cgltf_animation_path_type_scale:       { t_kind = G_TransformKind_Scale; value_size = sizeof(Vec3F32); }break;
            case cgltf_animation_path_type_weights:     { t_kind = G_TransformKind_Invalid; value_size = sizeof(Vec2F32); }break; // TODO: ignored for now
            default:                                    { InvalidPath; }break;
        }

        G_InterpolationMethod method;
        switch(sampler->interpolation)
        {
            case cgltf_interpolation_type_linear:       { method = G_InterpolationMethod_Linear; }break;
            case cgltf_interpolation_type_step:         { method = G_InterpolationMethod_Step; }break;
            case cgltf_interpolation_type_cubic_spline: { method = G_InterpolationMethod_Cubicspline; }break;
            default:                                    { InvalidPath; }break;
        }

        G_Key target_key = g_key_from_string(g_top_seed(), str8((U8*)(&channel->target_node), 8));

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
