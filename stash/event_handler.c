/*
 * Author: Jari Sundell
 *
 * Copyright (C) 2011 axsh Ltd.
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


#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include "checks.h"
#include "event_handler.h"
#include "log.h"
#include "timer.h"
#include "wrapper.h"


#ifdef UNIT_TESTING

#define static

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

#ifdef execute_timer_events
#undef execute_timer_events
#endif
#define execute_timer_events mock_execute_timer_events
extern void mock_execute_timer_events( int *next_timeout_usec );

#endif // UNIT_TESTING


typedef struct {
  int fd;
  int thread_id;
  event_fd_callback read_callback;
  event_fd_callback write_callback;
  void *read_data;
  void *write_data;
} event_fd;


enum {
  EVENT_HANDLER_INITIALIZED = 0x1,
  EVENT_HANDLER_RUNNING = 0x2,
  EVENT_HANDLER_STOP = 0x4,
  EVENT_HANDLER_FINALIZED = 0x8
};

int event_handler_state = 0;

event_fd event_list[ FD_SETSIZE ];
event_fd *event_last;

event_fd *event_fd_set[ FD_SETSIZE ];

fd_set event_read_set;
fd_set event_write_set;
fd_set current_read_set;
fd_set current_write_set;

int fd_set_size = 0;

external_callback_t external_callback = ( external_callback_t ) NULL;

#define MAX_THREADS 8
typedef struct _threaded_pool {
  pthread_t thread_id;
  int thread_num;
  int fd;
  event_fd_callback callback;
  void *data;
} threaded_pool;

static threaded_pool pool[ MAX_THREADS ];
static int nr_threads;
static pthread_mutex_t pool_mutex;

typedef void *( *thread_routine_t)( void * );
typedef void *thread_addr_t;

#define thread_create( t, start, arg, msg ) \
{ \
    int errcode; \
    \
    if ( ( errcode = pthread_create( t, \
             NULL, \
             ( thread_routine_t ) (start), \
             ( thread_addr_t ) ( arg ) ) ) ) { \
        error( "%s: %s\n", msg, strerror( errcode ) ); \
    } \
}


#define thread_wait( t, exitcode, msg ) \
{ \
    thread_addr_t code; \
    int errcode; \
    \
    if ( ( errcode = pthread_join( *( t ), \
          ( thread_addr_t ) ( ( exitcode ) == NULL ? &code : ( exitcode ) ) ) ) ) { \
        error( "%s: %s\n",msg, strerror( errcode ) ); \
    } \
}


static inline void pool_lock( void ) {
  pthread_mutex_lock( &pool_mutex );
}


static inline void pool_unlock( void ) {
  pthread_mutex_unlock( &pool_mutex );
}


static void
update_pool( void *index ) {
  int i;
  int *pool_idx = ( int * )index;

  pool_lock();
  for ( i = 0; i < MAX_THREADS; i++ ) {
    if ( i == *pool_idx && pool[ i ].thread_num ) {
      pool[ i ].thread_num = 0;
      nr_threads--;
    }
  }
  debug( "update_pool is called pool_idx %d nr_threads = %d" , *pool_idx, nr_threads );
  pool_unlock();
}


int get_next_thread() {
  int i = -1;

  pool_lock();
  for ( i = 0; i < MAX_THREADS; i++ ) {
    if ( !pool[ i ].thread_num ) {
      pool[ i ].thread_num = i + 1;
      nr_threads++;
      break;
    }
  }
  pool_unlock();
  if ( i == -1  || i == MAX_THREADS ) {
    error( "failed to get_next_thread %d", i );

#ifdef TEST
  int status;
    pool_lock();
    for ( i = 0;  i < MAX_THREADS; i++ ) {
      if ( pool[ i ].thread_num ) {
        thread_wait( &pool[ i ].thread_id, ( thread_addr_t ) &status, "thread_wait" );
      }
    }
    error(" completed wait" );
    pool_unlock();
#endif

    return -1;
  }
  debug( "returning next_thread %d", i );
  return i;
}
  



void 
*invoke_callback( void * args ) {
  threaded_pool *pool = ( threaded_pool * ) args;
  pool_lock();
  int pool_idx = pool->thread_num - 1;
  pool_unlock();

  pthread_cleanup_push( update_pool, ( void * ) &pool_idx );
  pool->callback( pool->fd, pool->data );
  pthread_cleanup_pop( 1 );

#ifdef TEST
  if ( pool->thread_num == 2 )
    update_pool( ( void * ) &pool->thread_num );
#endif

  int ret = 1;
  return ( void * ) ( intptr_t ) ret;
}

static void
_init_event_handler() {
  event_last = event_list;
  event_handler_state = EVENT_HANDLER_INITIALIZED;

  memset( event_list, 0, sizeof( event_fd ) * FD_SETSIZE );
  memset( event_fd_set, 0, sizeof( event_fd * ) * FD_SETSIZE );

  FD_ZERO( &event_read_set );
  FD_ZERO( &event_write_set );
}
void ( *init_event_handler )() = _init_event_handler;


static void
_finalize_event_handler() {
  if ( event_last != event_list ) {
    warn( "Event Handler finalized with %i fd event handlers still active. (%i, ...)",
          ( event_last - event_list ), ( event_last > event_list ? event_list->fd : -1 ) );
    return;
  }

  event_handler_state = EVENT_HANDLER_FINALIZED;
}
void ( *finalize_event_handler )() = _finalize_event_handler;


static bool
_run_event_handler_once( int timeout_usec ) {
  if ( external_callback != NULL ) {
    external_callback_t callback = external_callback;
    external_callback = NULL;

    callback();
  }

  memcpy( &current_read_set, &event_read_set, sizeof( fd_set ) );
  memcpy( &current_write_set, &event_write_set, sizeof( fd_set ) );

  struct timeval timeout;
  timeout.tv_sec = timeout_usec / 1000000;
  timeout.tv_usec = timeout_usec % 1000000;
  int set_count = select( fd_set_size, &current_read_set, &current_write_set, NULL, &timeout );

  if ( set_count == -1 ) {
    if ( errno == EINTR ) {
      return true;
    }
    error( "Failed to select ( errno = %s [%d] ).", strerror( errno ), errno );
    return false;

  }
  else if ( set_count == 0 ) {
    // timed out
    return true;
  }

  event_fd *event_itr = event_list;
  int pool_idx;

  while ( event_itr < event_last ) {
    event_fd current_event = *event_itr;

    if ( FD_ISSET( current_event.fd, &current_write_set ) ) {

#ifdef TEST
      current_event.write_callback( current_event.fd, current_event.write_data );
#endif

        pool_idx = get_next_thread();
        if ( pool_idx != -1 ) {
          pool[ pool_idx ].fd = current_event.fd;
          pool[ pool_idx ].callback = current_event.write_callback;
          pool[ pool_idx ].data = current_event.write_data;
          
          thread_create( &pool[ pool_idx ].thread_id, invoke_callback, &pool[ pool_idx ], "thread_create" );

#ifdef TEST
            invoke_callback( &pool[ pool_idx ] );
#endif

        }
    }

    if ( FD_ISSET( current_event.fd, &current_read_set ) ) {
      current_event.read_callback( current_event.fd, current_event.read_data );
    }

    // In the rare cases the current fd is closed, a new one is opened
    // with the same fd and is put in the same location we can just
    // wait for the next select call.
    if ( current_event.fd == event_itr->fd ) {
      event_itr = event_itr + 1;
    }
    else {
      debug( "run_event_handler_once: event fd is changed ( current = %d, new = %d )", current_event.fd, event_itr->fd ) ;
    }
  }

  return true;
}
bool ( *run_event_handler_once )( int ) = _run_event_handler_once;


static bool
_start_event_handler() {
  debug( "Starting event handler." );

  event_handler_state |= EVENT_HANDLER_RUNNING;

  int timeout_usec;
  while ( !( event_handler_state & EVENT_HANDLER_STOP ) ) {
    execute_timer_events( &timeout_usec );

    if ( !run_event_handler_once( timeout_usec ) ) {
      error( "Failed to run main loop." );
      return false;
    }
  }

  event_handler_state &= ~EVENT_HANDLER_RUNNING;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init( &attr );
  pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE_NP );
  pthread_mutex_init( &pool_mutex, &attr );

  debug( "Event handler terminated." );
  return true;
}
bool ( *start_event_handler )() = _start_event_handler;


static void
_stop_event_handler() {
  debug( "Terminating event handler." );
  event_handler_state |= EVENT_HANDLER_STOP;
}
void ( *stop_event_handler )() = _stop_event_handler;



static void
_set_fd_handler( int fd,
                 event_fd_callback read_callback, void *read_data,
                 event_fd_callback write_callback, void *write_data ) {
  debug( "Adding event handler for fd %i, %p, %p.", fd, read_callback, write_callback );

  // Currently just issue critical warnings instead of killing the
  // program."
  if ( event_fd_set[ fd ] != NULL ) {
    error( "Tried to add an already active fd event handler." );
    return;
  }

  if ( fd < 0 || fd >= FD_SETSIZE ) {
    error( "Tried to add an invalid fd." );
    return;
  }

  if ( event_last >= event_list + FD_SETSIZE ) {
    error( "Event handler list in invalid state." );
    return;
  }

  event_last->fd = fd;
  event_last->read_callback = read_callback;
  event_last->write_callback = write_callback;
  event_last->read_data = read_data;
  event_last->write_data = write_data;

  event_fd_set[ fd ] = event_last++;

  if ( fd >= fd_set_size ) {
    fd_set_size = fd + 1;
  }
}
void ( *set_fd_handler )( int fd, event_fd_callback read_callback, void *read_data, event_fd_callback write_callback, void *write_data ) = _set_fd_handler;


static void
_delete_fd_handler( int fd ) {
  debug( "Deleting event handler for fd %i.", fd );

  event_fd* event = event_list;

  while ( event != event_last && event->fd != fd ) {
    event++;
  }

  if ( event >= event_last || event_fd_set[ fd ] == NULL ) {
    error( "Tried to delete an inactive fd event handler." );
    return;
  }

  if ( FD_ISSET( fd, &event_read_set ) ) {
    error( "Tried to delete an fd event handler with active read notification." );
    //    return;
    FD_CLR( fd, &event_read_set );
  }

  if ( FD_ISSET( fd, &event_write_set ) ) {
    error( "Tried to delete an fd event handler with active write notification." );
    //    return;
    FD_CLR( fd, &event_write_set );
  }

  FD_CLR( fd, &current_read_set );
  FD_CLR( fd, &current_write_set );

  event_fd_set[ fd ] = NULL;

  if ( event != --event_last ) {
    memcpy( event, event_last, sizeof( event_fd ) );
    event_fd_set[ event->fd ] = event;
  }

  memset( event_last, 0, sizeof( event_fd ) );
  event_last->fd = -1;

  if ( fd == ( fd_set_size  - 1 ) ) {
    int i;
    for ( i = ( fd_set_size - 2 ); i >= 0; --i ) {
      if ( event_fd_set[ i ] != NULL ) {
        break;
      }
    }
    fd_set_size = i + 1;
  }
}
void ( *delete_fd_handler )( int fd ) = _delete_fd_handler;


static void
_set_readable( int fd, bool state ) {
  if ( fd < 0 || fd >= FD_SETSIZE ) {
    error( "Invalid fd to set_readable call; %i.", fd );
    return;
  }

  if ( event_fd_set[ fd ] == NULL || event_fd_set[ fd ]->read_callback == NULL ) {
    error( "Found fd in invalid state in set_readable; %i, %p.", fd, event_fd_set[ fd ] );
    return;
  }

  if ( state ) {
    FD_SET( fd, &event_read_set );
  }
  else {
    FD_CLR( fd, &event_read_set );
    FD_CLR( fd, &current_read_set );
  }
}
void ( *set_readable )( int fd, bool state ) = _set_readable;


static void
_set_writable( int fd, bool state ) {
  if ( fd < 0 || fd >= FD_SETSIZE ) {
    error( "Invalid fd to notify_writeable_event call; %i.", fd );
    return;
  }

  if ( event_fd_set[ fd ] == NULL || event_fd_set[ fd ]->write_callback == NULL ) {
    error( "Found fd in invalid state in notify_writeable_event; %i, %p.", fd, event_fd_set[ fd ] );
    return;
  }

  if ( state ) {
    FD_SET( fd, &event_write_set );
  }
  else {
    FD_CLR( fd, &event_write_set );
    FD_CLR( fd, &current_write_set );
  }
}
void ( *set_writable )( int fd, bool state ) = _set_writable;


static bool
_readable( int fd ) {
  return FD_ISSET( fd, &event_read_set );
}
bool ( *readable )( int fd ) = _readable;


static bool
_writable( int fd ) {
  return FD_ISSET( fd, &event_write_set );
}
bool ( *writable )( int fd ) = _writable;


static bool
_set_external_callback( external_callback_t callback ) {
  if ( external_callback != NULL ) {
    return false;
  }

  external_callback = callback;
  return true;
}
bool ( *set_external_callback )( external_callback_t callback ) = _set_external_callback;


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */