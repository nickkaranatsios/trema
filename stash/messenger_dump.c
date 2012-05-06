/*
 * Handles the message dumping.
 *
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
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


#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include "log.h"
#include "wrapper.h"
#include "messenger.h"
#include "messenger_dump.h"
#include "messenger_send.h"


/*
 * file scope gloabl definitions
 */
static char *_dump_service_name = NULL;
static char *_dump_app_name = NULL;


void
send_dump_message( uint16_t dump_type, const char *service_name, const void *data, uint32_t data_len ) {
  assert( service_name != NULL );

  debug( "Sending a dump message ( dump_type = %#x, service_name = %s, data = %p, data_len = %u ).",
         dump_type, service_name, data, data_len );

  size_t service_name_len, app_name_len;
  char *dump_buf, *p;
  message_dump_header *dump_hdr;
  size_t dump_buf_len;

  if ( _dump_service_name == NULL ) {
    debug( "Dump service name is not set." );
    return;
  }
  if ( strcmp( service_name, _dump_service_name ) == 0 ) {
    debug( "Source service name and destination service name are the same ( service name = %s ).", service_name );
    return;
  }

  struct timespec now;
  if ( clock_gettime( CLOCK_REALTIME, &now ) == -1 ) {
    error( "Failed to retrieve system-wide real-time clock ( %s [%d] ).", strerror( errno ), errno );
    return;
  }

  service_name_len = strlen( service_name ) + 1;
  app_name_len = strlen( _dump_app_name ) + 1;
  dump_buf_len = sizeof( message_dump_header ) + app_name_len + service_name_len + data_len;
  dump_buf = xmalloc( dump_buf_len );
  dump_hdr = ( message_dump_header * ) dump_buf;

  // header
  dump_hdr->sent_time.sec = htonl( ( uint32_t ) now.tv_sec );
  dump_hdr->sent_time.nsec = htonl( ( uint32_t ) now.tv_nsec );
  dump_hdr->app_name_length = htons( ( uint16_t ) app_name_len );
  dump_hdr->service_name_length = htons( ( uint16_t ) service_name_len );
  dump_hdr->data_length = htonl( data_len );

  // app name
  p = dump_buf;
  p += sizeof( message_dump_header );
  memcpy( p, _dump_app_name, app_name_len );

  // service name
  p += app_name_len;
  memcpy( p, service_name, service_name_len );

  // data
  p += service_name_len;
  memcpy( p, data, data_len );

  // send
  send_message( _dump_service_name, dump_type, dump_buf, dump_buf_len );

  xfree( dump_buf );
}


void
start_messenger_dump( const char *dump_app_name, const char *dump_service_name ) {
  assert( dump_app_name != NULL );
  assert( dump_service_name != NULL );

  debug( "Starting a message dumper ( dump_app_name = %s, dump_service_name = %s ).",
         dump_app_name, dump_service_name );

  if ( messenger_dump_enabled() ) {
    stop_messenger_dump();
  }
  _dump_service_name = xstrdup( dump_service_name );
  _dump_app_name = xstrdup( dump_app_name );
}


bool
messenger_dump_enabled( void ) {
  if ( _dump_service_name != NULL && _dump_app_name != NULL ) {
    return true;
  }

  return false;
}


void
stop_messenger_dump( void ) {
  assert( _dump_service_name != NULL );
  assert( _dump_app_name != NULL );

  debug( "Terminating a message dumper ( dump_app_name = %s, dump_service_name = %s ).",
         _dump_app_name, _dump_service_name );

  delete_send_service( _dump_service_name );

  xfree( _dump_service_name );
  _dump_service_name = NULL;

  xfree( _dump_app_name );
  _dump_app_name = NULL;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
