/*
 * Messenger dump structures and function prototypes.
 *
 * Author: Nick Karanatsios <nickkaranatsios@gmail.com>
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


#ifndef MESSENGER_DUMP_H
#define MESSENGER_DUMP_H


#include <net/if.h>


/* message dump format:
 * +-------------------+--------+------------+----+
 * |message_dump_header|app_name|service_name|data|
 * +-------------------+--------+------------+----+
 */
typedef struct message_dump_header {
  struct {   // same as struct timespec but fixed length
    uint32_t sec;
    uint32_t nsec;
  } sent_time;
  uint16_t app_name_length;
  uint16_t service_name_length;
  uint32_t data_length;
} message_dump_header;


enum {
  MESSENGER_DUMP_SENT,
  MESSENGER_DUMP_RECEIVED,
  MESSENGER_DUMP_RECV_CONNECTED,
  MESSENGER_DUMP_RECV_OVERFLOW,
  MESSENGER_DUMP_RECV_CLOSED,
  MESSENGER_DUMP_SEND_CONNECTED,
  MESSENGER_DUMP_SEND_REFUSED,
  MESSENGER_DUMP_SEND_OVERFLOW,
  MESSENGER_DUMP_SEND_CLOSED,
  MESSENGER_DUMP_LOGGER, 
  MESSENGER_DUMP_PCAP,
  MESSENGER_DUMP_SYSLOG,
  MESSENGER_DUMP_TEXT,
};


typedef struct pcap_dump_header {
  uint32_t datalink;
  uint8_t interface[ IF_NAMESIZE ];
} pcap_dump_header;


typedef struct logger_dump_header {
  struct {
    uint32_t sec;
    uint32_t nsec;
  } sent_time;
} logger_dump_header;


typedef struct syslog_dump_header {
  struct {
    uint32_t sec;
    uint32_t nsec;
  } sent_time;
} syslog_dump_header;


typedef struct text_dump_header {
  struct {
    uint32_t sec;
    uint32_t nsec;
  } sent_time;
} text_dump_header;


#endif // MESSENGER_DUMP_H


void start_messenger_dump( const char *dump_app_name, const char *dump_service_name );
void stop_messenger_dump( void );
bool messenger_dump_enabled( void );
void send_dump_message( uint16_t dump_type, const char *service_name, const void *data, uint32_t data_len );


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
