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


#ifndef PRIV_H
#define PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#define UNUSED( var ) ( void ) var;
#define check( rval, call ) { rval = call; if ( rval ) return rval; }
#define check_param( result, test, name )         \
  {               \
    if ( !( test ) ) {            \
      log_err( "Invalid " name " in %s",  \
               __FUNCTION__ );     \
      return result;          \
    }             \
  }
#define container_of( ptr_, type_, member_ )  \
    ( ( type_ * )( ( char * ) ptr_ - ( size_t )&( ( type_ * ) 0 )->member_ ) )


CLOSE_EXTERN
#endif // PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
