/*
 * Messenger context structure and function prototypes.
 *
 * Author: Nick Karanatsios
 *
 * Copyright (C) 2008-2012 NEC Corporation
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


#ifndef MESSENGER_CONTEXT_H
#define MESSENGER_CONTEXT_H


typedef struct messenger_context {
  uint32_t transaction_id;
  int life_count;
  void *user_data;
} messenger_context;


void init_messenger_context();
void age_context_db( void *user_data );
messenger_context *insert_context( void *user_data );
messenger_context *get_context( uint32_t transaction_id );
void delete_messenger_context( messenger_context * );
void delete_context_db();


#endif // MESSENGER_CONTEXT_H
