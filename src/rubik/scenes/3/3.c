internal RK_Scene *
rk_scene_entry__3()
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("2d_demo"), str8_lit("./src/rubik/scenes/3/default.rscn"));
    rk_push_node_bucket(ret->node_bucket);
    rk_push_res_bucket(ret->res_bucket);

    // scene settings
    {
        ret->omit_grid = 1;
        ret->omit_gizmo3d = 1;
    }

    RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
    RK_Parent_Scope(root)
    {
        // create the orthographic camera 
        RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "main_camera");
        {
            main_camera->camera3d->projection = RK_ProjectionKind_Orthographic;
            main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
            main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
            main_camera->camera3d->hide_cursor = 0;
            main_camera->camera3d->lock_cursor = 0;
            main_camera->camera3d->is_active = 1;
            main_camera->camera3d->zn = 0;
            main_camera->camera3d->zf = 1;
            // TOOD(XXX): how are we supposed to set it
            main_camera->camera3d->orthographic.top = 0;
            main_camera->camera3d->orthographic.top = 0;
            main_camera->camera3d->orthographic.top = 0;
            main_camera->camera3d->orthographic.top = 0;

            main_camera->node3d->transform.position = v3f32(0,0,0);
        }
        ret->active_camera = rk_handle_from_node(main_camera);
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
