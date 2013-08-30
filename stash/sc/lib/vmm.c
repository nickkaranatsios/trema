/*
 * Physical machine control platform vm control
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libvirt/libvirt.h>
#include <arpa/inet.h>
#include <linked_list.h>
#include <stdbool.h>
#include "wrapper.h"
#include "log_writer.h"
#include "service_controller.h"
#include "ssh.h"
#include "vmm.h"


static const char *URI_PREFIX = "qemu+ssh://";
static const char *URI_SUFFIX = "/system";
static const char *VM_IMAGES_DIR = "/var/lib/libvirt/images";
static const size_t MAX_CONFIG_LENGTH = 1000;
static const size_t MAX_URI_LENGTH = 100;
#define MAX_VM_NAME_LENGTH 20
static const size_t ETH_ADDRLEN = 6;
static const size_t IP_ADDRLEN = 4;

static const char *CONTROL_PLANE_NETMASK = "255.255.252.0";
static const char *CONTROL_PLANE_NAMESERVER = "172.16.19.254";
static const char *DATA_PLANE_NETMASK = "255.255.255.0";
static const char *CONTROL_PLANE_DOMAIN_NAME = "net.spf.cl.nec.co.jp";

static const char *PRIMARY_BRIDGE = "br0";
static const char *SECONDARY_BRIDGE = "br2";

typedef struct {
  uint32_t ip_address;
  uint8_t nbd_number;
} nbd;

typedef struct {
  char name[ MAX_VM_NAME_LENGTH ];
  char service_name[ SERVICE_NAME_LENGTH ];
} template_image;

list_element *nbds;
list_element *template_images;

static void
create_nbds() {
  create_list( &nbds );
}


static void
delete_nbds() {
  delete_list( nbds );
}


static nbd *
lookup_nbd( uint32_t ip_address, uint8_t nbd_number ) {
  for ( const list_element *e = nbds; e != NULL; e = e->next ) {
    nbd *n = e->data;
    if ( ( n->ip_address == ip_address ) && ( n->nbd_number == nbd_number ) ) {
      return n;
    }
  }

  return NULL;
}


uint8_t
create_nbd_number( uint32_t ip_address ) {
  for( uint8_t nbd_number = 0; nbd_number < 16; nbd_number++ ) {
    nbd *n = lookup_nbd( ip_address, nbd_number );
    if ( n == NULL ) {
      return nbd_number;
    }
  }
  return UINT8_MAX;
}

static nbd *
add_nbd( uint32_t ip_address, uint8_t nbd_number ) {
  nbd *n = xmalloc( sizeof( nbd ) );
  n->ip_address = ip_address;
  n->nbd_number = nbd_number;
  return n;
}

static void
free_nbd( nbd *n ) {
  free( n );
}

static uint8_t *
uint32_to_uint8_ip( uint32_t ip_uint32 ) {
  uint8_t *ip = xmalloc( IP_ADDRLEN );
  ip[ 0 ] = ( uint8_t )( ( ip_uint32 >> 24 ) & 0xff );
  ip[ 1 ] = ( uint8_t )( ( ip_uint32 >> 16 ) & 0xff );
  ip[ 2 ] = ( uint8_t )( ( ip_uint32 >> 8 ) & 0xff );
  ip[ 3 ] = ( uint8_t )( ip_uint32 & 0xff );
  return ip;
}

static char *
create_uri( uint32_t host_os_ip_address ) {
  char *uri = xmalloc( MAX_URI_LENGTH );
  uint8_t *ip = uint32_to_uint8_ip( host_os_ip_address );
  sprintf( uri, "%s%d.%d.%d.%d%s", URI_PREFIX, ip[0], ip[1], ip[2], ip[3], URI_SUFFIX );
  return uri;
}

static uint8_t *
uint64_to_uint8_mac( uint64_t mac_uint64 ) {
  uint8_t *mac = xmalloc( ETH_ADDRLEN );
  mac[ 0 ] = ( uint8_t )( ( mac_uint64 >> 40 ) & 0xff );
  mac[ 1 ] = ( uint8_t )( ( mac_uint64 >> 32 ) & 0xff );
  mac[ 2 ] = ( uint8_t )( ( mac_uint64 >> 24 ) & 0xff );
  mac[ 3 ] = ( uint8_t )( ( mac_uint64 >> 16 ) & 0xff );
  mac[ 4 ] = ( uint8_t )( ( mac_uint64 >> 8 ) & 0xff );
  mac[ 5 ] = ( uint8_t )( mac_uint64 & 0xff );

  return mac;
}

static void
initialize_domain_type( char *xmlconfig ) {
  char DOMAIN_TYPE[] = "<domain type='kvm'>";
  sprintf( xmlconfig, "%s", DOMAIN_TYPE );
}

static void
initialize_config( char *xmlconfig ) {
  initialize_domain_type( xmlconfig );  
}

static void
set_name( char *xmlconfig, char *vm_name ) {
  sprintf( xmlconfig, "%s<name>%s</name>", xmlconfig, vm_name );
}

static void
set_memory( char *xmlconfig, uint64_t memory ) {
  sprintf( xmlconfig, "%s<memory>%" PRId64 "</memory>", xmlconfig, memory );
}

static void
set_cpu( char *xmlconfig, uint32_t n_cpu ) {
  sprintf( xmlconfig, "%s<vcpu>%d</vcpu>", xmlconfig, n_cpu );
}

static void
set_os( char *xmlconfig ) {
  char OS_PREFIX[] = "<os>";
  char OS_TYPE[] = "<type arch='x86_64' machine='pc-1.0'>hvm</type>";
  char OS_BOOT[] = "<boot dev='hd'/>";
  char OS_SUFFIX[] = "</os>";

  sprintf( xmlconfig, "%s%s", xmlconfig, OS_PREFIX );
  sprintf( xmlconfig, "%s%s", xmlconfig, OS_TYPE );
  sprintf( xmlconfig, "%s%s", xmlconfig, OS_BOOT );
  sprintf( xmlconfig, "%s%s", xmlconfig, OS_SUFFIX );
}

static void
set_features( char *xmlconfig ) {
  char FEATURES[] = "<features><acpi/></features>";
  sprintf( xmlconfig, "%s%s", xmlconfig, FEATURES );
}

static void
initialize_devices( char *xmlconfig ) {
  char DEVICE_PREFIX[] = "<devices>";
  char DEVICE_EMULATOR[] = "<emulator>/usr/bin/kvm</emulator>";
  sprintf( xmlconfig, "%s%s", xmlconfig, DEVICE_PREFIX );
  sprintf( xmlconfig, "%s%s", xmlconfig, DEVICE_EMULATOR );
}

static void
finalize_devices( char *xmlconfig ) {
  char DEVICE_SUFFIX[] = "</devices>";
  sprintf( xmlconfig, "%s%s", xmlconfig, DEVICE_SUFFIX );
}

static void
set_disk( char *xmlconfig, char *vm_name ) {
  char DISK_PREFIX[] = "<disk type='file' device='disk'>";
  char DISK_DRIVER[] = "<driver name='qemu' type='qcow2'/>";
  char DISK_FILE_PREFIX[] = "<source file='";
  char DISK_FILE_SUFFIX[] = "'/>";
  char DISK_TARGET[] = "<target dev='vda' bus='virtio'/>";
  char DISK_SUFFIX[] = "</disk>";
  sprintf( xmlconfig, "%s%s", xmlconfig, DISK_PREFIX );
  sprintf( xmlconfig, "%s%s", xmlconfig, DISK_DRIVER );
  sprintf( xmlconfig, "%s%s%s/%s.img%s", xmlconfig, DISK_FILE_PREFIX, VM_IMAGES_DIR, vm_name, DISK_FILE_SUFFIX );
  sprintf( xmlconfig, "%s%s", xmlconfig, DISK_TARGET );
  sprintf( xmlconfig, "%s%s", xmlconfig, DISK_SUFFIX );
}

static void
set_interface( char *xmlconfig, const char *bridge, uint64_t mac_address ) {
  char INTERFACE_PREFIX[] = "<interface type='bridge'>";
  char INTERFACE_MAC_PREFIX[] = "<mac address='";
  char INTERFACE_MAC_SUFFIX[] = "'/>";
  char INTERFACE_BRIDGE_PREFIX[] = "<source bridge='";
  char INTERFACE_BRIDGE_SUFFIX[] = "'/>";
  char INTERFACE_MODEL[] = "<model type='virtio'/>";
  char INTERFACE_SUFFIX[] = "</interface>";

  uint8_t *mac = uint64_to_uint8_mac( mac_address );
  sprintf( xmlconfig, "%s%s", xmlconfig, INTERFACE_PREFIX );  
  sprintf( xmlconfig, "%s%s%02x:%02x:%02x:%02x:%02x:%02x%s", xmlconfig, INTERFACE_MAC_PREFIX, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], INTERFACE_MAC_SUFFIX );
  sprintf( xmlconfig, "%s%s%s%s", xmlconfig, INTERFACE_BRIDGE_PREFIX, bridge, INTERFACE_BRIDGE_SUFFIX );
  sprintf( xmlconfig, "%s%s", xmlconfig, INTERFACE_MODEL );
  sprintf( xmlconfig, "%s%s", xmlconfig, INTERFACE_SUFFIX );
}

static void
finalize_domain( char *xmlconfig ) {
  char DOMAIN_SUFFIX[] = "</domain>";
  sprintf( xmlconfig, "%s%s", xmlconfig, DOMAIN_SUFFIX );
}

static void
finalize_config( char *xmlconfig ) {
  finalize_domain( xmlconfig );
}


static char *
create_config( char *vm_name, list_element *ports, uint32_t n_cpus, uint64_t memory_size ) {
  char *config = xmalloc( MAX_CONFIG_LENGTH );
  initialize_config( config );
  set_name( config, vm_name );
  set_memory( config, memory_size );
  set_cpu( config, n_cpus );
  set_os( config );
  set_features( config );
  initialize_devices( config );
  set_disk( config, vm_name );

  list_element *e = ports;
  port_info *port = e->data;
  set_interface( config, PRIMARY_BRIDGE, port->mac_address );
  e = e->next;
  while ( e != NULL ) {
    port = e->data;
    set_interface( config, SECONDARY_BRIDGE, port->mac_address );
    e = e->next;
  }
 
  finalize_devices( config );
  finalize_config( config );
  return config;
}


static bool
create_vm_img( uint32_t ip_address, char *vm_name, const char *vm_template_image ) {
  char command[ MAX_SSH_COMMAND_LENGTH ];
  sprintf( command, "sudo qemu-img create -b %s/%s -f qcow2 %s/%s.img", VM_IMAGES_DIR, vm_template_image, VM_IMAGES_DIR, vm_name );

  char response[ MAX_SSH_RESPONSE_LENGTH ];
  int ret = execute_ssh_command( ip_address, command, response );
  if ( ret == 0 ) {
    return true;
  } else {
    printf("failed to create");
    return false;
  }
}

static void
set_ip_address( uint32_t host_os_ip_address, char *vm_name, list_element *ports, uint8_t nbd_number ) {
  char config[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( config, "auto lo\n" );
  sprintf( config, "%siface lo inet loopback\n", config );
  sprintf( config, "%sauto eth0\n", config );
  sprintf( config, "%siface eth0 inet static\n", config );

  list_element *e = ports;
  port_info *control_plane_port = e->data;
  uint8_t *control_plane_ip = uint32_to_uint8_ip( control_plane_port->ip_address );
  sprintf( config, "%s  address %d.%d.%d.%d\n", config, control_plane_ip[0], control_plane_ip[1], control_plane_ip[2], control_plane_ip[3] );
  sprintf( config, "%s  netmask %s\n", config, CONTROL_PLANE_NETMASK );
  sprintf( config, "%s  dns-nameservers %s\n", config, CONTROL_PLANE_NAMESERVER );
  sprintf( config, "%s  dns-search %s\n", config, CONTROL_PLANE_DOMAIN_NAME );

  e = e->next;
  uint8_t eth_number = 1;
  while ( e != NULL ) {
    port_info *port = e->data;
    uint8_t *ip = uint32_to_uint8_ip( port->ip_address );
    sprintf( config, "%sauto eth%d\n", config, eth_number );
    sprintf( config, "%siface eth%d inet static\n", config, eth_number );
    sprintf( config, "%s  address %d.%d.%d.%d\n", config, ip[0], ip[1], ip[2], ip[3] );
    sprintf( config, "%s  netmask %s\n", config, DATA_PLANE_NETMASK );
    e = e->next;
    eth_number++;
  }

  char command[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( command, "sudo qemu-nbd -c /dev/nbd%d %s/%s.img;", nbd_number, VM_IMAGES_DIR, vm_name );
  sprintf( command, "%s sudo mount /dev/nbd%dp1 /mnt;", command, nbd_number );
  sprintf( command, "%s sudo sh -c \"echo '%s' > /mnt/etc/network/interfaces\";", command, config );
  sprintf( command, "%s sudo umount /mnt;", command );
  sprintf( command, "%s sudo qemu-nbd -d /dev/nbd%d", command, nbd_number );
  char response[ MAX_SSH_RESPONSE_LENGTH ];
  int ret = execute_ssh_command( host_os_ip_address, command, response );
  if ( ret != RET_OK ) {
    printf("Failed to set ip address");
  }

  return;
}


static void
set_hostname( uint32_t host_os_ip_address, char *vm_name, uint8_t nbd_number ) {
  char config[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( config, "%s", vm_name );
  char command[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( command, "sudo qemu-nbd -c /dev/nbd%d %s/%s.img;", nbd_number, VM_IMAGES_DIR, vm_name );
  sprintf( command, "%s sudo mount /dev/nbd%dp1 /mnt;", command, nbd_number );
  sprintf( command, "%s sudo sh -c \"echo '%s' > /mnt/etc/hostname\";", command, config );
  sprintf( command, "%s sudo umount /mnt;", command );
  sprintf( command, "%s sudo qemu-nbd -d /dev/nbd%d", command, nbd_number );
  char response[ MAX_SSH_RESPONSE_LENGTH ];
  int ret = execute_ssh_command( host_os_ip_address, command, response );
  if ( ret != RET_OK ) {
    printf("Failed to set ip address");
  }

  return;
}


static void
set_hosts( uint32_t host_os_ip_address, char *vm_name, uint8_t nbd_number ) {
  char config[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( config, "127.0.0.1\tlocalhost\n" );
  sprintf( config, "%s127.0.1.1\t%s\n", config, vm_name );

  char command[ MAX_SSH_RESPONSE_LENGTH ];
  sprintf( command, "sudo qemu-nbd -c /dev/nbd%d %s/%s.img;", nbd_number, VM_IMAGES_DIR, vm_name );
  sprintf( command, "%s sudo mount /dev/nbd%dp1 /mnt;", command, nbd_number );
  sprintf( command, "%s sudo sh -c \"echo '%s' > /mnt/etc/hosts\";", command, config );
  sprintf( command, "%s sudo umount /mnt;", command );
  sprintf( command, "%s sudo qemu-nbd -d /dev/nbd%d", command, nbd_number );
  char response[ MAX_SSH_RESPONSE_LENGTH ];
  int ret = execute_ssh_command( host_os_ip_address, command, response );
  if ( ret != RET_OK ) {
    printf("Failed to set ip address");
  }

  return;
}


static void
create_template_images() {
  create_list( &template_images );
}


static void
delete_template_images() {
  delete_list( template_images );
}

static void
add_template_image( const char *template_image_name, const char *service_name ) {
  template_image *template = xmalloc( sizeof( template_image ) );
  strcpy( template->name, template_image_name );
  strcpy( template->service_name, service_name );
  append_to_tail( &template_images, template );
}

static template_image *
lookup_template_image( const char *service_name ) {
  for ( const list_element *e = template_images; e != NULL; e = e->next ) {
    template_image *template = e->data;
    if ( strcmp( template->service_name, service_name ) == 0 ) {
      return template;
    }
  }

  return NULL;
}


int
create_vm( uint32_t host_os_ip_address,
           char *vm_name,
           list_element *ports,
           uint32_t n_cpus,
           uint64_t memory_size,
           const char *service_name ) {
  template_image *template = lookup_template_image( service_name );
  bool ret = create_vm_img( host_os_ip_address, vm_name, template->name );
  if ( ret != true ) {
    return 1;
  }

  uint8_t nbd_number = create_nbd_number( host_os_ip_address );
  while ( nbd_number == UINT8_MAX ) {
    sleep( 1 );
    nbd_number = create_nbd_number( host_os_ip_address );
  }
  nbd *n = add_nbd( host_os_ip_address, nbd_number );
  set_ip_address( host_os_ip_address, vm_name, ports, nbd_number );
  set_hostname( host_os_ip_address, vm_name, nbd_number );
  set_hosts( host_os_ip_address, vm_name, nbd_number );
  free_nbd( n );

  char *uri = create_uri( host_os_ip_address );
  virConnectPtr conn = virConnectOpen( uri );
  if ( conn == NULL ) {
    log_warning("Failed to open connection to %s\n", uri );
    free( uri );
    return 1;
  }

  char *config = create_config( vm_name, ports, n_cpus, memory_size );
  virDomainPtr dom = virDomainCreateXML( conn, config, 0 );
  if ( !dom ) {
    log_warning("Domain creation failed");
    virConnectClose(conn);
    free( config );
    return 0;
  }

  free( config );
  free( uri );
  virConnectClose(conn);
  return 0;
}

static bool
check_wakeup_vm( uint32_t ip_address ) {
  FILE *Pping;
  char command[ BUFSIZ ], buf[ BUFSIZ ];
  uint8_t *ip = uint32_to_uint8_ip( ip_address );
  snprintf( command, BUFSIZ, "ping -c 1 %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
  if ( ( Pping = popen( command, "r" ) ) == NULL ) {
    log_err( "Failed to execute ping to %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3] );
    return false;
  }
  for ( int i = 0; i < 5; i++ ) {
    if ( fgets( buf, BUFSIZ, Pping ) == NULL ) {
      return false;
    }
  }
  int n_recieved;
  sscanf( buf, "1 packets transmitted, %d received,", &n_recieved );
  if ( n_recieved == 1 ) {
    return true;
  } else {
    return false;
  }
}

void
wait_for_wakeup_vm( uint32_t ip_address ) {
  bool vm_is_wokeup = check_wakeup_vm( ip_address );
  while ( vm_is_wokeup == false ) {
    vm_is_wokeup = check_wakeup_vm( ip_address );
  }
}

static bool
delete_vm_img( uint32_t ip_address, char *vm_name ) {
  char command[ MAX_SSH_COMMAND_LENGTH ];
  sprintf( command, "sudo rm %s/%s.img", VM_IMAGES_DIR, vm_name );
  char response[ MAX_SSH_RESPONSE_LENGTH ];
  int ret = execute_ssh_command( ip_address, command, response );
  if ( ret == 0 ) {
    return true;
  } else {
    return false;
  }
}

int
destroy_vm( uint32_t host_os_ip_address, char vm_name[] ) {
  char *uri = create_uri( host_os_ip_address );
  virConnectPtr conn = virConnectOpen( uri );
  if (conn == NULL) {
    log_warning("Failed to open connection to %s\n", uri );
    return 1;
  }

  virDomainPtr dom = virDomainLookupByName( conn, vm_name );
  if ( !dom ) {
    log_warning("No such vm");
    return 0;
  }
  int ret = virDomainDestroy( dom );

  free( uri );
  virConnectClose(conn);
  delete_vm_img( host_os_ip_address, vm_name );
  return ret;
}

bool
initialize_vmm() {
  create_nbds();
  create_template_images();
  add_template_image( "template_bras.img", SERVICE_NAME_BRAS );
  add_template_image( "template_iptv.img", SERVICE_NAME_IPTV );
  return true;
}

bool
finalize_vmm() {
  delete_nbds();
  delete_template_images();
  return true;
}

/*
 * Local variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

