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


#ifndef GENERIC_RECORD_H
#define GENERIC_RECORD_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif

/*
 * For generic records, we need to store the value implementation for
 * each field.  We also need to store an offset for each field, since
 * we're going to store the contents of each field directly in the
 * record, rather than via pointers.
 */

typedef struct jedex_generic_record_value_iface {
  jedex_generic_value_iface  parent;
  jedex_schema  schema;

  /** The total size of each value struct for this record. */
  size_t  instance_size;

  /** The number of fields in this record.  Yes, we could get this
   * from schema, but this is easier. */
  size_t  field_count;

  /** The offset of each field within the record struct. */
  size_t  *field_offsets;

  /** The value implementation for each field. */
  jedex_generic_value_iface  **field_ifaces;
} jedex_generic_record_value_iface;


typedef struct jedex_generic_record {
  /* The rest of the struct is taken up by the inline storage
   * needed for each field. */
} jedex_generic_record;


/** Return a pointer to the given field within a record struct. */
#define avro_generic_record_field( iface, rec, index ) \
  ( ( ( char * ) ( rec ) ) + ( iface )->field_offsets[( index )] )


CLOSE_EXTERN
#endif // VALUE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
