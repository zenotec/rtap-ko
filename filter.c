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
//    File: rtap_filter.c
//    Description:  Filters specific to a group of rules.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
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
#include "stats.h"
#include "filter.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef struct chain
{
    struct list_head list;
    spinlock_t lock;
} chain_t;

typedef struct rtap_filter
{
    struct list_head list;
    spinlock_t lock;
    rtap_filter_id_t fid;
    rtap_filter_type_t type;
    rtap_filter_cmd_t cmd;
    rtap_rule_id_t rid;
    char *arg;
} rtap_filter_t;

typedef rtap_filter_cmd_t (*rtap_filter_func_t)( rtap_filter_t *fp, struct sk_buff *skb );

//*****************************************************************************
// Function prototypes
//*****************************************************************************

rtap_filter_cmd_t rtap_rtap_filter_all( rtap_filter_t *fp, struct sk_buff *skb );
rtap_filter_cmd_t rtap_rtap_filter_radiotap( rtap_filter_t *fp, struct sk_buff *skb );
rtap_filter_cmd_t rtap_rtap_filter_80211_mac( rtap_filter_t *fp, struct sk_buff *skb );

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static rtap_filter_func_t rtap_filtertbl[] =
{
    [FILTER_TYPE_NONE] = NULL,
    [FILTER_TYPE_ALL] = &rtap_rtap_filter_all,
    [FILTER_TYPE_RADIOTAP] = &rtap_rtap_filter_radiotap,
    [FILTER_TYPE_80211_MAC] = &rtap_rtap_filter_80211_mac,
    [FILTER_TYPE_LAST] = NULL
};

static rtap_filter_t rtap_filters = { { 0 } }; // Dynamic rtap_filter list

//*****************************************************************************
// Functions
//*****************************************************************************

int
rtap_filter_recv( struct sk_buff *skb )
{

    rtap_filter_t *f = 0;
    rtap_filter_t *rtap_filter = 0;
    rtap_filter_t *tmp = 0;


    //printk( KERN_INFO "RTAP: Received\n" );

    // Loop through all rtap_filters
    spin_lock( &rtap_filters.lock );
    list_for_each_entry_safe( f, tmp, &rtap_filters.list, list )
    {
        if( rtap_filtertbl[f->type]( f, skb ) != FILTER_CMD_NONE )
        {
            rtap_filter = f;
            break;
        } // end if
    } // end loop 
    spin_unlock( &rtap_filters.lock );

    if( rtap_filter )
    {
        // Check if forward command was given
        switch( rtap_filter->cmd )
        {    
            case FILTER_CMD_FWRD:
                printk( KERN_INFO "RTAP: Forwarding\n" );
                // Update rtap_filter statistics
                stats_forwarded( rtap_filter->fid );
                //listener_send( skb );
                break;
            case FILTER_CMD_DROP:
                printk( KERN_INFO "RTAP: Dropping\n" );
                // Update rtap_filter statistics
                stats_dropped( rtap_filter->fid );
            case FILTER_CMD_NONE:
                break;
            default:
                printk( KERN_INFO "RTAP: Unknown command\n" );
                break;
        } // end switch 
    } // end if
    else
    {
        stats_dropped( 0 );
    } // end else

    // Free frame
    kfree_skb( skb );

    // Return success
    return( 0 );
}

//*****************************************************************************
//*****************************************************************************

static const char *rtap_filter_type_str( rtap_filter_type_t type )
{
    const char *str = 0;
    switch( type )
    {
        case FILTER_TYPE_NONE:
            str = "None";
            break;
        case FILTER_TYPE_ALL:
            str = "All";
            break;
        case FILTER_TYPE_RADIOTAP:
            str = "RadioTap";
            break;
        case FILTER_TYPE_80211_MAC:
            str = "802.11 MAC";
            break;
        default:
            str = "Unknown";
            break;
    }
    return( str );
}

//*****************************************************************************

static const char *rtap_filter_cmd_str( rtap_filter_cmd_t cmd )
{
    const char *str = 0;
    switch( cmd )
    {
        case FILTER_CMD_DROP:
            str = "Drop";
            break;
        case FILTER_CMD_FWRD:
            str = "Forward";
            break;
        default:
            str = "Unknown";
            break;
    }
    return( str );
}

//*****************************************************************************
//*****************************************************************************

rtap_filter_cmd_t
rtap_rtap_filter_all( rtap_filter_t *fp, struct sk_buff *skb )
{
    printk( KERN_INFO "RTAP: rtap_filter_all: %d\n", fp->cmd );
    return( fp->cmd );
}

//*****************************************************************************

rtap_filter_cmd_t
rtap_rtap_filter_radiotap( rtap_filter_t *fp, struct sk_buff *skb )
{
    rtap_filter_cmd_t ret_cmd = FILTER_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)skb->data;
    if ( rthdr ); // TODO: Implement
    switch( fp->rid )
    {
        default:
            break;
    }
    return( ret_cmd );
}

