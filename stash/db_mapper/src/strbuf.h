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


#ifndef STRBUF_PRIV_H
#define STRBUF_PRIV_H


#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


typedef struct strbuf {
  size_t alloc;
  size_t len;
  char *buf;
} strbuf;


char dummy[ 1 ];

#define STRBUF_INIT { 0, 0, dummy }


int prefixcmp( const char *str, const char *prefix );
int suffixcmp( const char *str, const char *suffix );
void strbuf_add( strbuf *sb, const void *s, size_t len );
void strbuf_addstr( strbuf *sb, const char *s );
void strbuf_addf( strbuf *sb, const char *fmt, ... );
void strbuf_init( strbuf *sb, size_t hint );
void strbuf_reset( strbuf *sb );
char *strbuf_rsplit( strbuf *sb, int delim );
strbuf **strbuf_split_str( const char *str, int delim, int max );
void strbuf_list_free( strbuf **sbs );
char *strbuf_detach( strbuf *sb, size_t *sz );
void strbuf_release( strbuf *sb );
void strbuf_rtrimn( strbuf *sb, size_t len );


CLOSE_EXTERN
#endif // MAPPER_PRIV_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */