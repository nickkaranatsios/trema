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


#ifndef DB_MAPPER_ERROR_H
#define DB_MAPPER_ERROR_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


typedef enum {
  SUCCESS,
  DB_NAME_NOT_FOUND,
  DB_TABLE_NOT_FOUND,
  DB_REC_NOT_FOUND,
  DB_TABLE_SCHEMA_NOT_FOUND
} db_mapper_error;


CLOSE_EXTERN
#endif // DB_MAPPER_ERROR_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
