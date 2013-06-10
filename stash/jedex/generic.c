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
#include <string.h>
#include <assert.h>
#include "jedex_schema.c"
#include "generic_boolean.c"
#include "generic_int.c"
#include "generic_long.c"
#include "generic_float.c"
#include "generic_double.c"
#include "generic_null.c"
#include "generic_string.c"
#include "generic_array.c"


int
main( int argc, char **argv ) {
  jedex_value_iface *iface = jedex_generic_boolean_class();
  jedex_value_iface *iface1 = jedex_generic_boolean_class();

  assert( iface == iface1 );
}


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
