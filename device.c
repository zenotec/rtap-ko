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

#include "device.h"

typedef struct
{
    struct list_head list;
    spinlock_t lock;
    struct net_device *netdev;
} device_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static device_t devices = { { 0 } };

static struct proc_dir_entry *proc_devs = NULL;

static int dev_proc_open( struct inode *inode, struct file *file );
static int dev_proc_close( struct inode *inode, struct file *file );
static ssize_t dev_proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off );
static ssize_t dev_proc_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off );

static const struct file_operations proc_fops =
{
    .owner      = THIS_MODULE,
    .open       = dev_proc_open,
    .release    = dev_proc_close,
    .read       = dev_proc_read,
    .write      = dev_proc_write,
};

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
static struct net_device *get_devbyname( const char *devname_ )
{

    struct net_device *netdev = first_net_device( &init_net );
    while ( netdev )
    {
        printk( KERN_INFO "found [%s]\n", netdev->name );
        if( ! strcmp( netdev->name, devname_ ) )
        {
            printk( "Using device %s!\n", devname_ );
            break;
        } // end if
        netdev = next_net_device( netdev );
    } // end while

    return( netdev );

}

//*****************************************************************************
int dev_list_init( void )
{
    spin_lock_init( &devices.lock );
    INIT_LIST_HEAD( &devices.list );
    proc_devs = create_proc_entry( "rtap_devs", 0644, NULL );
    proc_devs->proc_fops = &proc_fops;
    return( 0 );
}

//*****************************************************************************
int dev_list_exit( void )
{
    remove_proc_entry( "rtap_devs", NULL );
    return( dev_list_clear() );
}

//*****************************************************************************
struct net_device *dev_list_add( const char *devname_ )
{
    device_t *dev = 0;
    struct net_device *netdev = 0;

    // Lookup network device by given name
    netdev = get_devbyname( devname_ );
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

    // Return non-null network device pointer on success; null on error
    return( netdev );
}

//*****************************************************************************
struct net_device *dev_list_remove( const char *devname_ )
{
    struct list_head *p = 0;
    struct list_head *n = 0;
    device_t *dev = 0;
    struct net_device *netdev = 0;

    // Search for device in list and remove
    spin_lock( &devices.lock );
    list_for_each_safe( p, n, &devices.list )
    {
        dev = list_entry( p, device_t, list );
        if( ! strcmp( dev->netdev->name, devname_ ) )
        {
            netdev = dev->netdev;
            list_del( &dev->list );
            kfree( dev );
            break;
        } // end if
    } // end loop 
    spin_unlock( &devices.lock );

    // Return non-null network device pointer on success; null on error
    return( netdev );
}

int dev_list_clear( void )
{
    device_t *dev = 0;
    device_t *tmp = 0;

     // Remove all devices from list
    spin_lock( &devices.lock );
    list_for_each_entry_safe( dev, tmp, &devices.list, list )
    {
        list_del( &dev->list );
        kfree( dev );
    } // end loop 
    spin_unlock( &devices.lock );

    return( 0 );
}

static int dev_proc_open( struct inode *inode, struct file *file )
{
    printk( KERN_INFO "RTAP: dev_proc_open()" );
    try_module_get( THIS_MODULE );
    return( 0 );
}

static int dev_proc_close( struct inode *inode, struct file *file )
{
    printk( KERN_INFO "RTAP: dev_proc_close()" );
    module_put( THIS_MODULE );
    return( 0 );
}

static ssize_t dev_proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    printk( KERN_INFO "RTAP: dev_proc_read()" );
    return( 0 );
}

static ssize_t dev_proc_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
    printk( KERN_INFO "RTAP: dev_proc_write()" );
    return( 0 );
}

