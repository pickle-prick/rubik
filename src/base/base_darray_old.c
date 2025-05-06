#include "base_darray_old.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#define DARRAY_RAW_DATA(darray) ((int *)(darray)-2)
#define DARRAY_CAPACITY(darray) (DARRAY_RAW_DATA(darray)[0])
#define DARRAY_OCCUPIED(darray) (DARRAY_RAW_DATA(darray)[1])

int darray_size(void *darray)
{
  return darray != NULL ? DARRAY_OCCUPIED(darray) : 0;
}

void darray_free(void *darray)
{
  if(darray != NULL)
  {
    free(DARRAY_RAW_DATA(darray));
  }
}

void darray_clear(void *darray)
{
  if(darray != NULL)
  {
    DARRAY_OCCUPIED(darray) = 0;
  }
}

void *darray_hold(void *darray, int count, int item_size)
{
  int header_size = sizeof(int) * 2;
  assert(count > 0 && item_size > 0);

  if(darray == NULL)
  {
    int raw_data_size = header_size + count * item_size;
    int *base = (int *)malloc(raw_data_size);
    base[0] = count;
    base[1] = count;
    return (void *)(base + 2);
  }
  else if(DARRAY_OCCUPIED(darray) + count <= DARRAY_CAPACITY(darray))
  {
    DARRAY_OCCUPIED(darray) += count;
    return darray;
  }
  else
  {
    // reallocate
    int size_required = DARRAY_OCCUPIED(darray) + count;
    int double_curr = DARRAY_CAPACITY(darray) * 2;
    int capacity =
      size_required > double_curr ? size_required : double_curr;
    int occupied = size_required;
    int raw_size = header_size + capacity * item_size;
    int *base = (int *)realloc(DARRAY_RAW_DATA(darray), raw_size);
    base[0] = capacity;
    base[1] = occupied;
    return (void *)(base + 2);
  }
}
