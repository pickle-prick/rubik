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

/////////////////////////////////
// Mesh primitives

internal void
g_mesh_primitive_box(Arena *arena, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
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

    // TODO: fix it
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
g_mesh_primitive_sphere(Arena *arena,
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
g_mesh_primitive_cylinder(Arena *arena,
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
                indices[indice_idx++] = thisrow;
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
                indices[indice_idx++] = thisrow;
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
g_mesh_primitive_capsule(Arena *arena, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out, F32 radius, F32 height, U64 radial_segments, U64 rings)
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
