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
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//    File: rtap_rule.c
//    Description: TODO: Replace temp rtap_rules with those configurable from
//                 user space.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "listener.h"
#include "stats.h"
#include "rule.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef int (*rtap_rule_action_func)( struct rtap_rule* r, struct sk_buff *skb );

typedef struct rtap_rule
{
    struct list_head list;
    spinlock_t lock;
    rtap_rule_id_t rid;
    rtap_rule_action_t aid;
    rtap_rule_action_func func;
    void *arg;
} rtap_rule_t;

//*****************************************************************************
// Global variables
//*****************************************************************************

/* Global */

/* Local */

static struct rtap_rule rtap_rules = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************

static int
rtap_rule_action_none( struct rtap_rule* r, struct sk_buff *skb )
{
    if( r->aid != ACTION_NONE )
    {
        return( 0 );
    }
    return( 1 );
}

//*****************************************************************************

static int
rtap_rule_action_drop( struct rtap_rule* r, struct sk_buff *skb )
{
    if( r->aid != ACTION_DROP )
    {
        return( 0 );
    }
    return( 1 );
}

//*****************************************************************************

static int
rtap_rule_action_forward( struct rtap_rule* r, struct sk_buff *skb )
{
    struct rtap_listener* l = (struct rtap_listener*)r->arg;
    if( r->aid != ACTION_FWRD )
    {
        return( 0 );
    }
  
    printk( KERN_INFO "RTAP: Forward action rule\n" );

    listener_send( l, skb );

    return( 1 );
}

//*****************************************************************************

static int
rtap_rule_action_count( struct rtap_rule* r, struct sk_buff *skb )
{
    struct rtap_stats* s = (struct rtap_stats*)r->arg;
    if( r->aid != ACTION_CNT )
    {
        return( 0 );
    }
    return( 1 );
}

//*****************************************************************************

int
rtap_rule_remove( const rtap_rule_id_t rid )
{
    struct rtap_rule *r = 0;
    struct rtap_rule *tmp = 0;
    int ret = -1;

    // Search for filter id in list and remove
    spin_lock( &rtap_rules.lock );
    list_for_each_entry_safe( r, tmp, &rtap_rules.list, list )
    {
        if( r->rid == rid )
        {
            printk( KERN_INFO "RTAP: Removing filter statistics: %u\n", rid );
            list_del( &r->list );
            kfree( r );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &rtap_rules.lock );

    // Return non-null on success; null on error
    return( ret );
}

//*****************************************************************************

static int
rtap_rule_add( rtap_rule_id_t rid, rtap_rule_action_t aid, void* arg )
{
    struct rtap_rule *r = 0;

    // Remove any duplicates
    rtap_rule_remove( rid );

    // Allocate new statistics list item
    r = kmalloc( sizeof(struct rtap_rule), GFP_ATOMIC );
    if( ! r )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: rtap_rule[%u]\n", rid );
        return( 0 );
    } // end if
    memset( (void *)r, 0, sizeof( struct rtap_rule ) );

    // Initialize
    r->rid = rid;
    r->aid = aid;
    switch( aid )
    {
        case ACTION_NONE:
            r->func = rtap_rule_action_none;
            break;
        case ACTION_DROP:
            r->func = rtap_rule_action_drop;
            break;
        case ACTION_FWRD:
            r->func = rtap_rule_action_forward;
            r->arg = (void*)listener_findbyid( *(unsigned int*)arg );
            if( ! r->arg )
            {
                printk( KERN_ERR "RTAP: Cannot find listener identifier: %u\n",
                        *(unsigned int*)arg );
                kfree( r );
                return( 0 );
            }
            break;
        case ACTION_CNT:
            r->func = rtap_rule_action_count;
            break;
        default:
            return( 0 );
    }

    // Add statistics list item to tail of statistics list
    spin_lock( &rtap_rules.lock );
    list_add_tail( &r->list, &rtap_rules.list );
    spin_unlock( &rtap_rules.lock );

    // Return non-null on success, null on error
    return( 1 );
}

//*****************************************************************************

