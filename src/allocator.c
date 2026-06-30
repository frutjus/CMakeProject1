#include "allocator.h"
#include "defs.h"

void* alloc(allocator* a, long long int bytes)
{
  ASSERT(a->space >= bytes);

  void* retval = a->head;

  (char*)a->head += bytes;
  a->space -= bytes;

  return retval;
}
