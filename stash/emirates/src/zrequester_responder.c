/*
 * Copyright (C) 2013 NEC Corporation
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


#include "emirates.h"
#include "zhelpers.h"


static const char *
lookup_identity( zlist_t *list, const char *id ) {
  char *item = zlist_first( list );
  while ( item != NULL ) {
    if ( !strcmp( item, id ) ) {
      return item;
    }
    item = zlist_next( list );
  }

  return NULL;
}


static service_route *
lookup_service_route( zlist_t *list, 
  const char *service,
  const size_t service_size,
  const char *identity, 
  const size_t identity_size,
  bool partial_search ) {
  service_route *item = zlist_first( list );

  while ( item ) {
    if ( partial_search == true ) {
      if ( !memcmp( item->service, service, service_size ) ) {
        return item;
      }
    }
    else {
      if ( !memcmp( item->service, service, service_size ) && !( memcmp( item->identity, identity, identity_size ) ) ) {
        return item;
      }
    }
    item = zlist_next( list );
  }

  return NULL;
}


static char *
lookup_client_worker_id( zlist_t *list, const char *service, const size_t service_size ) {
  bool partial_search = true;
  service_route *sroute = lookup_service_route( list, service, service_size, "", 0, partial_search );
  if ( sroute != NULL ) {
    return sroute->identity;
  }

  return NULL;
}


static int
add_client_worker( zlist_t **list, char *id ) {
  if ( *list == NULL ) {
    *list = zlist_new();
  }

  int rc = 0;
  const char *item = lookup_identity( *list, id );
  if ( !item ) {
    rc = zlist_append( *list, id );
  }
  else {
    item = id;
  }

  return rc;
}


static char *
get_client_worker( zlist_t *list ) {
  if ( list == NULL ) {
    return NULL;
  }
  return zlist_first( list );
}


int
add_service_route( zlist_t **list, const char *service, const size_t service_size, const char *identity, const size_t identity_size ) {
  if ( *list == NULL ) {
    *list = zlist_new();
  }

  int rc = 0;
  service_route *item = lookup_service_route( *list, service, service_size, identity, identity_size, false );
  if ( !item ) {
    service_route *sroute = ( service_route * ) zmalloc( sizeof( *sroute ) );
    memcpy( sroute->service, service, service_size );
    sroute->service[ service_size + 1 ] = '\0';
    memcpy( sroute->identity, identity, identity_size );
    sroute->identity[ identity_size + 1 ] = '\0';
    rc = zlist_append( *list, sroute );
  }
  else {
    memcpy( item->service, service, service_size );
    item->service[ service_size + 1 ] = '\0';
    memcpy( item->identity, identity, identity_size );
    item->identity[ identity_size + 1 ] = '\0';
  }

  return rc;
}


static int
frontend_recv( proxy *self ) {
  zmsg_t *msg = zmsg_recv( self->frontend );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  printf( "fe nr_frames %d\n", nr_frames );

  if ( nr_frames ) {
    zframe_t *client_id_frame = zmsg_first( msg );
    size_t client_id_size = zframe_size( client_id_frame );
    assert( client_id_size < IDENTITY_MAX - 1 );

    char *client_id;
    client_id = ( char * ) zframe_data( client_id_frame );
    printf( "fe client id %s\n", client_id );

    zframe_t *empty_frame = zmsg_next( msg );
    size_t frame_size = zframe_size( empty_frame );
    assert( frame_size == 0 );

    zframe_t *msg_type_frame = zmsg_next( msg );
    frame_size = zframe_size( msg_type_frame );

    if ( !msg_is( ADD_SERVICE_REPLY, zframe_data( msg_type_frame ), frame_size ) ) {
      add_client_worker( &self->clients, strdup( client_id ) );

      zframe_t *service = zmsg_next( msg );
      size_t service_size = zframe_size( service  );
      assert( service_size < SERVICE_MAX - 1 );

      add_service_route( &self->replies, ( const char * ) zframe_data( service ), service_size, client_id, client_id_size );

      // send a reply back to client_id
      zmsg_remove( msg, service );
      zmsg_send( &msg, self->frontend );
    }
    else if ( !msg_is( REQUEST, zframe_data( msg_type_frame ), frame_size ) ) {

      zframe_t *service_frame = zmsg_next( msg );
      frame_size = zframe_size( service_frame );
      assert( frame_size != 0 );

      if ( self->requests != NULL ) {
        char *worker_id = lookup_client_worker_id( self->requests, ( const char * ) zframe_data( service_frame ), frame_size );
        if ( worker_id != NULL ) {
          zmsg_t *route_msg = zmsg_new(); 
          zmsg_addmem( route_msg, worker_id, strlen( worker_id ) );
          char blank;
          zmsg_addmem( route_msg, &blank, 0 );
          zmsg_addmem( route_msg, ( const char * ) zframe_data( client_id_frame ), client_id_size );
          zmsg_addmem( route_msg, &blank, 0 );
          zmsg_addmem( route_msg, REQUEST, strlen( REQUEST ) );
          zmsg_addmem( route_msg, ( const char * ) zframe_data( service_frame ), frame_size ); 
          zmsg_send( &route_msg, self->backend );
          zmsg_destroy( &route_msg );
#ifdef TEST
          s_sendmore( self->backend, worker_id );
          s_sendmore( self->backend, "" );
          s_sendmore( self->backend, client_id );
          s_sendmore( self->backend, "" );
          s_sendmore( self->backend, REQUEST );
          s_send( self->backend, "service data" );
#endif
        }
      }
      else {
        if ( self->workers ) {
          char *worker_id = get_client_worker( self->workers );
          zmsg_t *route_msg = zmsg_new(); 
          zmsg_addmem( route_msg, worker_id, strlen( worker_id ) );
          char blank;
          zmsg_addmem( route_msg, &blank, 0 );
          zmsg_addmem( route_msg, client_id, strlen( client_id ) );
          zmsg_addmem( route_msg, &blank, 0 );
          zmsg_addmem( route_msg, REQUEST, strlen( REQUEST ) );
          zmsg_addmem( route_msg, ( const char * ) zframe_data( service_frame ), frame_size ); 
          zmsg_send( &route_msg, self->backend );
          zmsg_destroy( &route_msg );
        }
      }
    }
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
send_reply( void *socket, const char *reply ) {
  zmsg_t *msg = zmsg_new();
  zmsg_addstr( msg, reply );
  zmsg_send( &msg, socket );
  zmsg_destroy( &msg );
}


static int
backend_recv( proxy *self ) {
  zmsg_t *msg = zmsg_recv( self->backend );
  if ( msg == NULL ) {
    return EINVAL;
  }
  
  size_t nr_frames = zmsg_size( msg );
  printf( "be frames %u\n", nr_frames );
  if ( nr_frames ) {
    zframe_t *worker_id_frame = zmsg_first( msg );
    size_t worker_id_size = zframe_size( worker_id_frame );
    assert( worker_id_size < IDENTITY_MAX - 1 );

    char *worker_id;
    worker_id = ( char * ) zframe_data( worker_id_frame );
    printf( "be worker id %s\n", worker_id );

    zframe_t *empty_frame = zmsg_next( msg );
    size_t frame_size = zframe_size( empty_frame );
    assert( frame_size == 0 );
   
    if ( nr_frames == 3 || nr_frames == 4 ) {
      zframe_t *msg_type_frame = zmsg_next( msg );
      frame_size = zframe_size( msg_type_frame );
      if ( !msg_is( ADD_SERVICE_REQUEST, zframe_data( msg_type_frame ), frame_size ) ) {
        add_client_worker( &self->workers, strdup( worker_id ) );

        zframe_t *service = zmsg_next( msg );
        size_t service_size = zframe_size( service  );
        assert( service_size < SERVICE_MAX -1 );

        add_service_route( &self->requests, ( const char * ) zframe_data( service ), service_size, worker_id, worker_id_size );
        zmsg_remove( msg, service );
        zmsg_send( &msg, self->backend );
      }
      else if ( !msg_is( READY, zframe_data( msg_type_frame ), frame_size ) ) {
        add_client_worker( &self->workers, strdup( worker_id ) );
      }
    }
    else {
      zframe_t *client_id_frame = zmsg_next( msg );
      size_t client_id_size = zframe_size( client_id_frame );
      assert( frame_size < IDENTITY_MAX - 1 );

      empty_frame = zmsg_next( msg );
      frame_size = zframe_size( empty_frame );
      assert( frame_size == 0 );

      zframe_t *msg_type_frame = zmsg_next( msg );
      size_t msg_type_size = zframe_size( msg_type_frame );

      if ( !msg_is( REPLY, zframe_data( msg_type_frame), frame_size ) ) {
        zframe_t *service_frame = zmsg_next( msg );
        size_t service_size = zframe_size( service_frame );
        assert( service_size != 0 );
printf( "sending a reply\n" );
        
        zmsg_t *route_msg = zmsg_new(); 
        zmsg_addmem( route_msg, ( const char * ) zframe_data( client_id_frame ), client_id_size );
        zmsg_addstr( route_msg, "" );
        zmsg_addmem( route_msg, ( const char * ) zframe_data( msg_type_frame ), msg_type_size );
        zmsg_addmem( route_msg, ( const char * ) zframe_data( service_frame ), service_size );
        zmsg_send( &route_msg, self->frontend ); 
        zmsg_destroy( &route_msg );
      }
    }
  }
  zmsg_destroy( &msg );

  return 0;
}


static void
start_fe_be( proxy *proxy ) {
  while ( 1 ) {
    zmq_pollitem_t items[] = {
      { proxy->frontend, 0, ZMQ_POLLIN, 0 },
      { proxy->backend, 0, ZMQ_POLLIN, 0 }
    };
    int poll_size = 2;
    int rc = zmq_poll( items, poll_size, -1 );
    if ( rc == -1 ) {
      break;
    }
    if ( items[ 0 ].revents & ZMQ_POLLIN ) {
      rc = frontend_recv( proxy );
    }
    if ( items[ 1 ].revents & ZMQ_POLLIN ) {
      rc = backend_recv( proxy );
    }
    if ( rc == -1 ) {
      break;
    }
  }
}

static void
fe_requester_responder( proxy *proxy ) {
  int rc;
  proxy->frontend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->frontend == NULL ) {
    log_err( "Failed to create frontend ROUTER socket errno: %d:%s", errno, zmq_strerror( errno ) );
    return;
  }

  rc = zsocket_bind( proxy->frontend, "tcp://*:%u", REQUESTER_BASE_PORT );
  if ( rc < 0 ) {
    log_err( "Failed to bind ROUTER frontend socket %p errno: %d:%s" ,proxy->frontend, errno, zmq_strerror( errno ) );
    return;
  }

  proxy->backend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->backend == NULL ) {
    log_err( "Failed to create backend socket errno: %d:%s\n", errno, zmq_strerror( errno ) );
    return;
  }

  rc = zsocket_bind( proxy->backend,  "tcp://*:%u", RESPONDER_BASE_PORT );
  if ( rc < 0 ) {
    log_err( "Failed to bind DEALER backend socket %p errno: %d:%s", proxy->backend, errno, zmq_strerror( errno ) );
    return;
  }
}


static void
proxy_fe( proxy *proxy ) {
  fe_requester_responder( proxy );
}


int
main (int argc, char *argv []) {
  proxy *self = ( proxy * ) zmalloc( sizeof( *self ) );
  memset( self, 0, sizeof( *self ) );
  self->ctx = zctx_new();

  proxy_fe( self );

  start_fe_be( self );

  log_err( "interrupted" );
  zctx_destroy( &self->ctx );
  free( self );
  
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
