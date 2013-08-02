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


#include "db_mapper.h"
#include "mapper_priv.h"


/*
 * sets the Redis cache using the set comand.
 */
void
cache_set( redisContext *rcontext,
           table_info *tinfo,
           const char *json,
           ref_data *ref,
           strbuf *sb ) {
  strbuf_reset( sb );
  strbuf_addf( sb, "%s|", tinfo->name );
  ref->command = sb;
  foreach_primary_key( tinfo, redis_clause, ref );
  size_t slen = 1;
  strbuf_rtrimn( sb, slen );
  size_t klen = sb->len; 
  strbuf_addf( sb, " %s", json );

  redisReply *reply = redisCommand( rcontext, "SET %b %b", sb->buf, klen, sb->buf + klen + 1, sb->len - klen - 1 ); 
  if ( reply != NULL ) {
    if ( reply->type != REDIS_REPLY_STATUS ) {
      log_err( "Failed to update the redis cache set %s", reply->str );
    }
    freeReplyObject( reply );
  }
  else {
    log_err( "Failed to update the redis cache %s", sb->buf );
  }
  // an example of how to read a cache value
  // reply = redisCommand( rcontext, "GET %s", "fruits|name|jackfruit" );
}


/*
 * Deletes a Redis cache value. 
 */
void
cache_del( redisContext *rcontext,
           table_info *tinfo,
           ref_data *ref,
           strbuf *sb ) {
  strbuf_reset( sb );
  strbuf_addf( sb, "%s|", tinfo->name );
  ref->command = sb;
  foreach_primary_key( tinfo, redis_clause, ref );
  size_t slen = 1;
  strbuf_rtrimn( sb, slen );
  size_t klen = sb->len; 

  redisReply *reply = redisCommand( rcontext, "DEL %b", sb->buf, klen ); 
  if ( reply != NULL ) {
    if ( reply->type != REDIS_REPLY_INTEGER ) {
      log_err( "Failed to update the redis cache set %s", reply->str );
    }
    freeReplyObject( reply );
  }
  else {
    log_err( "Failed to update the redis cache %s", sb->buf );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
