// typedef struct Dice Dice;
// struct Dice
// {
//     U64 x;
//     U64 y;
//     F32 rotation_inv_t; // 1->0
//     QuatF32 src_rotation;
//     QuatF32 target_rotation;
// };
// 
// RK_NODE_CUSTOM_UPDATE(dice_update)
// {
//     // unpack custom data
//     Dice *stone = (Dice*)node->custom_data;
// 
//     RK_Transform3D *transform = &node->node3d->transform;
// 
//     if(rk_key_match(scene->hot_key, node->key))
//     {
//         if(rk_state->sig.f & UI_SignalFlag_LeftClicked)
//         {
//             QuatF32 q = make_rotate_quat_f32(v3f32(0,-1,0), -0.25);
//             // transform->rotation = mul_quat_f32(q, transform->rotation);
// 
//             QuatF32 curr_quat = stone->rotation_inv_t > 0.001 ? stone->target_rotation : transform->rotation;
//             stone->target_rotation = mul_quat_f32(q, curr_quat);
//             stone->rotation_inv_t = 1.0;
//             stone->src_rotation = transform->rotation;
//             // stone->src_rotation = curr_quat;
//         }
//     }
// 
//     // Animation rotation
//     if(stone->rotation_inv_t > 0.001)
//     {
//         F32 fast_rate = 1 - pow_f32(2, (-20.f * dt_sec));
//         stone->rotation_inv_t += fast_rate * (0-stone->rotation_inv_t);
//         transform->rotation = mix_quat_f32(stone->target_rotation, stone->src_rotation, stone->rotation_inv_t);
//     }
// }

typedef struct CellAtom CellAtom;
struct CellAtom
{
    CellAtom *next;
    CellAtom *prev;
    U64 update_idx;
};

typedef struct Cell Cell;
struct Cell
{
    CellAtom *first_atom;
    CellAtom *last_atom;
    U64 atom_count;
};

typedef struct Board Board;
struct Board
{
    Cell *cells;
    U64 width;
    U64 height;
    U64 update_idx;
    U64 ticks_remain;

    Rng2F32 pane_rect;
    B32 pane_show;
};

RK_NODE_CUSTOM_UPDATE(board_update)
{
    Board *board_data = node->custom_data;

    B32 ticked = 0;
    {
        // TODO: just use ui layer to draw for now, we don't have 2d font node yet
        UI_Box *pane = rk_ui_pane_begin(&board_data->pane_rect, &board_data->pane_show, str8_lit("PROB"));
        ui_spacer(ui_em(0.215, 0.f));

        U64 w = board_data->width;
        U64 h = board_data->height;

        // font
        ui_push_font_size(ui_top_font_size()*2.3);
        ui_push_pref_height(ui_em(1.35f, 1.f));

        if(board_data->ticks_remain > 0)
        {
            ticked = 1;
            board_data->ticks_remain--;
        }

        for(U64 j = 0; j < h; j++)
        {
            UI_PrefWidth(ui_pct(1.f, 0.f))UI_Row for(U64 i = 0; i < w; i++)
            {
                Cell *cell = &board_data->cells[j*w + i];
                ui_set_next_text_alignment(UI_TextAlign_Center);
                UI_Signal sig = ui_buttonf("%I64d###%d_%d", cell->atom_count, j, i);

                // ticking
                if(ticked && cell->atom_count > 0)
                {
                    sig.box->hot_t = 1.0f;
                    sig.box->active_t = 1.0f;
                    for(CellAtom *atom = cell->first_atom; atom != 0;)
                    {
                        CellAtom *a = atom;
                        atom = atom->next;

                        if(a->update_idx == board_data->update_idx)
                        {
                            Cell *adjacents[5] = {0};
                            U64 adjacent_count = 0;

                            // remove from self first
                            DLLRemove(cell->first_atom, cell->last_atom, a);
                            cell->atom_count--;

                            // self
                            adjacents[adjacent_count++] = cell;

                            // top
                            if(j > 0)
                            {
                                Cell *c = &board_data->cells[(j-1)*w + i];
                                adjacents[adjacent_count++] = c;
                            }

                            // bottom
                            if(j < h-1)
                            {
                                Cell *c = &board_data->cells[(j+1)*w + i];
                                adjacents[adjacent_count++] = c;
                            }

                            // left
                            if(i > 0)
                            {
                                Cell *c = &board_data->cells[j*w + i - 1];
                                adjacents[adjacent_count++] = c;
                            }

                            // right
                            if(i < w-1)
                            {
                                Cell *c = &board_data->cells[j*w + i + 1];
                                adjacents[adjacent_count++] = c;
                            }

                            // random insert
                            {
                                U64 random_idx = rand() % adjacent_count;
                                Cell *c = adjacents[random_idx];
                                DLLPushBack(c->first_atom, c->last_atom, a);
                                c->atom_count++;
                                a->update_idx++;
                            }
                        }
                    }
                }
            }
        }

        ui_spacer(ui_em(0.315, 0.f));

        // get entropy
        U64 entropy = 0;
        {
            U64 cell_count = board_data->width*board_data->height;
            for(U64 i = 0; i < cell_count; i++)
            {
                Cell *c = &board_data->cells[i];
                if(c->atom_count > 0)
                {
                    entropy++;
                }
            }
        }

        UI_Row
        {
            ui_labelf("entropy: %I64d", entropy);
            ui_spacer(ui_pct(1.f, 0.f));
            ui_set_next_text_alignment(UI_TextAlign_Center);
            ui_set_next_pref_width(ui_text_dim(300.f, 0.f));
            if(ui_clicked(ui_buttonf("tick once"))) board_data->ticks_remain++;
            ui_set_next_pref_width(ui_text_dim(300.f, 0.f));
            if(ui_clicked(ui_buttonf("tick twice"))) board_data->ticks_remain+=2;
            ui_spacer(ui_em(.1f, 0.f));
        }

        ui_pop_font_size();
        ui_pop_pref_width();
        ui_pop_pref_height();

        rk_ui_pane_end();
    }

    if(ticked) board_data->update_idx++;
}

