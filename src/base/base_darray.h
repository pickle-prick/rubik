#ifndef BASE_DARRAY_H
#define BASE_DARRAY_H
#define darray_push(arena, darray, item)                                                 \
    do {                                                                                 \
        (darray) = darray_hold(arena, (darray), 1, sizeof(*(darray)));                   \
        (darray)[darray_size(darray) - 1] = (item);                                      \
    } while (0);

U64 darray_size(void *darray);
void *darray_hold(Arena *arena, void *darray, U64 count, U64 item_size);
// void darray_free(void *darray);
void darray_clear(void *darray);
#endif
