#ifndef ALLOCATION_H
#define ALLOCATION_H

#define raw_malloc( sz ) default_allocator( 0, ( sz ) )
#define raw_realloc( ptr, nsz ) default_allocator( ( ptr ), ( nsz ) )
#define raw_free( ptr ) default_allocator( ( ptr ), 0 )

#endif
