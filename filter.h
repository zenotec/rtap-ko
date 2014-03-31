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
//    File: filter.h
//    Description:  
//
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

typedef uint32_t filter_id_t;

typedef enum filter_type
{
    FILTER_TYPE_NONE = 0,
    FILTER_TYPE_RADIOTAP = 1,
    FILTER_TYPE_80211_MAC = 2,
    FILTER_TYPE_LAST
} filter_type_t;

typedef enum filter_cmd
{
    FILTER_CMD_NONE = 0,
    FILTER_CMD_DROP = 1,
    FILTER_CMD_FWRD = 2,
    FILTER_CMD_CONT = 3,
    FILTER_CMD_LAST
} filter_cmd_t;

typedef enum rule_id
{
    RULE_ID_NONE = 0,
    RULE_ID_MAC_SA = 1,
    RULE_ID_MAC_DA = 2,
    RULE_ID_MAC_FCTL = 3,
    RULE_ID_DROP = 4,
    RULE_ID_LAST
} rule_id_t;

typedef filter_cmd_t (*filter_func_t)( rule_id_t id, filter_cmd_t cmd,
                                        struct sk_buff *skb, void *arg );

//*****************************************************************************
// Global variables
//*****************************************************************************

extern const struct file_operations fltr_proc_fops;

//*****************************************************************************
// Function prototypes
//*****************************************************************************

extern int fltr_list_init( void );
extern int fltr_list_exit( void );

#endif

