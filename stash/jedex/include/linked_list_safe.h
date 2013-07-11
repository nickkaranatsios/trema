/*
 * Copyright (C) 2008-2013 NEC Corporation
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


#ifndef LINKED_LIST_SAFE_H
#define LINKED_LIST_SAFE_H


#include <pthread.h>
#include <stdbool.h>


typedef struct list_element_safe {
  struct list_element_safe *next;
  void *data;
} list_element_safe;


typedef struct {
  list_element_safe *head;
  pthread_mutex_t mutex;
} list;


list *create_list_safe();
bool insert_in_front_safe( list *l, void *data );
bool append_to_tail_safe( list *l, void *data );
bool delete_element_safe( list *l, const void *data );
bool delete_list_safe( list *l );
bool delete_list_totally_safe( list *l );


#endif // LINKED_LIST_SAFE_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
