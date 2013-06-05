#include "allocation.h"

static void *
default_allocator( void *ptr, size_t nsize ) {
  if ( nsize == 0 ) {
    free( ptr );
    return NULL;
  }
  else {
    return realloc( ptr, nsize );
  }
}
