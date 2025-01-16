/////////////////////////////////
// Scene serialization/deserialization

/////////////////////////////////
// GLTF2.0

internal RK_Handle
rk_tex_from_gltf_tex(cgltf_texture *tex_src, cgltf_data *data, String8 gltf_directory, RK_Key seed_key)
{
    RK_Handle ret = rk_handle_zero();
    if(tex_src == 0) return ret;

    U64 texture_idx = cgltf_texture_index(data, tex_src);
    RK_Key res_key = rk_res_key_from_stringf(RK_ResourceKind_Texture2D, seed_key, "texture_%d", texture_idx);
    ret = rk_resh_acquire_from_key(res_key);

    if(rk_handle_is_zero(ret))
    {
        ret = rk_resh_alloc(res_key, RK_ResourceKind_Texture2D, 1);
        RK_Texture2D *tex2d = rk_res_data_from_handle(ret);
        R_Tex2DSampleKind sample_kind = R_Tex2DSampleKind_Nearest;
        tex2d->sample_kind = sample_kind;

        Temp temp = scratch_begin(0,0);
        String8List path_parts = {0};
        str8_list_push(temp.arena, &path_parts, gltf_directory);
        str8_list_push(temp.arena, &path_parts, str8_cstring(tex_src->image->uri));
        String8 path = str8_path_list_join_by_style(temp.arena, &path_parts, PathStyle_Relative);

        int x,y,n;
        U8 *image_data = stbi_load((char*)path.str, &x, &y, &n, 4);
        tex2d->tex = r_tex2d_alloc(R_ResourceKind_Static, sample_kind, v2s32(x,y), R_Tex2DFormat_RGBA8, image_data);
        stbi_image_free(image_data);
        scratch_end(temp);
    }
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
            rk_trs_from_matrix((Mat4x4F32*)cn->matrix, &transform->position, &transform->rotation, &transform->scale);
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
        // TODO
    }

    // TODO: mesh gpu instancing

    // mesh
    if(mesh_src != 0) RK_Parent_Scope(ret)
    {
        RK_Handle skin_res = rk_handle_zero();
        B32 has_skin = 0;
        if(skin_src != 0)
        {
            U64 skin_idx = cgltf_skin_index(data, skin_src);
            RK_Key skin_key = rk_res_key_from_stringf(RK_ResourceKind_Skin, seed_key, "skin_%d", skin_idx);
            skin_res = rk_resh_acquire_from_key(skin_key);

            if(rk_handle_is_zero(skin_res))
            {
                skin_res = rk_resh_alloc(skin_key, RK_ResourceKind_Skin, 1);
                RK_Skin *skin = rk_res_data_from_handle(skin_res);

                skin->bind_count = skin_src->joints_count;
                AssertAlways(skin->bind_count < ArrayCount(skin->binds));
                Mat4x4F32 *inverse_bind_matrices = 0;
                {
                    cgltf_accessor *accessor = cn->skin->inverse_bind_matrices;
                    U64 offset = accessor->offset + accessor->buffer_view->offset;
                    AssertAlways(accessor->type == cgltf_type_mat4);
                    AssertAlways(accessor->stride == sizeof(Mat4x4F32));
                    inverse_bind_matrices = (Mat4x4F32 *)(((U8 *)accessor->buffer_view->buffer->data)+offset);
                }

                for(U64 i = 0; i < skin->bind_count; i++)
                {
                    U64 joint_idx = cgltf_node_index(data, skin_src->joints[i]);
                    RK_Key joint_key = rk_key_from_stringf(seed_key, "node_%d", joint_idx);
                    skin->binds[i].joint = joint_key;
                    skin->binds[i].inverse_bind_matrix = inverse_bind_matrices[i];
                }
            }
            has_skin = 1;
        }

        RK_Mesh *meshes = push_array(res_bucket->arena_ref, RK_Mesh, mesh_src->primitives_count);
        U64 mesh_idx = cgltf_mesh_index(data, mesh_src);
        for(U64 pi = 0; pi < mesh_src->primitives_count; pi++)
        {
            RK_Handle mesh_res = rk_handle_zero();
            RK_Key mesh_key = rk_res_key_from_stringf(RK_ResourceKind_Mesh, seed_key, "mesh_%d_%d", mesh_idx, pi);

            // fetch from cache first
            mesh_res = rk_resh_from_key(mesh_key);

            // create mesh if no cache was found
            if(rk_handle_is_zero(mesh_res))
            {
                mesh_res = rk_resh_alloc(mesh_key, RK_ResourceKind_Mesh, 1);
                RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);

                cgltf_primitive *primitive = &mesh_src->primitives[pi];

                // topology
                switch(primitive->type)
                {
                    case cgltf_primitive_type_lines:          { mesh->topology = R_GeoTopologyKind_Lines; }break;
                    case cgltf_primitive_type_line_strip:     { mesh->topology = R_GeoTopologyKind_LineStrip; }break;
                    case cgltf_primitive_type_triangles:      { mesh->topology = R_GeoTopologyKind_Triangles; }break;
                    case cgltf_primitive_type_triangle_strip: { mesh->topology = R_GeoTopologyKind_TriangleStrip; }break;
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
                    mesh->indices = indices;
                    mesh->indice_count = accessor->count;
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
                    mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, size, (void*)vertices_src, size);
                    mesh->vertex_count = vertex_count;
                }

                // load material
                RK_Handle material_res = rk_handle_zero();
                {
                    cgltf_material *material_src = primitive->material;
                    U64 material_idx = cgltf_material_index(data, material_src);
                    RK_Key material_key = rk_res_key_from_stringf(RK_ResourceKind_Material, seed_key, "material_%d", material_idx);
                    material_res = rk_resh_acquire_from_key(material_key);

                    if(rk_handle_is_zero(material_res))
                    {
                        material_res = rk_resh_alloc(material_key, RK_ResourceKind_Material, 1);
                        RK_Material *material = rk_res_data_from_handle(material_res);
                        {
                            material->name = push_str8_copy_static(str8_lit(material_src->name), material->name_buffer, ArrayCount(material->name_buffer));

                            // pbr
                            // if(material_src->has_pbr_metallic_roughness)
                            material->base_clr_tex = rk_tex_from_gltf_tex(material_src->pbr_metallic_roughness.base_color_texture.texture, data, gltf_directory, seed_key);
                            material->base_clr_factor = *(Vec4F32*)material_src->pbr_metallic_roughness.base_color_factor;
                            material->metallic_roughness_tex = rk_tex_from_gltf_tex(material_src->pbr_metallic_roughness.metallic_roughness_texture.texture, data, gltf_directory, seed_key);
                            material->metallic_factor = material_src->pbr_metallic_roughness.metallic_factor;
                            material->roughness_factor = material_src->pbr_metallic_roughness.roughness_factor;

                            // normal
                            material->normal_tex = rk_tex_from_gltf_tex(material_src->normal_texture.texture, data, gltf_directory, seed_key);
                            material->normal_scale = material_src->normal_texture.scale;

                            // occlusion texture
                            material->occlusion_tex = rk_tex_from_gltf_tex(material_src->occlusion_texture.texture, data, gltf_directory, seed_key);
                            material->occlusion_strength = material_src->occlusion_texture.scale;

                            // emissive texture
                            material->emissive_tex = rk_tex_from_gltf_tex(material_src->emissive_texture.texture, data, gltf_directory, seed_key);
                            material->emissive_factor = *(Vec3F32*)material_src->emissive_factor;
                        }
                    }
                }
                mesh->material = material_res;
            }

            // create mesh_inst3d for every primitive
            String8 name = push_str8f(scratch.arena, "%S_%d", ret->name, pi);
            RK_Key key = rk_key_from_stringf(seed_key, "node_%d_%d", cn_idx, pi);
            RK_NodeFlags flags = 0;
            if(has_skin) flags |= RK_NodeFlag_Float;
            RK_NodeTypeFlags type_flags = RK_NodeTypeFlag_Node3D|RK_NodeTypeFlag_MeshInstance3D;
            RK_Node *inst3d = rk_build_node_from_key(type_flags, flags, key);
            rk_node_equip_display_string(inst3d, name);
            inst3d->mesh_inst3d->mesh = mesh_res;
            inst3d->mesh_inst3d->skin = skin_res;
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
    RK_Handle ret = rk_handle_zero();
    RK_Key res_key = rk_res_key_from_string(RK_ResourceKind_PackedScene, rk_key_zero(), gltf_path);
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();

    if(rk_handle_is_zero(ret))
    {
        ret = rk_resh_alloc(res_key, RK_ResourceKind_PackedScene, 1);
        RK_PackedScene *packed_scene = rk_res_data_from_handle(ret);
        {
            packed_scene->res_bucket = rk_res_bucket_make(res_bucket->arena_ref, 4096-1);
            packed_scene->path = push_str8_copy_static(gltf_path, packed_scene->path_buffer, ArrayCount(packed_scene->path_buffer));
        }

        cgltf_options opts = {0};
        cgltf_data *data;
        cgltf_result parse_ret = cgltf_parse_file(&opts, (char*)gltf_path.str, &data);
        AssertAlways(parse_ret == cgltf_result_success && "Failed to load gltf");

        //- Load buffers
        cgltf_options buf_ld_opts = {0};
        cgltf_result buf_ld_ret = cgltf_load_buffers(&buf_ld_opts, data, (char *)gltf_path.str);
        AssertAlways(buf_ld_ret == cgltf_result_success);
        AssertAlways(data->scenes_count == 1);

        //- Select the first scene (TODO: only load default scene for now)
        cgltf_scene *root_scene = &data->scenes[0];
        AssertAlways(root_scene->nodes_count == 1);
        cgltf_node *cgltf_root = root_scene->nodes[0];

        packed_scene->root = rk_node_from_gltf_node(cgltf_root, data, res_key, str8_chop_last_slash(gltf_path));

        //- Load skeleton animations after model is loaded
        if(data->animations_count > 0) RK_Parent_Scope(packed_scene->root)
        {
            AssertAlways(data->animations_count < RK_MAX_ANIMATION_PER_INST_COUNT);

            // create AnimationPlayer
            RK_Key key = rk_key_from_stringf(res_key, "animation_player");
            RK_Node *n = rk_build_node_from_key(RK_NodeTypeFlag_AnimationPlayer, 0, key);
            rk_node_equip_display_string(n, str8_lit("animation_player"));
            {
                n->animation_player->animation_count = data->animations_count;
            }

            for(U64 i = 0; i < data->animations_count; i++)
            {
                n->animation_player->animations[i] = rk_animation_from_gltf_animation(data, &data->animations[i], res_key);
            }
        }
        
        cgltf_free(data); 
    }
    return ret;
}

