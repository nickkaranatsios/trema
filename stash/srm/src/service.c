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


#include "system_resource_manager.h"
#include "system_resource_manager_priv.h"


static void
param_stat_free( param_stats_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->param_stats_nr; i++ ) {
    param_stat *ps = tbl->param_stats[ i ];
    xfree( ps->name );
    xfree( ps );
  }
  xfree( tbl->param_stats );
}


param_stat *
param_stats_create( const char *name, param_stats_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->param_stats_nr; i++ ) {
    if ( !strncmp( name, tbl->param_stats[ i ]->name, strlen( tbl->param_stats[ i ]->name ) ) ) {
      return tbl->param_stats[ i ];
    }
  }
  ALLOC_GROW( tbl->param_stats, tbl->param_stats_nr + 1, tbl->param_stats_alloc );
  param_stat *stat = xmalloc( sizeof( *stat ) );
  stat->name = strdup( name );
  tbl->param_stats[ tbl->param_stats_nr++ ] = stat;
  
  return stat;
}


service *
service_create( const char *name, service_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->services_nr; i++ ) {
    if ( !strncmp( name, tbl->services[ i ]->name, strlen( tbl->services[ i ]->name ) ) ) {
      return tbl->services[ i ];
    }
  }
  ALLOC_GROW( tbl->services, tbl->services_nr + 1, tbl->services_alloc );
  service *self = xmalloc( sizeof( *self ) );
  self->name = strdup( name );
  tbl->services[ tbl->services_nr++ ] = self;

  return self;
}


void
service_free( service_table *tbl ) {
  for ( uint32_t i = 0; i < tbl->services_nr; i++ ) {
    xfree( tbl->services[ i ]->name );
    param_stats_table *ps_tbl = &tbl->services[ i ]->param_stats_tbl;
    param_stat_free( ps_tbl );
    xfree( tbl->services[ i ] );
  }
  xfree( tbl->services );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
