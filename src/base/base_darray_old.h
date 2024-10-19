#ifndef BASE_DARRAY_H
#define BASE_DARRAY_H
#define darray_push(darray, item)                                             \
    do {                                                                      \
        (darray) = darray_hold((darray), 1, sizeof(*(darray)));               \
        (darray)[darray_size(darray) - 1] = (item);                           \
    } while (0);

int darray_size(void *darray);
void *darray_hold(void *darray, int count, int item_size);
void darray_free(void *darray);
void darray_clear(void *darray);
#endif
