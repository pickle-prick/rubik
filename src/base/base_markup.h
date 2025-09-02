#ifndef BASE_MARKUP_H
#define BASE_MARKUP_H

internal void set_thread_name(String8 string);
internal void set_thread_namef(char *fmt, ...);
#define ThreadNameF(...) (set_thread_namef(__VA_ARGS__))

#endif // BASE_MARKUP_H
