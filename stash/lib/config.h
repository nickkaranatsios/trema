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


#ifndef CONFIG_H
#define CONFIG_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


typedef int ( *config_fn ) ( const char *key, const char *value, void *user_data );
int read_config( const char *filename, config_fn fn, void *user_data );


CLOSE_EXTERN
#endif // CONFIG_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