internal RK_Scene *
rk_scene_entry__1()
{
    RK_Scene *ret = rk_scene_alloc(str8_lit("prob1"), str8_lit("./src/rubik/scenes/1/default.rscn"));
    rk_push_node_bucket(ret->node_bucket);
    rk_push_res_bucket(ret->res_bucket);

    RK_Node *root = rk_build_node3d_from_stringf(0, 0, "root");
    RK_Parent_Scope(root)
    {
        // create the editor camera
        RK_Node *main_camera = rk_build_camera3d_from_stringf(0, 0, "main_camera");
        {
            main_camera->camera3d->projection = RK_ProjectionKind_Perspective;
            main_camera->camera3d->viewport_shading = RK_ViewportShadingKind_Material;
            main_camera->camera3d->polygon_mode = R_GeoPolygonKind_Fill;
            main_camera->camera3d->hide_cursor = 0;
            main_camera->camera3d->lock_cursor = 0;
            main_camera->camera3d->is_active = 1;
            main_camera->camera3d->zn = 0.1;
            main_camera->camera3d->zf = 200.f;
            main_camera->camera3d->perspective.fov = 0.25f;
            rk_node_push_fn(main_camera, editor_camera_fn);
            main_camera->node3d->transform.position = v3f32(0,-3,0);
        }
        ret->active_camera = rk_handle_from_node(main_camera);

        // RK_Handle grid_prototype_material;
        // {
        //     String8 png_path = str8_lit("./textures/gridbox-prototype-materials/prototype_512x512_blue2.png");
        //     RK_Key material_res_key = rk_res_key_from_string(RK_ResourceKind_Material, rk_key_zero(), png_path);
        //     grid_prototype_material = rk_resh_alloc(material_res_key, RK_ResourceKind_Material, 1);
        //     RK_Material *material = rk_res_data_from_handle(grid_prototype_material);

        //     // load texture
        //     RK_Key tex2d_key = rk_res_key_from_string(RK_ResourceKind_Texture2D, rk_key_zero(), png_path);
        //     RK_Handle tex2d_res = rk_resh_alloc(tex2d_key, RK_ResourceKind_Texture2D, 1);
        //     R_Tex2DSampleKind sample_kind = R_Tex2DSampleKind_Nearest;
        //     RK_Texture2D *tex2d = rk_res_data_from_handle(tex2d_res);
        //     {
        //         int x,y,n;
        //         U8 *image_data = stbi_load((char*)png_path.str, &x, &y, &n, 4);
        //         tex2d->tex = r_tex2d_alloc(R_ResourceKind_Static, sample_kind, v2s32(x,y), R_Tex2DFormat_RGBA8, image_data);
        //         stbi_image_free(image_data);
        //     }
        //     tex2d->sample_kind = sample_kind;

        //     material->name = push_str8_copy_static(str8_lit("prototype_512x512_blue2"), material->name_buffer, ArrayCount(material->name_buffer));
        //     material->base_clr_tex = tex2d_res;
        // }

        // board
        RK_Node *board = rk_build_node3d_from_stringf(0,0,"board");
        rk_node_push_fn(board, board_update);
        board->node3d->transform.position.y = -10;

        Arena *arena = ret->node_bucket->arena_ref;
        Board *board_data = push_array(arena, Board, 1);
        board_data->width = 9;
        board_data->height = 9;
        board_data->cells = push_array(arena, Cell, 9*9);
        // init cells
        {
            // update one cell
            {
                Cell *cell = &board_data->cells[30];
                for(U64 i = 0; i < 81; i++)
                {
                    CellAtom *atom = push_array(arena, CellAtom, 1);
                    DLLPushBack(cell->first_atom, cell->last_atom, atom);
                    cell->atom_count++;
                }
            }
            board_data->pane_rect = rk_state->window_rect;
            {
                board_data->pane_rect.x0 += 600;
                board_data->pane_rect.x1 -= 600;
                board_data->pane_rect.y0 += 600;
                board_data->pane_rect.y1 -= 600;
                board_data->pane_show = 1;
            }
        }
        board->custom_data = board_data;
    }

    ret->root = rk_handle_from_node(root);
    rk_pop_node_bucket();
    rk_pop_res_bucket();
    return ret;
}
