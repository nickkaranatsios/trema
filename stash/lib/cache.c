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


#include <string.h>
#include <hiredis/hiredis.h>
#include "log_writer.h"


/*
 * sets the Redis cache using the set comand.
 */
void
redis_cache_set( redisContext *rcontext,
           const char *key,
           const char *value ) {
  size_t klen = strlen( key );
  size_t value_len = strlen( value );

  redisReply *reply = redisCommand( rcontext, "SET %b %b", key, klen, value, value_len ); 
  if ( reply != NULL ) {
    if ( reply->type != REDIS_REPLY_STATUS ) {
      log_err( "Failed to update the redis cache set %s", reply->str );
    }
    freeReplyObject( reply );
  }
  else {
    log_err( "Failed to update the redis cache %s", key );
  }
  // an example of how to read a cache value
  // reply = redisCommand( rcontext, "GET %s", "fruits|name|jackfruit" );
}


/*
 * Deletes a Redis cache value. 
 */
void
redis_cache_del( redisContext *rcontext, const char *key ) {
  size_t klen = strlen( key );

  redisReply *reply = redisCommand( rcontext, "DEL %b", key, klen ); 
  if ( reply != NULL ) {
    if ( reply->type != REDIS_REPLY_INTEGER ) {
      log_err( "Failed to update the redis cache set %s", reply->str );
    }
    freeReplyObject( reply );
  }
  else {
    log_err( "Failed to update the redis cache %s", key );
  }
}


/*
 * Retrieves a Redis cache value (redisReply object) given by its key.
 * The caller should free the redis reply object after its use.
 * For example: GET fruits|name|jackfruit|country|Thailand
 * The fruits is the database table fruits. Name and country are the two
 * primary keys of the table.
 */
redisReply *
redis_cache_get( redisContext *rcontext, const char *key ) {
  redisReply *reply = redisCommand( rcontext, "GET %s", key );
  if ( reply != NULL ) {
    if ( reply->type == REDIS_REPLY_NIL ) {
      log_err( "Failed to read from the redis cache using key(%s)", key ); 
      freeReplyObject( reply );
      return NULL;
    }
  }

  return reply;
}


/*
 * Connects to the redis server and returns a redisContext object.
 * The redis server should be started from the redis source code installation
 * directory (redis-<version>/src) for example ./redis-server &
 * How to compile and install the read server read the README file under the
 * redis-<version> directory.
 */
redisContext *
redis_cache_connect( void ) {
  struct timeval timeout = { 1, 0 }; // 1 second
  redisContext *rcontext = redisConnectWithTimeout( "127.0.0.1", 6379, timeout );
  if ( rcontext == NULL || rcontext->err ) {
    if ( rcontext ) {
      log_err( "Connection to redis server failed error %s", rcontext->errstr ); 
      redisFree( rcontext );
      rcontext = NULL;
    }
    if ( rcontext == NULL ) {
      log_err( "Connection to redis server failed perhaps redis server is not started" );
    }
  }

  return rcontext;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
