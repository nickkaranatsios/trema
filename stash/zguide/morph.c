#include "czmq.h"

enum morph_type {
  MORPH_STRING,
  MORPH_INT32,
  MORPH_TYPE_END
};
typedef enum morph_type morph_type;

enum morph_class {
  MORPH_SCHEMA,
  MORPH_DATA
};
typedef enum morph_class morph_class;


struct morph_obj {
  morph_type type;
  morph_class class_type;
};
typedef struct morph_obj morph_schema;

static morph_schema **schemata;


#ifdef LATER
struct generic_value_iface {
  size_t ( *instance_size )( value_iface *iface );
  int ( *init )( value_iface *iface, void *self );
  int ( *done )( vlaue_iface *iface, void *self );
};
#endif


static morph_schema *
schema_string_init( void ) {
  morph_schema *schema_string = ( morph_schema * ) zmalloc( sizeof( struct morph_obj ) );
  schema_string->type = MORPH_SCHEMA;
  schema_string->class_type = MORPH_SCHEMA;

  return schema_string;
}
  

static morph_schema *
schema_int32_init( void ) {
  morph_schema *schema_int32 = ( morph_schema * ) zmalloc( sizeof( struct morph_obj ) );
  schema_int32->type = MORPH_INT32;
  schema_int32->class_type = MORPH_SCHEMA;

  return schema_int32;
}

void
morph_init() {
  schemata = ( morph_schema ** ) zmalloc( sizeof( morph_schema ** ) * MORPH_TYPE_END );
  schemata[ MORPH_STRING ] = schema_string_init();
  schemata[ MORPH_INT32 ] = schema_int32_init();
}


morph_schema *
morph_schema_string( void ) {
  return schemata[ MORPH_STRING ];
}

