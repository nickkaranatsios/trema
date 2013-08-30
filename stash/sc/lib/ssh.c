/*
 * Physical machine control platform ssh
 *
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

#include <libssh2.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "log_writer.h"
#include <stdbool.h>
#include "ssh.h"

static const int SSH_PORT = 22;
static const char *PASSPHRASE = "";

static char *
get_publickey () {
  char *publickey = malloc( 100 );
  sprintf( publickey, "/home/%s/.ssh/id_rsa.pub", getlogin() );
  return publickey;
}

static char *
get_privatekey () {
  char *privatekey = malloc( 100 );
  sprintf( privatekey, "/home/%s/.ssh/id_rsa", getlogin() );
  return privatekey;
}
 
static int
waitsocket( int socket_fd, LIBSSH2_SESSION *session ) {
  struct timeval timeout;
  timeout.tv_sec = 10;
  timeout.tv_usec = 0;
 
  fd_set fd;
  FD_ZERO( &fd );
 
  FD_SET( ( long unsigned int )socket_fd, &fd );
 
  int dir = libssh2_session_block_directions( session );

  fd_set *readfd = NULL; 
  if ( dir & LIBSSH2_SESSION_BLOCK_INBOUND ) {
    readfd = &fd;
  }

  fd_set *writefd = NULL;
  if( dir & LIBSSH2_SESSION_BLOCK_OUTBOUND ) {
    writefd = &fd;
  }
  int rc = select( socket_fd + 1, readfd, writefd, NULL, &timeout );
  return rc;
}

bool
initialize_ssh() {
  int rc = libssh2_init( 0 );
  if ( rc != 0 ) {
    log_err("libssh2 initialization failed (%d)\n", rc);
    return false;
  }
  return true;
}

static int
open_ssh_socket( char *hostname ) {
  unsigned long hostaddr = inet_addr( hostname );

  int sock = socket( AF_INET, SOCK_STREAM, 0 );
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons( ( uint16_t ) SSH_PORT );
  sin.sin_addr.s_addr = ( uint32_t )hostaddr;
  if ( connect( sock, ( struct sockaddr* )( &sin ), sizeof(struct sockaddr_in ) ) != 0 ) {
    log_err("Failed to open socket\n");
    return -1;
  }
  return sock;
}

static LIBSSH2_SESSION *
establish_ssh_session( int sock ) {

  LIBSSH2_SESSION *session = libssh2_session_init();
  if ( !session ) {
    return NULL;
  }

  libssh2_session_set_blocking( session, 0 );
  int rc;
  while ( ( rc = libssh2_session_handshake( session, sock ) ) == LIBSSH2_ERROR_EAGAIN );
  if ( rc ) {
    log_err("Failed to establish ssh session: %d\n", rc);
    return NULL;
  }
  return session;

}

static int
check_known_hosts( char *hostname, LIBSSH2_SESSION *session ) {
  LIBSSH2_KNOWNHOSTS *nh = libssh2_knownhost_init( session );
  if( !nh ) {
    return 2;
  }

  libssh2_knownhost_readfile( nh, "known_hosts", LIBSSH2_KNOWNHOST_FILE_OPENSSH );
  libssh2_knownhost_writefile( nh, "dumpfile", LIBSSH2_KNOWNHOST_FILE_OPENSSH);
  
  size_t len;
  int type;
  const char *fingerprint = libssh2_session_hostkey( session, &len, &type );

  if( fingerprint ) {
    struct libssh2_knownhost *host;
    libssh2_knownhost_checkp( nh, hostname, SSH_PORT, fingerprint, len,
                              LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &host);
  } else {
    return 3;
  }
  libssh2_knownhost_free( nh );
  return 0;
}

static int
authenticate_user( LIBSSH2_SESSION *session ) {
  int rc;
  while ( ( rc = libssh2_userauth_publickey_fromfile_ex( session, getlogin(),
                                                         ( unsigned int )strlen( getlogin() ), get_publickey(),
                                                         get_privatekey(), PASSPHRASE ) ) == LIBSSH2_ERROR_EAGAIN );
  if ( rc ) {
    log_warning("Authentication by public key failed\n");
    return -1;
  }
  return 0;
}

static LIBSSH2_CHANNEL *
create_channel( int sock, LIBSSH2_SESSION *session ) {
  LIBSSH2_CHANNEL *channel;
  while( ( ( channel = libssh2_channel_open_session( session ) ) == NULL ) &&
         ( libssh2_session_last_error( session, NULL, NULL, 0 ) == LIBSSH2_ERROR_EAGAIN ) ) {
    waitsocket( sock, session );
  }
  if( channel == NULL ) {
    log_err("Failed to create channel");
    return NULL;
  }
  return channel;
}

static void 
execute_command( int sock, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel, char *command, char *response ){
  int rc;
while( ( rc = libssh2_channel_process_startup( channel, "exec", sizeof("exec") - 1, command, ( unsigned int )strlen( command ) ) ) == LIBSSH2_ERROR_EAGAIN ) {
    waitsocket( sock, session );
  }

/*
  while( ( rc = ( int )libssh2_channel_exec( channel, command ) ) == LIBSSH2_ERROR_EAGAIN ) {
    waitsocket( sock, session );
  }
*/
  if ( rc != 0 ) {
    return;
  }
  for( ;; ) {
    do {
      char channel_response[ MAX_SSH_RESPONSE_LENGTH ];
      rc = ( int )libssh2_channel_read( channel, channel_response, sizeof( channel_response ) );
      if ( rc > 0 ) {
        strcat( response, channel_response );
      }
    } while( rc > 0 );

    if ( rc == LIBSSH2_ERROR_EAGAIN ) {
      waitsocket( sock, session );
    } else {
      break;
    }
  }
}

static int 
close_channel( int sock, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel ) {
  int rc;
  while ( ( rc = libssh2_channel_close( channel ) ) == LIBSSH2_ERROR_EAGAIN ) {
    waitsocket( sock, session );
  }

  int exitcode = 127;
  if ( rc == 0 ) {
    exitcode = libssh2_channel_get_exit_status( channel );
    char EXIT_SIGNAL[ 4 ] = "none";
    libssh2_channel_get_exit_signal( channel, (char **)&EXIT_SIGNAL, NULL, NULL, NULL, NULL, NULL);
  }
  libssh2_channel_free( channel );
  return exitcode;
}

static void
close_ssh_session( LIBSSH2_SESSION *session ){
  libssh2_session_disconnect( session, "" );
  libssh2_session_free( session );
}

static void
close_ssh_socket( int sock ) {
  close( sock );
}

bool
finalize_ssh() {
  libssh2_exit();
  return true;
}

int 
execute_ssh_command( uint32_t ip_address, char *command, char *response ) {
  struct in_addr addr;
  addr.s_addr = htonl( ip_address );
  char *hostname = inet_ntoa( addr );
 
  int sock = open_ssh_socket( hostname );
  if ( sock == -1 ) {
    return 1;
  }

  LIBSSH2_SESSION *session = establish_ssh_session( sock );
  if ( session == NULL ) {
    return 1;
  }

  if ( check_known_hosts( hostname, session ) != 0 ) {
    return 1;
  }

  if ( authenticate_user( session ) != 0 ) {
    return 1;
  }

  LIBSSH2_CHANNEL *channel = create_channel( sock, session );
  if ( channel == NULL ) {
    return 1;
  }

  execute_command( sock, session, channel, command, response );

  int exitcode = close_channel( sock, session, channel );

  close_ssh_session( session );
  close_ssh_socket( sock );

  return exitcode;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
