#include "czmq.h"
#include "morph_private.h"


typedef struct wrapped_buffer wrapped_buffer;
struct wrapped_buffer {
  const void *buf;
  size_t size;
  void *user_data;
  void ( *free )( wrapped_buffer *self );
};


struct raw_string {
  wrapped_buffer wrapped;
};
typedef struct raw_string raw_string;


struct wrapped_resizable {
  size_t buf_size;
};
#define wrapped_resizable_size( sz ) \
  ( sizeof( struct wrapped_resizable ) + sz )

#define raw_string_realloc( ptr, asize, nsize ) raw_realloc( ( ptr ), ( nsize ) )
#define raw_string_free( ptr, asize ) raw_realloc( ( ptr ), 0 )

#define wrapped_buffer_free(self) \
  do { \
    if ( ( self )->free != NULL ) { \
      ( self )->free( ( self ) ); \
    } \
  } while (0)

static void
wrapped_resizable_free( wrapped_buffer *self ) {
  struct wrapped_resizable *resizable = self->user_data;
  raw_string_free( resizable, wrapped_resizable_size( resizable->buf_size ) );
}
  

// returns the length of the last stored string
#define raw_string_length( str ) ( ( str )->wrapped.size )
// returns a pointer to the stored string
#define raw_string_get( str ) ( ( str )->wrapped.buf )
#define is_resizable( buf ) \
  ( ( buf ).free == wrapped_resizable_free )


static int
wrapped_resizable_resize( wrapped_buffer *self, size_t desired ) {
  struct wrapped_resizable *resizable = ( struct wrapped_resizable * ) self->user_data;
  if ( resizable->buf_size >= desired ) {
    return 0;
  }
  size_t new_buf_size = resizable->buf_size * 2;
  if ( desired > new_buf_size ) {
    new_buf_size = desired;
  }
  struct wrapped_resizable *new_resizable = raw_string_realloc( resizable, wrapped_resizable_size( resizable->buf_size ), wrapped_resizable_size( new_buf_size ) );
  if ( new_resizable == NULL ) {
    return ENOMEM;
  }
  new_resizable->buf_size = new_buf_size;
  char *old_buf = ( char * ) resizable;
  char *new_buf = ( char * ) new_resizable;
  ptrdiff_t offset = ( char * ) self->buf - old_buf;
  self->buf = new_buf + offset;
  self->user_data = new_resizable;

  return 0;
}


static int
wrapped_resizable_new( wrapped_buffer *dest, size_t buf_size ) {
  size_t allocated_size = wrapped_resizable_size( buf_size );
  struct wrapped_resizable *resizable = ( struct wrapped_resizable * ) zmalloc( allocated_size );
  if ( resizable == NULL ) {
    return ENOMEM;
  }
  resizable->buf_size = buf_size;
  dest->buf = ( ( char * ) resizable ) + sizeof( struct wrapped_resizable );
  dest->size = buf_size;
  dest->user_data = resizable;
  dest->free = wrapped_resizable_free;

  return 0;
}

void
raw_string_init( raw_string *str ) {
  memset( str, 0, sizeof( raw_string ) );
}


void
raw_string_clear( raw_string *str ) {
  if ( is_resizable ( str->wrapped ) ) {
    str->wrapped.size = 0;
  }
  else {
    wrapped_buffer_free( &str->wrapped );
    raw_string_init( str );
  }
}


static int
raw_string_ensure_buf( raw_string *str, size_t length ) {
  int rval;

  if ( is_resizable( str->wrapped ) ) {
    return wrapped_resizable_resize( &str->wrapped, length );
  }
  else {
    wrapped_buffer orig = str->wrapped;
    check( rval, wrapped_resizable_new( &str->wrapped, length ) );
    if ( orig.size > 0 ) {
      size_t to_copy = ( orig.size < length ) ? orig.size : length;
      memcpy( ( void * ) str->wrapped.buf, orig.buf, to_copy );
    }
    wrapped_buffer_free( &orig );

    return 0;
  }
}


void
raw_string_set( raw_string *str, const char *src ) {
  size_t length = strlen( src );
  raw_string_ensure_buf( str, length + 1 );
  memcpy( ( void * ) str->wrapped.buf, src, length + 1 );
  str->wrapped.size = length + 1;
}


void
raw_string_append( raw_string *str, const char *src ) {
  if ( raw_string_length( str ) == 0 ) {
    return raw_string_set( str, src );
  }
  size_t length = strlen( src );
  raw_string_ensure_buf( str, str->wrapped.size + length );
  memcpy( ( char * ) str->wrapped.buf + str->wrapped.size - 1, src, length + 1 );
  str->wrapped.size += length;
}


void
raw_string_done( raw_string *str ) {
  wrapped_buffer_free( &str->wrapped );
  raw_string_init( str );
}


void
test_raw_string( void ) {
  raw_string str;

  raw_string_init( &str );
  raw_string_append( &str, "a" );
  raw_string_append( &str, "abcdefgh" );
  raw_string_append( &str, "abcd" );
  printf( "concatenated string %s\n", ( char * ) raw_string_get( &str ) );
  raw_string_done( &str );
}

