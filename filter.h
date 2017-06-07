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
//    File: rtap_filter.h
//    Description:  
//      Filter id, rtap_filter type, rule id, rtap_filter command, string
//
//    Examples: 
//      Forward all beacons:
//        rtap_filter id:      designates order of operation
//        rtap_filter type:    FILTER_TYPE_80211_MAC (3)
//        rule id:        RULE_ID_MAC_FCTL (4)
//        rtap_filter command: FILTER_CMD_FWRD (2)
//        rtap_filter string:  0080 (Beacon frame)
//      Forward all probe requests:
//        rtap_filter id:      designates order of operation
//        rtap_filter type:    FILTER_TYPE_80211_MAC (3)
//        rule id:        RULE_ID_MAC_FCTL (4)
//        rtap_filter command: FILTER_CMD_FWRD (2)
//        rtap_filter string:  0040 (Probe request frame)
//*****************************************************************************

#ifndef __FILTER_H__
#define __FILTER_H__

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/skbuff.h>

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef uint32_t rtap_filter_id_t;

typedef enum rtap_filter_type
{
    FILTER_TYPE_NONE = 0,
    FILTER_TYPE_ALL = 1,
    FILTER_TYPE_RADIOTAP = 2,
    FILTER_TYPE_80211_MAC = 3,
    FILTER_TYPE_IP = 4,
    FILTER_TYPE_UDP = 5,
    FILTER_TYPE_TCP = 6,
    FILTER_TYPE_LAST
} rtap_filter_type_t;

typedef enum rtap_filter_cmd
{
    FILTER_CMD_NONE = 0,
    FILTER_CMD_DROP = 1,
    FILTER_CMD_FWRD = 2,
    FILTER_CMD_CNT  = 3,
    FILTER_CMD_LAST
} rtap_filter_cmd_t;

typedef enum rtap_filter_subtype
{
    RULE_ID_NONE = 0,
    RULE_ID_RTAP_DB = 1,
    RULE_ID_MAC_SA = 2,
    RULE_ID_MAC_DA = 3,
    RULE_ID_MAC_FCTL = 4,
    RULE_ID_LAST
} rtap_filter_subtype_t;

typedef int (*rtap_filter_func)( struct sk_buff *skb );

//*****************************************************************************
// Global variables
//*****************************************************************************

extern const struct file_operations rtap_filter_fops;

//*****************************************************************************
// Function prototypes
//*****************************************************************************

extern int rtap_filter_init( void );
extern int rtap_filter_exit( void );

extern int rtap_filter_register( rtap_filter_func func );
extern int rtap_filter_unregister( rtap_filter_func func );

extern int rtap_filter_recv( struct sk_buff *skb );

#endif

