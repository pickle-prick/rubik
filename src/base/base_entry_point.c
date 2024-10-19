internal void
main_thread_base_entry_point(void (*entry_point)(CmdLine *cmdline), char **arguments, U64 arguments_count)
{
        Temp scratch = scratch_begin(0, 0);
        String8List command_line_argument_strings = os_string_list_from_argcv(scratch.arena, (int)arguments_count, arguments);
        CmdLine cmdline = cmd_line_from_string_list(scratch.arena, command_line_argument_strings);
        B32 capture = cmd_line_has_flag(&cmdline, str8_lit("capture"));

        // NOTE: preload modules
        tctx_set_thread_name("[main thread]");
        fp_init();
        f_init();
        os_gfx_init();

        entry_point(&cmdline);
}