//*****************************************************************************

rtap_filter_cmd_t
rtap_rtap_filter_80211_mac( rtap_filter_t *fp, struct sk_buff *skb )
{
    rtap_filter_cmd_t ret_cmd = FILTER_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr =
        (struct ieee80211_radiotap_header *)skb->data;
    struct ieee80211_hdr *fhdr =
        (struct ieee80211_hdr *)((uint8_t *)rthdr + cpu_to_le16( rthdr->it_len ) );
    uint8_t mac[6] = { 0 };
    uint16_t fctl = 0;

    switch( fp->rid )
    {
        case RULE_ID_MAC_SA:
            // Convert mac string to binary
            sscanf( fp->arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            //printk( KERN_INFO "RTAP: SA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            //printk(KERN_INFO "RTAP: SA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         fhdr->addr2[0], fhdr->addr2[1], fhdr->addr2[2],
            //         fhdr->addr2[3], fhdr->addr2[4], fhdr->addr2[5]);
            if( ! memcmp( fhdr->addr2, mac, sizeof( fhdr->addr2 ) ) )
            {
                ret_cmd = fp->cmd;
            } // end if
            break;
        case RULE_ID_MAC_DA:
            // Convert mac string to binary
            sscanf( fp->arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            //printk(KERN_INFO "RTAP: DA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            //printk(KERN_INFO "RTAP: DA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
            //         fhdr->addr1[0], fhdr->addr1[1], fhdr->addr1[2],
            //         fhdr->addr1[3], fhdr->addr1[4], fhdr->addr1[5]);
            if( ! memcmp( fhdr->addr1, fp->arg, sizeof( fhdr->addr1 ) ) )
            {
                ret_cmd = fp->cmd;
            } // end if
            break;
        case RULE_ID_MAC_FCTL:
            // Convert fctl string to binary
            sscanf( fp->arg, "%hx", &fctl );
            //printk(KERN_INFO "RTAP: FCTL(exp): 0x%04x\n", fctl);
            //printk(KERN_INFO "RTAP: FCTL(obs): 0x%04x\n", cpu_to_le16(fhdr->frame_control));
            if( cpu_to_le16( fhdr->frame_control ) == fctl )
            {
                ret_cmd = fp->cmd;
            } // end if
            break;
        default:
            break;
    }
    return( ret_cmd );
}

//*****************************************************************************

static int
rtap_filter_remove( rtap_filter_id_t fid )
{
    rtap_filter_t *rtap_filter = 0;
    rtap_filter_t *tmp = 0;
    int ret = -1;

    // Search for device in list and remove
    spin_lock( &rtap_filters.lock );
    list_for_each_entry_safe( rtap_filter, tmp, &rtap_filters.list, list )
    {
        if( rtap_filter->fid == fid )
        {
            printk( KERN_INFO "RTAP: Removing rtap_filter: %d\n", fid );
            // Add rtap_filter statistics entry
            stats_remove( fid );
            // Remove rtap_filter from list
            list_del( &rtap_filter->list );
            kfree( rtap_filter->arg );
            kfree( rtap_filter );
            ret = 0;
            break;
        } // end if
    } // end loop 
    spin_unlock( &rtap_filters.lock );

    // Return non-null network device pointer on success; null on error
    return( ret );
}

//*****************************************************************************

static int
rtap_filter_add( rtap_filter_id_t fid, rtap_filter_type_t type,
                 rtap_rule_id_t rid, rtap_filter_cmd_t cmd, char *arg )
{
    rtap_filter_t *rtap_filter = 0;

    // Remove any duplicate rtap_filters
    rtap_filter_remove( fid );

    // Validate rtap_filter type
    if( (type <= FILTER_TYPE_NONE) || (type >= FILTER_TYPE_LAST) )
    {
        printk( KERN_ERR "RTAP: Filter type %d not supported\n", type );
        return( -1 );
    } // end if

    // Validate rule id
    if( (rid < RULE_ID_NONE) || (rid >= RULE_ID_LAST) )
    {
        printk( KERN_ERR "RTAP: Rule id %d not supported\n", rid );
        return( -1 );
    } // end if

    // Validate rtap_filter command
    if( (cmd <= FILTER_CMD_NONE) || (cmd >= FILTER_CMD_LAST) )
    {
        printk( KERN_ERR "RTAP: Filter command %d not supported\n", cmd );
        return( -1 );
    } // end if

    // Allocate new rtap_filter list item
    rtap_filter = kmalloc( sizeof(rtap_filter_t), GFP_ATOMIC );
    if( ! rtap_filter )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory\n" );
        return( -1 );
    } // end if
    memset( (void *)rtap_filter, 0, sizeof( rtap_filter_t ) );

    // Populate rtap_filter list item
    rtap_filter->fid = fid;
    rtap_filter->type = type;
    rtap_filter->cmd = cmd;
    rtap_filter->rid = rid;
    rtap_filter->arg = arg;

    printk( KERN_INFO "RTAP: Adding rtap_filter: #%d Type[%s] Cmd[%s] Arg: %s\n",
            fid, rtap_filter_type_str(type), rtap_filter_cmd_str(cmd), arg );

    // Add rtap_filter statistics entry
    stats_add( fid );

    // Add rtap_filter list item to tail of device list
    spin_lock( &rtap_filters.lock );
    list_add_tail( &rtap_filter->list, &rtap_filters.list );
    spin_unlock( &rtap_filters.lock );

    // Return non-null network device pointer on success; null on error
    return( 0 );
}

//*****************************************************************************

static int
rtap_filter_clear( void )
{
    rtap_filter_t *rtap_filter = 0;
    rtap_filter_t *tmp = 0;

    // Remove all rtap_filters from list
    spin_lock( &rtap_filters.lock );
    list_for_each_entry_safe( rtap_filter, tmp, &rtap_filters.list, list )
    {
        printk( KERN_INFO "RTAP: Removing rtap_filter: %d\n", rtap_filter->fid );
        list_del( &rtap_filter->list );
        kfree( rtap_filter->arg );
        kfree( rtap_filter );
    } // end loop 
    spin_unlock( &rtap_filters.lock );

    return( 0 );
}

//*****************************************************************************

int
rtap_filter_init( void )
{
    spin_lock_init( &rtap_filters.lock );
    INIT_LIST_HEAD( &rtap_filters.list );
    return( 0 );
}

//*****************************************************************************

int
rtap_filter_exit( void )
{
    return( rtap_filter_clear() );
}

//*****************************************************************************
//*****************************************************************************

static int
rtap_filter_show( struct seq_file *file, void *arg )
{
    rtap_filter_t *rtap_filter = 0;
    rtap_filter_t *tmp = 0;

    // Iterate over all rtap_filters in list
    spin_lock( &rtap_filters.lock );
    list_for_each_entry_safe( rtap_filter, tmp, &rtap_filters.list, list )
    {
        seq_printf( file, "%d\tFilter Type[%d]\tRule[%d]\tCmd[%d]\tArg: %s\n",
         rtap_filter->fid, rtap_filter->type, rtap_filter->rid, rtap_filter->cmd, rtap_filter->arg );
    } // end loop 
    spin_unlock( &rtap_filters.lock );

    return( 0 );
}

//*****************************************************************************

static int
rtap_filter_open( struct inode *inode, struct file *file )
{
    return( single_open( file, rtap_filter_show, NULL ) );
}

//*****************************************************************************

static int
rtap_filter_close( struct inode *inode, struct file *file )
{
    return( single_release( inode, file ) );
}

//*****************************************************************************

static ssize_t
rtap_filter_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
    return( seq_read( file, buf, cnt, off ) );
}

