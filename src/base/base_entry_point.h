#ifndef BASE_ENTRY_POINT_H
#define BASE_ENTRY_POINT_H

internal void main_thread_base_entry_point(void (*entry_point)(CmdLine *cmdline), char **arguments, U64 arguments_count);

#endif // BASE_ENTRY_POINT_H

