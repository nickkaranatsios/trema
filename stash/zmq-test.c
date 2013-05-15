// gcc -Wall -g zmq-test.c -L/usr/local/lib -lzmq -o zmq-test
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <zmq.h>
#include <zmq_utils.h>
 
#define IO_THREADS_DFLT 1
#define MAX_SOCKETS_DFLT 1024

static void *context;
static void *socket;
static int socket_options[ 32 ];

void
check_error( const char *err, const int rc ) {
  if ( rc ) {
    printf( "%s failed error %d errno %d\n", err, rc, errno );
  }
}
 
void
initialize_zmq() {
  context = zmq_ctx_new();
  check_error( "zmq_ctx_new()", context == NULL ? -1 : 0 );
  assert( context );
  int rc;

  rc = zmq_ctx_set( context, ZMQ_IO_THREADS, IO_THREADS_DFLT );
  check_error( "zmq_ctx_set() - io_threads", rc );
  rc = zmq_ctx_set( context, ZMQ_MAX_SOCKETS, MAX_SOCKETS_DFLT );
  check_error( "zmq_ctx_set() - max_sockets", rc );
}


void
finalize_zmq() {
  assert( context );
  int rc;

  rc = zmq_ctx_destroy( context );
  check_error( "zmq_ctx_destroy()", rc );
  context = NULL;
}


static
const char *socket_type_name_map( int type ) {
  const char *type_name_map[] = {
    "PAIR",
    "PUB"
  };
  assert( type < sizeof( type_name_map ) / sizeof( type_name_map[ 0 ] ) ); 
  return type_name_map[ type ];
}


void
populate_option_lookup() {
  const int options_int_length[] = {
    ZMQ_EVENTS,
    ZMQ_LINGER,
    ZMQ_RCVTIMEO,
    ZMQ_SNDTIMEO,
    ZMQ_RECONNECT_IVL,
    ZMQ_FD,
    ZMQ_TYPE,
    ZMQ_BACKLOG
  };
  const int options_long_int_length[] = {
    ZMQ_RCVMORE,
    ZMQ_AFFINITY
  };

  int i;
  for ( i = 0; i < sizeof( options_int_length ) / sizeof( options_int_length[ 0 ] ); i++ ) {
    socket_options[ options_int_length[ i ] ] = 0;
  }
}


void
initialize_zmq_socket( void *context, int type, void *receiver_class ) {
  assert( context );

  socket = zmq_socket( context, ZMQ_PUB ); 
  check_error( "zmq_socket()", socket == NULL ? -1 : 0 );
  const char *name = socket_type_name_map( type );
  printf( "socket type %s\n", name );
  populate_option_lookup();
}


int
zmq_socket_bind( const char *address ) {
  assert( socket );
  return zmq_bind( socket, address );
}

int
zmq_socket_setsockopt( int name, int value ) {
  size_t length;
  void *value_ptr;
  
  if ( !socket_options[ name ] ) {
    length = 4;
    value_ptr = malloc( length );
    memcpy( value_ptr, &value, length );
  }
  return zmq_setsockopt( socket, name, &value_ptr, length );
}

void
finalize_zmq_socket( void *socket ) {
  int rc = zmq_close( socket );
  check_error( "zmq_close()", rc );
  socket = NULL;
}




int
main( int argc, char **argv ) {
  int major;
  int minor;
  int patch;
 
  zmq_version( &major, &minor, &patch );
  printf( "zeromq version %d.%d.%d\n", major, minor, patch );
 
  initialize_zmq();

  zmq_msg_t message;
  int rc;

  rc = zmq_msg_init( &message );
  check_error( "zmq_msg_init()", rc );
  printf( "default msg size %u\n", zmq_msg_size( &message ) );

  initialize_zmq_socket( context, ZMQ_PUB, NULL );
  rc = zmq_socket_setsockopt( ZMQ_LINGER, 100 );
  check_error( "zmq_setsockopt()", rc );

  check_error( "zmq_bind()", rc );
  rc = zmq_socket_bind( "tcp://127.0.0.1:5555" );
  check_error( "zmq_bind()", rc );
  finalize_zmq_socket( socket );

  finalize_zmq();
  return 0;
}
