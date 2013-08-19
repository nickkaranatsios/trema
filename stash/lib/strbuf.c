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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include "log_writer.h"
#include "array_util.h"
#include "strbuf.h"


static inline size_t
strbuf_avail( strbuf *sb ) {
  return sb->alloc ? sb->alloc - sb->len - 1 : 0;
} 


static inline void
strbuf_setlen( strbuf *sb, size_t len ) {
  if ( len > ( sb->alloc ? sb->alloc - 1 : 0 ) ) { 
    return;
  }
  sb->len = len;
  sb->buf[ len ] = '\0';
}


static void
strbuf_grow( strbuf *sb, size_t extra ) {
  int new_buf = !sb->alloc;

  if ( new_buf ) {
    sb->buf = NULL;
  }
  ALLOC_GROW( sb->buf, sb->len + extra + 1, sb->alloc );
  if ( new_buf ) {
    sb->buf[ 0 ] = '\0';
  }
}


static void
strbuf_vaddf( strbuf *sb, const char *fmt, va_list ap ) {
  int len;
  va_list cp;

  if ( !strbuf_avail( sb ) ) {
    strbuf_grow( sb, 64 );
  }
  va_copy( cp, ap );
  len = vsnprintf( sb->buf + sb->len, sb->alloc - sb->len, fmt, cp );
  va_end( cp );
  if ( len < 0 ) {
    log_err( "vsnprintf error %d", len );
    return;
  }
  size_t slen = ( size_t ) len;
  if ( slen > strbuf_avail( sb ) ) {
    strbuf_grow( sb, slen );
    len = vsnprintf( sb->buf + sb->len, sb->alloc - sb->len, fmt, ap );
    if ( slen > strbuf_avail( sb ) ) {
      log_err( "vsnprintf error %d", len );
      return;
    }
  }
  strbuf_setlen( sb, sb->len + slen );
}


static strbuf **
strbuf_split_buf( const char *str, size_t slen, int delim, int max ) {
  int alloc = 2, pos = 0;
  const char *n, *p;
  strbuf **ret;
  strbuf *t;

  ret = calloc( ( size_t ) alloc, sizeof( strbuf * ) );
  p = n = str;
  while ( n < str + slen ) {
    int len;
    if ( max <= 0 || pos + 1 < max ) {
      n = memchr( n, delim, slen - ( size_t ) ( n - str ) );
    }
    else {
      n = NULL;
    }
    if ( pos + 1 >= alloc ) {
      alloc = alloc * 2;
      ret = realloc( ret, sizeof( strbuf * ) * ( size_t ) alloc );
    }
    if ( !n ) {
      n = str + slen - 1;
    }
    len = n - p + 1;
    t = malloc( sizeof( strbuf ) );
    size_t slen = ( size_t ) len;
    strbuf_init( t, slen );
    strbuf_add( t, p, slen );
    ret[ pos ] = t;
    ret[ ++pos ] = NULL;
    p = ++n;
  }

  return ret;
}

void
strbuf_add( strbuf *sb, const void *data, size_t len ) {
  strbuf_grow( sb, len );
  memcpy( sb->buf + sb->len, data, len );
  strbuf_setlen( sb, sb->len + len );
}


int
prefixcmp( const char *str, const char *prefix ) {
  for ( ;; str++, prefix++ ) {
    if ( !*prefix ) {
      return 0;
    }
    else if ( *str != *prefix ) {
      return ( uint8_t ) *prefix - ( uint8_t ) *str;
    }
  }
}


int
suffixcmp( const char *str, const char *suffix ) {
  size_t len = strlen( str ), suflen = strlen( suffix );

  if ( len < suflen) {
    return -1;
  }
  else {
    return strcmp( str + len - suflen, suffix );
  }
}


void
strbuf_rtrimn( strbuf *sb, size_t len ) {
  if ( sb->len - len > 0 ) {
    sb->len -= len;
    sb->buf[ sb->len ] = '\0';
  }
}


void
strbuf_addstr( strbuf *sb, const char *s ) {
  strbuf_add( sb, s, strlen( s ) );
}


void
strbuf_addf( strbuf *sb, const char *fmt, ... ) {
  va_list ap;
  va_start( ap, fmt );
  strbuf_vaddf( sb, fmt, ap );
  va_end( ap );
}



void
strbuf_list_free( strbuf **sbs ) {
  strbuf **s = sbs;

  while ( *s ) {
    strbuf_release( *s );
    free( *s++ );
  }
  free( sbs );
}


char *
strbuf_rsplit( strbuf *sb, int delim ) {
  char *p = sb->buf;
  char *end;

  end = strrchr( p, delim );
  if ( end ) {
    end = end + 1;
  }

  return end;
}


strbuf **
strbuf_split_str( const char *str, int delim, int max ) {
  return strbuf_split_buf( str, strlen( str ), delim, max );
}


void
strbuf_init( strbuf *sb, size_t hint ) {
  sb->alloc = sb->len = 0;
  sb->buf = dummy;
  if ( hint ) {
    strbuf_grow( sb, hint );
  }
}   


void
strbuf_reset( strbuf *sb ) {
  strbuf_setlen( sb, 0 );
}

char *
strbuf_detach( strbuf *sb, size_t *sz ) {
  char *res;

  strbuf_grow( sb, 0 );
  res = sb->buf;
  if ( sz ) {
    *sz = sb->len;
  }
  strbuf_init( sb, 0 );

  return res;
}


void
strbuf_release( strbuf *sb ) {
  if ( sb->alloc ) {
    free( sb->buf );
    strbuf_init( sb, 0 );
  }
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
