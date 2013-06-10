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


#ifndef WRAPPED_BUFFER_PRIV_H
#define WRAPPED_BUFFER_PRIV_H

#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


struct jedex_wrapped_copy {
	size_t  allocated_size;
};


struct jedex_wrapped_resizable {
  size_t buf_size;
};


#define jedex_wrapped_resizable_size( sz ) \
  ( sizeof( struct jedex_wrapped_resizable ) + ( sz ) )

CLOSE_EXTERN
#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
