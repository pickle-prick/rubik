#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define DARRAY_RAW_DATA(darray) ((U64 *)(darray)-2)
#define DARRAY_CAPACITY(darray) (DARRAY_RAW_DATA(darray)[0])
#define DARRAY_OCCUPIED(darray) (DARRAY_RAW_DATA(darray)[1])
#define DARRAY_HEADER_SIZE      sizeof(U64) * 2

U64 darray_size(void *darray) {
        return darray != NULL ? DARRAY_OCCUPIED(darray) : 0;
}

void darray_clear(void *darray) {
        if (darray != NULL) {
                DARRAY_OCCUPIED(darray) = 0;
        }
}

void *darray_hold(Arena *arena, void *darray, U64 count, U64 item_size) {
        assert(count > 0 && item_size > 0);

        if (darray == NULL) {
                U64 raw_data_size = DARRAY_HEADER_SIZE + count * item_size;
                U64 *base = (U64 *)push_array(arena, U64, raw_data_size);
                base[0] = count;
                base[1] = count;
                return (void *)(base + 2);
        } else if (DARRAY_OCCUPIED(darray) + count <= DARRAY_CAPACITY(darray)) {
                DARRAY_OCCUPIED(darray) += count;
                return darray;
        } else {
                // reallocate
                U64 size_required = DARRAY_OCCUPIED(darray) + count;
                U64 double_curr = DARRAY_CAPACITY(darray) * 2;
                U64 capacity = size_required > double_curr ? size_required : double_curr;
                U64 occupied = size_required;
                U64 raw_size = DARRAY_HEADER_SIZE + capacity * item_size;

                U64 *base = (U64 *)push_array(arena, U64, raw_size);
                MemoryCopy(base+2, darray, DARRAY_OCCUPIED(darray) * item_size);

                base[0] = capacity;
                base[1] = occupied;
                return (void *)(base + 2);
        }
}