//*****************************************************************************

static loff_t
rtap_filter_lseek( struct file *file, loff_t off, int cnt )
{
    return( seq_lseek( file, off, cnt ) );
}

//*****************************************************************************

static ssize_t
rtap_filter_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
    char fltrstr[256+1] = { 0 };
    int fid; // rtap_filter id
    int type; // rtap_filter type
    int cmd; // rtap_filter cmd
    int rid; // rule id
    char *str; // rtap_filter argument string
    int ret = 0;

    if( ! cnt )
    {
        rtap_filter_clear();
        return( cnt );
    } // end if

    // Allocate new rtap_filter list item
    str = kmalloc( 256+1, GFP_ATOMIC );
    if( ! str )
    {
        printk( KERN_CRIT "RTAP: Cannot allocate memory\n" );
        return( -1 );
    } // end if
    memset( (void *)str, 0, 256+1 );

    cnt = (cnt >= 256) ? 256 : cnt;
    copy_from_user( fltrstr, buf, cnt );
    ret = sscanf( fltrstr, "%d %d %d %d %256s", &fid, &type, &rid, &cmd, str );

    if( (ret == 0) && (strlen(fltrstr) == 1) && (fltrstr[0] == '-') )
    {
        rtap_filter_clear();
    } // end if
    else if( (ret == 5) && (fid > 0) )
    {
        rtap_filter_add( fid, type, rid, cmd, str );
    } // end else if
    else if( (ret == 1) && (fid < 0) )
    {
        rtap_filter_remove( -fid );
    } // end if
    else
    {
        printk( KERN_ERR "RTAP: Failed parsing rtap_filter string: %s\n", fltrstr );
        return( -1 );
    } // end else

    // Return number of bytes written
    return( cnt );
}

//*****************************************************************************
//*****************************************************************************
const struct file_operations rtap_filter_fops =
{
    .owner      = THIS_MODULE,
    .open       = rtap_filter_open,
    .release    = rtap_filter_close,
    .read       = rtap_filter_read,
    .llseek     = rtap_filter_lseek,
    .write      = rtap_filter_write,
};

