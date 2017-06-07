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
//    File: stats.c
//    Description: 
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

#include "stats.h"

typedef struct
{
    struct list_head list;
    spinlock_t lock;
    unsigned int fid;
    unsigned int total;
    unsigned int forwarded;
    unsigned int dropped;
} stats_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static stats_t stats = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************

int
stats_remove( const unsigned int fid )
{
    stats_t *s = 0;
    stats_t *tmp = 0;
    int ret = -1;

    // Search for filter id in list and remove
    spin_lock( &stats.lock );
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        if( s->fid == fid )
        {
            printk( KERN_INFO "RTAP: Removing filter statistics: %u\n", s->fid );
            list_del( &s->list );
            kfree( s );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &stats.lock );

    // Return non-null on success; null on error
    return( ret );
}

//*****************************************************************************

int
stats_add( const unsigned int fid )
{
    stats_t *s = 0;

    // Remove any duplicates
    stats_remove( fid );

    // Allocate new statistics list item
    s = kmalloc( sizeof(stats_t), GFP_ATOMIC );
    if( ! s )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory: stats[%u]\n", fid );
        return( 0 );
    } // end if
    memset( (void *)s, 0, sizeof( stats_t ) );

    // Initialize
    s->fid = fid;

    // Add statistics list item to tail of statistics list
    spin_lock( &stats.lock );
    list_add_tail( &s->list, &stats.list );
    spin_unlock( &stats.lock );

    // Return non-null on success, null on error
    return( 1 );
}

//*****************************************************************************

static int
stats_clear( void )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    // Remove all filter stats from list
    spin_lock( &stats.lock );
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        printk( KERN_INFO "RTAP: Removing filter statistics: %u\n", s->fid );
        list_del( &s->list );
        kfree( s );
    } // end loop 
    spin_unlock( &stats.lock );

    return( 0 );
}

//*****************************************************************************

int
stats_init( void )
{
    spin_lock_init( &stats.lock );
    INIT_LIST_HEAD( &stats.list );
    return( 0 );
}

//*****************************************************************************

int
stats_exit( void )
{
    return( stats_clear() );
}

//*****************************************************************************

int
stats_forwarded( const unsigned int fid )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    spin_lock( &stats.lock );
    stats.total++;
    stats.forwarded++;
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        if( s->fid == fid )
        {
            s->total++;
            s->forwarded++;
            break;
        } // end if
    } // end loop 
    spin_unlock( &stats.lock );
    return( 0 );
}

//*****************************************************************************

int
stats_dropped( const unsigned int fid )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    spin_lock( &stats.lock );
    stats.total++;
    stats.dropped++;
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        if( s->fid == fid )
        {
            s->total++;
            s->dropped++;
            break;
        } // end if
    } // end loop 
    spin_unlock( &stats.lock );
    return( 0 );
}

//*****************************************************************************

int
stats_error( const unsigned int fid )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    spin_lock( &stats.lock );
    stats.total++;
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        if( s->fid == fid )
        {
            s->total++;
            break;
        } // end if
    } // end loop 
    spin_unlock( &stats.lock );
    return( 0 );
}

//*****************************************************************************

int
stats_reset( const unsigned int fid )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    spin_lock( &stats.lock );
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        if( s->fid == fid )
        {
            s->total = 0;
            s->forwarded = 0;
            s->dropped = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &stats.lock );
    return( 0 );
}

//*****************************************************************************

static int
proc_show( struct seq_file *file, void *arg )
{
    stats_t *s = 0;
    stats_t *tmp = 0;

    // Print header
    seq_printf( file, "----------------------------------------------\n");
    seq_printf( file, "| fid |    total   |  forwarded |   dropped  |\n");
    seq_printf( file, "|--------------------------------------------|\n");
    seq_printf( file, "| %3u | %10u | %10u | %10u |\n", stats.fid, stats.total, stats.forwarded, stats.dropped );

    // Iterate over all filter stats in list
    spin_lock( &stats.lock );
    list_for_each_entry_safe( s, tmp, &stats.list, list )
    {
        seq_printf( file, "| %3u | %10u | %10u | %10u |\n", s->fid, s->total, s->forwarded, s->dropped );
    } // end loop 
    spin_unlock( &stats.lock );

    // Print footer
    seq_printf( file, "----------------------------------------------\n");
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
    char cmdstr[256+1] = { 0 };
    int fid = 0;
    int ret = 0;

    cnt = (cnt >= 256) ? 256 : cnt;
    copy_from_user( cmdstr, buf, cnt );
    ret = sscanf( cmdstr, "%d", &fid );

    if( (ret == 1) && (fid > 0) )
    {
        stats_reset( fid );
    } // end if
    else
    {
        printk( KERN_ERR "RTAP: Failed parsing command string: %s\n", cmdstr );
        return( -1 );
    } // end else

    return( cnt );

}

//*****************************************************************************
//*****************************************************************************
const struct file_operations stats_fops =
{
    .owner      = THIS_MODULE,
    .open       = proc_open,
    .release    = proc_close,
    .read       = proc_read,
    .llseek     = proc_lseek,
    .write      = proc_write,
};

