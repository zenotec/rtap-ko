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
//    File: device.c
//    Description: 
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "device.h"

typedef struct
{
    struct list_head list;
    spinlock_t lock;
    struct net_device *netdev;
    struct packet_type pt;
} device_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static device_t devices = { { 0 } };
static void *dev_list_pktfunc = NULL;

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
static struct net_device *
get_devbyname( const char *devname )
{

    struct net_device *netdev = first_net_device( &init_net );
    while ( netdev )
    {
        if( ! strcmp( netdev->name, devname ) )
        {
            printk( KERN_INFO "RTAP: Found device: %s\n", netdev->name );
            break;
        } // end if
        netdev = next_net_device( netdev );
    } // end while

    return( netdev );

}

//*****************************************************************************
static int
device_remove( const char *devname )
{
    device_t *dev = 0;
    device_t *tmp = 0;
    int ret = -1;

    // Search for device in list and remove
    spin_lock( &devices.lock );
    list_for_each_entry_safe( dev, tmp, &devices.list, list )
    {
        if( ! strcmp( dev->netdev->name, devname ) )
        {
            printk( KERN_INFO "RTAP: Removing device: %s\n", dev->netdev->name );
            dev_remove_pack( &dev->pt );
            list_del( &dev->list );
            kfree( dev );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &devices.lock );

    // Return non-null network device pointer on success; null on error
    return( ret );
}

//*****************************************************************************
static struct net_device *
device_add( const char *devname )
{
    device_t *dev = 0;
    struct net_device *netdev = 0;

    // First remove any existing devices with same name
    dev_list_remove( devname );

    // Lookup network device by given name
    netdev = get_devbyname( devname );
    if( ! netdev )
    {
        printk( KERN_ERR "RTAP: Device '%s' not found\n", devname );
        return( 0 );
    } // end if

    // Allocate new device list item
    dev = kmalloc( sizeof(device_t), GFP_ATOMIC );
    if( ! dev )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: dev[%s]\n", devname );
        return( 0 );
    } // end if
    memset( (void *)dev, 0, sizeof( device_t ) );

    // Populate device list item
    dev->netdev = netdev;
    dev->pt.type = htons(ETH_P_ALL);
    dev->pt.func = dev_list_pktfunc;

    // Add device list item to tail of device list
    spin_lock( &devices.lock );
    list_add_tail( &dev->list, &devices.list );
    spin_unlock( &devices.lock );

    // Register for packet
    dev_add_pack( &dev->pt );

    // Return non-null network device pointer on success; null on error
    return( netdev );
}

//*****************************************************************************
static int
device_clear( void )
{
    device_t *dev = 0;
    device_t *tmp = 0;

    // Remove all devices from list
    spin_lock( &devices.lock );
    list_for_each_entry_safe( dev, tmp, &devices.list, list )
    {
        printk( KERN_INFO "RTAP: Removing device: %s\n", dev->netdev->name );
        dev_remove_pack( &dev->pt );
        list_del( &dev->list );
        kfree( dev );
    } // end loop 
    spin_unlock( &devices.lock );

    return( 0 );
}

//*****************************************************************************
int
device_init( void *func )
{
    spin_lock_init( &devices.lock );
    INIT_LIST_HEAD( &devices.list );
    return( 0 );
}

//*****************************************************************************
int
device_exit( void )
{
    return( device_clear() );
}

//*****************************************************************************
//*****************************************************************************
static int
device_show( struct seq_file *file, void *arg )
{
    device_t *dev = 0;
    device_t *tmp = 0;

    // Iterate over all devices in list
    spin_lock( &devices.lock );
    list_for_each_entry_safe( dev, tmp, &devices.list, list )
    {
        seq_printf( file, "dev[%s]\n", dev->netdev->name );
    } // end loop 
    spin_unlock( &devices.lock );

    return( 0 );
}

//*****************************************************************************
static int
device_open( struct inode *inode, struct file *file )
{
    return( single_open( file, device_show, NULL ) );
}

//*****************************************************************************
static int
device_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
device_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
device_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
device_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
    char devstr[256+1] = { 0 };
    char devname[256+1] = { 0 };
    int ret = 0;

    cnt = (cnt >= 256) ? 256 : cnt;
    copy_from_user( devstr, buf, cnt );
    ret = sscanf( devstr, "%256s", devname );

    if( (ret == 1) && (strlen(devname) == 1) && (devname[0] == '-') )
    {
        dev_list_clear();
    } // end if
    else if( (ret == 1) && (strlen(devname) > 1) )
    {
        if( devname[0] == '-' )
        {
            dev_list_remove( &devname[1] );
        } // end if
        else if( devname[0] == '+' )
        {
            dev_list_add( &devname[1] );
        } // end else
        else
        {
            dev_list_add( devname );
        } // end else
    } // end else if
    else
    {
        printk( KERN_ERR "RTAP: Failed parsing device string: %s\n", devstr );
        return( -1 );
    } // end else

    return( cnt );

}

//*****************************************************************************
//*****************************************************************************
const struct file_operations device_fops =
{
    .owner      = THIS_MODULE,
    .open       = device_open,
    .release    = device_close,
    .read       = device_read,
    .llseek     = device_lseek,
    .write      = device_write,
};

