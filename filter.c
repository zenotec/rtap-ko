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
//    File: filter.c
//    Description:  Filters specific to a group of rules.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "rtap-ko.h"
#include "device.h"
#include "listener.h"
#include "rule.h"
#include "filter.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef struct
{
    struct list_head list;
    spinlock_t lock;
} filter_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static filter_t filters = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

extern filter_cmd_t rtap_filter_drop(rule_id_t id, filter_cmd_t cmd, void *buf, void *val);
extern filter_cmd_t rtap_filter_radiotap(rule_id_t id, filter_cmd_t cmd, void *buf, void *val);
extern filter_cmd_t rtap_filter_80211_mac(rule_id_t id, filter_cmd_t cmd, void *buf, void *val);

filter_cmd_t
fltr_list_recv( struct sk_buff *skb )
{
    filter_cmd_t cmd = FILTER_CMD_NONE;
    return( cmd );
}

int
rtap_func( struct sk_buff *skb, struct net_device *dev,
           struct packet_type *pt, struct net_device *orig_dev )
{

    filter_cmd_t cmd = FILTER_CMD_NONE;

    // Run through filters
    cmd = fltr_list_recv( skb );

    // Check if forward command was given
    if( cmd == FILTER_CMD_FWRD )
    {
        ip_list_send( skb );
    } // end if

    // Free frame
    kfree_skb( skb );

    // Return success
    return( 0 );
}

//*****************************************************************************
filter_cmd_t
rtap_filter_drop(rule_id_t id, filter_cmd_t cmd, void *buf, void *val)
{
    return(FILTER_CMD_DROP);
}

//*****************************************************************************
filter_cmd_t
rtap_filter_radiotap( rule_id_t id, filter_cmd_t cmd, void *buf, void *val )
{
    filter_cmd_t ret_cmd = FILTER_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = 0;
    if ( rthdr ); // TODO: Implement
    switch(id)
    {
        default:
            break;
    }
    return(ret_cmd);
}

//*****************************************************************************
filter_cmd_t
rtap_filter_80211_mac( rule_id_t id, filter_cmd_t cmd, void *buf, void *val )
{
    filter_cmd_t ret_cmd = FILTER_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)buf;
    struct ieee80211_hdr *fhdr = (struct ieee80211_hdr *)((uint8_t *)rthdr + cpu_to_le16( rthdr->it_len ) );
    switch( id )
    {
        case RULE_ID_MAC_SA:
            //printk( KERN_INFO "RTAP: SA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2],
            //         ((uint8_t *)val)[3], ((uint8_t *)val)[4], ((uint8_t *)val)[5] );
            //printk(KERN_INFO "RTAP: SA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         fhdr->addr2[0], fhdr->addr2[1], fhdr->addr2[2],
            //         fhdr->addr2[3], fhdr->addr2[4], fhdr->addr2[5]);
            if( !memcmp(fhdr->addr2, val, sizeof(fhdr->addr2)) )
            {
                ret_cmd = cmd;
            } // end if
            break;
        case RULE_ID_MAC_DA:
            //printk(KERN_INFO "RTAP: DA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2],
            //         ((uint8_t *)val)[3], ((uint8_t *)val)[4], ((uint8_t *)val)[5]);
            //printk(KERN_INFO "RTAP: DA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         fhdr->addr1[0], fhdr->addr1[1], fhdr->addr1[2],
            //         fhdr->addr1[3], fhdr->addr1[4], fhdr->addr1[5]);
            if( !memcmp(fhdr->addr1, val, sizeof(fhdr->addr1)) )
            {
                ret_cmd = cmd;
            } // end if
            break;
        case RULE_ID_MAC_FCTL:
            //printk(KERN_INFO "RTAP: FCTL(exp): 0x%04x\n", *(uint16_t *)val);
            //printk(KERN_INFO "RTAP: FCTL(obs): 0x%04x\n", cpu_to_le16(fhdr->frame_control));
            if( cpu_to_le16(fhdr->frame_control) == *(uint16_t *)val )
            {
                ret_cmd = cmd;
            } // end if
            break;
        default:
            break;
    }
    return( ret_cmd );
}

