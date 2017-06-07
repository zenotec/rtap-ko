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
//    File: proc.c
//    Description: Implements the top level proc filesystem directory for
//                 all rtap modules that provide proc access.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/proc_fs.h>

#include "listener.h"
#include "device.h"
#include "rule.h"
#include "filter.h"
#include "stats.h"

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static struct proc_dir_entry *rtap_proc_dir = NULL;

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
int
rtap_proc_init( void )
{
    rtap_proc_dir = proc_mkdir( "rtap", NULL );
    proc_create( "devices", 0666, rtap_proc_dir, &device_fops );
    proc_create( "rules", 0666, rtap_proc_dir, &rule_fops );
    proc_create( "listeners", 0666, rtap_proc_dir, &listener_fops );
    proc_create( "filters", 0666, rtap_proc_dir, &filter_fops );
    proc_create( "stats", 0666, rtap_proc_dir, &stats_fops );
    return( 0 );
}

//*****************************************************************************
int
rtap_proc_exit( void )
{
    remove_proc_entry( "devices", rtap_proc_dir );
    remove_proc_entry( "rules", rtap_proc_dir );
    remove_proc_entry( "listeners", rtap_proc_dir );
    remove_proc_entry( "filters", rtap_proc_dir );
    remove_proc_entry( "stats", rtap_proc_dir );
    remove_proc_entry( "rtap", NULL );
    return( 0 );
}

