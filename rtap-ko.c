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

#include "rtap-ko.h"
#include "rule.h"
#include "filter.h"
#include "stats.h"
#include "device.h"
#include "listener.h"
#include "proc.h"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

static int __init
rtap_init( void )
{

    listener_init();
    stats_list_init();
    fltr_list_init();
    rtap_proc_init();

    printk( KERN_INFO "RTAP: Driver registered (%s)\n", DRIVER_VERSION );

    /* Return ok */
    return( 0 );

}

static void __exit
rtap_exit( void )
{
    printk( KERN_INFO "RTAP: Unloading module...\n" );

    listener_exit();
    stats_list_exit();
    fltr_list_exit();
    rtap_proc_exit();

    printk( KERN_INFO "RTAP: ...done.\n" );

    return;
}

module_init(rtap_init);
module_exit(rtap_exit);

