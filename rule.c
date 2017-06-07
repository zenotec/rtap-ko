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
//    File: rule.c
//    Description: TODO: Replace temp rules with those configurable from
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

#include "rule.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

struct rule;

typedef int (*rule_action_func)( struct rule r, struct sk_buff *skb );

typedef struct rule
{
    struct list_head list;
    spinlock_t lock;
    rule_id_t rid;
    rule_action_func func;
    void *arg;
} rule_t;


//*****************************************************************************
// Global variables
//*****************************************************************************

/* Global */

/* Local */

static rule_t rules = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************

static int
rule_action_none( struct rule r, struct sk_buff *skb )
{
    return( 0 );
}

//*****************************************************************************

static int
rule_action_drop( struct rule r, struct sk_buff *skb )
{
    return( 0 );
}

//*****************************************************************************

static int
rule_action_forward( struct rule r, struct sk_buff *skb )
{
    return( 0 );
}

//*****************************************************************************

static int
rule_action_count( struct rule r, struct sk_buff *skb )
{
    return( 0 );
}

//*****************************************************************************

int
rule_remove( const rule_id_t rid )
{
    rule_t *r = 0;
    rule_t *tmp = 0;
    int ret = -1;

    // Search for filter id in list and remove
    spin_lock( &rules.lock );
    list_for_each_entry_safe( r, tmp, &rules.list, list )
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
    spin_unlock( &rules.lock );

    // Return non-null on success; null on error
    return( ret );
}

//*****************************************************************************

static int
rule_add( rule_id_t rid, rule_action_t action, void* arg )
{
    rule_t *r = 0;

    // Remove any duplicates
    rule_remove( rid );

    // Allocate new statistics list item
    r = kmalloc( sizeof(rule_t), GFP_ATOMIC );
    if( ! r )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: rule[%u]\n", rid );
        return( 0 );
    } // end if
    memset( (void *)r, 0, sizeof( rule_t ) );

    // Initialize
    r->rid = rid;
    switch( action )
    {
        case ACTION_NONE:
            r->func = rule_action_none;
            break;
        case ACTION_DROP:
            r->func = rule_action_drop;
            break;
        case ACTION_FWRD:
            r->func = rule_action_forward;
            break;
        case ACTION_CNT:
            r->func = rule_action_count;
            break;
        default:
            return( 0 );
    }
    r->arg = arg;

    // Add statistics list item to tail of statistics list
    spin_lock( &rules.lock );
    list_add_tail( &r->list, &rules.list );
    spin_unlock( &rules.lock );

    // Return non-null on success, null on error
    return( 1 );
}

//*****************************************************************************
static int
rule_clear( void )
{
    rule_t *r = 0;
    rule_t *tmp = 0;

    // Remove all filter stats from list
    spin_lock( &rules.lock );
    list_for_each_entry_safe( r, tmp, &rules.list, list )
    {
        printk( KERN_INFO "RTAP: Removing rule: %u\n", r->rid );
        list_del( &r->list );
        kfree( r );
    } // end loop 
    spin_unlock( &rules.lock );

    return( 0 );
}


