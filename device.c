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
        printk( KERN_INFO "RTAP: Found dev[%s]\n", netdev->name );
        if( ! strcmp( netdev->name, devname ) )
        {
            printk( KERN_INFO "RTAP: Using dev[%s]\n", devname );
            break;
        } // end if
        netdev = next_net_device( netdev );
    } // end while

    return( netdev );

}

//*****************************************************************************
static struct net_device *
dev_list_add( const char *devname )
{
    device_t *dev = 0;
    struct net_device *netdev = 0;

    // Lookup network device by given name
    netdev = get_devbyname( devname );
    if( ! netdev )
    {
        printk( KERN_ERR "RTAP: Network device not found: dev[%s]\n", devname );
        return( 0 );
    } // end if

    // Allocate new device list item
    dev = kmalloc( sizeof(device_t), GFP_ATOMIC );
    if( ! dev )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: dev[%s]\n", devname );
        return( 0 );
    } // end if

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
dev_list_remove( const char *devname )
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
static int
dev_list_clear( void )
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
dev_list_init( void *func )
{
    spin_lock_init( &devices.lock );
    INIT_LIST_HEAD( &devices.list );
    dev_list_pktfunc = func;
    return( 0 );
}

//*****************************************************************************
int
dev_list_exit( void )
{
    return( dev_list_clear() );
}

//*****************************************************************************
//*****************************************************************************
static int
dev_proc_show( struct seq_file *file, void *arg )
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
dev_proc_open( struct inode *inode, struct file *file )
{
    return( single_open( file, dev_proc_show, NULL ) );
}

//*****************************************************************************
static int
dev_proc_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
dev_proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
dev_proc_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
dev_proc_write( struct file *file, const char __user *buf, size_t cnt,
                  loff_t *off )
{
    char devname[256] = { 0 };
    if( ! cnt )
    {
        dev_list_clear();
    } // end if
    else
    {
        copy_from_user( devname, buf, cnt );
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
    } // end else
    return( cnt );
}

const struct file_operations dev_proc_fops =
{
    .owner      = THIS_MODULE,
    .open       = dev_proc_open,
    .release    = dev_proc_close,
    .read       = dev_proc_read,
    .llseek     = dev_proc_lseek,
    .write      = dev_proc_write,
};

