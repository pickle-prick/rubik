global U64 global_update_tick_idx = 0;

internal void
main_thread_base_entry_point(int argc, char **argv)
{
    Temp scratch = scratch_begin(0, 0);
    tctx_set_thread_name("[main thread]");

    //- rjf: parse command line
    String8List command_line_argument_strings = os_string_list_from_argcv(scratch.arena, argc, argv);
    CmdLine cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);

    // NOTE: preload modules
    fp_init();
    f_init();
    os_gfx_init();

    entry_point(&cmdline);
    scratch_end(scratch);
}

internal U64
update_tick_idx(void)
{
    U64 result = ins_atomic_u64_eval(&global_update_tick_idx);
    return result;
}

internal B32
update(void)
{
    ins_atomic_u64_inc_eval(&global_update_tick_idx);
#if OS_FEATURE_GRAPHICAL
    // B32 result = frame();
    // TODO(k): we redraw constantly
    B32 result = 0;
#else
    B32 result = 0;
#endif
    return result;
}
