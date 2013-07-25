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


/*
 * Parse command line arguments.
 * Read schema using jedex_initialize
 * Initialize and connect to db.
 */
static void
mapper_init( int argc, char **argv ) {
  mapper_args *args = xmalloc( sizeof( *args ) );
  parse_options( args, argc, argv );
  mapper *mapper = xmalloc( sizeof( *mapper ) );
  db_init( mapper );
  db_connect( mapper, args );
}


int
main( int argc, char **argv ) {
  UNUSED( argc );
  UNUSED( argv );

  mapper_init( argc, argv );
  // after initialization call emirates main loop
  return 0;
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
