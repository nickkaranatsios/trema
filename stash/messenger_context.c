/*
 * Messenger context processing.
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


#include <assert.h>
#include "log.h"
#include "checks.h"
#include "hash_table.h"
#include "wrapper.h"
#include "messenger_context.h"



static hash_table *context_db = NULL;
static uint32_t last_transaction_id = 0;


static void
_delete_context( void *key, void *value, void *user_data ) {
  assert( value != NULL );
  UNUSED( key );
  UNUSED( user_data );
  messenger_context *context = value;

  debug( "Deleting a context ( transaction_id = %#x, life_count = %d, user_data = %p ).",
         context->transaction_id, context->life_count, context->user_data );

  delete_hash_entry( context_db, &context->transaction_id );
  xfree( context );
}


void
delete_messenger_context( messenger_context *context ) {
  assert( context != NULL );

  _delete_context( &context->transaction_id, context, NULL );
}


static void
_age_context( void *key, void *value, void *user_data ) {
  assert( value != NULL );
  UNUSED( key );
  UNUSED( user_data );
  messenger_context *context = value;
  context->life_count--;
  if ( context->life_count <= 0 ) {
    delete_messenger_context( context );
  }
}


void
age_context_db( void *user_data ) {
  UNUSED( user_data );

  debug( "Aging context database ( context_db = %p ).", context_db );

  foreach_hash( context_db, _age_context, NULL );
}


messenger_context *
get_context( uint32_t transaction_id ) {
  debug( "Looking up a context ( transaction_id = %#x ).", transaction_id );

  return lookup_hash_entry( context_db, &transaction_id );
}


messenger_context *
insert_context( void *user_data ) {
  messenger_context *context = xmalloc( sizeof( messenger_context ) );

  context->transaction_id = ++last_transaction_id;
  context->life_count = 10;
  context->user_data = user_data;

  debug( "Inserting a new context ( transaction_id = %#x, life_count = %d, user_data = %p ).",
         context->transaction_id, context->life_count, context->user_data );

  messenger_context *old = insert_hash_entry( context_db, &context->transaction_id, context );
  if ( old != NULL ) {
    delete_messenger_context( old );
  }

  return context;
}


void
delete_context_db( void ) {
  debug( "Deleting context database ( context_db = %p ).", context_db );

  if ( context_db != NULL ) {
    foreach_hash( context_db, _delete_context, NULL );
    delete_hash( context_db );
    context_db = NULL;
  }
}


void
init_messenger_context() {
  context_db = create_hash( compare_uint32, hash_uint32 );
}
  

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
