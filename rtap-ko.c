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
//    File: rtap-ko.c
//    Description: Main module for rtap.ko. Contains top level module
//                 initialization and parameter parsing.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/skbuff.h>
#include <linux/netdevice.h>

#include "rtap-ko.h"
#include "rule.h"
#include "filter.h"
#include "device.h"
#include "listener.h"
#include "proc.h"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

int
rtap_func( struct sk_buff *skb, struct net_device *dev,
           struct packet_type *pt, struct net_device *orig_dev )
{

    rule_t *rp = &ruletbl[0];
    rule_cmd_t cmd = RULE_CMD_NONE;

    // Run all rules on frame until drop/forward is returned
    while( rp->func && ( cmd = rp->func( rp->id, rp->cmd, skb->data, rp->val ) ) == RULE_CMD_NONE )
    {
        rp++;
    } // end while

    // Check if forward command was given
    if( cmd == RULE_CMD_FORWARD )
    {
        ip_list_send( skb );
    } // end if

    // Free frame
    kfree_skb( skb );

    // Return success
    return( 0 );
}

static int __init
rtap_init( void )
{

    dev_list_init( rtap_func );
    ip_list_init();
    rtap_proc_init();

    printk( KERN_INFO "RTAP: Driver registered (%s)\n", DRIVER_VERSION );

    /* Return ok */
    return( 0 );

}

static void __exit
rtap_exit( void )
{
    printk( KERN_INFO "RTAP: Unloading module...\n" );

    dev_list_exit();
    ip_list_exit();
    rtap_proc_exit();

    printk( KERN_INFO "RTAP: ...done.\n" );

    return;
}

module_init(rtap_init);
module_exit(rtap_exit);

