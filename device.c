//*****************************************************************************
//    Copyright (C) 2014 ZenoTec LLC (http://www.zenotec.net)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 021100301 USA.
//
//    File: rule.c
//    Description: TODO: Replace temp rules with those configurable from
//                 user space.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/list.h>

#include "device.h"

typedef struct
{
    struct list_head list;
    spinlock_t lock;
    struct net_device* netdev;
} device_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static device_t devices = { 0 };

//*****************************************************************************
// Functions
//*****************************************************************************

struct net_device* get_devbyname( const char* devname_ )
{

    struct net_device* netdev = first_net_device( &init_net );
    while ( netdev )
    {
        printk( KERN_INFO "found [%s]\n", netdev->name );
        if( ! strcmp( netdev->name, dev ) )
        {
            printk( "Using device %s!\n", dev );
            break;
        } // end if
        netdev = next_net_device( netdev );
    } // end while

    return( netdev );

}

int device_init( void )
{
    spin_lock_init( &devices.lock );
    INIT_LIST_HEAD( &devices.list );
    return( 0 );
}

int device_exit( void )
{
}

int device_add( const char* devname_ )
{
    device_t* dev = 0;
    struct net_device* netdev = get_devbyname( devname_ );
    if( ! netdev )
    {
        printk( KERN_ERR "Network device not found: %s", devname_ );
        return( 0 );
    } // end if

    // Allocate new device list item
    dev = kmalloc( sizeof(device_t), GFP_ATOMIC );
    if( ! dev )
    {
        printk( KERN_CRIT "Cannot allocate memory: %s", devname_ );
        return( 0 );
    } // end if

    // Populate device list item
    dev->netdev = netdev;

    // Add device list item to tail of device list
    spin_lock( &devices.lock );
    list_add_tail( &dev->list, &devices.list );
    spin_unlock( &devices.lock );

    // Return network device pointer 
    return( netdev );
}

int device_remove( const char* devname_ )
{
    device_t* dev = 0;
    struct net_device* netdev = get_devbyname( devname_ );
    if( ! netdev )
    {
        return( 0 );
    } // end if

    // Remove device from list
    spin_lock( &devices.lock );
    list_del( &dev->list );
    spin_unlock( &devices.lock );

    // Return non-null network device pointer indicating success
    return( netdev );
}
