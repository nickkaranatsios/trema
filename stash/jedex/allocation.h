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


#ifndef ALLOCATION_H
#define ALLOCATION_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


void *jedex_allocator( void *ptr, size_t nsize );


#define jedex_realloc( ptr, nsz ) \
  jedex_allocator( ( ptr ), ( nsz ) )

#define jedex_malloc( sz ) ( jedex_realloc( NULL, ( sz ) ) )
#define jedex_free( ptr ) ( jedex_realloc( ( ptr ), 0 ) )
#define jedex_new( type ) ( jedex_realloc( NULL, sizeof( type ) ) )


void *jedex_calloc( size_t count, size_t size );


CLOSE_EXTERN
#endif // ALLOCATION_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
