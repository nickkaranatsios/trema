#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "allocation.c"


struct raw_array {
  size_t element_size;
  size_t element_count;
  size_t allocated_size;
  void *data;
};
typedef struct raw_array raw_array;

#define raw_array_size( array ) ( ( array )->element_count )
#define raw_array_get( array, element_type, index ) \
  ( ( ( element_type * ) ( array )->data )[ index ] )
#define raw_array_get_raw( array, index ) \
  ( ( char * ) ( array )->data + ( array )->element_size * index )

#define raw_array_realloc( ptr, asize, nsize ) raw_realloc( ( ptr ), ( nsize ) ) 
#define raw_array_free( ptr, asize ) raw_free( ( ptr ) )

void
raw_array_init( raw_array *array, size_t element_size ) {
  memset( array, 0, sizeof( raw_array ) );
  array->element_size = element_size;
}


void
raw_array_clear( raw_array *array ) {
  array->element_count = 0;
}


void
raw_array_done( raw_array *array ) {
  if ( array->data ) {
    raw_array_free( array->data, array->allocated_size );
  }
  memset( array, 0, sizeof( raw_array ) );
}


int
raw_array_ensure_size( raw_array *array, size_t desired_count ) {
  size_t required_size = array->element_size * desired_count;
  if ( array->allocated_size >= required_size ) {
    return 0;
  }

  size_t new_size;
  if ( array->allocated_size == 0 ) {
    new_size = 10 * array->element_size;
  }
  else {
    new_size = array->allocated_size * 2;
  }
  if ( required_size > new_size ) {
    new_size = required_size;
  }
  
  array->data = raw_array_realloc( array->data, array->allocated_size, new_size );
  if ( array->data == NULL ) {
    return ENOMEM;
  }
  array->allocated_size = new_size;

  return 0;
}


void *
raw_array_append( raw_array *array ) {
  int rval;

  rval = raw_array_ensure_size( array, array->element_count + 1 );
  if ( rval ) {
    return NULL;
  }
  size_t offset = array->element_size * array->element_count;
  array->element_count++;

  return ( char * ) array->data + offset;
}


void
test_raw_array( void ) {
  raw_array long_array;
  raw_array_init( &long_array, sizeof( long ) );
  long *element;
  element = ( long * ) raw_array_append( &long_array );
  *element = 1234;
  element = ( long * ) raw_array_append( &long_array );
  *element = 5678;

  if ( raw_array_size( &long_array ) != 2 ) {
    printf( "long array incorrect size\n" );
    exit( 1 );
  }
  long test_element = raw_array_get( &long_array, long, 0 );
  printf( "test_element = %ld\n", test_element );

  long next_element = raw_array_get( &long_array, long, 1 );
  printf( "next_element = %ld\n", next_element );
  raw_array_done( &long_array );
}
