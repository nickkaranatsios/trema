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


#include "jedex.h"


static pthread_key_t generic_union_key;
static pthread_once_t generic_union_key_once = PTHREAD_ONCE_INIT;


static void
make_generic_union_key( void ) {
  pthread_key_create( &generic_union_key, free );
}


static int
jedex_generic_union_reset( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  jedex_generic_union *self = ( jedex_generic_union * ) vself;
  /* Keep the same branch selected, for the common case that we're
   * about to reuse it. */
  if ( self->discriminant >= 0 ) {
    jedex_value value = {
      jedex_generic_union_branch_iface( iface, self ),
      jedex_generic_union_branch( self )
    };
    return jedex_value_reset(&value);
  }

  return 0;
}


static jedex_type
jedex_generic_union_get_type( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( viface );
  UNUSED( vself );

  return JEDEX_UNION;
}


static jedex_schema *
jedex_generic_union_get_schema( const jedex_value_iface *viface, const void *vself ) {
  UNUSED( vself );

  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  return iface->schema;
}


static int
jedex_generic_union_get_discriminant( const jedex_value_iface *viface, const void *vself, int *out ) {
  UNUSED( viface );

  const jedex_generic_union *self = ( const jedex_generic_union * ) vself;
  *out = self->discriminant;

  return 0;
}


static int
jedex_generic_union_get_current_branch( const jedex_value_iface *viface, const void *vself, jedex_value *branch ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  const jedex_generic_union *self = ( const jedex_generic_union * ) vself;
  if ( self->discriminant < 0 ) {
    log_err( "Union has no selected branch" );
    return EINVAL;
  }

  branch->iface = jedex_generic_union_branch_iface( iface, self );
  branch->self = jedex_generic_union_branch( self );

  return 0;
}


static int
jedex_generic_union_set_branch( const jedex_value_iface *viface, void *vself, int discriminant, jedex_value *branch ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  int rval;
  jedex_generic_union *self = ( jedex_generic_union * ) vself;

  /*
   * If the new desired branch is different than the currently
   * active one, then finalize the old branch and initialize the
   * new one.
   */
  if ( self->discriminant != discriminant ) {
    if ( self->discriminant >= 0 ) {
      jedex_value_done( jedex_generic_union_branch_giface( iface, self ), jedex_generic_union_branch( self ) );
    }
    self->discriminant = discriminant;
    if ( discriminant >= 0 ) {
      check( rval, jedex_value_init( jedex_generic_union_branch_giface( iface, self ), jedex_generic_union_branch( self ) ) );
    }
  }

  if ( branch != NULL ) {
    branch->iface = jedex_generic_union_branch_iface( iface, self );
    branch->self = jedex_generic_union_branch( self );
  }

  return 0;
}


static size_t
jedex_generic_union_instance_size( const jedex_value_iface *viface ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );

  return iface->instance_size;
}


static int
jedex_generic_union_init( const jedex_value_iface *viface, void *vself ) {
  UNUSED( viface );

  jedex_generic_union *self = ( jedex_generic_union * ) vself;
  self->discriminant = -1;

  return 0;
}


static void
jedex_generic_union_done( const jedex_value_iface *viface, void *vself ) {
  const jedex_generic_union_value_iface *iface = container_of( viface, jedex_generic_union_value_iface, parent );
  jedex_generic_union *self = ( jedex_generic_union * ) vself;
  if ( self->discriminant >= 0 ) {
    jedex_value_done( jedex_generic_union_branch_giface( iface, self ), jedex_generic_union_branch( self ) );
    self->discriminant = -1;
  }
}

jedex_generic_value_iface *
jedex_generic_union_class( void ) {
  pthread_once( &generic_union_key_once, make_generic_union_key );

  jedex_generic_value_iface *generic_union = ( jedex_generic_value_iface * ) pthread_getspecific( generic_union_key ); 
  if ( generic_union == NULL ) {
    generic_union = ( jedex_generic_value_iface * ) jedex_new( jedex_generic_value_iface );
    pthread_setspecific( generic_union_key, generic_union );

    memset( &generic_union->parent, 0, sizeof( generic_union->parent ) );
    generic_union->parent.reset = jedex_generic_union_reset;
    generic_union->parent.get_type = jedex_generic_union_get_type;
    generic_union->parent.get_schema = jedex_generic_union_get_schema;
    generic_union->parent.get_discriminant = jedex_generic_union_get_discriminant;
    generic_union->parent.get_current_branch = jedex_generic_union_get_current_branch;

    generic_union->parent.set_branch = jedex_generic_union_set_branch;
    generic_union->instance_size = jedex_generic_union_instance_size;
    generic_union->init = jedex_generic_union_init;
    generic_union->done = jedex_generic_union_done;
  }

  return generic_union;
}


jedex_value_iface *
jedex_value_union_class( void ) {
  return &( jedex_generic_union_class() )->parent;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
