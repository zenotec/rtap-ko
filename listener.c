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

#include <linux/module.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/if_ether.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/byteorder/generic.h>

#include "listener.h"
#include "ksocket.h"

typedef struct rtap_listener
{
    struct list_head list;
    spinlock_t lock;
    const char *ipaddr;
    struct sockaddr_in in_addr;
    ksocket_t sockfd;
} rtap_listener_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static rtap_listener_t rtap_listeners = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
static int
listener_add( const char *ipaddr, const short port )
{
    rtap_listener_t *listener = NULL;

    // Allocate new listener list item
    listener = kmalloc( sizeof( rtap_listener_t ), GFP_ATOMIC );
    if( ! listener )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: %s\n", ipaddr );
        return( -1 );
    } // end if
    memset( (void *)listener, 0, sizeof( rtap_listener_t ) );

    // Convert IP string to socket address
    memset( &listener->in_addr, 0, sizeof( listener->in_addr ) );
    listener->in_addr.sin_family = AF_INET;
    inet_aton( ipaddr, &listener->in_addr.sin_addr );
    listener->in_addr.sin_port = htons( port );
    listener->sockfd = ksocket( AF_INET, SOCK_DGRAM, 0 );
    if( listener->sockfd == NULL )
    {
        printk( KERN_ERR "RTAP: Cannot create socket\n" );
        kfree( listener );
        return( -1 );
    } // end if

    // Save IP string
    listener->ipaddr = inet_ntoa( listener->in_addr.sin_addr ); 
    if( ! listener->ipaddr )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: %s\n", ipaddr );
        kfree( listener );
        return( -1 );
    } // end if

    // Add device list item to tail of device list
    printk( KERN_INFO "RTAP: Adding listener: %s:%hu\n", listener->ipaddr, port );
    spin_lock( &rtap_listeners.lock );
    list_add_tail( &listener->list, &rtap_listeners.list );
    spin_unlock( &rtap_listeners.lock );

    // Return NULL on success; negative on error
    return( 0 );
}

//*****************************************************************************
static int
listener_remove( const char *ipaddr, const short port )
{
    rtap_listener_t *listener = NULL;
    rtap_listener_t *tmp = NULL;
    int ret = -1;

    // Search for listener in list and remove
    spin_lock( &rtap_listeners.lock );
    list_for_each_entry_safe( listener, tmp, &rtap_listeners.list, list )
    {
        if( ! strcmp( listener->ipaddr, ipaddr ) && (listener->in_addr.sin_port == htons( port )) )
        {
            printk( KERN_INFO "RTAP: Removing listener: %s:%hu\n",
                    listener->ipaddr, ntohs(listener->in_addr.sin_port) );
            list_del( &listener->list );
            kfree( listener->ipaddr );
            kfree( listener );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &rtap_listeners.lock );

    // Return NULL on success; negative on error
    return( ret );
}

//*****************************************************************************
static int
listener_clear( void )
{
    rtap_listener_t *listener = NULL;
    rtap_listener_t *tmp = NULL;

    // Remove all listeners from list
    spin_lock( &rtap_listeners.lock );
    list_for_each_entry_safe( listener, tmp, &rtap_listeners.list, list )
    {
        printk( KERN_INFO "RTAP: Removing listener: %s:%hu\n",
                listener->ipaddr, ntohs(listener->in_addr.sin_port) );
        list_del( &listener->list );
        kfree( listener->ipaddr );
        kfree( listener );
    } // end loop 
    spin_unlock( &rtap_listeners.lock );

    return( 0 );
}

//*****************************************************************************
int
listener_init( void )
{
    spin_lock_init( &rtap_listeners.lock );
    INIT_LIST_HEAD( &rtap_listeners.list );
    return( 0 );
}

//*****************************************************************************
int
listener_send( struct rtap_listener* l, struct sk_buff* skb )
{

    ksendto( l->sockfd, skb->data, skb->len, 0,
             (const struct sockaddr *)&l->in_addr, sizeof( l->in_addr ) );

    return( 0 );
}
//*****************************************************************************
int
listener_exit( void )
{
    return( listener_clear() );
}

//*****************************************************************************
//*****************************************************************************
static int
proc_show( struct seq_file *file, void *arg )
{
    rtap_listener_t *listener = NULL;
    rtap_listener_t *tmp = NULL;

    // Iterate over all listeners in list
    spin_lock( &rtap_listeners.lock );
    list_for_each_entry_safe( listener, tmp, &rtap_listeners.list, list )
    {
        seq_printf( file, "%s:%hu\n", listener->ipaddr,
                    ntohs(listener->in_addr.sin_port) );
    } // end loop 
    spin_unlock( &rtap_listeners.lock );

    return( 0 );
}

//*****************************************************************************
static int
proc_open( struct inode *inode, struct file *file )
{
    return( single_open( file, proc_show, NULL ) );
}

//*****************************************************************************
static int
proc_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
proc_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
proc_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
    char str[256+1] = { 0 };
    char ipaddr[256+1] = { 0 };
    short port = 8888;
    int ret = 0;

    cnt = (cnt >= 256) ? 256 : cnt;
    copy_from_user( str, buf, cnt );
    
    ret = sscanf( str, "%s %hu", ipaddr, &port );

	//printk( KERN_INFO "RTAP: IP: %s, PORT: %hu\n", ipaddr, port);

    if( (ret == 1) && (strlen(ipaddr) == 1) && (ipaddr[0] == '-') )
    {
        listener_clear();
    } // end if
    else if( (ret >= 1) && (strlen(ipaddr) > 1) )
    {
        if( ipaddr[0] == '-' )
        {
            listener_remove( &ipaddr[1], port );
        } // end if
        else if( ipaddr[0] == '+' )
        {
            listener_add( &ipaddr[1], port );
        } // end else
        else
        {
            listener_add( ipaddr, port );
        } // end else
    } // end else if
    else
    {
        printk( KERN_ERR "RTAP: Failed parsing IP string: %s\n", str );
        return( -1 );
    } // end else

    return( cnt );
}

//*****************************************************************************
//*****************************************************************************
const struct file_operations listener_fops =
{
    .owner      = THIS_MODULE,
    .open       = proc_open,
    .release    = proc_close,
    .read       = proc_read,
    .llseek     = proc_lseek,
    .write      = proc_write,
};

