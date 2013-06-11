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


#ifndef DATA_H
#define DATA_H

#ifdef __cplusplus
extern "C" {
#define CLOSE_EXTERN }
#else
#define CLOSE_EXTERN
#endif


#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------
 * Arrays
 */

/**
 * A resizeable array of fixed-size elements.
 */

typedef struct jedex_raw_array {
	size_t element_size;
	size_t element_count;
	size_t allocated_size;
	void *data;
} jedex_raw_array;

/**
 * Initializes a new jedex_raw_array that you've allocated yourself.
 */

void jedex_raw_array_init( jedex_raw_array *array, size_t element_size );

/**
 * Finalizes an jedex_raw_array.
 */

void jedex_raw_array_done( jedex_raw_array *array );

/**
 * Clears an jedex_raw_array.  This does not deallocate any space; this
 * allows us to reuse the underlying array buffer as we start to re-add
 * elements to the array.
 */

void jedex_raw_array_clear( jedex_raw_array *array );

/**
 * Ensures that there is enough allocated space to store the given
 * number of elements in an jedex_raw_array.  If we can't allocate that
 * much space, we return ENOMEM.
 */

int jedex_raw_array_ensure_size( jedex_raw_array *array, size_t desired_count );

/**
 * Ensures that there is enough allocated space to store the given
 * number of elements in an jedex_raw_array.  If the array grows as a
 * result of this operation, we will fill in any newly allocated space
 * with 0 bytes.  If we can't allocate that much space, we return
 * ENOMEM.
 */

int jedex_raw_array_ensure_size0( jedex_raw_array *array, size_t desired_count );

/**
 * Returns the number of elements in an jedex_raw_array.
 */

#define jedex_raw_array_size( array ) ( ( array )->element_count )

/**
 * Returns the given element of an jedex_raw_array as a <code>void
 * *</code>.
 */

#define jedex_raw_array_get_raw( array, index ) \
	( ( char * ) ( array )->data + ( array )->element_size * index )

/**
 * Returns the given element of an jedex_raw_array, using element_type
 * as the type of the elements.  The result is *not* a pointer to the
 * element; you get the element itself.
 */

#define jedex_raw_array_get( array, element_type, index ) \
	( ( ( element_type * ) ( array )->data )[ index ] )

/**
 * Appends a new element to an jedex_raw_array, expanding it if
 * necessary.  Returns a pointer to the new element, or NULL if we
 * needed to reallocate the array and couldn't.
 */

void *jedex_raw_array_append( jedex_raw_array *array );


/*---------------------------------------------------------------------
 * Maps
 */

/**
 * The type of the elements in a map's elements array.
 */

typedef struct jedex_raw_map_entry {
	const char *key;
} jedex_raw_map_entry;

/**
 * A string-indexed map of fixed-size elements.
 */

typedef struct jedex_raw_map {
	jedex_raw_array elements;
	void *indices_by_key;
} jedex_raw_map;

/**
 * Initializes a new jedex_raw_map that you've allocated yourself.
 */

void jedex_raw_map_init( jedex_raw_map *map, size_t element_size );

/**
 * Finalizes an jedex_raw_map.
 */

void jedex_raw_map_done(jedex_raw_map *map );

/**
 * Clears an jedex_raw_map.
 */

void jedex_raw_map_clear(jedex_raw_map *map);

/**
 * Ensures that there is enough allocated space to store the given
 * number of elements in an jedex_raw_map.  If we can't allocate that
 * much space, we return ENOMEM.
 */

int jedex_raw_map_ensure_size( jedex_raw_map *map, size_t desired_count );

/**
 * Returns the number of elements in an jedex_raw_map.
 */

#define jedex_raw_map_size( map )  jedex_raw_array_size( &( ( map )->elements ) )

/**
 * Returns the jedex_raw_map_entry for a given index.
 */

#define jedex_raw_get_entry( map, index ) \
	( ( jedex_raw_map_entry * ) \
	 jedex_raw_array_get_raw( &( map )->elements, index ) )

/**
 * Returns the given element of an jedex_raw_array as a <code>void
 * *</code>.  The indexes are assigned based on the order that the
 * elements are added to the map.
 */

#define jedex_raw_map_get_raw( map, index ) \
	(jedex_raw_array_get_raw( &( map )->elements, index) + \
	 sizeof( jedex_raw_map_entry ) )

/**
 * Returns the element of an jedex_raw_map with the given numeric
 * index.  The indexes are assigned based on the order that the elements
 * are added to the map.
 */

#define jedex_raw_map_get_by_index( map, element_type, index ) \
	( *( ( element_type * ) jedex_raw_map_get_raw( map, index ) ) )

/**
 * Returns the key of the element with the given numeric index.
 */

#define jedex_raw_map_get_key( map, index ) \
	( jedex_raw_get_entry( map, index )->key )

/**
 * Returns the element of an jedex_raw_map with the given string key.
 * If the given element doesn't exist, returns NULL.  If @ref index
 * isn't NULL, it will be filled in with the index of the element.
 */

void *jedex_raw_map_get( const jedex_raw_map *map, const char *key, size_t *index );

/**
 * Retrieves the element of an jedex_raw_map with the given string key,
 * creating it if necessary.  A pointer to the element is placed into
 * @ref element.  If @ref index isn't NULL, it will be filled in with
 * the index of the element.  We return 1 if the element is new; 0 if
 * it's not, and a negative error code if there was some problem.
 */

int jedex_raw_map_get_or_create( jedex_raw_map *map, const char *key, void **element, size_t *index );


/*---------------------------------------------------------------------
 * Wrapped buffers
 */

/**
 * A pointer to an unmodifiable external memory region, along with
 * functions for freeing that buffer when it's no longer needed, and
 * copying it.
 */

typedef struct jedex_wrapped_buffer  jedex_wrapped_buffer;

struct jedex_wrapped_buffer {
	/** A pointer to the memory region */
	const void *buf;

	/** The size of the memory region */
	size_t size;

	/** Additional data needed by the methods below */
	void *user_data;

	/**
	 * A function that will be called when the memory region is no
	 * longer needed.  This pointer can be NULL if nothing special
	 * needs to be done to free the buffer.
	 */
	void ( *free ) ( jedex_wrapped_buffer *self );

	/**
	 * A function that makes a copy of a portion of a wrapped
	 * buffer.  This doesn't have to involve duplicating the memory
	 * region, but it should ensure that the free method can be
	 * safely called on both copies without producing any errors or
	 * memory corruption.  If this function is NULL, then we'll use
	 * a default implementation that calls @ref
	 * jedex_wrapped_buffer_new_copy.
	 */
	int ( *copy ) ( jedex_wrapped_buffer *dest, const jedex_wrapped_buffer *src, size_t offset, size_t length );

	/**
	 * A function that "slices" a wrapped buffer, causing it to
	 * point at a subset of the existing buffer.  Usually, this just
	 * requires * updating the @ref buf and @ref size fields.  If
	 * you don't need to do anything other than this, this function
	 * pointer can be left @c NULL.  The function can assume that
	 * the @a offset and @a length parameters point to a valid
	 * subset of the existing wrapped buffer.
	 */
	int ( *slice ) ( jedex_wrapped_buffer *self, size_t offset, size_t length );
};

/**
 * Free a wrapped buffer.
 */

#define jedex_wrapped_buffer_free( self ) \
	do { \
		if ( ( self )->free != NULL ) { \
			( self )->free( ( self ) ); \
		} \
	} while (0)

/**
 * A static initializer for an empty wrapped buffer.
 */

#define AVRO_WRAPPED_BUFFER_EMPTY  { NULL, 0, NULL, NULL, NULL, NULL }

/**
 * Moves a wrapped buffer.  After returning, @a dest will wrap the
 * buffer that @a src used to point at, and @a src will be empty.
 */

void jedex_wrapped_buffer_move( jedex_wrapped_buffer *dest, jedex_wrapped_buffer *src );

/**
 * Copies a buffer.
 */

int jedex_wrapped_buffer_copy( jedex_wrapped_buffer *dest,
			                         const jedex_wrapped_buffer *src,
			                         size_t offset,
                               size_t length);

/**
 * Slices a buffer.
 */

int jedex_wrapped_buffer_slice( jedex_wrapped_buffer *self,
			                          size_t offset,
                                size_t length ); 
/**
 * Creates a new wrapped buffer wrapping the given memory region.  You
 * have to ensure that buf stays around for as long as you need to new
 * wrapped buffer.  If you copy the wrapped buffer (using
 * jedex_wrapped_buffer_copy), this will create a copy of the data.
 * Additional copies will reuse this new copy.
 */

int
jedex_wrapped_buffer_new( jedex_wrapped_buffer *dest, const void *buf, size_t length );

/**
 * Creates a new wrapped buffer wrapping the given C string.
 */

#define jedex_wrapped_buffer_new_string( dest, str ) \
    ( jedex_wrapped_buffer_new( ( dest ), ( str ), strlen( ( str ) ) +1 ) )

/**
 * Creates a new wrapped buffer containing a copy of the given memory
 * region.  This new copy will be reference counted; if you copy it
 * further (using jedex_wrapped_buffer_copy), the new copies will share a
 * single underlying buffer.
 */

int jedex_wrapped_buffer_new_copy( jedex_wrapped_buffer *dest, const void *buf, size_t length );

/**
 * Creates a new wrapped buffer containing a copy of the given C string.
 */

#define jedex_wrapped_buffer_new_string_copy(dest, str) \
    ( jedex_wrapped_buffer_new_copy( ( dest ), ( str ), strlen( ( str ) ) +1 ) )


/*---------------------------------------------------------------------
 * Strings
 */

/**
 * A resizable buffer for storing strings and bytes values.
 */

typedef struct jedex_raw_string {
	jedex_wrapped_buffer wrapped;
} jedex_raw_string;

/**
 * Initializes an jedex_raw_string that you've allocated yourself.
 */

void jedex_raw_string_init( jedex_raw_string *str );

/**
 * Finalizes an jedex_raw_string.
 */

void jedex_raw_string_done( jedex_raw_string *str );

/**
 * Returns the length of the data stored in an jedex_raw_string.  If
 * the buffer contains a C string, this length includes the NUL
 * terminator.
 */

#define jedex_raw_string_length( str )  ( ( str )->wrapped.size )

/**
 * Returns a pointer to the data stored in an jedex_raw_string.
 */

#define jedex_raw_string_get( str )  ( ( str )->wrapped.buf )

/**
 * Fills an jedex_raw_string with a copy of the given buffer.
 */

void jedex_raw_string_set_length( jedex_raw_string *str, const void *src, size_t length );

/**
 * Fills an jedex_raw_string with a copy of the given C string.
 */

void jedex_raw_string_set( jedex_raw_string *str, const char *src );

/**
 * Appends the given C string to an jedex_raw_string.
 */

void jedex_raw_string_append( jedex_raw_string *str, const char *src );

/**
 * Appends the given C string to an jedex_raw_string, using the
 * provided length instead of calling strlen(src).
 */

void jedex_raw_string_append_length( jedex_raw_string *str, const void *src, size_t length );
/**
 * Gives control of a buffer to an jedex_raw_string.
 */

void
jedex_raw_string_give( jedex_raw_string *str, jedex_wrapped_buffer *src );

/**
 * Returns an jedex_wrapped_buffer for the content of the string,
 * ideally without copying it.
 */

int jedex_raw_string_grab( const jedex_raw_string *str, jedex_wrapped_buffer *dest );

/**
 * Clears an jedex_raw_string.
 */

void jedex_raw_string_clear( jedex_raw_string *str );


/**
 * Tests two jedex_raw_string instances for equality.
 */

int jedex_raw_string_equals( const jedex_raw_string *str1, const jedex_raw_string *str2 ); 

/*---------------------------------------------------------------------
 * Memoization
 */

/**
 * A specialized map that can be used to memoize the results of a
 * function.  The API allows you to use two keys as the memoization
 * keys; if you only need one key, just use NULL for the second key.
 * The result of the function should be a single pointer, or an integer
 * that can be cast into a pointer (i.e., an intptr_t).
 */

typedef struct jedex_memoize {
	void  *cache;
} jedex_memoize;

/**
 * Initialize an jedex_memoize that you've allocated for yourself.
 */

void jedex_memoize_init( jedex_memoize *mem );

/**
 * Finalizes an jedex_memoize.
 */

void jedex_memoize_done( jedex_memoize *mem );

/**
 * Search for a cached value in an jedex_memoize.  Returns a boolean
 * indicating whether there's a value in the cache for the given keys.
 * If there is, the cached result is placed into @ref result.
 */

int jedex_memoize_get( jedex_memoize *mem,
		                   void *key1,
                       void *key2,
                       void **result );

/**
 * Stores a new cached value into an jedex_memoize, overwriting it if
 * necessary.
 */

void jedex_memoize_set( jedex_memoize *mem,
		                    void *key1,
                        void *key2,
		                    void *result );

/**
 * Removes any cached value for the given key from an jedex_memoize.
 */

void jedex_memoize_delete( jedex_memoize *mem, void *key1, void *key2 );


CLOSE_EXTERN
#endif


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
