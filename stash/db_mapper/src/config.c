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


#include "mapper_priv.h"


#define MAXNAME 128


static FILE *config_file;
static const char *config_file_name;
static int config_linenr;
static int config_file_eof;


static int
get_next_char( void ) {
  int c;
  FILE *f;

  c = '\n';
  if ( ( f = config_file ) != NULL ) {
    c = fgetc( f );
    if ( c == '\r' ) {
      c = fgetc( f );
      if ( c != '\n' ) {
        ungetc( c, f );
          c = '\r';
      }
    }
    if ( c == '\n' ) {
      config_linenr++;
    }
    if ( c == EOF ) {
      config_file_eof = 1;
      c = '\n';
    }
  }
  return c;
}


static inline int
iskeychar( int c ) {
  return isalnum( c ) || c == '-' || c == '_';
}


static int
get_extended_base_var( char *name, int baselen, int c ) {
  do {
    if ( c == '\n' ) {
      goto error_incomplete_line;
    }
    c = get_next_char();
  } while ( isspace( c ) );

  /* We require the format to be '[base "extension"]' */
  if ( c != '"' ) {
    return -1;
  }
  name[ baselen++ ] = '.';

  for ( ;; ) {
    int c = get_next_char();
    if ( c == '\n' ) {
      goto error_incomplete_line;
    }
    if ( c == '"' ) {
      break;
    }
    if ( c == '\\' ) {
      c = get_next_char();
      if ( c == '\n' ) {
        goto error_incomplete_line;
      }
    }
    name[ baselen++ ] = ( char ) c;
    if ( baselen > MAXNAME / 2 ) {
      return -1;
    }
  }

  /* Final ']' */
  if ( get_next_char() != ']' ) {
    return -1;
  }

  return baselen;

error_incomplete_line:
  return -1;
}


static int
get_base_var( char *name ) {
  int baselen = 0;

  for ( ;; ) {
    int c = get_next_char();
    if ( config_file_eof ) {
      return -1;
    }
    if ( c == ']' ) {
      return baselen;
    }
    if ( isspace( c ) ) {
      return get_extended_base_var( name, baselen, c );
    }
    if ( !iskeychar( c ) && c != '.' ) {
      return -1;
    }
    if ( baselen > MAXNAME / 2 ) {
      return -1;
    }
    name[ baselen++ ] = ( char ) tolower( c );
  }
}


static char
*parse_value( void ) {
  static char value[ 1024 ];
  int comment = 0;
  int space = 0;
  unsigned int len = 0;

  for ( ;; ) {
    int c = get_next_char();
    if ( len >= sizeof( value ) - 1 ) {
      return NULL;
    }
    if ( c == '\n' ) {
      value[ len ] = 0;
      return value;
    }
    if ( comment ) {
      continue;
    }
    if ( isspace( c ) ) {
      if ( len ) {
        space++;
      }
      continue;
    }
    if ( c == ';' || c == '#' ) {
      comment = 1;
      continue;
    }
    for ( ; space; space-- )
      value[ len++ ] = ' ';
      if ( c == '\\' ) {
        c = get_next_char();
        switch ( c ) {
          case '\n':
          continue;
          case 't':
            c = '\t';
          break;
         case 'b':
           c = '\b';
         break;
         case 'n':
           c = '\n';
         break;
         /* Some characters escape as themselves */
         case '\\': case '"':
         break;
         /* Reject unknown escape sequences */
         default:
         return NULL;
       }
       value[ len++ ] = ( char ) c;
       continue;
     }
     value[ len++ ] = ( char ) c;
  }
}


static int
get_value( config_fn fn, void *user_data, char *name, int len ) {
  int c;
  char *value;

  for ( ;; ) {
    c = get_next_char();
    if ( config_file_eof ) {
      break;
    }
    if ( !iskeychar( c ) ) {
      break;
    }
    name[ len++ ] = ( char ) tolower( c );
    if ( len >= MAXNAME ) {
      return -1;
    }
  }
  name[ len ] = 0;
  while ( c == ' ' || c == '\t' ) {
    c = get_next_char();
  }
  value = NULL;
  if ( c != '\n' ) {
    if ( c != '=' ) {
      return -1;
    }
    value = parse_value();
    if (!value) {
      return -1;
    }
  }

  return fn( name, value, user_data );
}


static int 
parse_file( config_fn fn, void *user_data ) {
  static char var[ MAXNAME ];
  int c, comment = 0, baselen = 0;

  for ( ;; ) {
    c = get_next_char();
    if ( c == '\n' ) {
      if ( config_file_eof ) {
        return 0;
      }
      comment = 0;
      continue;
    }
    if ( comment || isspace( c ) ) {
      continue;
    }
    if ( c == '#' || c == ';' ) {
      comment = 1;
      continue;
    }
    if ( c == '[' ) {
      baselen = get_base_var( var );
      if ( baselen <= 0 ) {
        break;
      }
      var[ baselen++ ] = '.';
      var[ baselen ] = 0;
      continue;
    }
    if ( !isalpha( c ) ) {
      break;
    }
    var[ baselen ] = ( char ) tolower( c );
    if ( get_value( fn, user_data, var, baselen + 1 ) < 0 ) {
      break;
    }
  }
  printf( "Bad config line %d in %s", config_linenr, config_file_name );

  return -1;
}


int
read_config( const char *filename, config_fn fn, void *user_data ) {
  FILE *fp = fopen( filename, "r" );
  int ret = -1;

  if ( fp ) {
    config_file = fp;
    config_file_name = filename;
    config_linenr = 1;
    config_file_eof = 0;
    ret = parse_file( fn, user_data );
    fclose( fp );
    config_file_name = NULL;
  }

  return ret;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
