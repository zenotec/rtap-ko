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
//    File: listener.c
//    Description: 
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/if_ether.h>
#include <linux/byteorder/generic.h>

#include "listener.h"
#include "ksocket.h"

typedef struct
{
    struct list_head list;
    spinlock_t lock;
    struct sockaddr_in in_addr;
    ksocket_t sockfd;
} listener_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static listener_t listeners = { { 0 } };

static struct proc_dir_entry *proc_devs = NULL;

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
static int
ip_list_add( const char *ipaddr )
{
    listener_t *listener = NULL;

    // Allocate new listener list item
    listener = kmalloc( sizeof( listener_t ), GFP_ATOMIC );
    if( ! listener )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: %s", ipaddr );
        return( -1 );
    } // end if

    // Convert ip string to socket address
    memset( &listener->in_addr, 0, sizeof( listener->in_addr ) );
    listener->in_addr.sin_family = AF_INET;
    listener->in_addr.sin_port = htons( 8888 );
    listener->in_addr.sin_addr.s_addr = inet_addr( ipaddr );
    listener->sockfd = ksocket( AF_INET, SOCK_DGRAM, 0 );
    if( listener->sockfd == NULL )
    {
        printk( KERN_ERR "RTAP: Cannot create socket\n" );
        kfree( listener );
        return( -1 );
    } // end if

    // Add device list item to tail of device list
    spin_lock( &listeners.lock );
    list_add_tail( &listener->list, &listeners.list );
    spin_unlock( &listeners.lock );

    // Return NULL on success; negative on error
    return( 0 );
}

//*****************************************************************************
static int
ip_list_remove( const char *ipaddr )
{
    listener_t *listener = NULL;
    listener_t *tmp = NULL;
    char *ipstr = 0;
    int ret = -1;

    // Search for listener in list and remove
    spin_lock( &listeners.lock );
    list_for_each_entry_safe( listener, tmp, &listeners.list, list )
    {
        ipstr = inet( listener->ipaddr )
        if( ! strcmp( str, ipaddr ) )
        {
            list_del( &listener->list );
            kfree( listener );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &listeners.lock );

    // Return NULL on success; negative on error
    return( ret );
}

//*****************************************************************************
static int
ip_list_clear( void )
{
    listener_t *listener = NULL;
    listener_t *tmp = NULL;

    // Remove all listeners from list
    spin_lock( &listeners.lock );
    list_for_each_entry_safe( listener, tmp, &listeners.list, list )
    {
        list_del( &listener->list );
        kfree( listener );
    } // end loop 
    spin_unlock( &listeners.lock );

    return( 0 );
}

//*****************************************************************************
int
ip_list_init( void *func )
{
    spin_lock_init( &listeners.lock );
    INIT_LIST_HEAD( &listeners.list );
    proc_devs = proc_create( "rtap_listener", 0644, NULL, &ip_proc_fops );
    return( 0 );
}

//*****************************************************************************
int
ip_list_exit( void )
{
    remove_proc_entry( "rtap_devs", NULL );
    return( ip_list_clear() );
}

//*****************************************************************************
//*****************************************************************************
static int
ip_proc_show( struct seq_file *file, void *arg )
{
    listener_t *listener = NULL;
    listener_t *tmp = NULL;

    // Iterate over all listeners in list
    spin_lock( &listeners.lock );
    list_for_each_entry_safe( listener, tmp, &listeners.list, list )
    {
        //seq_printf( file, "%s\n", l->name );
    } // end loop 
    spin_unlock( &listeners.lock );

    return( 0 );
}

//*****************************************************************************
static int
ip_proc_open( struct inode *inode, struct file *file )
{
    return( single_open( file, ip_proc_show, NULL ) );
}

//*****************************************************************************
static int
ip_proc_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
ip_proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
ip_proc_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
ip_proc_write( struct file *file, const char __user *buf, size_t cnt,
                  loff_t *off )
{
    char ipaddr[256] = { 0 };
    if( ! cnt )
    {
        ip_list_clear();
    } // end if
    else
    {
        copy_from_user( ipaddr, buf, cnt );
        if( ipaddr[0] == '-' )
        {
            ip_list_remove( &ipaddr[1] );
        } // end if
        else if( ipaddr[0] == '+' )
        {
            ip_list_add( &ipaddr[1] );
        } // end else
        else
        {
            ip_list_add( ipaddr );
        } // end else
    } // end else
    return( cnt );
}

const struct file_operations ip_proc_fops =
{
    .owner      = THIS_MODULE,
    .open       = ip_proc_open,
    .release    = ip_proc_close,
    .read       = ip_proc_read,
    .llseek     = ip_proc_lseek,
    .write      = ip_proc_write,
};

