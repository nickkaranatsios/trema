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


#define DEFAULT_SCHEMA_DIR "."
#define DEFAULT_SCHEMA_FN  "test_schema"


static char *
file_read( const char *dirpath, const char *fn ) {
  char filepath[ PATH_MAX ];
  FILE *fp;
  int rval;

  snprintf( filepath, sizeof( filepath ), "%s/%s", dirpath, fn );
  struct stat buf;
  rval = stat( filepath, &buf );
  if ( rval ) {
    return NULL;
  }
  char *jscontent = ( char * ) jedex_malloc( buf.st_size + 1 );
  fp = fopen( filepath, "r" );
  if ( !fp ) {
    return NULL;
  }
  rval = fread( jscontent, 1, buf.st_size, fp );
  fclose( fp );
  jscontent[ rval ] = '\0';

  return jscontent;
}


jedex_value *
jedex_value_from_iface( jedex_value_iface *val_iface ) {
   jedex_value *val = ( jedex_value * ) jedex_new( jedex_value );
   if ( val == NULL ) {
     jedex_free( val_iface );
     return NULL;
   }
   if ( jedex_generic_value_new( val_iface, val ) ) {
     jedex_free( val_iface );
     jedex_free( val );
     return NULL;
   }

   return val; 
}


static jedex_value *
first_element( const jedex_parcel *parcel ) {
  list_element *e = parcel->values_list->head;

  return e == NULL ? NULL : e->data;
}


static jedex_value *
lookup_schema_name( const jedex_parcel *parcel, const char *schema_name ) {
  if ( !schema_name || !strcmp( schema_name, "" ) ) {
    return first_element( parcel );
  }
  for ( list_element *e = parcel->values_list->head; e != NULL; e = e->next ) {
    jedex_value *item = e->data;
    jedex_schema *item_schema = jedex_value_get_schema( item );
    if ( is_jedex_record( item_schema ) ) {
     if ( !strcmp( schema_name, ( jedex_schema_to_record( item_schema ) )->name ) ) {
        return item;
      }
    }
  }

  return NULL;
}


static int
append_to_parcel( jedex_parcel **parcel, jedex_schema *schema, jedex_value_iface *val_iface ) {
  if ( *parcel == NULL ) {
    *parcel = ( jedex_parcel * ) jedex_new( jedex_parcel );
    if ( *parcel == NULL ) {
      return ENOMEM;
    }
    ( *parcel )->values_list = create_list();
  }
  ( *parcel )->schema = schema;
  int rc = 0;

  if ( val_iface != NULL ) {
    jedex_value *val = jedex_value_from_iface( val_iface );
    if ( val != NULL ) {
      append_to_tail( ( *parcel )->values_list, val );
    }
    else {
      rc = EINVAL;
    }
  }
  else {
    rc = EINVAL;
  }

  return rc;
}


static jedex_value_iface *
jedex_value_iface_from_sub_schema( jedex_schema *schema, const char *sub_schema_name ) {
  jedex_value_iface *val_iface = NULL;

  if ( !strcmp( sub_schema_name, "" ) ) {
    val_iface = jedex_generic_class_from_schema( schema );
  }
  else {
    jedex_schema *sub_schema = jedex_schema_get_subschema( schema, sub_schema_name );
    if ( sub_schema != NULL ) {
      val_iface = jedex_generic_class_from_schema( sub_schema );
    }
  }

  return val_iface == NULL ? NULL: val_iface;
}


jedex_value *
jedex_parcel_value( const jedex_parcel *parcel, const char *schema_name ) {
  assert( parcel );
  assert( parcel->values_list );

  return lookup_schema_name( parcel, schema_name );
}


jedex_parcel *
jedex_parcel_create( jedex_schema *schema, const char **sub_schema_names ) {
  jedex_value_iface *val_iface = NULL;
  jedex_parcel *parcel = NULL;

  if ( *sub_schema_names ) {
    int nr_sub_schema_names = 0;
    while ( *( sub_schema_names + nr_sub_schema_names ) ) {
      nr_sub_schema_names++;
    }
    for ( int i = 0; i < nr_sub_schema_names; i++ ) {
      val_iface = jedex_value_iface_from_sub_schema( schema, sub_schema_names[ i ] );
      if ( append_to_parcel( &parcel, schema, val_iface ) ) {
        return NULL;
      }
    }
  }
  else {
    val_iface = jedex_generic_class_from_schema( schema );
    if ( append_to_parcel( &parcel, schema, val_iface ) ) {
      return NULL;
    }
  }

  return parcel;
}


jedex_schema *
jedex_initialize( const char *schema_name ) {
  char schema_fn[ FILENAME_MAX ];
  char *jsontext;

  if ( !strcmp( schema_name, "" ) ) {
    strncpy( schema_fn, DEFAULT_SCHEMA_FN, sizeof( schema_fn ) - 1 );
  }
  else {
    strncpy( schema_fn, schema_name, sizeof( schema_fn ) - 1 );
  }
  schema_fn[ sizeof( schema_fn ) - 1 ] = '\0';

  jsontext = file_read( DEFAULT_SCHEMA_DIR, schema_fn );
  jedex_schema *schema = NULL;
  if ( jsontext != NULL ) {
    jedex_schema_from_json( jsontext, &schema );
    free( jsontext );
  }

  return schema == NULL ? NULL : schema;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
