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


#include "db_mapper.h"
#include "mapper_priv.h"


#ifdef TEST
static void
set_save_topic( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "dbname", &field, &index );
  jedex_value_set_string( &field, "groceries" );
  jedex_value_get_by_name( val, "topic", &field, &index );
  jedex_value_set_string( &field, "save_groceries" );
  jedex_value_get_by_name( val, "tables", &field, &index );
  jedex_value element;
  jedex_value_append( &field, &element, NULL );
  jedex_value_set_string( &element, "fruits" );
}


static void
set_find_fruits( jedex_value *val ) {
  jedex_value field;
  size_t index;
  jedex_value_get_by_name( val, "name", &field, &index );
  jedex_value_set_string( &field, "mango" );
}

static void
set_find_all_fruits( jedex_value *val ) {
  jedex_value branch;
  size_t index;
  
  jedex_value_get_by_name( val, "fruits", &branch, &index );
}


void
set_topic( mapper *self ) {
  jedex_value_iface *val_iface;

  jedex_schema *request_schema = jedex_schema_get_subschema( self->request_reply_schema, "save_all" );
  val_iface = jedex_generic_class_from_schema( request_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  set_save_topic( val );
  char *json;
  jedex_value_to_json( val, true, &json );
  if ( json ) {
    request_save_topic_callback( val, json, self );
  }
}


void
set_find_record( mapper *self ) {
  jedex_schema *fruits_schema = jedex_schema_get_subschema( self->schema, "fruits" );
  assert( fruits_schema );

  jedex_value_iface *val_iface;

  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  assert( val );
  set_find_fruits( val );
  char *json;
  jedex_value_to_json( val, true, &json );
  if ( json ) {
    request_find_record_callback( val, json, self );
  }
}


void
set_fruits( jedex_value *val, const char *name, float price ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  //assert( index == 0 );
  jedex_value_set_string( &field, name );
  jedex_value_get_by_name( val, "price", &field, &index );
  //assert( index == 1 );
  jedex_value_set_float( &field, price );
}


void
set_many_fruits( jedex_value *val ) {
  jedex_value element;
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "jackfruit", 5.38f );
  jedex_value_append( val, &element, NULL );
  set_fruits( &element, "pineapple", 2.66f );
}


void
update_fruits( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  jedex_value_set_string( &field, "jackfruit" );

  jedex_value_get_by_name( val, "price", &field, &index );
  float price = 7.8888f;
  jedex_value_set_float( &field, price );
}


void
delete_fruits( jedex_value *val ) {
  jedex_value field;
  size_t index;

  jedex_value_get_by_name( val, "name", &field, &index );
  jedex_value_set_string( &field, "pineapple" );
}


void
set_find_all_records( mapper *self ) {
  jedex_schema *fruits_schema = jedex_schema_get_subschema( self->schema, "fruits" );
  assert( fruits_schema );

  jedex_value_iface *val_iface;
  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  assert( val );
  //set_find_all_fruits( val );
  char *json;
  jedex_value_to_json( val, true, &json );
  assert( json );
}


void
insert_publish_data( mapper *self ) {
  jedex_schema *fruits_schema = jedex_schema_get_subschema( self->schema, "fruits" );
  assert( fruits_schema );

  jedex_schema *array_schema = jedex_schema_array( fruits_schema );

  jedex_value_iface *array_class = jedex_generic_class_from_schema( array_schema );
  assert( array_class );

  jedex_value val;
  jedex_generic_value_new( array_class, &val );
  set_many_fruits( &val );

  char *json;
  jedex_value_to_json( &val, true, &json );
  if ( json ) {
    request_insert_record_callback( &val, json, self ); 
  }

#ifdef MANY 
  jedex_value *val = jedex_value_from_iface( val_iface );
  assert( val );

  //set_fruits( val );
  set_many_fruits( val );
  char *json;
  jedex_value_to_json( val, true, &json );

  if ( json ) {
    request_insert_record_callback( val, json, self ); 
  }
#endif
}


void
set_update_record( mapper *self ) {
  jedex_schema *fruits_schema = jedex_schema_get_subschema( self->schema, "fruits" );
  assert( fruits_schema );

  jedex_value_iface *val_iface;
  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  assert( val );

  update_fruits( val );
  char *json;
  jedex_value_to_json( val, true, &json );

  if ( json ) {
    request_update_record_callback( val, json, self );
  }
}


void
set_delete_record( mapper *self ) {
  jedex_schema *fruits_schema = jedex_schema_get_subschema( self->schema, "fruits" );
  assert( fruits_schema );

  jedex_value_iface *val_iface;
  val_iface = jedex_generic_class_from_schema( fruits_schema );
  jedex_value *val = jedex_value_from_iface( val_iface );
  assert( val );

  delete_fruits( val );
  char *json;
  jedex_value_to_json( val, true, &json );

  if ( json ) {
    request_delete_record_callback( val, json, self );
  }
}
#endif


/*
 * Db_mapper when it receives a save_topic message would create the database
 * and the given tables. Db_mapper expects to receive multiple save_topic
 * messages.
 * Db_mapper makes the following assumption that the record object key 
 * found in any request message that it receives corresponds to a database table.
 * It also expects that schema information is available for this and every other
 * table db_mapper is capable of processing. Db_mapper's -s (--schema) option
 * should be used to set this schema file to read. In most cases the schema file
 * contains an array of records.
 */
int
main( int argc, char **argv ) {
  mapper *self = NULL;
  self = mapper_initialize( &self, argc, argv );
  if ( self != NULL ) {
    // set_topic( self );
    // set_find_all_records( self );
    // set_find_record( self );
    // insert_publish_data( self );
    // set_update_record( self );
    // set_delete_record( self );
    emirates_loop( self->emirates );
  }
  else {
    log_err( "Initialization failed exiting ..." );
  }


  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
