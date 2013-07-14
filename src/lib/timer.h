/*
 * Timer events and callbacks.
 *
 * Author: Yasuhito Takamiya <yasuhito@gmail.com>
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


#ifndef TIMER_H
#define TIMER_H


#include <stdbool.h>
#include <time.h>

#ifdef __APPLE__ && __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 0
struct itimerspec {
  struct timespec it_interval;
  struct timespec it_value;
};

int clock_gettime( int clk_id, struct timespec* ts );
#endif


typedef void ( *timer_callback )( void *user_data );


extern bool ( *init_timer )( void );
extern bool ( *finalize_timer )( void );

extern bool ( *add_timer_event_callback )( struct itimerspec *interval, timer_callback callback, void *user_data );
extern bool ( *add_periodic_event_callback )( const time_t seconds, timer_callback callback, void *user_data );

extern bool ( *delete_timer_event )( timer_callback callback, void *user_data );

extern void ( *execute_timer_events )( int *next_timeout_usec );


#endif // TIMER_H


/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