static int
rtap_rule_clear( void )
{
    struct rtap_rule *r = 0;
    struct rtap_rule *tmp = 0;

    // Remove all filter rtap_rule from list
    spin_lock( &rtap_rules.lock );
    list_for_each_entry_safe( r, tmp, &rtap_rules.list, list )
    {
        printk( KERN_INFO "RTAP: Removing rtap_rule: %u\n", r->rid );
        list_del( &r->list );
        kfree( r );
    } // end loop 
    spin_unlock( &rtap_rules.lock );

    return( 0 );
}

//*****************************************************************************

int
rtap_rule_init( void )
{
    spin_lock_init( &rtap_rules.lock );
    INIT_LIST_HEAD( &rtap_rules.list );
    return( 0 );
}

//*****************************************************************************

int
rtap_rule_exit( void )
{
    return( rtap_rule_clear() );
}

//*****************************************************************************

static const char *rtap_rule_action_str( struct rtap_rule* r )
{
    const char *str = 0;
    switch( r->aid )
    {
        case ACTION_NONE:
            str = "None";
            break;
        case ACTION_DROP:
            str = "Drop";
            break;
        case ACTION_FWRD:
            str = "Forward";
            break;
        case ACTION_CNT:
            str = "Count";
            break;
        default:
            str = "Unknown";
            break;
    }
    return( str );
}

//*****************************************************************************

static const char *rtap_rule_arg_str( struct rtap_rule* r, char* str, size_t len )
{
    switch( r->aid )
    {
        case ACTION_NONE:
        case ACTION_DROP:
            break;
        case ACTION_FWRD:
            snprintf( str, len, " -> %s:%hu",
                      listener_ipaddr((struct rtap_listener*)r->arg),
                      listener_port((struct rtap_listener*)r->arg) );
            break;
        case ACTION_CNT:
            break;
        default:
            strncpy( str, "Unknown", len );
            break;
    }
    return( str );
}

//*****************************************************************************

static int
rtap_rule_show( struct seq_file *file, void *arg )
{
    struct rtap_rule *r = 0;
    struct rtap_rule *tmp = 0;

    // Print header
    seq_printf( file, "------------------------------------------------\n");
    seq_printf( file, "|  Id |    action   |      Argument            |\n");
    seq_printf( file, "|----------------------------------------------|\n");

    // Iterate over all filter stats in list
    spin_lock( &rtap_rules.lock ); 
    list_for_each_entry_safe( r, tmp, &rtap_rules.list, list )
    {
        char arg_str[24 + 1] = { 0 };
        rtap_rule_arg_str( r, arg_str, sizeof(arg_str) );
        seq_printf( file, "| %3d | %11s | %-24s |\n",
                    r->rid, rtap_rule_action_str( r ), arg_str );
    } // end loop 
    spin_unlock( &rtap_rules.lock );

    seq_printf( file, "------------------------------------------------\n");

    return( 0 );
}

//*****************************************************************************
 
static int
rtap_rule_open( struct inode *inode, struct file *file )
{
    return( single_open( file, rtap_rule_show, NULL ) );
}

//*****************************************************************************

static int
rtap_rule_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
rtap_rule_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
rtap_rule_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
rtap_rule_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{   
    char cmdstr[256+1] = { 0 };
    unsigned int rid = 0;
    unsigned int aid = 0;
    unsigned int arg = 0;
    int ret = 0;
    
    cnt = (cnt >= 256) ? 256 : cnt;
    copy_from_user( cmdstr, buf, cnt );
    ret = sscanf( cmdstr, "%u %u %u", &rid, &aid, &arg );
    
    if( (ret == 1) && (rid > 0) )
    {   
        rtap_rule_remove( rid );
    } // end if
    else if ( (ret > 1) && (rid > 0) && (aid > 0) )
    {
        rtap_rule_add( rid, aid, (void*)&arg);
    }
    else
    {
        printk( KERN_ERR "RTAP: Failed parsing command string: %s\n", cmdstr );
        return( -1 );
    } // end else

    return( cnt );

}

//*****************************************************************************
//*****************************************************************************
const struct file_operations rtap_rule_fops =
{
    .owner      = THIS_MODULE,
    .open       = rtap_rule_open,
    .release    = rtap_rule_close,
    .read       = rtap_rule_read,
    .llseek     = rtap_rule_lseek,
    .write      = rtap_rule_write,
};


