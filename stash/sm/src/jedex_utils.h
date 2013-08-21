/*
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


#ifndef JEDEX_UTILS_H
#define JEDEX_UTILS_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif



jedex_schema *_sub_schema_find( const char *name, size_t tbl_size, jedex_schema *sub_schemas[] );

#define sub_schema_find( name, sub_schema_tbl ) \
  _sub_schema_find( name, ARRAY_SIZE( sub_schema_tbl ), sub_schema_tbl )


jedex_value *_jedex_value_find( const char *name, size_t tbl_size, jedex_value *rval[] );


#define jedex_value_find( name, rval_tbl ) \
  _jedex_value_find( name, ARRAY_SIZE( rval_tbl ), rval_tbl )


CLOSE_EXTERN
#endif // JEDEX_UTILS_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
