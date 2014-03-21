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
//    Description:  Filters specific to a group of rules.
//
//*****************************************************************************

#ifndef __FILTER_H__
#define __FILTER_H__

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef enum filter_id
{
    FILTER_ID_NONE = 0,
    FILTER_ID_DROP = 1,
    FILTER_ID_RADIOTAP = 2,
    FILTER_ID_80211_MAC = 3,
    FILTER_ID_LAST
} filter_id_t;

//*****************************************************************************
// Function prototypes
//*****************************************************************************
extern rule_cmd_t rtap_filter_drop(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);
extern rule_cmd_t rtap_filter_radiotap(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);
extern rule_cmd_t rtap_filter_80211_mac(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);

#endif