internal RK_Handle
rk_animation_from_gltf_animation(cgltf_data *data, cgltf_animation *animation_src, RK_Key seed)
{
    RK_Handle ret;
    RK_ResourceBucket *res_bucket = rk_top_res_bucket();

    // NOTE(k): animation is unique in gltf, so we don't fetch from cache
    U64 animation_idx = cgltf_animation_index(data, animation_src);
    RK_Key res_key = rk_res_key_from_stringf(RK_ResourceKind_Animation, seed, "animation_%d", animation_idx);
    ret = rk_resh_alloc(res_key, RK_ResourceKind_Animation, 1);
    RK_Animation *animation = rk_res_data_from_handle(ret);

    // fill basic info
    {
        animation->name = push_str8_copy_static(str8_cstring(animation_src->name), animation->name_buffer, ArrayCount(animation->name_buffer));
        animation->track_count = animation_src->channels_count;
    }

    F32 max_track_duration_sec = 0;

    // tracks
    for(U64 i = 0; i < animation->track_count; i++)
    {
        RK_Track *track = rk_track_alloc();
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
            RK_TrackFrame *frame = rk_track_frame_alloc();
            
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
        DLLPushBack(animation->first_track, animation->last_track, track);
    }

    animation->duration_sec = max_track_duration_sec;
    return ret;
}

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
        // TODO: how to handle skin
        rk_node_from_packed_scene_node(child, seed_key);
    }
    return ret;
}