//*****************************************************************************
static int
fltr_list_add( filter_id_t id )
{
    filter_t *filter = 0;

    // Allocate new filter list item
    filter = kmalloc( sizeof(filter_t), GFP_ATOMIC );
    if( ! filter )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory\n" );
        return( -1 );
    } // end if
    memset( (void *)filter, 0, sizeof( filter_t ) );

    // Populate filter list item

    // Add device list item to tail of device list
    spin_lock( &filters.lock );
    list_add_tail( &filter->list, &filters.list );
    spin_unlock( &filters.lock );

    // Return non-null network device pointer on success; null on error
    return( 0 );
}

//*****************************************************************************
static int
fltr_list_remove( filter_id_t id )
{
    filter_t *filter = 0;
    filter_t *tmp = 0;
    int ret = -1;

    // Search for device in list and remove
    spin_lock( &filters.lock );
    list_for_each_entry_safe( filter, tmp, &filters.list, list )
    {
        if( ! 1 )
        {
            printk( KERN_INFO "RTAP: Removing filter: %s\n", "1" );
            list_del( &filter->list );
            kfree( filter );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &filters.lock );

    // Return non-null network device pointer on success; null on error
    return( ret );
}

//*****************************************************************************
static int
fltr_list_clear( void )
{
    filter_t *filter = 0;
    filter_t *tmp = 0;

    // Remove all filters from list
    spin_lock( &filters.lock );
    list_for_each_entry_safe( filter, tmp, &filters.list, list )
    {
        printk( KERN_INFO "RTAP: Removing device: %s\n", "1" );
        list_del( &filter->list );
        kfree( filter );
    } // end loop 
    spin_unlock( &filters.lock );

    return( 0 );
}

//*****************************************************************************
int
fltr_list_init( void )
{
    spin_lock_init( &filters.lock );
    INIT_LIST_HEAD( &filters.list );
    dev_list_init( rtap_func );
    return( 0 );
}

//*****************************************************************************
int
fltr_list_exit( void )
{
    dev_list_exit();
    return( fltr_list_clear() );
}

//*****************************************************************************
//*****************************************************************************
static int
fltr_proc_show( struct seq_file *file, void *arg )
{
    filter_t *filter = 0;
    filter_t *tmp = 0;

    // Iterate over all filters in list
    spin_lock( &filters.lock );
    list_for_each_entry_safe( filter, tmp, &filters.list, list )
    {
        seq_printf( file, " Id \n" );
    } // end loop 
    spin_unlock( &filters.lock );

    return( 0 );
}

//*****************************************************************************
static int
fltr_proc_open( struct inode *inode, struct file *file )
{
    return( single_open( file, fltr_proc_show, NULL ) );
}

//*****************************************************************************
static int
fltr_proc_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************
static ssize_t
fltr_proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************
static loff_t
fltr_proc_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************
static ssize_t
fltr_proc_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
    char fltrstr[256+1] = { 0 };
    filter_id_t id;

    if( ! cnt )
    {
        fltr_list_clear();
    } // end if
    else
    {
        cnt = (cnt >= 256) ? 256 : cnt;
        copy_from_user( fltrstr, buf, cnt );
        sscanf( fltrstr, "%d", (int *)&id );
        if( fltrstr[0] == '-' )
        {
            fltr_list_remove( id );
        } // end if
        else if( fltrstr[0] == '+' )
        {
            fltr_list_add( id );
        } // end else
        else
        {
            fltr_list_add( id );
        } // end else
    } // end else
    return( cnt );
}

//*****************************************************************************
//*****************************************************************************
const struct file_operations fltr_proc_fops =
{
    .owner      = THIS_MODULE,
    .open       = fltr_proc_open,
    .release    = fltr_proc_close,
    .read       = fltr_proc_read,
    .llseek     = fltr_proc_lseek,
    .write      = fltr_proc_write,
};

