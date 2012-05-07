/*
 * Author: Toshio Koide
 *
 * Copyright (C) 2008-2012 NEC Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "event_handler.h"
#include "log.h"
#include "messenger.h"
#include "timer.h"
#include "wrapper.h"


#ifdef UNIT_TESTING

#define static

// Redirect socket functions to mock functions in the unit test.
#ifdef socket
#undef socket
#endif
#define socket mock_socket
extern int mock_socket( int domain, int type, int protocol );

#ifdef bind
#undef bind
#endif
#define bind mock_bind
extern int mock_bind( int sockfd, const struct sockaddr *addr, socklen_t addrlen );

#ifdef listen
#undef listen
#endif
#define listen mock_listen
extern int mock_listen( int sockfd, int backlog );

#ifdef close
#undef close
#endif
#define close mock_close
extern int mock_close( int fd );

#ifdef connect
#undef connect
#endif
#define connect mock_connect
extern int mock_connect( int sockfd, const struct sockaddr *addr, socklen_t addrlen );

#ifdef select
#undef select
#endif
#define select mock_select
extern int mock_select( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout );

#ifdef accept
#undef accept
#endif
#define accept mock_accept
extern int mock_accept( int sockfd, struct sockaddr *addr, socklen_t *addrlen );

#ifdef recv
#undef recv
#endif
#define recv mock_recv
extern ssize_t mock_recv( int sockfd, void *buf, size_t len, int flags );

#ifdef send
#undef send
#endif
#define send mock_send
extern ssize_t mock_send( int sockfd, const void *buf, size_t len, int flags );

#ifdef setsockopt
#undef setsockopt
#endif
#define setsockopt mock_setsockopt
extern int mock_setsockopt( int s, int level, int optname, const void *optval, socklen_t optlen );

#ifdef clock_gettime
#undef clock_gettime
#endif
#define clock_gettime mock_clock_gettime
extern int mock_clock_gettime( clockid_t clk_id, struct timespec *tp );

#ifdef error
#undef error
#endif
#define error mock_error
extern void mock_error( const char *format, ... );

#ifdef debug
#undef debug
#endif
#define debug mock_debug
extern void mock_debug( const char *format, ... );

#ifdef warn
#undef warn
#endif
#define warn mock_warn
extern void mock_warn( const char *format, ... );

#ifdef add_periodic_event_callback
#undef add_periodic_event_callback
#endif
#define add_periodic_event_callback mock_add_periodic_event_callback
extern bool mock_add_periodic_event_callback( const time_t seconds, void ( *callback )( void *user_data ), void *user_data );

#endif // UNIT_TESTING


static bool initialized = false;
static bool finalized = false;


bool
init_messenger( const char *working_directory ) {
  assert( working_directory != NULL );

  init_event_handler();

  if ( initialized ) {
    warn( "Messenger is already initialized." );
    return true;
  }
  init_messenger_recv( working_directory );
  init_messenger_send( working_directory );
  init_messenger_context();


  initialized = true;
  finalized = false;
  return initialized;
}


bool
finalize_messenger() {
  debug( "Finalizing messenger." );

  if ( !initialized ) {
    warn( "Messenger is not initialized yet." );
    return false;
  }
  if ( finalized ) {
    warn( "Messenger is already finalized." );
    return true;
  }

  if ( messenger_dump_enabled() ) {
    stop_messenger_dump();
  }
  delete_all_receive_queues();
  delete_all_send_queues();
  delete_context_db();

  initialized = false;
  finalized = true;

  finalize_event_handler();

  return true;
}


int
flush_messenger() {
  int connected_count, sending_count, reconnecting_count, closed_count;

  debug( "Flushing send queues." );

  while ( true ) {
    number_of_send_queue( &connected_count, &sending_count, &reconnecting_count, &closed_count );
    if ( sending_count == 0 ) {
      return reconnecting_count;
    }
    run_event_handler_once( 100000 );
  }
}


bool
start_messenger() {
  debug( "Starting messenger." );
  add_periodic_event_callback( 10, age_context_db, NULL );
  start_messenger_send();
  return true;
}


bool
stop_messenger() {
  debug( "Terminating messenger." );

  return true;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