internal RK_Node *
rk_node_from_packed_scene(String8 string, RK_Handle handle)
{
    RK_PackedScene *packed_scene = rk_res_data_from_handle(handle);
    RK_Key seed_key = rk_key_from_string(rk_key_zero(), string);
    RK_Node *ret = 0;
    if(packed_scene != 0)
    {
        ret = rk_node_from_packed_scene_node(packed_scene->root, seed_key);
        ret->instance = handle;
        ret->name = push_str8_copy_static(string, ret->name_buffer, ArrayCount(ret->name_buffer));
    }
    return ret;
}

/////////////////////////////////
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
    U64 i,j,prevrow,thisrow,vertex_idx, indice_idx;
    F32 x,z;

    Vec2F32 start_pos = scale_2f32(size, -0.5);
    // Face Y
    Vec3F32 normal = {0.0f, -1.0f, 0.0f};

    U64 vertex_count   = (subdivide_d+2) * (subdivide_w+2);
    U64 indice_count   = (subdivide_d+1) * (subdivide_w+1) * 6 * both_face*2;
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
rk_mesh_primitive_circle_line(Arena *arena, F32 radius, U64 segments, R_Vertex **vertices_out, U64 *vertices_count_out, U32 **indices_out, U64 *indices_count_out)
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

internal RK_Node *
rk_box_node(String8 string, Vec3F32 size, U64 subdivide_w, U64 subdivide_h, U64 subdivide_d)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_box(scratch.arena, size, subdivide_w,subdivide_h,subdivide_d, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_BoxPrimitive;            
            mesh->src.box_primitive.subdivide_w = subdivide_w;
            mesh->src.box_primitive.subdivide_h = subdivide_h;
            mesh->src.box_primitive.subdivide_d = subdivide_d;
            mesh->src.box_primitive.size = size;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_Node *
rk_plane_node(String8 string, Vec2F32 size, U64 subdivide_w, U64 subdivide_d)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_plane(scratch.arena, size, subdivide_w,subdivide_d, 0, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_BoxPrimitive;            
            mesh->src.plane_primitive.subdivide_w = subdivide_w;
            mesh->src.plane_primitive.subdivide_d = subdivide_d;
            mesh->src.plane_primitive.size = size;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_Node *
rk_sphere_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_sphere(scratch.arena, radius, height, radial_segments, rings, is_hemisphere, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_SpherePrimitive;
            mesh->src.sphere_primitive.radius = radius;
            mesh->src.sphere_primitive.height = height;
            mesh->src.sphere_primitive.radial_segments = radial_segments;
            mesh->src.sphere_primitive.rings = rings;
            mesh->src.sphere_primitive.is_hemisphere = is_hemisphere;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_Node *
rk_cylinder_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 cap_top, B32 cap_bottom)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_cylinder(scratch.arena, radius, height, radial_segments, rings, cap_top, cap_bottom, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_CylinderPrimitive;
            mesh->src.cylinder_primitive.radius = radius;
            mesh->src.cylinder_primitive.height = height;
            mesh->src.cylinder_primitive.radial_segments = radial_segments;
            mesh->src.cylinder_primitive.rings = rings;
            mesh->src.cylinder_primitive.cap_top = cap_top;
            mesh->src.cylinder_primitive.cap_bottom = cap_bottom;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_Node *
rk_capsule_node(String8 string, F32 radius, F32 height, U64 radial_segments, U64 rings)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_capsule(scratch.arena, radius, height, radial_segments, rings, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_CapsulePrimitive;
            mesh->src.cylinder_primitive.radius = radius;
            mesh->src.cylinder_primitive.height = height;
            mesh->src.cylinder_primitive.radial_segments = radial_segments;
            mesh->src.cylinder_primitive.rings = rings;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_Node *
rk_torus_node(String8 string, F32 inner_radius, F32 outer_radius, U64 rings, U64 ring_segments)
{
    RK_Key res_key;
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
        res_key = rk_res_key_from_string(RK_ResourceKind_Mesh, rk_key_zero(), str8((U8*)hash_part, sizeof(hash_part)));
    }

    RK_Handle mesh_res = rk_resh_acquire_from_key(res_key);
    if(rk_handle_is_zero(mesh_res))
    {
        mesh_res = rk_resh_alloc(res_key, RK_ResourceKind_Mesh, 1);
        RK_Mesh *mesh = rk_res_data_from_handle(mesh_res);
        {
            Temp scratch = scratch_begin(0,0);
            R_Vertex *vertices;
            U64 vertex_count;
            U32 *indices;
            U64 indice_count;
            rk_mesh_primitive_torus(scratch.arena, inner_radius, outer_radius, rings, ring_segments, &vertices, &vertex_count, &indices, &indice_count);
            U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
            U64 indice_buffer_size = sizeof(U32) * indice_count;

            mesh->topology = R_GeoTopologyKind_Triangles;
            mesh->vertices = r_buffer_alloc(R_ResourceKind_Static, vertex_buffer_size, vertices, vertex_buffer_size);
            mesh->vertex_count = vertex_count;
            mesh->indices = r_buffer_alloc(R_ResourceKind_Static, indice_buffer_size, indices, indice_buffer_size);
            mesh->indice_count = indice_count;
            mesh->src_kind = RK_MeshSourceKind_TorusPrimitive;
            mesh->src.torus_primitive.inner_radius = inner_radius;
            mesh->src.torus_primitive.outer_radius = outer_radius;
            mesh->src.torus_primitive.rings = rings;
            mesh->src.torus_primitive.ring_segments = ring_segments;
            scratch_end(scratch);
        }
    }

    RK_Node *ret = rk_build_mesh_inst3d_from_string(0, 0, string);
    ret->mesh_inst3d->mesh = mesh_res;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_plane_filled(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec2F32 size, B32 both_face, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec4F32 clr, B32 draw_edge, B32 omit_light)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices;
    U64 vertex_count;
    U32 *indices;
    U64 indice_count;
    rk_mesh_primitive_plane(arena, size, 0,0, both_face, &vertices, &vertex_count, &indices, &indice_count);

    Mat4x4F32 xform_t = make_translate_4x4f32(origin);
    Mat4x4F32 xform_r = mat_4x4f32(1.0f);
    {
        Vec3F32 k_hat = cross_3f32(i_hat, j_hat);
        xform_r.v[0][0] = i_hat.x;
        xform_r.v[0][1] = i_hat.y;
        xform_r.v[0][2] = i_hat.z;

        xform_r.v[1][0] = j_hat.x;
        xform_r.v[1][1] = j_hat.y;
        xform_r.v[1][2] = j_hat.z;

        xform_r.v[2][0] = k_hat.x;
        xform_r.v[2][1] = k_hat.y;
        xform_r.v[2][2] = k_hat.z;
    }
    Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = omit_light;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Triangles;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = 1.f;
    ret->xform = xform;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_box_filled(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 size, Vec3F32 origin, Vec3F32 i_hat, Vec3F32 j_hat, Vec4F32 clr, B32 draw_edge, B32 omit_light)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices;
    U64 vertex_count;
    U32 *indices;
    U64 indice_count;
    rk_mesh_primitive_box(arena, size, 0,0,0, &vertices, &vertex_count, &indices, &indice_count);
    U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
    U64 indice_buffer_size = sizeof(U32) * indice_count;

    Mat4x4F32 xform_t = make_translate_4x4f32(origin);
    Mat4x4F32 xform_r = mat_4x4f32(1.0f);
    {
        Vec3F32 k_hat = cross_3f32(i_hat, j_hat);
        xform_r.v[0][0] = i_hat.x;
        xform_r.v[0][1] = i_hat.y;
        xform_r.v[0][2] = i_hat.z;

        xform_r.v[1][0] = j_hat.x;
        xform_r.v[1][1] = j_hat.y;
        xform_r.v[1][2] = j_hat.z;

        xform_r.v[2][0] = k_hat.x;
        xform_r.v[2][1] = k_hat.y;
        xform_r.v[2][2] = k_hat.z;
    }
    Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = omit_light;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Triangles;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = 1.f;
    ret->xform = xform;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_sphere(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 origin, F32 radius, F32 height, U64 radial_segments, U64 rings, B32 is_hemisphere, Vec4F32 clr, B32 draw_edge, B32 omit_light)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices;
    U64 vertex_count;
    U32 *indices;
    U64 indice_count;
    rk_mesh_primitive_sphere(arena, radius, height, radial_segments, rings, is_hemisphere, &vertices, &vertex_count, &indices, &indice_count);
    U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
    U64 indice_buffer_size = sizeof(U32) * indice_count;

    Mat4x4F32 xform = make_translate_4x4f32(origin);
    // NOTE(k): don't need to rotate, it's sphere

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = omit_light;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Triangles;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = 1.f;
    ret->xform = xform;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_cone(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, F32 height, U64 radial_segments, Vec4F32 clr, B32 draw_edge, B32 omit_light)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices;
    U64 vertex_count;
    U32 *indices;
    U64 indice_count;
    rk_mesh_primitive_cone(arena, radius, height, radial_segments, 1, &vertices, &vertex_count, &indices, &indice_count);
    U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
    U64 indice_buffer_size = sizeof(U32) * indice_count;

    Mat4x4F32 xform_t = make_translate_4x4f32(origin);
    Mat4x4F32 xform_r;
    {
        Vec3F32 j_hat = v3f32(0,-1,0);
        F32 dot = dot_3f32(j_hat, normal);
        // same direction
        if(dot == 1)
        {
            xform_r = mat_4x4f32_from_quat_f32(make_indentity_quat_f32());
        }
        // reverse direction
        else if(dot == -1)
        {
            xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(v3f32(1,0,0), 0.5));
        }
        else
        {
            Vec3F32 cross = cross_3f32(j_hat, normal);
            F32 turn = turns_from_radians_f32(acosf(dot_3f32(normal, j_hat)));
            xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(cross, turn));
        }
    }
    Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = omit_light;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Triangles;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = 1.f;
    ret->xform = xform;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_line(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 start, Vec3F32 end, Vec4F32 clr, F32 line_width, B32 draw_edge)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices = push_array(arena, R_Vertex, 2);
    vertices[0].pos = start;
    vertices[1].pos = end;
    U32 *indices = push_array(arena, U32, 2);
    indices[0] = 0;
    indices[1] = 1;

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->vertices = vertices;
    ret->vertex_count = 2;
    ret->indices = indices;
    ret->indice_count = 2;
    ret->topology = R_GeoTopologyKind_Lines;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->draw_edge = 0;
    ret->omit_light = 1;
    ret->color = clr;
    ret->line_width = line_width;
    ret->xform = mat_4x4f32(1);
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_circle(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 origin, Vec3F32 normal, F32 radius, U64 segments, Vec4F32 clr, F32 line_width, B32 draw_edge)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    R_Vertex *vertices;
    U64 vertex_count;
    U32 *indices;
    U64 indice_count;
    rk_mesh_primitive_circle_line(arena, radius, segments, &vertices, &vertex_count, &indices, &indice_count);
    U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
    U64 indice_buffer_size = sizeof(U32) * indice_count;

    Mat4x4F32 xform_t = make_translate_4x4f32(origin);
    Mat4x4F32 xform_r;
    {
        Vec3F32 j_hat = v3f32(0,-1,0);
        Vec3F32 cross = cross_3f32(j_hat, normal);
        F32 turn = turns_from_radians_f32(acosf(dot_3f32(normal, j_hat)));
        xform_r = mat_4x4f32_from_quat_f32(make_rotate_quat_f32(cross, turn));
    }
    Mat4x4F32 xform = mul_4x4f32(xform_t, xform_r);

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = 1;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Lines;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = line_width;
    ret->xform = xform;
    return ret;
}

internal RK_DrawNode *
rk_drawlist_push_arc_filled(Arena *arena, RK_DrawList *drawlist, RK_Key key, Vec3F32 origin, Vec3F32 a, Vec3F32 b, U64 segments, B32 both_face, Vec4F32 clr, F32 line_width, B32 draw_edge, Mat4x4F32 xform)
{
    RK_DrawNode *ret = rk_drawlist_push(arena, drawlist);
    D_Bucket *bucket = d_top_bucket();

    U64 vertex_count = segments+2;
    U64 indice_count = segments*3*(2*both_face);
    R_Vertex *vertices = push_array(arena, R_Vertex, vertex_count);
    U32 *indices = push_array(arena, U32, indice_count);

    // create mesh primitives using slerp
    {
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
    }

    U64 vertex_buffer_size = sizeof(R_Vertex) * vertex_count;
    U64 indice_buffer_size = sizeof(U32) * indice_count;

    ret->key = key;
    ret->draw_bucket = bucket;
    ret->draw_edge = draw_edge;
    ret->omit_light = 1;
    ret->vertices = vertices;
    ret->vertex_count = vertex_count;
    ret->indices = indices;
    ret->indice_count = indice_count;
    ret->topology = R_GeoTopologyKind_Triangles;
    ret->polygon = R_GeoPolygonKind_Fill;
    ret->color = clr;
    ret->line_width = line_width;
    ret->xform = xform;
    return ret;
}
