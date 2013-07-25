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


#include "emirates_priv.h"
#include "zrequester_responder.h"


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
lookup_service_route( zlist_t *list, const service_route *sroute,  bool partial_search ) {
  service_route *item = zlist_first( list );

  while ( item ) {
    if ( partial_search ) {
      if ( !memcmp( item->service, sroute->service, strlen( sroute->service ) ) ) {
        return item;
      }
    }
    else {
      if ( !memcmp( item, sroute, sizeof( *item ) ) ) {
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
  
  service_route sroute;
  memcpy( &sroute.service, service, service_size );
  sroute.service[ service_size ] = '\0';
  service_route *found = lookup_service_route( list, &sroute, partial_search );
  if ( found != NULL ) {
    return found->identity;
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
add_service_route( zlist_t **list, const service_route *sroute, bool replace ) {
  if ( *list == NULL ) {
    *list = zlist_new();
  }

  int rc = 0;
  service_route *item = NULL;
  bool partial_search;
  if ( replace ) {
    partial_search = true;
    item = lookup_service_route( *list, sroute, partial_search );
  }
  else {
    partial_search = false;
    item = lookup_service_route( *list, sroute, partial_search );
  }
  if ( !item ) {
    service_route *new_sroute = ( service_route * ) zmalloc( sizeof( *sroute ) );
    memcpy( new_sroute, sroute, sizeof( *new_sroute ) );
    rc = zlist_append( *list, new_sroute );
  }
  else {
    memcpy( item, sroute, sizeof( *item ) );
  }

  return rc;
}


static void
route_add_service_reply( proxy *self, 
                         zmsg_t *msg,
                         zframe_t *client_id_frame,
                         zframe_t *service_frame ) {

  char client_id[ IDENTITY_MAX ];

  memcpy( &client_id, ( char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
  add_client_worker( &self->clients, strdup( client_id ) );

  size_t service_frame_size = zframe_size( service_frame  );
  assert( service_frame_size < SERVICE_MAX );

  service_route sroute;
  memcpy( &sroute.service, ( const char * ) zframe_data( service_frame ), service_frame_size );
  sroute.service[ service_frame_size ] = '\0';
  memcpy( &sroute.identity, client_id, zframe_size( client_id_frame ) );
  sroute.identity[ zframe_size( client_id_frame ) ] = '\0';
#ifndef LOAD_BALANCING
  bool replace = true;
  add_service_route( &self->replies, &sroute, replace );
#else 
  bool replace = false;
  add_service_route( &self->replies, &sroute, replace );
#endif

  // send a reply back to client_id
  zmsg_remove( msg, service_frame );
  zmsg_send( &msg, self->frontend );
}


static void
route_request( proxy *self,
               zframe_t *client_id_frame,
               zframe_t *service_frame,
               zframe_t *data_frame ) {
  size_t service_frame_size = zframe_size( service_frame );
  assert( service_frame_size < SERVICE_MAX );

  char *worker_id = NULL;
  if ( self->requests != NULL ) {
    worker_id = lookup_client_worker_id( self->requests, ( const char * ) zframe_data( service_frame ), service_frame_size );
  }
  else {
    if ( self->workers ) {
      worker_id = get_client_worker( self->workers );
    }
  }
  if ( worker_id != NULL ) {
    char client_id[ IDENTITY_MAX ];
    memcpy( client_id, ( const char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
    client_id[ zframe_size( client_id_frame ) ] = '\0';
printf( "routing request to  worker_id %s from client %s\n", worker_id, client_id );
    zmsg_t *route_msg = zmsg_new(); 
    zmsg_addmem( route_msg, worker_id, strlen( worker_id ) );
    char blank;
    zmsg_addmem( route_msg, &blank, 0 );
    zmsg_addmem( route_msg, ( const char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
    zmsg_addmem( route_msg, &blank, 0 );
    zmsg_addmem( route_msg, REQUEST, strlen( REQUEST ) );
    zmsg_addmem( route_msg, ( const char * ) zframe_data( service_frame ), service_frame_size ); 
    zmsg_addmem( route_msg, ( const char * ) zframe_data( data_frame ), zframe_size( data_frame ) );
    zmsg_send( &route_msg, self->backend );
  }
}


static void
route_client_msg( proxy *self, zmsg_t *msg, zframe_t *client_id_frame, zframe_t *msg_type_frame ) {
  size_t msg_type_frame_size = zframe_size( msg_type_frame );

  zframe_t *service_frame = zmsg_next( msg );
  if ( !msg_is( ADD_SERVICE_REPLY, zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    route_add_service_reply( self, msg, client_id_frame, service_frame );
  }
  else if ( !msg_is( REQUEST, zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    zframe_t *data_frame = zmsg_next( msg );
    route_request( self, client_id_frame, service_frame, data_frame );
  }
}


static void
client_recv( proxy *self, zmsg_t *msg ) {
  zframe_t *client_id_frame = zmsg_first( msg );
  size_t client_id_size = zframe_size( client_id_frame );
  assert( client_id_size < IDENTITY_MAX );


  zframe_t *empty_frame = zmsg_next( msg );
  size_t frame_size = zframe_size( empty_frame );
  assert( frame_size == 0 );

  zframe_t *msg_type_frame = zmsg_next( msg );
  frame_size = zframe_size( msg_type_frame );
  route_client_msg( self, msg, client_id_frame, msg_type_frame );
}


static int
frontend_recv( proxy *self ) {
  zmsg_t *msg = zmsg_recv( self->frontend );
  if ( msg == NULL ) {
    return EINVAL;
  }

  size_t nr_frames = zmsg_size( msg );
  printf( "fe nr_frames %zu\n", nr_frames );
  assert( nr_frames > 2 );

  client_recv( self, msg );

  return 0;
}


static void
route_add_service_request( proxy *self,
                           zmsg_t *msg,
                           zframe_t *worker_id_frame ) {

  char worker_id[ IDENTITY_MAX ];
  size_t worker_id_frame_size = zframe_size( worker_id_frame );
  assert( worker_id_frame_size < IDENTITY_MAX );

  memcpy( worker_id, ( const char * ) zframe_data( worker_id_frame ), worker_id_frame_size );
  worker_id[ worker_id_frame_size ] = '\0';
  add_client_worker( &self->workers, strdup( worker_id ) );

  zframe_t *service_frame = zmsg_next( msg );
  size_t service_frame_size = zframe_size( service_frame  );
  assert( service_frame_size < SERVICE_MAX );

  service_route sroute;
  memcpy( sroute.service, ( const char * ) zframe_data( service_frame ), service_frame_size );
  sroute.service[ service_frame_size ] = '\0';
  memcpy( sroute.identity, worker_id, worker_id_frame_size );
  sroute.identity[ worker_id_frame_size ] = '\0';
#ifndef LOAD_BALANCING
  bool replace = true;
  add_service_route( &self->requests, &sroute, replace );
#else
  bool replace = false;
  add_service_route( &self->requests, &sroute, replace );
#endif
  zmsg_remove( msg, service_frame );
  zmsg_send( &msg, self->backend );
}


static void
route_ready( proxy *self, zframe_t *worker_id_frame ) {
  const char *worker_id = ( const char * ) zframe_data( worker_id_frame );
  add_client_worker( &self->workers, strdup( worker_id ) );
}


static void
route_msg_back_to_worker( proxy *self,
                     zmsg_t *msg,
                     zframe_t *worker_id_frame,
                     zframe_t *msg_type_frame ) {
   
  size_t msg_type_frame_size = zframe_size( msg_type_frame );
  if ( !msg_is( ADD_SERVICE_REQUEST, zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    route_add_service_request( self, msg, worker_id_frame );
  }
  else if ( !msg_is( READY, zframe_data( msg_type_frame ), msg_type_frame_size ) ) {
    route_ready( self, worker_id_frame );
  }
}


static void
route_worker_reply( proxy *self,
                    zmsg_t *msg ) {
  zframe_t *client_id_frame = zmsg_next( msg );
  size_t client_id_frame_size = zframe_size( client_id_frame );
  assert( client_id_frame_size < IDENTITY_MAX );

  zframe_t *empty_frame = zmsg_next( msg );
  size_t empty_frame_size = zframe_size( empty_frame );
  assert( empty_frame_size == 0 );

  zframe_t *msg_type_frame = zmsg_next( msg );

  if ( !msg_is( REPLY, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) || 
    !msg_is( REPLY_TIMEOUT, zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) ) ) {
    char client_id[ IDENTITY_MAX ];
    memcpy( client_id, ( const char * ) zframe_data( client_id_frame ), zframe_size( client_id_frame ) );
    client_id[ zframe_size( client_id_frame ) ] = '\0';
printf( "routing worker reply to client id %s\n", client_id );
    zframe_t *service_frame = zmsg_next( msg );
    size_t service_frame_size = zframe_size( service_frame );
    assert( service_frame_size != 0 );
        
    zmsg_t *route_msg = zmsg_new(); 
    zmsg_addmem( route_msg, ( const char * ) zframe_data( client_id_frame ), client_id_frame_size );
    zmsg_addstr( route_msg, "" );
    zmsg_addmem( route_msg, ( const char * ) zframe_data( msg_type_frame ), zframe_size( msg_type_frame ) );
    zmsg_addmem( route_msg, ( const char * ) zframe_data( service_frame ), service_frame_size );
    if ( zframe_more( service_frame ) ) {
      zframe_t *data_frame = zmsg_next( msg );
      zmsg_addmem( route_msg, ( const char * ) zframe_data( data_frame ), zframe_size( data_frame ) );
    }
    zmsg_send( &route_msg, self->frontend ); 
  }
}


static void
worker_recv( proxy *self, zmsg_t *msg ) {
  zframe_t *worker_id_frame = zmsg_first( msg );

  zframe_t *empty_frame = zmsg_next( msg );
  size_t empty_frame_size = zframe_size( empty_frame );
  assert( empty_frame_size == 0 );

  size_t nr_frames = zmsg_size( msg );
  if ( nr_frames == 3 || nr_frames == 4 ) {
    zframe_t *msg_type_frame = zmsg_next( msg );
    route_msg_back_to_worker( self, msg, worker_id_frame, msg_type_frame );
  }
  else if ( nr_frames >= 5 ) {
    route_worker_reply( self, msg );   
  }
}


static int
backend_recv( proxy *self ) {
  zmsg_t *msg = zmsg_recv( self->backend );
  if ( msg == NULL ) {
    return EINVAL;
  }
  size_t nr_frames = zmsg_size( msg );
  printf( "be nr_frames %zu\n", nr_frames );
  worker_recv( self, msg );

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


static int
fe_requester_responder( proxy *proxy ) {
  int rc;
  proxy->frontend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->frontend == NULL ) {
    log_err( "Failed to create frontend ROUTER socket errno: %d:%s", errno, zmq_strerror( errno ) );
    return EINVAL;
  }

  rc = zsocket_bind( proxy->frontend, "tcp://*:%u", REQUESTER_BASE_PORT );
  if ( rc < 0 ) {
    log_err( "Failed to bind ROUTER frontend socket %p errno: %d:%s" ,proxy->frontend, errno, zmq_strerror( errno ) );
    return EINVAL;
  }

  proxy->backend = zsocket_new( proxy->ctx, ZMQ_ROUTER );
  if ( proxy->backend == NULL ) {
    log_err( "Failed to create backend socket errno: %d:%s\n", errno, zmq_strerror( errno ) );
    return EINVAL;
  }

  rc = zsocket_bind( proxy->backend,  "tcp://*:%u", RESPONDER_BASE_PORT );
  if ( rc < 0 ) {
    log_err( "Failed to bind DEALER backend socket %p errno: %d:%s", proxy->backend, errno, zmq_strerror( errno ) );
    return EINVAL;
  }


  return 0;
}


static int
proxy_fe( proxy *proxy ) {
  return fe_requester_responder( proxy );
}


int
main (int argc, char *argv []) {
  UNUSED( argc );
  UNUSED( argv );
  proxy *self = ( proxy * ) zmalloc( sizeof( *self ) );
  memset( self, 0, sizeof( *self ) );
  self->ctx = zctx_new();

  if ( !proxy_fe( self ) ) {
    start_fe_be( self );
  }

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
