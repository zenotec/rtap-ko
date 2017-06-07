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
//    File: rule.h
//    Description: TODO: Replace temp rules with those configurable from
//                 user space.
//
//*****************************************************************************

#ifndef __RULE_H__
#define __RULE_H__

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef uint32_t rtap_rule_id_t;

typedef enum rtap_rule_action
{
    ACTION_NONE = 0,
    ACTION_DROP = 1,
    ACTION_FWRD = 2,
    ACTION_CNT  = 3,
    ACTION_LAST
} rtap_rule_action_t;

struct rtap_rule;

//*****************************************************************************
// Global variables
//*****************************************************************************

extern const struct file_operations rtap_rule_fops;

//*****************************************************************************
// Function prototypes
//*****************************************************************************

extern int
rtap_rule_init( void );

extern int
rtap_rule_exit( void );

#endif
